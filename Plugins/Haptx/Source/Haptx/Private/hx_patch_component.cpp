// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_patch_component.h>
#include <Runtime/Engine/Classes/Engine/StaticMeshActor.h>
#include <Runtime/Engine/Classes/Engine/World.h>
#include <Runtime/Engine/Classes/GameFramework/Actor.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h>
#include <Haptx/Public/hx_core_actor.h>
#include <Haptx/Public/hx_patch_socket.h>
#include <Haptx/Public/hx_physical_material.h>

DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxPatch::updateTraces"),
    STAT_updateTraces, STATGROUP_HxPatch)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxPatch::updateTraces - objectHit"),
    STAT_updateTraces_objectHit, STATGROUP_HxPatch)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxPatch::updateTraces - parentHit"),
    STAT_updateTraces_parentHit, STATGROUP_HxPatch)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxPatch::updateTraces - visualizeTraces"),
    STAT_updateTraces_visualizeTraces, STATGROUP_HxPatch)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxPatch::visualizeTactileFeedbackOutput"),
    STAT_visualizeTactileFeedbackOutput, STATGROUP_HxPatch)

void FHxPatchSecondaryTickFunction::ExecuteTick(
    float DeltaTime,
    ELevelTick TickType,
    ENamedThreads::Type CurrentThread,
    const FGraphEventRef& CompletionGraphEvent) {
  if (Target && !Target->IsPendingKill() && !Target->IsUnreachable()) {
    FScopeCycleCounterUObject ActorScope(Target);
    Target->TickComponentSecondary(DeltaTime, TickType, *this);
  }
}

FString FHxPatchSecondaryTickFunction::DiagnosticMessage() {
  return Target->GetFullName() + TEXT("UHxPatchComponent Secondary Tick.");
}

UHxPatchComponent::UHxPatchComponent(const FObjectInitializer& ObjectInitializer): 
    Super(ObjectInitializer), t_object_trace_directions_({FVector::DownVector}),
    l_object_trace_directions_(), is_enabled_(true) {
  bTickInEditor = false;
  PrimaryComponentTick.bCanEverTick = true;
  // Ticking gets enabled at the end of BeginPlay().
  PrimaryComponentTick.bStartWithTickEnabled = false;
  PrimaryComponentTick.TickGroup = TG_PostPhysics;
  SecondaryComponentTick.bCanEverTick = true;
  SecondaryComponentTick.bStartWithTickEnabled = false;
  SecondaryComponentTick.TickGroup = TG_PostUpdateWork;
  GlobalCreatePhysicsDelegate.AddUObject(this, &UHxPatchComponent::OnGlobalCreatePhysics);
}

void UHxPatchComponent::OnGlobalCreatePhysics(UActorComponent* component) {
  if (component == GetAttachParent()) {
    alignWithLocatingSocket();
  }
}

void UHxPatchComponent::BeginPlay() {
  Super::BeginPlay();

  if (!is_enabled_ || !isPawnLocallyControlled(getPawn(GetOwner()))) {
    return;
  }

  if (!IsTemplate() && SecondaryComponentTick.bCanEverTick) {
    SecondaryComponentTick.Target = this;
    SecondaryComponentTick.SetTickFunctionEnable(SecondaryComponentTick.bStartWithTickEnabled);
    SecondaryComponentTick.RegisterTickFunction(GetOwner()->GetLevel());
  }

  connectToCore();
  registerTactors();

  if (!is_enabled_) {
    return;
  }

  configureTraceParameters();

  // Warn if unreal unit not equal to 1 cm
  float uu_to_cm = GetWorld()->GetWorldSettings()->WorldToMeters / 100.f;
  if (uu_to_cm != 1) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "Unreal units to centimeters conversion is currently %f, please set 'World to Meters' to 100 for proper scaling."), uu_to_cm));
  }

  if (IsValid(hx_core_)) {
    hx_core_->PrimaryActorTick.AddPrerequisite(this, PrimaryComponentTick);
  }

  // If we've made it this far without an error, it's time to start ticking.
  if (is_enabled_) {
    PrimaryComponentTick.AddPrerequisite(hx_core_, hx_core_->GlobalFirstTick);
    PrimaryComponentTick.SetTickFunctionEnable(true);
    SecondaryComponentTick.AddPrerequisite(hx_core_, hx_core_->GlobalFirstTick);
    SecondaryComponentTick.SetTickFunctionEnable(true);
  }
}

void UHxPatchComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!is_enabled_) {
    return;
  }

  updateTraces();
}

void UHxPatchComponent::TickComponentSecondary(
    float DeltaTime,
    ELevelTick TickType,
    FHxPatchSecondaryTickFunction& ThisTickFunction) {
  if (!is_enabled_) {
    return;
  }

  if (visualize_tactile_feedback_) {
    visualizeTactileFeedbackOutput();
  }
}

void UHxPatchComponent::ignoreActor(AActor* actor) {
  if (IsValid(actor)) {
    object_trace_ignore_actors_.Add(actor);
  }
}

void UHxPatchComponent::updateTraceOrigins(USkeletalMeshComponent* skeletal_mesh,
    UStaticMesh* static_mesh) {
  if (!IsValid(skeletal_mesh) || !IsValid(skeletal_mesh->SkeletalMesh) ||
      static_mesh == nullptr || !IsValid(GetWorld())) {
    return;
  }

  // Generate a static mesh actor representing the skeletal mesh in its base pose.
  AStaticMeshActor* static_mesh_actor = GetWorld()->SpawnActor<AStaticMeshActor>(
      FVector::ZeroVector, FRotator::ZeroRotator, {});
  static_mesh_actor->SetActorScale3D(skeletal_mesh->GetComponentScale());
  UStaticMeshComponent* static_mesh_comp = static_mesh_actor->GetStaticMeshComponent();
  static_mesh_comp->SetMobility(EComponentMobility::Stationary);
  static_mesh_comp->SetStaticMesh(static_mesh);
  static_mesh_comp->SetCollisionObjectType(skeletal_mesh->GetCollisionObjectType());
  static_mesh_comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

  // Compute the world transform where this patch would be if it were attached to the static mesh.

  // The world transform of the root bone of the skeletal mesh.
  const FTransform w_root = static_mesh_actor->GetActorTransform();
  // The transform of the bone this patch is attached to relative to the root bone in the skeletal
  // mesh's base pose.
  FTransform l_attach_bone;
  if (!getRefPoseTransformRelativeToRoot(skeletal_mesh, GetAttachSocketName(), l_attach_bone)) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "UHxPatchComponent::updateTraceOrigins(): Invalid bone \"%s\" on skeletal mesh %s."),
        *GetAttachSocketName().ToString(), *skeletal_mesh->GetFullName()));
    return;
  }
  // The transform of this patch relative to its attach bone.
  const FTransform l_patch = GetRelativeTransform();
  // The world transform of this patch on the static mesh.
  const FTransform w_patch = l_patch * l_attach_bone * w_root;

  // Trace against the mesh from each tactor and cache the location that was hit. This is the
  // best approximation of where the "nerves" on the avatar hand are located that engage
  // corresponding tactors.
  FCollisionQueryParams query_params;
  query_params.bTraceComplex = true;
  for (auto& tactor_datum : tactor_data_) {
    tactor_datum.trace_origin_valid = false;

    if (tactor_datum.callbacks == nullptr) {
      AHxCoreActor::logError(FString::Printf(TEXT(
          "UHxPatchComponent::updateTraceOrigins(): Null callbacks for tactor %d."),
          tactor_datum.tactor.id));
      continue;
    }

    FTransform w_tactor = tactor_datum.callbacks->getUnrealLocalTransform() * w_patch;
    FVector w_tactor_position_cm = w_tactor.GetLocation();
    FVector w_trace_direction = -w_tactor.GetRotation().GetUpVector().GetSafeNormal();

    // Start with really short traces about the tactor origin and walk outward until we either
    // hit the hand or reach our max trace length.
    float trace_offset_cm = self_trace_increment_cm_;
    while (trace_offset_cm < max_self_trace_offset_cm_) {
      const FVector w_self_trace_start = w_tactor_position_cm +
          GetComponentScale() * trace_offset_cm * w_trace_direction;
      const FVector w_self_trace_end = w_tactor_position_cm -
          GetComponentScale() * trace_offset_cm * w_trace_direction;

      FHitResult hit;
      tactor_datum.trace_origin_valid = static_mesh_actor->ActorLineTraceSingle(hit,
          w_self_trace_start, w_self_trace_end, static_mesh_comp->GetCollisionObjectType(),
          query_params);
      if (tactor_datum.trace_origin_valid) {
        FTransform w_hit = FTransform(w_tactor.Rotator(), hit.Location, w_tactor.GetScale3D());
        GetAttachParent()->GetSocketTransform(GetAttachSocketName());
        tactor_datum.l_trace_origin = UKismetMathLibrary::ComposeTransforms(w_hit,
            w_patch.Inverse());
        break;
      } else {
        trace_offset_cm += self_trace_increment_cm_;
      }
    }
  }

  static_mesh_actor->Destroy();
}

void UHxPatchComponent::hardDisable() {
  AHxCoreActor::logRestartMessage();
  // Hard disable
  is_enabled_ = false;
  PrimaryComponentTick.SetTickFunctionEnable(false);
  SecondaryComponentTick.SetTickFunctionEnable(false);
}

void UHxPatchComponent::registerTactors() {
  if (!IsValid(hx_core_)) {
    AHxCoreActor::logError(TEXT(
        "UHxPatchComponent::registerTactors(): Null core."));
    hardDisable();
    return;
  }

  IPeripheralLink* peripheral_link_parent = Cast<IPeripheralLink>(GetOwner());
  if (peripheral_link_parent == nullptr) {
    AHxCoreActor::logError(TEXT(
        "UHxPatchComponent::registerTactors(): Owner doesn't implement IPeripheralLinkInterface."));
    hardDisable();
    return;
  }

  if (peripheral_link_parent->getPeripheral() == nullptr) {
    AHxCoreActor::logError(TEXT(
        "UHxPatchComponent::registerTactors(): Null peripheral from owner."));
    hardDisable();
    return;
  }
  peripheral_id_ = peripheral_link_parent->getPeripheral()->id;

  IHxPatchSocket* hx_patch_socket_parent = Cast<IHxPatchSocket>(GetOwner());
  if (hx_patch_socket_parent == nullptr) {
    AHxCoreActor::logError(TEXT(
        "UHxPatchComponent::registerTactors(): Owner doesn't implement IHxPatchSocketInterface."));
    hardDisable();
    return;
  }

  ERelativeDirection rel_dir;
  FName subbed_locating_feature = FName(*locating_feature_);
  TSet<FName> subbed_coverage_regions;
  if (hx_patch_socket_parent->tryGetRelativeDirection(&rel_dir)) {
    subbed_locating_feature = FName(*substituteRelDir(locating_feature_, rel_dir));
    for (auto& coverage_region : coverage_regions_) {
      subbed_coverage_regions.Add(FName(*substituteRelDir(coverage_region, rel_dir)));
    }
  }

  int64_t bone_ci_body_id = 0u;
  if (!hx_patch_socket_parent->tryGetCiBodyId(*this, &bone_ci_body_id)) {
    return;
  }

  for (auto it = peripheral_link_parent->getPeripheral()->tactors.begin();
      it != peripheral_link_parent->getPeripheral()->tactors.end(); it++) {
    if (subbed_locating_feature == HAPTX_NAME_TO_FNAME(it->parent) &&
        subbed_coverage_regions.Contains(HAPTX_NAME_TO_FNAME(it->coverage_region))) {
      std::shared_ptr<WeldedComponentCallbacks> callbacks =
          std::make_shared<WeldedComponentCallbacks>(this, NAME_None, unrealFromHx(it->transform));
      tactor_data_.emplace_back(*it, callbacks);

      hx_core_->getContactInterpreter().registerTactor(peripheral_id_, *it,
          tactor_parameters_.unwrap(), bone_ci_body_id, callbacks);
    }
  }
}

void UHxPatchComponent::configureTraceParameters() {
  // If we're using the object type whitelist
  if (hx_core_->allow_tactile_feedback_collision_type_whitelist_) {
    // Add each type from the whitelist to the query parameters
    for (auto type : hx_core_->tactile_feedback_collision_types_) {
      object_trace_collision_type_whitelist_.Add(UEngineTypes::ConvertToObjectType(type));
    }
  }
  else {
    for (EObjectTypeQuery type = ObjectTypeQuery1; type != ObjectTypeQuery_MAX; ++((int&)(type))) {
      object_trace_collision_type_whitelist_.Add(type);
    }
  }
}

void UHxPatchComponent::updateTraces() {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updateTraces)
  // The length and width [cm] to draw trace visualization boxes.
  const float L_TRACE_BOX_LENGTH_WIDTH_CM = 0.25f;
  // The thickness [cm] to draw trace visualization boxes.
  const float L_TRACE_BOX_THICKNESS_CM = 0.125f;
  // The thickness to draw traces.
  const float L_TRACE_THICKNESS_CM = 0.07f;
  // The color to draw with if we hit an object.
  FColor OBJECT_HIT_COLOR = DEBUG_PURPLE_OR_TEAL;
  // The color to draw with if we hit nothing.
  FColor OBJECT_MISS_COLOR = DEBUG_BLACK;
  // The color to draw the true tactor location.
  FColor TACTOR_COLOR = DEBUG_WHITE;
  // The radius to draw the sphere at the true tactor location.
  float TACTOR_RADIUS_CM = 0.1f;
  // The radius of sampling sphere traces in centimeters.
  // TODO: Make this a configurable property.
  const float SPHERE_TRACE_RADIUS_CM = 0.280625f;

  if (!IsValid(hx_core_)) {
    return;
  }

  // This array will hold the complete set of ray traces in world space.
  TArray<FVector> w_trace_directions;
  w_trace_directions.SetNum(l_object_trace_directions_.Num() + t_object_trace_directions_.Num());
  // Populate the component trace section of the array.
  for (int i = 0; i < l_object_trace_directions_.Num(); i++) {
    w_trace_directions[i] = GetComponentTransform().TransformVector(
        l_object_trace_directions_[i]).GetSafeNormal();
  }

  for (const auto& tactor_datum : tactor_data_) {
    const HaptxApi::Tactor& tactor = tactor_datum.tactor;

    if (!tactor_datum.trace_origin_valid) {
      continue;
    }

    // Cached trace origin information from self trace.
    const FTransform w_trace_origin = UKismetMathLibrary::ComposeTransforms(
        tactor_datum.l_trace_origin, GetComponentTransform());
    const FVector w_trace_origin_pos_cm = w_trace_origin.GetLocation();

    // Populate the tactor trace section of the array.
    for (int i = 0; i < t_object_trace_directions_.Num(); i++) {
      w_trace_directions[l_object_trace_directions_.Num() + i] =
          w_trace_origin.TransformVector(t_object_trace_directions_[i]).GetSafeNormal();
    }

    for (const auto& w_trace_direction : w_trace_directions) {
      // Object trace characterization.
      const FVector w_object_trace_start_cm = w_trace_origin_pos_cm -
          GetComponentScale() * w_trace_direction *
          (object_trace_offset_cm_ + SPHERE_TRACE_RADIUS_CM);
      const FVector w_object_trace_end_cm = w_trace_origin_pos_cm +
          object_trace_distance_cm_ * GetComponentScale() * w_trace_direction;
      const float scaled_sphere_trace_radius_cm = SPHERE_TRACE_RADIUS_CM *
          componentAverage(GetComponentScale());

      // Perform a sphere trace looking for nearby objects.
      int64_t object_id;
      bool object_hit = false;
      FHitResult object_trace_hit_result;
      if (UKismetSystemLibrary::SphereTraceSingleForObjects(this, w_object_trace_start_cm,
          w_object_trace_end_cm, scaled_sphere_trace_radius_cm, object_trace_collision_type_whitelist_,
          object_trace_complex_, object_trace_ignore_actors_,EDrawDebugTrace::None,
          object_trace_hit_result, true) &&
          hx_core_->tryRegisterObjectWithCi(object_trace_hit_result.Component.Get(),
          object_trace_hit_result.BoneName, false,  object_id)) {
        SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updateTraces_objectHit)
        object_hit = true;

        const float distance = FMath::Max(0.0f, FVector::DotProduct(
            object_trace_hit_result.ImpactPoint - w_trace_origin_pos_cm, w_trace_direction));
        std::vector<HaptxApi::Vector2D> uv_coordinates;
        uv_coordinates.reserve(object_trace_uv_channels_.Num());
        for (int32 uv_channel : object_trace_uv_channels_) {
          FVector2D uv = FVector2D::ZeroVector;
          if (UGameplayStatics::FindCollisionUV(object_trace_hit_result, uv_channel, uv)) {
            uv_coordinates.push_back(FVECTOR2D_TO_VECTOR2D(uv));
          }
        }
        hx_core_->getContactInterpreter().addSampleResult(
            peripheral_id_,
            tactor.id,
            hxFromUnrealVector(w_trace_direction),
            object_id,
            hxFromUnrealLength(distance),
            hxFromUnrealLength(object_trace_hit_result.Location),
            hxFromUnrealVector(object_trace_hit_result.Normal),
            uv_coordinates);

        if (visualize_traces_) {
          // Draw the point where the object was hit.
          HxDebugDrawSystem::sphere(
              GetOwner(),
              w_object_trace_start_cm + (object_trace_hit_result.Distance * w_trace_direction),
              scaled_sphere_trace_radius_cm, OBJECT_HIT_COLOR);
        }
      }

      if (visualize_traces_) {
        const FColor draw_color = object_hit ? OBJECT_HIT_COLOR : OBJECT_MISS_COLOR;

        // Draw a line representing the trace toward objects.
        HxDebugDrawSystem::line(GetOwner(), w_object_trace_start_cm, w_object_trace_end_cm,
            draw_color, L_TRACE_THICKNESS_CM);

        // Draw a sphere representing the trace origin.
        HxDebugDrawSystem::sphere(GetOwner(), w_trace_origin_pos_cm, scaled_sphere_trace_radius_cm,
            draw_color);

        if (tactor_datum.callbacks != nullptr) {
          const FTransform w_tactor = tactor_datum.callbacks->getUnrealWorldTransform();

          // Draw the true tactor location for completeness.
          HxDebugDrawSystem::sphere(GetOwner(), w_tactor.GetLocation(), TACTOR_RADIUS_CM,
              TACTOR_COLOR);
        }
      }
    }
  }
}

void UHxPatchComponent::visualizeTactileFeedbackOutput() {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeTactileFeedbackOutput)
  // The length [cm] and width [cm] to draw tactor visualization boxes.
  const float L_BOX_LENGTH_WIDTH_CM = 0.3f;
  // The thickness [cm] to draw tactor visualization boxes.
  const float L_BOX_THICKNESS_CM = 0.15f;
  // The color to draw tactor output visualizations when tactors are not engaged.
  const FColor COLOR_CLOSED = DEBUG_BLACK;
  // The color to draw tactor output visualizations when tactors are engaged, and at their 
  // lower height target bound.
  const FColor COLOR_OPEN_LOW = DEBUG_BLUE_OR_YELLOW;
  // The color to draw tactor output visualizations when tactors are open, and at their upper
  // height target bound. A LERP is applied between this value and TA_COLOR_OPEN_LOW based on
  // height target.
  const FColor COLOR_OPEN_HIGH = DEBUG_PURPLE_OR_TEAL;

  IHxPatchSocket* hx_patch_socket_parent = Cast<IHxPatchSocket>(GetOwner());
  if (!IsValid(hx_core_) || hx_patch_socket_parent == nullptr) {
    return;
  }

  // Draw tactor output visualizations offset from tactor positions so that they can be seen even
  // during contact.
  FVector w_offset_direction = GetComponentTransform().GetRotation().GetAxisZ();
  FVector w_tactor_viz_offset_cm = GetComponentScale() *
      tactile_feedback_visualization_parameters_.draw_offset * w_offset_direction;
  for (const auto& tactor_datum : tactor_data_) {
    const HaptxApi::Tactor& tactor = tactor_datum.tactor;

    if (tactor_datum.callbacks == nullptr) {
      continue;
    }
    FTransform w_tactor_transform = tactor_datum.callbacks->getUnrealWorldTransform();

    // Offset position and color based on height target.
    float tactor_height_target_cm = 0.0f;
    if (hx_core_->getContactInterpreter().tryGetTactorHeightTargetM(peripheral_id_, tactor.id,
        &tactor_height_target_cm)) {
      tactor_height_target_cm = unrealFromHxLength(FMath::Clamp(tactor_height_target_cm,
          tactor.height_min_m, tactor.height_max_m));
    }
    FVector w_tactor_viz_height_offset_cm = GetComponentScale() *
        tactile_feedback_visualization_parameters_.height_target_scale *
        tactor_height_target_cm * w_tactor_transform.GetRotation().GetAxisZ();
    FColor tactor_viz_color = COLOR_CLOSED;
    if (tactor_height_target_cm > 0.0f) {
      tactor_viz_color = FMath::Lerp(
          FLinearColor(COLOR_OPEN_LOW),
          FLinearColor(COLOR_OPEN_HIGH),
          tactor_height_target_cm / unrealFromHxLength(tactor.height_max_m)).
          ToFColor(false);
    }

    HxDebugDrawSystem::box(
        GetOwner(),
        w_tactor_transform.GetLocation() + w_tactor_viz_offset_cm + w_tactor_viz_height_offset_cm,
        GetComponentScale() *
          FVector(L_BOX_LENGTH_WIDTH_CM, L_BOX_LENGTH_WIDTH_CM, L_BOX_THICKNESS_CM) / 2.0f,
        w_tactor_transform.GetRotation(),
        tactor_viz_color);
  }
}

bool UHxPatchComponent::connectToCore() {
  // Make sure there's a HaptxApi::Core in the level, and make sure it has tried to open
  hx_core_ = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  // If there's no core in the world that has successfully opened all interfaces now
  if (hx_core_ == nullptr) {
    hardDisable();
    return false;
  }
  return true;
}

void UHxPatchComponent::alignWithLocatingSocket() {
  IHxPatchSocket* hx_patch_socket_parent = Cast<IHxPatchSocket>(GetOwner());
  if (hx_patch_socket_parent == nullptr) {
    AHxCoreActor::logError(TEXT(
        "UHxPatchComponent::alignWithLocatingSocket(): Owner doesn't implement IHxPatchSocketInterface."));
    hardDisable();
    return;
  }

  ERelativeDirection rel_dir;
  FName locating_feature = FName(*locating_feature_);
  if (hx_patch_socket_parent->tryGetRelativeDirection(&rel_dir)) {
    locating_feature = FName(*substituteRelDir(locating_feature_, rel_dir));
  }

  FTransform w_locating_feature;
  if (!hx_patch_socket_parent->tryGetLocatingFeatureTransform(locating_feature,
      &w_locating_feature)) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "UHxPatchComponent::alignWithLocatingSocket(): Failed to get transform for locating feature %s."),
        *locating_feature.ToString()));
    hardDisable();
    return;
  }

  UPrimitiveComponent* parent_component = Cast<UPrimitiveComponent>(GetAttachParent());
  if (!IsValid(parent_component)) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "UHxPatchComponent::alignWithLocatingSocket(): Invalid parent component."),
        *locating_feature.ToString()));
    hardDisable();
    return;
  }

  // Align with locating socket.
  FTransform w_parent_socket = parent_component->GetSocketTransform(GetAttachSocketName(),
      ERelativeTransformSpace::RTS_World);
  FTransform l_locating_feature = UKismetMathLibrary::ComposeTransforms(
      w_locating_feature,
      w_parent_socket.Inverse());

  FTransform l_patch = UKismetMathLibrary::ComposeTransforms(locating_feature_offset_,
      l_locating_feature);
  SetRelativeTransform(l_patch);
}
