// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_hand_actor.h>
#include <vector>
#include <Runtime/Core/Public/Modules/ModuleManager.h>
#include <Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h>
#include <Runtime/Engine/Classes/Engine/SkeletalMeshSocket.h>
#include <Runtime/Engine/Classes/GameFramework/GameStateBase.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsAsset.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintTemplate.h>
#include <Runtime/Engine/Public/DrawDebugHelpers.h>
#include <Runtime/Engine/Public/Net/UnrealNetwork.h>
#include <Runtime/Engine/Public/PhysicsReplication.h>
#include <Runtime/HeadMountedDisplay/Public/IHeadMountedDisplay.h>
#include <Runtime/HeadMountedDisplay/Public/IHeadMountedDisplayModule.h>
#include <HaptxApi/avatar_animation_optimizer.h>
#include <HaptxApi/default_hand_ik.h>
#include <HaptxApi/names.h>
#include <HaptxApi/openvr_wrapper.h>
#include <HaptxApi/simulated_peripheral_database.h>
#include <HaptxApi/thimble_compensator.h>
#include <HaptxApi/user_profile_database.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/hx_core_actor.h>
#include <Haptx/Public/hx_on_screen_log.h>
#include <Haptx/Public/hx_patch_component.h>
#include <Haptx/Public/hx_physical_material.h>

FOnLeftHandInitialized AHxHandActor::on_left_hand_initialized;
FOnRightHandInitialized AHxHandActor::on_right_hand_initialized;
TWeakObjectPtr<AHxHandActor> AHxHandActor::left_hand_;
TWeakObjectPtr<AHxHandActor> AHxHandActor::right_hand_;
TMap<UPrimitiveComponent*, FGlobalPhysicsAuthorityObjectData>
    AHxHandActor::global_physics_authority_data_from_comp_;

DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::TickPrimary()"),
    STAT_TickPrimary, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::TickSecondary()"),
    STAT_TickSecondary, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::updateHandAnimation()"),
    STAT_updateHandAnimation, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::NotifyHit()"),
    STAT_NotifyHit, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::NotifyHit() - CI"),
    STAT_NotifyHit_CI, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::NotifyHit() - GD"),
    STAT_NotifyHit_GD, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::visualizeForceFeedbackOutput()"),
    STAT_visualizeForceFeedbackOutput, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::visualizeMocapData()"),
    STAT_visualizeMocapData, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::visualizeHandAnimation()"),
    STAT_visualizeHandAnimation, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::visualizeHandAnimation2()"),
    STAT_visualizeHandAnimation2, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::updatePhysicsState()"),
    STAT_updatePhysicsState, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::updateReplicatedConstraints()"),
    STAT_updateReplicatedConstraints, STATGROUP_AHxHandActor)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("AHxHandActor::setLocalConstraintsPhysicallyEnabled()"),
    STAT_setLocalConstraintsPhysicallyEnabled, STATGROUP_AHxHandActor)

#define HAND_AS_TEXT (hand_ == ERelativeDirection::LEFT ? TEXT("left") : TEXT("right"))

void FHxHandSecondaryTickFunction::ExecuteTick(
    float DeltaTime,
    ELevelTick TickType,
    ENamedThreads::Type CurrentThread,
    const FGraphEventRef& CompletionGraphEvent) {
  if (Target && !Target->IsPendingKill() && !Target->IsUnreachable()) {
    FScopeCycleCounterUObject ActorScope(Target);
    Target->TickSecondary(DeltaTime, TickType, *this);
  }
}

FString FHxHandSecondaryTickFunction::DiagnosticMessage() {
  return Target->GetFullName() + TEXT("AHxHandActor Secondary Tick.");
}

AHxHandActor::AHxHandActor(const FObjectInitializer& object_initializer) :
    ASkeletalMeshActor(object_initializer), hand_(ERelativeDirection::LEFT),
    hand_scale_factor_(1.f), enable_contact_damping_(true), linear_contact_damping_(300.0f),
    angular_contact_damping_(30.0f), enable_glove_slip_compensation_(true),
    glove_slip_compensation_parameters_(), enable_thimble_compensation_(true),
    thimble_compensation_parameters_(), enable_corrective_teleportation_(true),
    corrective_teleportation_distance_(50.0f), mocap_origin_(nullptr),
    simulated_animation_aggressiveness_1_s(10.0f), visualize_force_feedback_(false),
    force_feedback_visualization_parameters_(), visualize_motion_capture_(false),
    visualize_hand_animation_(false), visualize_hand_animation_2_(false), dis_vis_parameters_(),
    physics_targets_transmission_frequency_hz_(50.0f), physics_targets_buffer_duration_s_(0.05f),
    physics_state_transmission_frequency_hz_(50.0f), physics_state_buffer_duration_s_(0.05f),
    physics_authority_zone_radius_hysteresis_(0.1f),
    visualize_displacement_(true), toggle_dis_vis_action_(TEXT("HxToggleDisplacementVis")),
    toggle_mocap_vis_action_(TEXT("HxToggleMocapVis")),
    toggle_trace_vis_action_(TEXT("HxToggleTraceVis")),
    toggle_tactile_feedback_vis_action_(TEXT("HxToggleTactileFeedbackVis")),
    toggle_force_feedback_vis_action_(TEXT("HxToggleForceFeedbackVis")),
    toggle_hand_animation_vis_action_(TEXT("HxToggleHandAnimationVis")),
    toggle_hand_animation_vis_2_action_(TEXT("HxToggleHandAnimationVis2")), bone_names_(),
    is_enabled_(true), is_input_bound_(false), is_client_physics_authority_(true),
    local_constraints_need_disabled_(false),
    l_middle1_cm_(FVector::ZeroVector), palm_constraint_(nullptr), joints_(),
    contacting_object_ids_(), damping_constraint_from_object_id_(), palm_extent_(0.0f),
    actors_to_ignore_(), physics_state_(), replicated_constraints_(), local_constraints_(),
    PhysicsAuthorityZone(nullptr), objects_in_physics_authority_zone_(),
    comps_that_need_replication_targets_removed_(), num_physics_authority_zone_overlaps_(0),
    physics_targets_buffer_tail_i_(0), physics_targets_buffer_head_i_(0),
    physics_targets_buffer_started_(false), physics_state_buffer_tail_i_(0),
    physics_state_buffer_head_i_(0),
    physics_state_buffer_started_(false), time_of_last_physics_transmission_s_(0.0f),
    follow_time_s_(0.0f), physics_authority_zone_radius_enlarged_cm_(0.0f),
    physics_authority_zone_radius_nominal_cm_(0.0f), w_uhp_hand_scale_factor_(1.f),
    hand_needs_scale_update_(false), recently_warned_about_tracking_ref_being_off_(false),
    first_tick_has_happened_(false), palm_needs_first_teleport_(true), dis_vis_pmc_(nullptr),
    dis_vis_mat_inst_(nullptr), finger_body_parameters_(), palm_body_parameters_(),
    retractuator_parameters_(), bone_data_from_bone_name_(), hx_core_(nullptr),
    gesture_(HaptxApi::Gesture::PRECISION_GRASP), last_simulated_anim_frame_(), mocap_system_(),
    glove_slip_compensator_(), glove_(nullptr),
    hand_joint_bone_names_{
    {bone_names_.thumb1, bone_names_.thumb2, bone_names_.thumb3},
    {bone_names_.index1, bone_names_.index2, bone_names_.index3},
    {bone_names_.middle1, bone_names_.middle2, bone_names_.middle3},
    {bone_names_.ring1, bone_names_.ring2, bone_names_.ring3},
    {bone_names_.pinky1, bone_names_.pinky2, bone_names_.pinky3}},
    hand_bone_names_{
    {bone_names_.thumb1, bone_names_.thumb2, bone_names_.thumb3},
    {bone_names_.index1, bone_names_.index2, bone_names_.index3},
    {bone_names_.middle1, bone_names_.middle2, bone_names_.middle3},
    {bone_names_.ring1, bone_names_.ring2, bone_names_.ring3},
    {bone_names_.pinky1, bone_names_.pinky2, bone_names_.pinky3}},
    fingertip_names_{bone_names_.thumb4, bone_names_.index4, bone_names_.middle4,
    bone_names_.ring4, bone_names_.pinky4},
    touch_pad_was_pressed_(false), pawn_(nullptr), 
    last_openvr_wrapper_return_code_(HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS) {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = false;
  PrimaryActorTick.TickGroup = TG_PrePhysics;
  SecondaryTick.bCanEverTick = true;
  SecondaryTick.bStartWithTickEnabled = false;
  SecondaryTick.TickGroup = TG_PostUpdateWork;
  bReplicates = true;

  linear_drive_.bEnablePositionDrive = true;
  linear_drive_.bEnableVelocityDrive = true;
  linear_drive_.Stiffness = 1.1e5f;
  linear_drive_.Damping = 1.1e3f;
  linear_drive_.MaxForce = 1.1e4f;
  angular_drive_.bEnablePositionDrive = true;
  angular_drive_.bEnableVelocityDrive = true;
  angular_drive_.Stiffness = 6.3e7f;
  angular_drive_.Damping = 6.3e5f;
  angular_drive_.MaxForce = 1.1e6f;

  female_left_hand_skeletal_mesh_ = ConstructorHelpers::FObjectFinder<USkeletalMesh>(
      TEXT("/haptx/HaptXHand/Meshes/female_left_unreal_hand")).Object;
  female_left_hand_static_mesh_ = ConstructorHelpers::FObjectFinder<UStaticMesh>(
      TEXT("/haptx/HaptXHand/Meshes/female_left_unreal_hand_static")).Object;
  female_right_hand_skeletal_mesh_ = ConstructorHelpers::FObjectFinder<USkeletalMesh>(
      TEXT("/haptx/HaptXHand/Meshes/female_right_unreal_hand")).Object;
  female_right_hand_static_mesh_ = ConstructorHelpers::FObjectFinder<UStaticMesh>(
      TEXT("/haptx/HaptXHand/Meshes/female_right_unreal_hand_static")).Object;
  male_left_hand_skeletal_mesh_ = ConstructorHelpers::FObjectFinder<USkeletalMesh>(
      TEXT("/haptx/HaptXHand/Meshes/male_left_unreal_hand")).Object;
  male_left_hand_static_mesh_ = ConstructorHelpers::FObjectFinder<UStaticMesh>(
      TEXT("/haptx/HaptXHand/Meshes/male_left_unreal_hand_static")).Object;
  male_right_hand_skeletal_mesh_ = ConstructorHelpers::FObjectFinder<USkeletalMesh>(
      TEXT("/haptx/HaptXHand/Meshes/male_right_unreal_hand")).Object;
  male_right_hand_static_mesh_ = ConstructorHelpers::FObjectFinder<UStaticMesh>(
      TEXT("/haptx/HaptXHand/Meshes/male_right_unreal_hand_static")).Object;
  light_female_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/light_female_hand")).Object;
  medium_female_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/medium_female_hand")).Object;
  dark_female_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/dark_female_hand")).Object;
  neutral_female_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/neutral_hand")).Object;
  light_male_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/light_male_hand")).Object;
  medium_male_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/medium_male_hand")).Object;
  dark_male_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/dark_male_hand")).Object;
  neutral_male_hand_material_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/neutral_hand")).Object;

  dis_vis_mat_ = ConstructorHelpers::FObjectFinder<UMaterialInterface>(
      TEXT("/haptx/HaptXHand/Materials/displacement_visualizer")).Object;

  palm_body_parameters_.compliance_cm_cn *= 2.0f;

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (IsValid(smc)) {
    smc->SetSimulatePhysics(true);
    smc->BodyInstance.bNotifyRigidBodyCollision = true;
    smc->SetCollisionObjectType(ECC_Pawn);
    smc->GlobalCreatePhysicsDelegate.AddUObject(this, &AHxHandActor::OnGlobalCreatePhysics);
    smc->SkeletalMesh = hand_ == ERelativeDirection::LEFT ?
        male_left_hand_skeletal_mesh_ : male_right_hand_skeletal_mesh_;
    smc->SetMaterial(0, neutral_male_hand_material_);
  }

  PhysicsAuthorityZone = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsAuthorityZone"));
  PhysicsAuthorityZone->SetupAttachment(smc, bone_names_.middle1);
  PhysicsAuthorityZone->SetRelativeTransform(FTransform::Identity);
  PhysicsAuthorityZone->SetSphereRadius(15.0f);
  PhysicsAuthorityZone->SetCollisionObjectType(ECC_GameTraceChannel18);
  PhysicsAuthorityZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  PhysicsAuthorityZone->SetGenerateOverlapEvents(true);
  PhysicsAuthorityZone->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
  PhysicsAuthorityZone->SetCollisionResponseToChannel(ECC_GameTraceChannel18,
      ECollisionResponse::ECR_Overlap);
  PhysicsAuthorityZone->OnComponentBeginOverlap.AddDynamic(this,
      &AHxHandActor::onPhysicsAuthorityZoneBeginOverlap);
  PhysicsAuthorityZone->OnComponentEndOverlap.AddDynamic(this,
      &AHxHandActor::onPhysicsAuthorityZoneEndOverlap);
}

void AHxHandActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AHxHandActor, hand_);
  DOREPLIFETIME(AHxHandActor, hand_scale_factor_);
  DOREPLIFETIME(AHxHandActor, w_uhp_hand_scale_factor_);
  DOREPLIFETIME(AHxHandActor, is_client_physics_authority_);
  DOREPLIFETIME(AHxHandActor, physics_targets_transmission_frequency_hz_);
  DOREPLIFETIME(AHxHandActor, physics_state_transmission_frequency_hz_);
  DOREPLIFETIME(AHxHandActor, mocap_origin_);
}

void AHxHandActor::OnGlobalCreatePhysics(UActorComponent* component) {
  if (!IsValid(GetWorld()) || !GetWorld()->IsGameWorld()) {
    return;
  }

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc) || smc != component) {
    return;
  }

  FBodyInstance* palm_inst = smc->GetBodyInstance(bone_names_.palm);
  if (palm_inst != nullptr) {
    palm_extent_ = palm_inst->GetBodyBounds().GetExtent().Size();
  }

  const FRotator w_palm_rot = smc->GetSocketRotation(bone_names_.palm);
  const FVector w_middle1_pos_cm = smc->GetSocketLocation(bone_names_.middle1);
  const FVector w_palm_pos_cm = smc->GetSocketLocation(bone_names_.palm);
  l_middle1_cm_ = w_palm_rot.GetInverse().RotateVector(w_middle1_pos_cm - w_palm_pos_cm);

  for (int f_i = 0; f_i < HaptxApi::F_LAST; f_i++) {
    for (int fj_i = 0; fj_i < HaptxApi::FJ_LAST; fj_i++) {
      joints_[f_i][fj_i] = smc->FindConstraintInstance(hand_joint_bone_names_[f_i][fj_i]);
      if (joints_[f_i][fj_i] == nullptr) {
        UE_LOG(HaptX, Error,
          TEXT("There is no constraint associated with bone \"%s\" in the %s hand physics asset."),
          *hand_joint_bone_names_[f_i][fj_i].ToString(), HAND_AS_TEXT);
        hardDisable();
        return;
      }
    }
  }

  // Update the transforms of any child HxPatchComponents to reflect our real-world hand size.
  TArray<UActorComponent*> patches = GetComponentsByClass(UHxPatchComponent::StaticClass());
  for (int i = 0; i < patches.Num(); i++) {
    UHxPatchComponent* patch = Cast<UHxPatchComponent>(patches[i]);
    if (patch && patch->GetAttachParent() == smc) {
      patch->SetRelativeScale3D(FVector::OneVector / w_uhp_hand_scale_factor_);
      if (isLocallyControlled()) {
        patch->updateTraceOrigins(GetSkeletalMeshComponent(), static_mesh_);
      }
    }
  }

  initPalmConstraint();

  registerBones();

  if (isLocallyControlled()) {
    uninitializeDisplacementVisualizer();
    if (visualize_displacement_) {
      initializeDisplacementVisualizer();
    }
  }

  // ###############################################################################################
  // # Update the avatar profile to match the exact dimensions of the avatar hand (which may be
  // # significantly different than the user's hand).
  // ###############################################################################################
  HaptxApi::RelativeDirection rel_dir = static_cast<HaptxApi::RelativeDirection>(hand_);
  HaptxApi::RelativeDirection other_rel_dir = rel_dir == HaptxApi::RelativeDirection::RD_LEFT ?
      HaptxApi::RelativeDirection::RD_RIGHT : HaptxApi::RelativeDirection::RD_LEFT;

  // This is the measured location of the user's MCP3 joint relative to the hardware idealized MCP3
  // location. We keep using it so the avatar palm stays perfectly aligned with the user's palm.
  HaptxApi::Vector3D mcp3_mcp3 =
      user_profile_.mcp3_joint1_pos_offsets_m[rel_dir][HaptxApi::Finger::F_MIDDLE];

  FVector w_mcp3 = smc->GetSocketLocation(bone_names_.middle1);
  // MCP3's rotation changes with hand animation so we use the palm's rotation instead.
  FRotator palm_world = smc->GetSocketRotation(bone_names_.palm).GetInverse();
  float w_comp_scale = smc->GetComponentScale().X;
  if (w_comp_scale == 0.0f) {
    UE_LOG(HaptX, Error, TEXT("AHxHandActor::OnGlobalCreatePhysics(): Component scale cannot be zero."))
  } else if (w_uhp_hand_scale_factor_ == 0.0f) {
    UE_LOG(HaptX, Error, TEXT("AHxHandActor::OnGlobalCreatePhysics(): User profile scale cannot be zero."))
  } else {
    // Used to filter out component scale.
    float scale_factor = w_uhp_hand_scale_factor_ / w_comp_scale;
    for (int f_i = 0; f_i < static_cast<int>(HaptxApi::Finger::F_LAST); f_i++) {
      HaptxApi::Finger f = static_cast<HaptxApi::Finger>(f_i);

      // Compute the location of joint1 relative to MCP3 in world space.
      FVector w_joint1 =  smc->GetSocketLocation(
          hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT1]);
      FVector mcp3_joint1 = palm_world.RotateVector((w_joint1 - w_mcp3) / scale_factor);

      avatar_profile_.mcp3_joint1_pos_offsets_m[rel_dir][f_i] = hxFromUnrealLength(mcp3_joint1) +
          mcp3_mcp3;
      avatar_profile_.mcp3_joint1_pos_offsets_m[other_rel_dir][f_i] =
          avatar_profile_.mcp3_joint1_pos_offsets_m[rel_dir][f_i].scale({1.0f, -1.0f, 1.0f});

      avatar_profile_.finger_bone_lengths_m[f_i][HaptxApi::FingerBone::FB_PROXIMAL] =
          (1.0f / scale_factor) * hxFromUnrealLength(FVector::Distance(
          smc->GetSocketLocation(hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT2]),
          smc->GetSocketLocation(hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT1])));
      avatar_profile_.finger_bone_lengths_m[f_i][HaptxApi::FingerBone::FB_MEDIAL] =
          (1.0f / scale_factor) * hxFromUnrealLength(FVector::Distance(
          smc->GetSocketLocation(hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT3]),
          smc->GetSocketLocation(hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT2])));
      avatar_profile_.finger_bone_lengths_m[f_i][HaptxApi::FingerBone::FB_DISTAL] =
          (1.0f / scale_factor) * hxFromUnrealLength(FVector::Distance(
          smc->GetSocketLocation(fingertip_names_[f_i]),
          smc->GetSocketLocation(hand_joint_bone_names_[f_i][HaptxApi::FingerJoint::FJ_JOINT3])));
    }
  }
}

void AHxHandActor::BeginPlay() {
  Super::BeginPlay();

  if (!is_enabled_) {
    return;
  }

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    AHxCoreActor::logError(TEXT("Invalid skeletal mesh component, disabling myself."), true);
    hardDisable();
    return;
  }

  if (!connectToCore()) {
    AHxCoreActor::logError(TEXT("AHxHandActor::BeginPlay(): Failed to connect to core."), true);
    hardDisable();
    return;
  }

  // Configure this hand to ignore other hands under the same pawn. This prevents hands from
  // feeling themselves and their counterparts, as well as prevents unwanted grasps from forming.
  actors_to_ignore_.Add(this);
  TArray<AActor*> actors;
  UGameplayStatics::GetAllActorsOfClass(this, AHxHandActor::StaticClass(), actors);
  TArray<UActorComponent*> my_patches = GetComponentsByClass(UHxPatchComponent::StaticClass());
  for (auto& actor : actors) {
    AHxHandActor* other_hand = Cast<AHxHandActor>(actor);
    if (IsValid(other_hand) && other_hand != this && other_hand->pawn_ == pawn_) {
      // Configure this hand to ignore the other hand.
      actors_to_ignore_.Add(other_hand);
      for (auto& my_comp : my_patches) {
        UHxPatchComponent* my_patch = Cast<UHxPatchComponent>(my_comp);
        if (IsValid(my_patch)) {
          my_patch->ignoreActor(other_hand);
        }
      }

      // Configure the other hand to ignore this hand.
      other_hand->actors_to_ignore_.Add(this);
      TArray<UActorComponent*> other_patches =
          other_hand->GetComponentsByClass(UHxPatchComponent::StaticClass());
      for (auto& other_comp : other_patches) {
        UHxPatchComponent* other_patch = Cast<UHxPatchComponent>(other_comp);
        if (IsValid(other_patch)) {
          other_patch->ignoreActor(this);
        }
      }
    }
  }

  if (isLocallyControlled()) {
    if (checkForDuplicateHands()) {
      AHxCoreActor::logError(FString::Printf(
          TEXT("Multiple hands claim to be the %s hand, disabling myself."), HAND_AS_TEXT), true);
      hardDisable();
      return;
    }

    loadUserProfile();
    registerRetractuators();
  }

  physics_authority_zone_radius_enlarged_cm_ = (1.0f + physics_authority_zone_radius_hysteresis_) *
      PhysicsAuthorityZone->GetUnscaledSphereRadius();
  physics_authority_zone_radius_nominal_cm_ = PhysicsAuthorityZone->GetUnscaledSphereRadius();

  if (!IsTemplate()) {
    SecondaryTick.Target = this;
    SecondaryTick.SetTickFunctionEnable(SecondaryTick.bStartWithTickEnabled);
    SecondaryTick.RegisterTickFunction(GetLevel());
  }

  if (is_enabled_) {
    PrimaryActorTick.AddPrerequisite(hx_core_, hx_core_->GlobalFirstTick);
    PrimaryActorTick.SetTickFunctionEnable(true);
    SecondaryTick.AddPrerequisite(hx_core_, hx_core_->GlobalFirstTick);
    SecondaryTick.SetTickFunctionEnable(true);
  }
}

void AHxHandActor::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_TickPrimary)

  if (!is_enabled_) {
    return;
  }

  // Ideally we could call this in an event-like fashion, but doing it here in Tick() is the only
  // reliable method we've discovered to make sure the hand scales correctly during initial
  // replication.
  if (hand_needs_scale_update_) {
    updateHandScale();
    hand_needs_scale_update_ = false;
  }

  // We can't do this in on-overlap code since movement replication can cause overlaps, leading to
  // modification of data structures that are being iterated over.
  if (comps_that_need_replication_targets_removed_.Num() > 0) {
    UWorld* world = GetWorld();
    if (IsValid(world)) {
      FPhysScene* phys_scene = world->GetPhysicsScene();
      if (phys_scene != nullptr) {
        FPhysicsReplication* phys_rep = phys_scene->GetPhysicsReplication();
        if (phys_rep != nullptr) {
          for (auto& comp : comps_that_need_replication_targets_removed_) {
            if (IsValid(comp)) {
              phys_rep->RemoveReplicatedTarget(comp);
            }
          }
          comps_that_need_replication_targets_removed_.Empty();
        }
      }
    }
  }

  if (isLocallyControlled()) {
    // Configuration that happens only once per session and that has to happen after BeginPlay()
    if (!first_tick_has_happened_) {
      first_tick_has_happened_ = true;

      bindInput();

      if (hand_ == ERelativeDirection::LEFT) {
        left_hand_ = TWeakObjectPtr<AHxHandActor>(this);
        on_left_hand_initialized.Broadcast(this);
      }
      else {
        right_hand_ = TWeakObjectPtr<AHxHandActor>(this);
        on_right_hand_initialized.Broadcast(this);
      }
    }

    updateHandAnimation(DeltaTime);

    if (visualize_hand_animation_2_) {
      visualizeHandAnimation2();
    }
  }

  if (isAuthoritative() && !isLocallyControlled() && isPhysicsAuthority()) {
    // The server being authoritative over a hand being driven by a client.
    interpolatePhysicsTargets(DeltaTime);
  } else if (!isPhysicsAuthority()) {
    // Any other hand that isn't a physics authority.
    interpolatePhysicsState(DeltaTime);
  }
}

void AHxHandActor::TickSecondary(
    float DeltaTime,
    ELevelTick TickType,
    FHxHandSecondaryTickFunction& ThisTickFunction) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_TickSecondary)
  if (!is_enabled_) {
    return;
  }

  // Remove any contact damping constraints that should no longer exist.
  TArray<int64> keys_to_remove;
  for (auto& key_value : damping_constraint_from_object_id_) {
    if (!contacting_object_ids_.Contains(key_value.Key)) {
      keys_to_remove.Add(key_value.Key);
    }
  }
  for (int64 key : keys_to_remove) {
    UPhysicsConstraintComponent** constraint_ptr_ptr =
        damping_constraint_from_object_id_.Find(key);
    if (constraint_ptr_ptr != nullptr) {
      notifyLocalConstraintDestroyed(*constraint_ptr_ptr);
      destroyPhysicsConstraintComponent(*constraint_ptr_ptr);
      damping_constraint_from_object_id_.Remove(key);
    }
  }
  contacting_object_ids_.Empty();

  // Server only code.
  if (isAuthoritative()) {
    serverUpdatePhysicsAuthority();
  }

  if (isPhysicsAuthority()) {
    AGameStateBase* game_state = UGameplayStatics::GetGameState(GetWorld());
    if (IsValid(game_state)) {
      float time_s = game_state->GetServerWorldTimeSeconds();
      float period_s = 1.0f / physics_state_transmission_frequency_hz_;
      // Insert a half period of offset for left hands to stagger messages.
      if (time_s - time_of_last_physics_transmission_s_ -
          (hand_ == ERelativeDirection::LEFT ? 0.5f * period_s : 0.0f) > period_s) {
        getRigidBodyStates(GetSkeletalMeshComponent(), physics_state_.w_body_states);
        getPhysicsStatesOfObjectsInAuthorityZone(physics_state_.w_object_states);
        getLocalConstraintStates(physics_state_.constraint_states);
        if (isAuthoritative()) {
          multicastUpdatePhysicsState(time_s, physics_state_);
        } else {
          serverUpdatePhysicsState(time_s, physics_state_);
        }
        time_of_last_physics_transmission_s_ = time_s;
      }
    }
  }

  if (isLocallyControlled()) {
    if (visualize_force_feedback_) {
      visualizeForceFeedbackOutput();
    }

    if (isDisplacementVisualizerActive()) {
      updateDisplacementVisualizer(DeltaTime);
    }

    if (visualize_contact_damping_) {
      visualizeContactDamping();
    }
  }

  if (IsValid(hx_core_) && hx_core_->visualize_network_state_) {
    visualizeNetworkState();
  }
}

void AHxHandActor::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
    UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal,
    FVector NormalImpulse, const FHitResult& Hit) {
  Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse,
      Hit);
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_NotifyHit)

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!is_enabled_ || !IsValid(hx_core_) || !IsValid(MyComp)|| MyComp != smc ||
      !IsValid(OtherComp) || actors_to_ignore_.Contains(Other)) {
    return;
  }
  FHxHandActorBoneData* bone_data = bone_data_from_bone_name_.Find(Hit.MyBoneName);

  HaptxApi::Vector3D normal_impulse_ns =
      hxFromUnrealLength(NormalImpulse.Size() * Hit.ImpactNormal);  // Removes direction ambiguity.
  if (isLocallyControlled()) {
    // Register contact with CI.
    int64_t ci_object_id;
    if (bone_data == nullptr || !bone_data->has_ci_body_id) {
      UE_LOG(HaptX, Error,
          TEXT("AHxHandActor::NotifyHit: Bone not registered with CI %s."),
          *Hit.MyBoneName.ToString())
    } else if (!hx_core_->tryRegisterObjectWithCi(OtherComp, Hit.BoneName, false, ci_object_id)) {
      UE_LOG(HaptX, Error,
          TEXT("AHxHandActor::NotifyHit: Object not registered with CI %s:%s."),
          *OtherComp->GetName(), *Hit.BoneName.ToString())
    } else {
      SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_NotifyHit_CI)
      hx_core_->getContactInterpreter().addContact(
          ci_object_id,
          bone_data->ci_body_id,
          normal_impulse_ns);
    }
  }

  // Register contact with GD.
  int64_t gd_object_id;
  if (bone_data == nullptr || !bone_data->has_gd_body_id) {
    UE_LOG(HaptX, Error,
        TEXT("AHxHandActor::NotifyHit: Bone not registered with GD %s."),
        *Hit.MyBoneName.ToString())
  } else if (!hx_core_->tryRegisterObjectWithGd(OtherComp, Hit.BoneName, false, gd_object_id)) {
    UE_LOG(HaptX, Error,
        TEXT("AHxHandActor::NotifyHit: Object not registered with GD %s:%s."),
        *OtherComp->GetName(), *Hit.BoneName.ToString())
  } else {
    SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_NotifyHit_GD)
    HaptxApi::GraspDetector::GraspContactInfo gci;
    gci.object_id = gd_object_id;
    gci.grasp_body_id = bone_data->gd_body_id;
    gci.contact_location = hxFromUnrealLength(Hit.Location);
    gci.impulse = normal_impulse_ns;
    hx_core_->getGraspDetector().addGraspContact(gci);
  }

  // Damp the motion of objects in the palm to help with holding.
  if (bone_data != nullptr && bone_data->can_engage_contact_damping) {
    FBodyInstance* other_inst = getBodyInstance(OtherComp, Hit.BoneName);
    if (other_inst != nullptr && other_inst->bSimulatePhysics) {
      UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(
          other_inst->GetSimplePhysicalMaterial());
      bool contact_damping_enabled = (IsValid(hx_phys_mat) &&
          hx_phys_mat->override_contact_damping_enabled_) ? hx_phys_mat->contact_damping_enabled_ :
          enable_contact_damping_;
      FBodyInstance* palm_inst = smc->GetBodyInstance(bone_names_.palm);
      if (contact_damping_enabled && palm_inst != nullptr) {
        // The color to draw the contact damping trace if it hits the hand.
        static const FColor HAND_HIT_COLOR = DEBUG_PURPLE_OR_TEAL;
        // The color to draw the contact damping trace if it misses the hand.
        static const FColor HAND_MISSED_COLOR = DEBUG_RED_OR_GREEN;
        // How thick to draw the contact damping trace.
        static const float L_TRACE_THICKNESS_CM = 0.07f;
        // The radius of the sphere to draw at the contact damping trace hit location
        // (assuming it hits).
        static const float L_HIT_RADIUS_CM = 0.2f;

        // Only allow contact damping if the hand is "below" the object in the context of gravity.
        FVector w_trace_begin_cm = other_inst->GetCOMPosition();
        FVector w_trace_end_cm = w_trace_begin_cm -
            FVector::UpVector * (FMath::Abs(FVector::DotProduct(
            w_trace_begin_cm - palm_inst->GetCOMPosition(), FVector::UpVector)) + palm_extent_);

        bool hit_hand = false;
        FHitResult hand_trace_result;
        static FCollisionObjectQueryParams pawn_whitelist = { ECollisionChannel::ECC_Pawn };
        if (GetWorld()->LineTraceSingleByObjectType(hand_trace_result, w_trace_begin_cm,
            w_trace_end_cm, pawn_whitelist) && hand_trace_result.Component.Get() == smc) {
          FHxHandActorBoneData* hand_trace_bone_data =
              bone_data_from_bone_name_.Find(hand_trace_result.BoneName);
          if (hand_trace_bone_data != nullptr &&
              hand_trace_bone_data->can_engage_contact_damping) {
            int64 other_id = getBodyInstanceId(other_inst);
            if (!damping_constraint_from_object_id_.Contains(other_id)) {
              other_inst->SetLinearVelocity(FVector::ZeroVector, false);
              UPhysicsConstraintComponent* damping_constraint = createContactDampingConstraint(
                  OtherComp, Hit.BoneName);
              damping_constraint_from_object_id_.Add(other_id, damping_constraint);
              notifyLocalConstraintCreated(damping_constraint);
            }
            contacting_object_ids_.Add(other_id);
            hit_hand = true;

            if (visualize_contact_damping_) {
              HxDebugDrawSystem::sphere(
                  GetOwner(),
                  hand_trace_result.Location,
                  componentAverage(smc->GetComponentScale()) * L_HIT_RADIUS_CM,
                  HAND_HIT_COLOR);
            }
          }
        }

        if (visualize_contact_damping_) {
          HxDebugDrawSystem::line(GetOwner(), w_trace_begin_cm, w_trace_end_cm,
              hit_hand ? HAND_HIT_COLOR : HAND_MISSED_COLOR,
              componentAverage(smc->GetComponentScale()) * L_TRACE_THICKNESS_CM);
        }
      }
    }
  }
}

FString AHxHandActor::GetUserProfileName()
{
  FString name;
  std::wstring active_username;
  if (!HaptxApi::UserProfileDatabase::getActiveUsername(&active_username)) {
    name = FString(TEXT("No user profile loaded"));
  }
  else {
    name = active_username.c_str();
  }
  return name;
}

float AHxHandActor::GetUserHandLength()
{
  float length = 0.0f;
  HaptxApi::UserProfile user_profile;

  std::wstring active_username;

  if (HaptxApi::UserProfileDatabase::getActiveUsername(&active_username) &&
    HaptxApi::UserProfileDatabase::getUserProfile(active_username, &user_profile)) {
    length = user_profile.getBasicHandDimsValueM(HaptxApi::BHD_LENGTH);
  }

  return length;
}

float AHxHandActor::GetUserHandWidth()
{
  float width = 0.0f;
  HaptxApi::UserProfile user_profile;

  std::wstring active_username;

  if (HaptxApi::UserProfileDatabase::getActiveUsername(&active_username) &&
    HaptxApi::UserProfileDatabase::getUserProfile(active_username, &user_profile)) {
    width = user_profile.getBasicHandDimsValueM(HaptxApi::BHD_BREADTH);
  }

  return width;
}

ERelativeDirection AHxHandActor::getHand() const {
  return hand_;
}

void AHxHandActor::bindInput() {
  if (IsValid(InputComponent) && !is_input_bound_) {
    is_input_bound_ = true;

    if (isLocallyControlled()) {
      InputComponent->BindAction(toggle_mocap_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleMocapVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_trace_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleTraceVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_tactile_feedback_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleTactileFeedbackVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_force_feedback_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleForceFeedbackVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_hand_animation_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleHandAnimationVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_hand_animation_vis_2_action_, IE_Pressed, this,
          &AHxHandActor::toggleHandAnimationVisualizer2).bConsumeInput = false;
      InputComponent->BindAction(toggle_dis_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleDisplacementVisualizer).bConsumeInput = false;
      InputComponent->BindAction(toggle_contact_damping_vis_action_, IE_Pressed, this,
          &AHxHandActor::toggleContactDampingVisualizer).bConsumeInput = false;
    }
  }
}

AHxHandActor* AHxHandActor::getLeftHand() {
  if (AHxHandActor::left_hand_.IsValid()) {
    return AHxHandActor::left_hand_.Get();
  }
  else {
    return nullptr;
  }
}

AHxHandActor* AHxHandActor::getRightHand() {
  if (AHxHandActor::right_hand_.IsValid()) {
    return AHxHandActor::right_hand_.Get();
  }
  else {
    return nullptr;
  }
}

bool AHxHandActor::isLocallyControlled() const {
  return isPawnLocallyControlled(pawn_);
}

bool AHxHandActor::isAuthoritative() const {
  return isActorAuthoritative(this);
}

void AHxHandActor::setHandScaleFactor(float hand_scale_factor) {
  serverSetHandScaleFactor(hand_scale_factor);
}

void AHxHandActor::teleportPalm(FVector new_location, FRotator new_rotation) {
  if (isPhysicsAuthority()) {
    teleportHand(new_location, new_rotation.Quaternion());
  } else {
    serverTeleportHand(new_location, new_rotation.Quaternion());
  }
}

void AHxHandActor::teleportMiddle1(FVector new_location, FRotator new_rotation) {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  FVector w_palm_cm = FTransform(new_rotation, new_location).TransformPosition(-l_middle1_cm_);
  if (isPhysicsAuthority()) {
    teleportHand(w_palm_cm, new_rotation.Quaternion());
  } else {
    serverTeleportHand(w_palm_cm, new_rotation.Quaternion());
  }
}

void AHxHandActor::notifyLocalConstraintCreated(UPhysicsConstraintComponent* constraint) {
  if (IsValid(constraint) && isConstraintValidForReplication(constraint)) {
    if (!isPhysicsAuthority() && !local_constraints_need_disabled_) {
      // Since we're using replicated constraints instead of local constraints.
      setConstraintPhysicallyEnabled(constraint, false);
    }
    local_constraints_.Add(constraint);
  }
}

void AHxHandActor::notifyLocalConstraintDestroyed(UPhysicsConstraintComponent* constraint) {
  local_constraints_.Remove(constraint);
}

void AHxHandActor::toggleMocapVisualizer() {
  visualize_motion_capture_ = !visualize_motion_capture_;
}

void AHxHandActor::toggleTraceVisualizer() {
  if (!IsValid(GetSkeletalMeshComponent())) {
    return;
  }

  TArray<USceneComponent*> children;
  GetSkeletalMeshComponent()->GetChildrenComponents(true, children);
  for (auto child : children) {
    UHxPatchComponent* patch = Cast<UHxPatchComponent>(child);
    if (patch != nullptr) {
      patch->visualize_traces_ = !patch->visualize_traces_;
    }
  }
}

void AHxHandActor::toggleTactileFeedbackVisualizer() {
  if (!IsValid(GetSkeletalMeshComponent())) {
    return;
  }

  TArray<USceneComponent*> children;
  GetSkeletalMeshComponent()->GetChildrenComponents(true, children);
  for (auto child : children) {
    UHxPatchComponent* patch = Cast<UHxPatchComponent>(child);
    if (patch != nullptr) {
      patch->visualize_tactile_feedback_ = !patch->visualize_tactile_feedback_;
    }
  }
}

void AHxHandActor::toggleForceFeedbackVisualizer() {
  visualize_force_feedback_ = !visualize_force_feedback_;
}

void AHxHandActor::toggleHandAnimationVisualizer() {
  visualize_hand_animation_ = !visualize_hand_animation_;
}

void AHxHandActor::toggleHandAnimationVisualizer2() {
  visualize_hand_animation_2_ = !visualize_hand_animation_2_;
}

void AHxHandActor::toggleDisplacementVisualizer() {
  setDisplacementVisualizerActive(!isDisplacementVisualizerActive());
}

void AHxHandActor::toggleContactDampingVisualizer() {
  visualize_contact_damping_ = !visualize_contact_damping_;
}

void AHxHandActor::setDisplacementVisualizerActive(bool active) {
  if (active) {
    if (!isDisplacementVisualizerActive()) {
      initializeDisplacementVisualizer();
    }
  } else {
    if (isDisplacementVisualizerActive()) {
      uninitializeDisplacementVisualizer();
    }
  }
  visualize_displacement_ = active;
}

bool AHxHandActor::isDisplacementVisualizerActive() {
  return IsValid(dis_vis_pmc_);
}

FVector AHxHandActor::getTrackedDisplacement() const {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return FVector::ZeroVector;
  }
  FVector w_pos_cm = smc->GetBoneLocation(bone_names_.middle1, EBoneSpaces::WorldSpace);
  FVector w_target_pos_cm = physics_state_.targets.w_middle1_pos_cm;

  return w_target_pos_cm - w_pos_cm;
}

std::shared_ptr<HaptxApi::Peripheral> AHxHandActor::getPeripheral() {
  if (!connectToCore()) {
    return nullptr;
  } else {
    return glove_;
  }
}

bool AHxHandActor::tryGetLocatingFeatureTransform(FName locating_feature, FTransform* w_transform) {
  if (w_transform == nullptr) {
    return false;
  }

  auto smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return false;
  }

  HaptxApi::RelativeDirection rel_dir = static_cast<HaptxApi::RelativeDirection>(hand_);
  if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getName(HaptxApi::getBodyPartJoint(rel_dir,
      HaptxApi::Finger::F_MIDDLE, HaptxApi::FingerJoint::FJ_JOINT1)))) {
    // Middle1's rotation changes with hand animation in the game engine so we use the palm's
    // rotation instead.
    FTransform w_palm = smc->GetSocketTransform(bone_names_.palm);
    FTransform w_middle1 = smc->GetSocketTransform(bone_names_.middle1);
    *w_transform = FTransform(w_palm.GetRotation(), w_middle1.GetLocation(),
        w_middle1.GetScale3D());
    return true;
  } else if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getFingertipName(rel_dir,
      HaptxApi::Finger::F_THUMB))) {
    *w_transform = smc->GetSocketTransform(bone_names_.thumb4);
    return true;
  } else if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getFingertipName(rel_dir,
      HaptxApi::Finger::F_INDEX))) {
    *w_transform = smc->GetSocketTransform(bone_names_.index4);
    return true;
  } else if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getFingertipName(rel_dir,
      HaptxApi::Finger::F_MIDDLE))) {
    *w_transform = smc->GetSocketTransform(bone_names_.middle4);
    return true;
  } else if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getFingertipName(rel_dir,
      HaptxApi::Finger::F_RING))) {
    *w_transform = smc->GetSocketTransform(bone_names_.ring4);
    return true;
  } else if (locating_feature == HAPTX_NAME_TO_FNAME(HaptxApi::getFingertipName(rel_dir,
      HaptxApi::Finger::F_PINKY))) {
    *w_transform = smc->GetSocketTransform(bone_names_.pinky4);
    return true;
  } else {
    return false;
  }
}

bool AHxHandActor::tryGetRelativeDirection(ERelativeDirection* rel_dir) {
  if (rel_dir == nullptr) {
    return false;
  }

  *rel_dir = hand_;
  return true;
}

bool AHxHandActor::tryGetCiBodyId(const UHxPatchComponent &patch, int64_t *ci_body_id) {
  if (ci_body_id == nullptr) {
    return false;
  }

  FHxHandActorBoneData* find_result = bone_data_from_bone_name_.Find(patch.GetAttachSocketName());
  if (find_result == nullptr) {
    return false;
  }

  *ci_body_id = find_result->ci_body_id;
  return true;
}

void AHxHandActor::hardDisable() {
  AHxCoreActor::logRestartMessage();
  // Hard disable
  is_enabled_ = false;
  PrimaryActorTick.SetTickFunctionEnable(false);
  SecondaryTick.SetTickFunctionEnable(false);

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (IsValid(smc)) {
    smc->SetSimulatePhysics(false);
    smc->SetAllBodiesSimulatePhysics(false);
    smc->SetNotifyRigidBodyCollision(false);
  }
}

bool AHxHandActor::checkForDuplicateHands() const {
  TArray<AActor*> actors;
  UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHxHandActor::StaticClass(), actors);

  for (AActor* actor : actors) {
    AHxHandActor* ahh = Cast<AHxHandActor>(actor);
    if (ahh != nullptr && ahh != this && ahh->isLocallyControlled() && ahh->hand_ == hand_ &&
        ahh->is_enabled_) {
      return true;
    }
  }

  return false;
}

void AHxHandActor::pushPhysicsTargets(float time_s, const FHandPhysicsTargets& targets) {
  int write_index = physics_targets_buffer_head_i_;
  if (!physics_targets_buffer_started_) {
    write_index = physics_targets_buffer_tail_i_;
    physics_targets_buffer_started_ = true;
  } else if (time_s < physics_targets_buffer_[physics_targets_buffer_.GetPreviousIndex(
      physics_targets_buffer_head_i_)].time_s) {
    // Don't insert out-of-order frames. TODO: Refactor to insert in middle instead of drop.
    return;
  } else if (physics_targets_buffer_head_i_ == physics_targets_buffer_tail_i_) {
    // Make sure the buffer doesn't eat itself.
    physics_targets_buffer_tail_i_ =
        physics_targets_buffer_.GetNextIndex(physics_targets_buffer_tail_i_);
  }
  physics_targets_buffer_head_i_ =
      physics_targets_buffer_.GetNextIndex(physics_targets_buffer_head_i_);

  FHandPhysicsTargetsFrame frame;
  frame.time_s = time_s;
  frame.targets = targets;
  physics_targets_buffer_[write_index] = frame;
}

void AHxHandActor::interpolatePhysicsTargets(float delta_time_s) {
  if (!physics_targets_buffer_started_) {
    return;
  }

  // Grow or shrink delta time depending on how expected lag compares to actual lag.
  int newest_frame_index =
      physics_targets_buffer_.GetPreviousIndex(physics_targets_buffer_head_i_);
  if (physics_targets_buffer_duration_s_ > 0.0f) {
    float lag_s = physics_targets_buffer_[newest_frame_index].time_s - follow_time_s_;
    float lag_weighted_delta_time_s = delta_time_s * lag_s / physics_targets_buffer_duration_s_;
    follow_time_s_ += lag_weighted_delta_time_s;
  } else {
    follow_time_s_ += delta_time_s;
  }

  int a_i = physics_targets_buffer_tail_i_;
  int b_i = physics_targets_buffer_tail_i_;
  if (follow_time_s_ < physics_targets_buffer_[physics_targets_buffer_tail_i_].time_s) {
    follow_time_s_ = physics_targets_buffer_[physics_targets_buffer_tail_i_].time_s;
  } else if (follow_time_s_ > physics_targets_buffer_[newest_frame_index].time_s) {
    // This is where we would extrapolate.
    return;
  } else {
    for (int i = physics_targets_buffer_tail_i_; i != physics_targets_buffer_head_i_;
        i = physics_targets_buffer_.GetNextIndex(i)) {
      if (physics_targets_buffer_[i].time_s <= follow_time_s_) {
        a_i = i;

        int next_index = physics_targets_buffer_.GetNextIndex(i);
        if (next_index == physics_targets_buffer_head_i_) {
          b_i = i;
        } else if (physics_targets_buffer_[next_index].time_s < follow_time_s_) {
          physics_targets_buffer_tail_i_ = next_index;
        }
      } else {
        // Stop at the first frame ahead of us in time.
        b_i = i;
        break;
      }
    }
  }

  if (a_i == b_i) {
    updatePhysicsTargets(physics_targets_buffer_[a_i].targets);
  } else {
    FHandPhysicsTargetsFrame& a = physics_targets_buffer_[a_i];
    FHandPhysicsTargetsFrame& b = physics_targets_buffer_[b_i];

    if (b.time_s - a.time_s > 0.0f) {
      float alpha = (follow_time_s_ - a.time_s) / (b.time_s - a.time_s);
      FHandPhysicsTargets c = FHandPhysicsTargets::interpolate(a.targets, b.targets, alpha);
      updatePhysicsTargets(c);
    }
  }
}

void AHxHandActor::pushPhysicsState(float time_s, const FHandPhysicsState& state) {
  int write_index = physics_state_buffer_head_i_;
  if (!physics_state_buffer_started_) {
    write_index = physics_state_buffer_tail_i_;
    physics_state_buffer_started_ = true;
  } else if (time_s < physics_state_buffer_[physics_state_buffer_.GetPreviousIndex(
      physics_state_buffer_head_i_)].time_s) {
    // Don't insert out-of-order frames. TODO: Refactor to insert in middle instead of drop.
    return;
  } else if (physics_state_buffer_head_i_ == physics_state_buffer_tail_i_) {
    // Make sure the buffer doesn't eat itself.
    physics_state_buffer_tail_i_ =
        physics_state_buffer_.GetNextIndex(physics_state_buffer_tail_i_);
  }
  physics_state_buffer_head_i_ = physics_state_buffer_.GetNextIndex(physics_state_buffer_head_i_);

  FHandPhysicsStateFrame frame;
  frame.time_s = time_s;
  frame.state = state;
  physics_state_buffer_[write_index] = frame;
}

void AHxHandActor::interpolatePhysicsState(float delta_time_s) {
  if (!physics_state_buffer_started_) {
    return;
  }

  // Grow or shrink delta time depending on how expected lag compares to actual lag.
  int newest_frame_index = physics_state_buffer_.GetPreviousIndex(physics_state_buffer_head_i_);
  if (physics_state_buffer_duration_s_ > 0.0f) {
    float lag_s = physics_state_buffer_[newest_frame_index].time_s - follow_time_s_;
    float lag_weighted_delta_time_s = delta_time_s * lag_s / physics_state_buffer_duration_s_;
    follow_time_s_ += lag_weighted_delta_time_s;
  } else {
    follow_time_s_ += delta_time_s;
  }

  int a_i = physics_state_buffer_tail_i_;
  int b_i = physics_state_buffer_tail_i_;
  if (follow_time_s_ < physics_state_buffer_[physics_state_buffer_tail_i_].time_s) {
    follow_time_s_ = physics_state_buffer_[physics_state_buffer_tail_i_].time_s;
  } else if (follow_time_s_ > physics_state_buffer_[newest_frame_index].time_s) {
    // This is where we would extrapolate.
    return;
  } else {
    for (int i = physics_state_buffer_tail_i_; i != physics_state_buffer_head_i_;
        i = physics_state_buffer_.GetNextIndex(i)) {
      if (physics_state_buffer_[i].time_s <= follow_time_s_) {
        a_i = i;

        int next_index = physics_state_buffer_.GetNextIndex(i);
        if (next_index == physics_state_buffer_head_i_) {
          b_i = i;
        } else if (physics_state_buffer_[next_index].time_s < follow_time_s_) {
          physics_state_buffer_tail_i_ = next_index;
        }
      } else {
        // Stop at the first frame ahead of us in time.
        b_i = i;
        break;
      }
    }
  }

  if (a_i == b_i) {
    updatePhysicsState(physics_state_buffer_[a_i].state);
  } else {
    FHandPhysicsStateFrame& a = physics_state_buffer_[a_i];
    FHandPhysicsStateFrame& b = physics_state_buffer_[b_i];

    if (b.time_s - a.time_s > 0.0f) {
      float alpha = (follow_time_s_ - a.time_s) / (b.time_s - a.time_s);
      FHandPhysicsState c = FHandPhysicsState::interpolate(a.state, b.state, alpha);
      updatePhysicsState(c);
    }
  }
}

void AHxHandActor::updatePhysicsTargets(const FHandPhysicsTargets& targets) {
  physics_state_.targets = targets;
  if (!IsValid(palm_constraint_)) {
    return;
  }
  palm_constraint_->SetLinearPositionTarget(targets.w_middle1_pos_cm);
  FRotator w_middle1_orient = targets.w_middle1_orient.Rotator();
  palm_constraint_->SetAngularOrientationTarget(w_middle1_orient);

  if (IsValid(dis_vis_pmc_)) {
    dis_vis_pmc_->SetBoneLocationByName(bone_names_.palm, targets.w_middle1_pos_cm -
        w_middle1_orient.RotateVector(l_middle1_cm_), EBoneSpaces::WorldSpace);
    dis_vis_pmc_->SetBoneRotationByName(bone_names_.palm, w_middle1_orient,
        EBoneSpaces::WorldSpace);
  }

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  static const int num_joints = HaptxApi::F_LAST * HaptxApi::FJ_LAST;
  if (targets.l_joint_orients.Num() == num_joints) {
    for (int f_i = 0; f_i < HaptxApi::F_LAST; f_i++) {
      for (int fj_i = 0; fj_i < HaptxApi::FJ_LAST; fj_i++) {
        if (joints_[f_i][fj_i] == nullptr || !joints_[f_i][fj_i]->IsValidConstraintInstance()) {
          continue;
        }

        int flat_index = HaptxApi::FJ_LAST * f_i + fj_i;
        joints_[f_i][fj_i]->SetAngularOrientationTarget(targets.l_joint_orients[flat_index]);

        if (IsValid(dis_vis_pmc_)) {
          int32 bone_index = dis_vis_pmc_->GetBoneIndex(hand_joint_bone_names_[f_i][fj_i]);
          if (bone_index > 0 && bone_index < dis_vis_pmc_->BoneSpaceTransforms.Num()) {
            FQuat base_rotation = UKismetMathLibrary::MakeRotFromXY(joints_[f_i][fj_i]->PriAxis2,
                joints_[f_i][fj_i]->SecAxis2).Quaternion();
            dis_vis_pmc_->BoneSpaceTransforms[bone_index].SetRotation(
                base_rotation * targets.l_joint_orients[flat_index]);
            dis_vis_pmc_->MarkRefreshTransformDirty();
          }
        }
      }
    }
  }
}

bool AHxHandActor::serverUpdatePhysicsTargets_Validate(float time_s, const FHandPhysicsTargets&) {
  return true;
}

void AHxHandActor::serverUpdatePhysicsTargets_Implementation(
    float time_s, const FHandPhysicsTargets& targets) {
  if (!isLocallyControlled()) {
    pushPhysicsTargets(time_s, targets);
  }
}

void AHxHandActor::updatePhysicsState(const FHandPhysicsState& state) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updatePhysicsState)
  updatePhysicsTargets(state.targets);

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc) || state.w_body_states.Num() != smc->Bodies.Num()) {
    return;
  }

  for (int i = 0; i < smc->Bodies.Num(); i++) {
    FBodyInstance* body = smc->Bodies[i];
    if (body == nullptr || !body->IsValidBodyInstance()) {
      continue;
    }

    const FRigidBodyState& body_state = state.w_body_states[i];
    body->SetBodyTransform(FTransform(body_state.Quaternion, body_state.Position),
        ETeleportType::TeleportPhysics);
    body->SetLinearVelocity(body_state.LinVel, false);
    body->SetAngularVelocityInRadians(FMath::DegreesToRadians(body_state.AngVel), false);
  }

  for (auto& w_object_state : state.w_object_states) {
    if (!IsValid(w_object_state.component)) {
      continue;
    }

    FBodyInstance* body = nullptr;
    USkeletalMeshComponent* other_smc = Cast<USkeletalMeshComponent>(w_object_state.component);
    if (IsValid(other_smc)) {
      body = getBodyInstance(other_smc, other_smc->GetBoneName(w_object_state.body_index));
    } else {
      body = getBodyInstance(w_object_state.component, NAME_None);
    }

    if (body != nullptr && body->IsValidBodyInstance()) {
      body->SetBodyTransform(
          FTransform(w_object_state.state.Quaternion, w_object_state.state.Position),
          ETeleportType::TeleportPhysics);
      body->SetLinearVelocity(w_object_state.state.LinVel, false);
      body->SetAngularVelocityInRadians(FMath::DegreesToRadians(w_object_state.state.AngVel),
          false);
    }
  }

  if (local_constraints_need_disabled_) {
    local_constraints_need_disabled_ = false;
    setLocalConstraintsPhysicallyEnabled(false);
  }
  updateReplicatedConstraints(state.constraint_states);
}

bool AHxHandActor::serverUpdatePhysicsState_Validate(float time_s,
    const FHandPhysicsState& state) {
  return true;
}

void AHxHandActor::serverUpdatePhysicsState_Implementation(float time_s,
    const FHandPhysicsState& state) {
  if (isPhysicsAuthority() && !isLocallyControlled()) {
    // This is the function the client would have called, had it known it didn't have authority.
    // This is expected to execute while an updated value of "is_client_physics_authority_ = false"
    // is traveling over the network to the client. During this time the client still thinks it has
    // authority so it will be executing serverUpdatePhysicsState() when it should be executing
    // serverUpdatePhysicsTargets().
    if (!isLocallyControlled()) {
      pushPhysicsTargets(time_s, state.targets);
    }
  } else {
    multicastUpdatePhysicsState(time_s, state);
  }
}

bool AHxHandActor::multicastUpdatePhysicsState_Validate(float time_s,
    const FHandPhysicsState& state) {
  return true;
}

void AHxHandActor::multicastUpdatePhysicsState_Implementation(float time_s,
    const FHandPhysicsState& state) {
  if (!isPhysicsAuthority()) {
    pushPhysicsState(time_s, state);
  }
}

void AHxHandActor::updateReplicatedConstraints(const TArray<FConstraintPhysicsState>& states) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updateReplicatedConstraints)
  TSet<int32> new_constraint_ids;
  new_constraint_ids.Reserve(states.Num());

  // Create new constraints.
  for (int i = 0; i < states.Num(); i++) {
    const FConstraintPhysicsState& constraint_state = states[i];
    new_constraint_ids.Add(constraint_state.id);

    if (!replicated_constraints_.Contains(constraint_state.id)) {
      UPhysicsConstraintComponent* constraint = NewObject<UPhysicsConstraintComponent>(this);
      constraint->RegisterComponent();
      constraint->OverrideComponent1 = constraint_state.component1;
      constraint->OverrideComponent2 = constraint_state.component2;
      constraint->SetWorldScale3D(constraint_state.w_scale);
      constraint->ConstraintInstance = constraint_state.constraint_instance.deserialize();
      initPhysicsConstraintComponent(constraint, false);
      replicated_constraints_.Add(constraint_state.id, constraint);
    }
  }

  // Figure out which constraints are outdated.
  TArray<int32> ids_to_remove;
  ids_to_remove.Reserve(replicated_constraints_.Num());
  for (auto& key_value : replicated_constraints_) {
    if (!new_constraint_ids.Contains(key_value.Key)) {
      ids_to_remove.Add(key_value.Key);
    }
  }

  // Remove outdated constraints.
  for (int32 id : ids_to_remove) {
    UPhysicsConstraintComponent** constraint = replicated_constraints_.Find(id);
    if (constraint != nullptr && IsValid(*constraint) &&
        (*constraint)->ConstraintInstance.IsValidConstraintInstance()) {
      (*constraint)->ConstraintInstance.TermConstraint();
    }
    replicated_constraints_.Remove(id);
  }
}

void AHxHandActor::teleportHand(const FVector& w_position_cm, const FQuat& w_orient) {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  // Update the palm constraint's target position and location so the hand doesn't try to
  // immediately snap back to its previous position. These values will be updated next frame.
  if (IsValid(palm_constraint_) &&
      palm_constraint_->ConstraintInstance.IsValidConstraintInstance()) {
    FVector w_middle1_pos_cm =
        FTransform(w_orient, w_position_cm).TransformPosition(l_middle1_cm_);
    palm_constraint_->SetLinearPositionTarget(w_middle1_pos_cm);
    palm_constraint_->SetAngularOrientationTarget(w_orient.Rotator());
  }

  smc->SetWorldLocationAndRotation(w_position_cm, w_orient, false,
      nullptr, ETeleportType::TeleportPhysics);
}

bool AHxHandActor::serverTeleportHand_Validate(const FVector&, const FQuat&) {
  return true;
}

void AHxHandActor::serverTeleportHand_Implementation(const FVector& w_position_cm,
    const FQuat& w_orient) {
  if (isPhysicsAuthority()) {
    teleportHand(w_position_cm, w_orient);
  }
}

bool AHxHandActor::serverSetHandScaleFactor_Validate(float) {
  return true;
}

void AHxHandActor::serverSetHandScaleFactor_Implementation(float hand_scale_factor) {
  if (hand_scale_factor > 0.0f) {
    hand_scale_factor_ = hand_scale_factor;
    hand_needs_scale_update_ = true;
  }
}

void AHxHandActor::updateHandScale() {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  smc->SetWorldScale3D(FVector(w_uhp_hand_scale_factor_ * hand_scale_factor_));
  smc->RecreatePhysicsState();
}

void AHxHandActor::initPalmConstraint() {
  if (IsValid(palm_constraint_)) {
    palm_constraint_->BreakConstraint();
  }
  else {
    palm_constraint_ = NewObject<UPhysicsConstraintComponent>(this,
      UPhysicsConstraintComponent::StaticClass(), TEXT("Palm Mocap Constraint"));
    palm_constraint_->SetupAttachment(GetRootComponent());
    palm_constraint_->RegisterComponent();
    FConstraintInstance& constraint = palm_constraint_->ConstraintInstance;
    constraint.SetDisableCollision(true);
    constraint.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
    constraint.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
    constraint.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);
    constraint.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);
    constraint.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Free);
    constraint.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Free);
    constraint.ProfileInstance.LinearDrive.XDrive = linear_drive_;
    constraint.ProfileInstance.LinearDrive.YDrive = linear_drive_;
    constraint.ProfileInstance.LinearDrive.ZDrive = linear_drive_;
    constraint.ProfileInstance.AngularDrive.AngularDriveMode = EAngularDriveMode::SLERP;
    constraint.ProfileInstance.AngularDrive.SlerpDrive = angular_drive_;
  }

  if (IsValid(GetSkeletalMeshComponent())) {
    palm_constraint_->SetConstrainedComponents(GetSkeletalMeshComponent(), bone_names_.palm,
        nullptr, FName());
    // Drive middle1's transform instead of the root's.
    palm_constraint_->SetConstraintReferenceFrame(EConstraintFrame::Frame1,
        FTransform(FQuat::Identity, l_middle1_cm_));
    // Use the world as the origin.
    palm_constraint_->SetConstraintReferenceFrame(EConstraintFrame::Frame2, FTransform::Identity);
  }
}

UPhysicsConstraintComponent* AHxHandActor::createContactDampingConstraint(
    UPrimitiveComponent* other_comp, FName other_bone) {
  UPhysicsConstraintComponent* damping_constraint = NewObject<UPhysicsConstraintComponent>(
      this, UPhysicsConstraintComponent::StaticClass());
  damping_constraint->RegisterComponent();

  // Center the constraint on the other body to decouple the bodies translation from the
  // constraint's rotation.
  FBodyInstance* other_body = other_comp->GetBodyInstance(other_bone);
  if (other_body != nullptr) {
    damping_constraint->SetWorldLocation(other_body->GetCOMPosition());
  }
  damping_constraint->ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
  damping_constraint->ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
  damping_constraint->ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);
  damping_constraint->ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);
  damping_constraint->ConstraintInstance.SetAngularSwing1Motion(
      EAngularConstraintMotion::ACM_Free);
  damping_constraint->ConstraintInstance.SetAngularSwing2Motion(
      EAngularConstraintMotion::ACM_Free);

  FConstraintDrive linear_damping_drive;
  linear_damping_drive.bEnableVelocityDrive = true;
  UHxPhysicalMaterial* hx_phys_mat =
      Cast<UHxPhysicalMaterial>(getPhysicalMaterial(other_comp, other_bone));
  linear_damping_drive.Damping = (IsValid(hx_phys_mat) &&
      hx_phys_mat->override_linear_contact_damping_) ? hx_phys_mat->linear_contact_damping_ :
      linear_contact_damping_;

  FConstraintDrive angular_damping_drive;
  angular_damping_drive.bEnableVelocityDrive = true;
  angular_damping_drive.Damping = (IsValid(hx_phys_mat) &&
      hx_phys_mat->override_angular_contact_damping_) ? hx_phys_mat->angular_contact_damping_ :
      angular_contact_damping_;

  damping_constraint->ConstraintInstance.ProfileInstance.LinearDrive.XDrive = linear_damping_drive;
  damping_constraint->ConstraintInstance.ProfileInstance.LinearDrive.YDrive = linear_damping_drive;
  damping_constraint->ConstraintInstance.ProfileInstance.LinearDrive.ZDrive = linear_damping_drive;
  damping_constraint->ConstraintInstance.ProfileInstance.AngularDrive.AngularDriveMode =
      EAngularDriveMode::SLERP;
  damping_constraint->ConstraintInstance.ProfileInstance.AngularDrive.SlerpDrive =
      angular_damping_drive;
  damping_constraint->SetLinearVelocityTarget(FVector::ZeroVector);
  damping_constraint->SetAngularOrientationTarget(FRotator::ZeroRotator);
  damping_constraint->SetConstrainedComponents(GetSkeletalMeshComponent(), bone_names_.palm,
      other_comp, other_bone);
  return damping_constraint;
}

void AHxHandActor::onPhysicsAuthorityZoneBeginOverlap(UPrimitiveComponent* overlapped_component,
    AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index,
    bool from_sweep, const FHitResult& sweep_result) {
  if (!IsValid(other_comp) || other_actor == this || !IsValid(other_actor)) {
    return;
  }

  // If it's another hand, increment our tracker and return early.
  AHxHandActor* other_hand_actor = Cast<AHxHandActor>(other_actor);
  if (IsValid(other_hand_actor) && other_hand_actor->PhysicsAuthorityZone == other_comp) {
    if (other_hand_actor->pawn_ != pawn_) {
      num_physics_authority_zone_overlaps_++;
      if (num_physics_authority_zone_overlaps_ == 1 && IsValid(PhysicsAuthorityZone)) {
        PhysicsAuthorityZone->SetSphereRadius(physics_authority_zone_radius_enlarged_cm_);
      }
    }
    return;
  }

  // Increment our internal overlap tracking map.
  int& internal_count = objects_in_physics_authority_zone_.FindOrAdd(other_comp);
  internal_count++;

  // Increment the global overlap tracking map.
  FGlobalPhysicsAuthorityObjectData& global_data =
      global_physics_authority_data_from_comp_.FindOrAdd(other_comp);
  int& global_count = global_data.physics_authority_zone_count_from_pawn.FindOrAdd(pawn_, 0);
  global_count++;

  // While an object is being managed by HaptX's networking system we must make sure Unreal's
  // replicate movement feature is disabled on the object.
  // Apparently Unreal's replicate movement feature only operates on the root component of the
  // actor.
  if (!isAuthoritative() && global_data.physics_authority_zone_count_from_pawn.Num() == 1 &&
      global_count == 1 && isMovementReplicated(other_comp)) {
    other_actor->bReplicateMovement = false;
    global_data.was_replicating_movement = true;
    // It's unsafe to clear replication targets at this time.
    comps_that_need_replication_targets_removed_.Add(other_comp);
  }
}

void AHxHandActor::onPhysicsAuthorityZoneEndOverlap(UPrimitiveComponent* overlapped_component,
    AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index) {
  if (!IsValid(other_comp) || !IsValid(other_actor) || other_actor == this) {
    return;
  }

  AHxHandActor* other_hand_actor = Cast<AHxHandActor>(other_actor);
  if (IsValid(other_hand_actor) && other_hand_actor->PhysicsAuthorityZone == other_comp &&
      other_hand_actor->pawn_ != pawn_) {
    num_physics_authority_zone_overlaps_--;
    if (num_physics_authority_zone_overlaps_ == 0 && IsValid(PhysicsAuthorityZone)) {
      PhysicsAuthorityZone->SetSphereRadius(physics_authority_zone_radius_nominal_cm_);
    }
    return;
  }

  int* internal_count = objects_in_physics_authority_zone_.Find(other_comp);
  if (internal_count != nullptr) {
    (*internal_count)--;

    if ((*internal_count) <= 0) {
      objects_in_physics_authority_zone_.Remove(other_comp);
    }
  }

  auto global_data_it = global_physics_authority_data_from_comp_.Find(other_comp);
  if (global_data_it != nullptr) {
    FGlobalPhysicsAuthorityObjectData& global_data = (*global_data_it);

    int* global_count_it = global_data.physics_authority_zone_count_from_pawn.Find(pawn_);
    if (global_count_it != nullptr) {
      int& global_count = (*global_count_it);

      global_count--;
      if (global_count <= 0) {
        global_data.physics_authority_zone_count_from_pawn.Remove(pawn_);

        if (global_data.physics_authority_zone_count_from_pawn.Num() == 0) {
          if (!isAuthoritative() && global_data.was_replicating_movement) {
            other_actor->bReplicateMovement = true;
          }

          global_physics_authority_data_from_comp_.Remove(other_comp);
        }
      }
    }
  }
}

bool AHxHandActor::serverUpdatePhysicsAuthority_Validate() {
  return true;
}

void AHxHandActor::serverUpdatePhysicsAuthority_Implementation() {
  if (!IsValid(hx_core_)) {
    return;
  }

  // Cache the current value so we can determine if it has changed at the end of the function.
  bool is_client_physics_authority_cached = is_client_physics_authority_;

  EPhysicsAuthorityMode mode = hx_core_->getPhysicsAuthorityMode();
  switch (mode) {
  case (EPhysicsAuthorityMode::DYNAMIC): {
    // Evaluate whether any multi-player interactions are happening.
    if (num_physics_authority_zone_overlaps_ > 0) {
      is_client_physics_authority_ = false;
    } else {
      bool shared_object_found = false;
      for (auto& it : objects_in_physics_authority_zone_) {
        auto global_data_it = global_physics_authority_data_from_comp_.Find(it.Key);
        if (global_data_it != nullptr &&
            global_data_it->physics_authority_zone_count_from_pawn.Num() > 1) {
          shared_object_found = true;
          break;
        }
      }
      is_client_physics_authority_ = !shared_object_found;
    }
    break;
  }
  case (EPhysicsAuthorityMode::CLIENT):
    is_client_physics_authority_ = true;
    break;
  default:
  case (EPhysicsAuthorityMode::SERVER):
    is_client_physics_authority_ = false;
    break;
  }

  if (is_client_physics_authority_ != is_client_physics_authority_cached) {
    OnRep_is_client_physics_authority_();
  }
}

bool AHxHandActor::isPhysicsAuthority() const {
  return (isAuthoritative() && !is_client_physics_authority_) ||
      (isLocallyControlled() && is_client_physics_authority_);
}

void AHxHandActor::getPhysicsStatesOfObjectsInAuthorityZone(
    TArray<FObjectPhysicsState>& object_states) {
  // There will be at least one FBodyInstance per overlap.
  int num_overlaps = 0;
  for (auto& it : objects_in_physics_authority_zone_) {
    num_overlaps += it.Value;
  }
  object_states.Empty(num_overlaps);

  for (auto& iter : objects_in_physics_authority_zone_) {
    UPrimitiveComponent* comp = iter.Key;
    if (!IsValid(comp)) {
      continue;
    }

    USkeletalMeshComponent* smc = Cast<USkeletalMeshComponent>(comp);
    if (IsValid(smc)) {
      // Syncing the entire skeletal mesh prevents energy from being added to the system in joint
      // error.
      for (auto& body : smc->Bodies) {
        if (!body->IsValidBodyInstance() || !body->bSimulatePhysics) {
          continue;
        }

        FObjectPhysicsState w_object_state;
        w_object_state.component = comp;
        w_object_state.body_index = body->InstanceBodyIndex;
        body->GetRigidBodyState(w_object_state.state);
        object_states.Add(w_object_state);
      }
    } else {
      if (!comp->BodyInstance.IsValidBodyInstance() || !comp->BodyInstance.bSimulatePhysics) {
        continue;
      }

      FObjectPhysicsState w_object_state;
      w_object_state.component = comp;
      w_object_state.body_index = comp->BodyInstance.InstanceBodyIndex;
      comp->BodyInstance.GetRigidBodyState(w_object_state.state);
      object_states.Add(w_object_state);
    }
  }
}

void AHxHandActor::getLocalConstraintStates(TArray<FConstraintPhysicsState>& constraint_states) {
  constraint_states.Empty(local_constraints_.Num());
  for (auto& constraint : local_constraints_) {
    if (IsValid(constraint) && isConstraintValidForReplication(constraint)) {
      FConstraintPhysicsState constraint_state;
      constraint_state.id = constraint->GetUniqueID();
      constraint_state.component1 = constraint->OverrideComponent1.Get();
      constraint_state.component2 = constraint->OverrideComponent2.Get();
      constraint_state.w_scale = constraint->GetComponentScale();
      constraint_state.constraint_instance = constraint->ConstraintInstance;
      constraint_states.Add(constraint_state);
    }
  }
}

bool AHxHandActor::isConstraintValidForReplication(UPhysicsConstraintComponent* constraint) const {
  if (!IsValid(constraint)) {
    return false;
  }

  // One of the components needs to be this hand.
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  UPrimitiveComponent* component1 = constraint->OverrideComponent1.Get();
  UPrimitiveComponent* component2 = constraint->OverrideComponent2.Get();
  if (component1 != smc && component2 != smc) {
    return false;
  }

  // Both components can be the hand.
  if (component1 == smc && component2 == smc) {
    return true;
  }

  // One of the components may be null, which indicates a constraint with the world origin.
  if (component1 == nullptr || component2 == nullptr) {
    return true;
  }

  // A constrained object must be synced inside our physics authority zone.
  if (component1 != smc) {
    return objects_in_physics_authority_zone_.Contains(component1);
  }
  if (component2 != smc) {
    return objects_in_physics_authority_zone_.Contains(component2);
  }

  return false;
}

void AHxHandActor::setLocalConstraintsPhysicallyEnabled(bool enabled) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_setLocalConstraintsPhysicallyEnabled)
  for (auto& constraint : local_constraints_) {
    setConstraintPhysicallyEnabled(constraint, enabled);
  }
}

void AHxHandActor::clearReplicatedConstraints() {
  for (auto& key_value : replicated_constraints_) {
    destroyPhysicsConstraintComponent(key_value.Value);
  }
  replicated_constraints_.Empty();
}

void AHxHandActor::updateHandAnimation(float delta_time) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updateHandAnimation)

  if (!IsValid(palm_constraint_) || glove_ == nullptr) {
    return;
  }

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  // World space positioning of the hand.
  FTransform w_mcp3_original = FTransform(
      palm_constraint_->ConstraintInstance.ProfileInstance.AngularDrive.OrientationTarget,
      palm_constraint_->ConstraintInstance.ProfileInstance.LinearDrive.PositionTarget,
      FVector::OneVector);
  FTransform w_vive = FTransform::Identity;
  if (tryGetHandLocationAndRotation(w_mcp3_original, w_vive)) {
    if (recently_warned_about_tracking_ref_being_off_) {
      recently_warned_about_tracking_ref_being_off_ = false;
      AHxOnScreenLog::clearFromScreen(tracking_ref_off_debug_message_key_);
    }
  } else {
    warnAboutTrackingRefOff();
  }

  // Select which profile to animate with based on optimization mode.
  HaptxApi::UserProfile* anim_profile = nullptr;
  switch (hand_anim_optimization_mode_) {
  case EHandAnimationOptimizationMode::DYNAMIC:
    anim_profile = &avatar_anim_optimized_profile_;
    break;
  case EHandAnimationOptimizationMode::JOINT_ANGLES:
    anim_profile = &user_profile_;
    break;
  case EHandAnimationOptimizationMode::FINGERTIP_POSITIONS:
    anim_profile = &avatar_profile_;
    break;
  default:
    anim_profile = &user_profile_;
    break;
  }

  HaptxApi::MocapFrame mocap_frame_original;  // Motion-capture data directly from hardware
  HaptxApi::MocapFrame mocap_frame;  // Motion-capture data adjusted for compensators
  FTransform w_mcp3 = w_mcp3_original;  // MCP3 placement adjusted for compensators
  HaptxApi::AnimFrame anim_frame;  // Hand animation information
  // Simulated animation of the fingertips.
  if (glove_->is_simulated) {
    if (last_simulated_anim_frame_.l_orientations.empty()) {
      last_simulated_anim_frame_ = HaptxApi::SimulatedGestures::getAnimFrame(gesture_, 0.0f);
    }

    if (HaptxApi::OpenvrWrapper::isReady()) {
      bool touch_pad_pressed = false;
      if (HaptxApi::OpenvrWrapper::getControllerButtonPressed(HaptxApi::RelativeDirection(hand_),
          HaptxApi::ControllerButton::TOUCH_PAD, &touch_pad_pressed) ==
          HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS &&
          touch_pad_pressed && !touch_pad_was_pressed_) {
        float touch_pad_value;
        if (HaptxApi::OpenvrWrapper::getControllerAxisValue(HaptxApi::RelativeDirection(hand_),
            HaptxApi::ControllerAxis::TOUCH_PAD_X, &touch_pad_value) ==
            HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS) {
          if (touch_pad_value > 0.0f) {
            gesture_ = (HaptxApi::Gesture)(((int)gesture_ + 1) % (int)HaptxApi::Gesture::LAST);
          } else {
            gesture_ = (HaptxApi::Gesture)(((int)gesture_ - 1 + (int)HaptxApi::Gesture::LAST) %
                (int)HaptxApi::Gesture::LAST);
          }
        }
      }
      touch_pad_was_pressed_ = touch_pad_pressed;

      float trigger_value = 0.0f;
      if(HaptxApi::OpenvrWrapper::getControllerAxisValue(HaptxApi::RelativeDirection(hand_),
          HaptxApi::ControllerAxis::TRIGGER, &trigger_value) ==
          HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS) {
        HaptxApi::AnimFrame target_anim_frame = HaptxApi::SimulatedGestures::getAnimFrame(gesture_,
            trigger_value);
        last_simulated_anim_frame_ = HaptxApi::AnimFrame::slerp(last_simulated_anim_frame_,
            target_anim_frame, delta_time * simulated_animation_aggressiveness_1_s);
      }
    }
    anim_frame = last_simulated_anim_frame_;
  // Motion capture based animation of the fingertips.
  } else {
    auto mocap_system = mocap_system_.lock();
    if (mocap_system != nullptr && mocap_system->isReady()) {
      HaptxApi::HyleasSystem::ReturnCode hs_ret = mocap_system->update();
      if (hs_ret != HaptxApi::HyleasSystem::ReturnCode::SUCCESS) {
        UE_LOG(HaptX, Error, TEXT(
            "AHxHandActor::updateHandAnimation(): Motion capture system failed to update with error code %d: %s."),
            (int)hs_ret, *STRING_TO_FSTRING(HaptxApi::HyleasSystem::toString(hs_ret)))
      }

      hs_ret = mocap_system->addToMocapFrame(&mocap_frame_original);
      if (hs_ret != HaptxApi::HyleasSystem::ReturnCode::SUCCESS) {
        UE_LOG(HaptX, Error, TEXT(
            "AHxHandActor::updateHandAnimation(): Motion capture system failed to add to mocap frame with error code %d: %s."),
            (int)hs_ret, *STRING_TO_FSTRING(HaptxApi::HyleasSystem::toString(hs_ret)))
      }
      mocap_frame = mocap_frame_original;
    }

    if (enable_glove_slip_compensation_ && glove_slip_compensator_ != nullptr &&
        glove_slip_compensator_->applyToMocapFrame(&mocap_frame, delta_time,
        glove_slip_compensation_parameters_.aggressiveness_1_s,
        glove_slip_compensation_parameters_.on_threshold)) {
      w_mcp3.AddToTranslation(w_mcp3.TransformVector(unrealFromHxLength(
          glove_slip_compensator_->getMcp3SlipOffsetM())));
    }

    if (enable_thimble_compensation_) {
      HaptxApi::ThimbleCompensator::applyToMocapFrame(&mocap_frame,
          hxFromUnrealLength(thimble_compensation_parameters_.correction_dist_threshold_cm),
          hxFromUnrealLength(thimble_compensation_parameters_.max_correction_dist_cm),
          hxFromUnrealLength(thimble_compensation_parameters_.max_correction_amount_cm));
    }

    if (hand_anim_optimization_mode_ == EHandAnimationOptimizationMode::DYNAMIC &&
        !HaptxApi::AvatarAnimationOptimizer::optimize(
        static_cast<HaptxApi::RelativeDirection>(hand_),  user_profile_, avatar_profile_,
        mocap_frame, &avatar_anim_optimized_profile_, dynamic_hand_anim_rel_dist_threshold_)) {
      UE_LOG(HaptX, Error,
          TEXT("AHxHandActor::updateHandAnimation(): Failed to optimize hand animation."))
      anim_profile = &user_profile_;
    }

    HaptxApi::DefaultHandIk::addToAnimFrame(glove_, mocap_frame, user_profile_, *anim_profile,
        &anim_frame);
  }

  if (visualize_motion_capture_) {
    // Mocap visualizer shows unadjusted values.
    visualizeMocapData(mocap_frame_original, w_mcp3_original, w_vive);
  }

  if (visualize_hand_animation_) {
    visualizeHandAnimation(anim_frame, *anim_profile, w_mcp3);
  }

  // Generate physics targets
  FHandPhysicsTargets targets;
  // This is subtle, but we know an error can exist between the hardware idealized location of
  // MCP3 (which almost everything is positioned relative to), and the user's actual MCP3 location
  // (which is where we would ideally like to position the avatar hand). By correcting for that
  // error at the last minute we can position the hand correctly without messing up all our motion
  // tracking offsets.
  FTransform l_mcp3_user = FTransform(unrealFromHxLength(
      user_profile_.mcp3_joint1_pos_offsets_m[HaptxApi::RelativeDirection(hand_)][HaptxApi::Finger::F_MIDDLE]));
  FTransform w_mcp3_user = UKismetMathLibrary::ComposeTransforms(l_mcp3_user, w_mcp3);
  targets.w_middle1_pos_cm = w_mcp3_user.GetLocation();
  targets.w_middle1_orient = w_mcp3_user.GetRotation();
  targets.l_joint_orients.SetNumUninitialized(HaptxApi::F_LAST * HaptxApi::FJ_LAST);
  for (int f_i = 0; f_i < HaptxApi::F_LAST; f_i++) {
    for (int fj_i = 0; fj_i < HaptxApi::FJ_LAST; fj_i++) {
      HaptxApi::BodyPartJoint joint = HaptxApi::getBodyPartJoint(
          HaptxApi::RelativeDirection(hand_), (HaptxApi::Finger)f_i, (HaptxApi::FingerJoint)fj_i);

      int flat_index = HaptxApi::FJ_LAST * f_i + fj_i;
      auto orientation = anim_frame.l_orientations.find(joint);
      if (orientation != anim_frame.l_orientations.end()) {
        targets.l_joint_orients[flat_index] = unrealFromHx(orientation->second);
      } else {
        targets.l_joint_orients[flat_index] = FQuat::Identity;
      }
    }
  }

  if (!isPhysicsAuthority()) {
    AGameStateBase* game_state = UGameplayStatics::GetGameState(GetWorld());
    if (IsValid(game_state)) {
      float time_s = game_state->GetServerWorldTimeSeconds();
      float period_s = 1.0f / physics_targets_transmission_frequency_hz_;
      // Insert a half period of offset for left hands to stagger messages.
      if (time_s - time_of_last_physics_transmission_s_ -
          (hand_ == ERelativeDirection::LEFT ? 0.5f * period_s : 0.0f) > period_s) {
        serverUpdatePhysicsTargets(time_s, targets);
        time_of_last_physics_transmission_s_ = time_s;
      }
    }
  } else {
    updatePhysicsTargets(targets);
  }

  const float tp_dist_threshold = corrective_teleportation_distance_ *
      FMath::Max(hand_scale_factor_, 1.0f);
  const float position_error = (smc->GetSocketLocation(bone_names_.middle1) -
      targets.w_middle1_pos_cm).Size();

  // Teleport the palm (if needed). Always happens in the first frame of good tracking.
  if (palm_needs_first_teleport_ ||
      (enable_corrective_teleportation_ && tp_dist_threshold < position_error)) {
    teleportMiddle1(targets.w_middle1_pos_cm, targets.w_middle1_orient.Rotator());
    smc->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
    smc->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    palm_needs_first_teleport_ = false;
  }
}

void AHxHandActor::loadUserProfile() {
  std::wstring active_username;
  if (!HaptxApi::UserProfileDatabase::getActiveUsername(&active_username) ||
      !HaptxApi::UserProfileDatabase::getUserProfile(active_username, &user_profile_)) {
    AHxCoreActor::logWarning(
        "Failed to load active user profile. Using the default profile instead.", true);
    user_profile_ = HaptxApi::UserProfile();
  }
  if (glove_ == nullptr) {
    AHxCoreActor::logError("AHxHandActor::loadUserProfile(): Null glove.");
    return;
  }
  // Start the interpolated profile as a perfect copy of the user profile.
  avatar_anim_optimized_profile_ = user_profile_;
  glove_slip_compensator_ = std::make_unique<HaptxApi::GloveSlipCompensator>(
      *glove_, user_profile_);

  // Set the correct material on the hand according to skin tone from user profile
  UMaterialInterface* hand_material = nullptr;
  switch (user_profile_.skin_tone) {
    case HaptxApi::SkinTone::LIGHT: {
      hand_material = user_profile_.sex == HaptxApi::BiologicalSex::FEMALE ?
          light_female_hand_material_ : light_male_hand_material_;
      break;
    }
    case HaptxApi::SkinTone::MEDIUM: {
      hand_material = user_profile_.sex == HaptxApi::BiologicalSex::FEMALE ?
          medium_female_hand_material_ : medium_male_hand_material_;
      break;
    }
    case HaptxApi::SkinTone::DARK: {
      hand_material = user_profile_.sex == HaptxApi::BiologicalSex::FEMALE ?
          dark_female_hand_material_ : dark_male_hand_material_;
      break;
    }
    case HaptxApi::SkinTone::NEUTRAL: {
      hand_material = user_profile_.sex == HaptxApi::BiologicalSex::FEMALE ?
          neutral_female_hand_material_ : neutral_male_hand_material_;
      break;
    }
  }

  // Select the mesh we want to load

  USkeletalMesh* mesh;
  if (hand_ == ERelativeDirection::LEFT && user_profile_.sex == HaptxApi::BiologicalSex::FEMALE) {
    mesh = female_left_hand_skeletal_mesh_;
    static_mesh_ = female_left_hand_static_mesh_;
  } else if (hand_ == ERelativeDirection::RIGHT &&
        user_profile_.sex == HaptxApi::BiologicalSex::FEMALE) {
    mesh = female_right_hand_skeletal_mesh_;
    static_mesh_ = female_right_hand_static_mesh_;
  } else if (hand_ == ERelativeDirection::LEFT && user_profile_.sex !=
        HaptxApi::BiologicalSex::FEMALE) {
    mesh = male_left_hand_skeletal_mesh_;
    static_mesh_ = male_left_hand_static_mesh_;
  } else {
    mesh = male_right_hand_skeletal_mesh_;
    static_mesh_ = male_right_hand_static_mesh_;
  }

  if (mesh == nullptr) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "Could not load skeletal mesh asset for %s %s hand."), user_profile_.sex ==
        HaptxApi::BiologicalSex::FEMALE ? TEXT("female") : TEXT("male"), HAND_AS_TEXT), true);
    hardDisable();
    return;
  }

  // Figure out how much to scale the hand to match the user's hand dimensions

  const FVector l_middle4_pos_cm =
      mesh->GetComposedRefPoseMatrix(bone_names_.middle4).TransformPosition(FVector::ZeroVector);
  const FVector l_mcp3_pos_cm =
      mesh->GetComposedRefPoseMatrix(bone_names_.middle1).TransformPosition(FVector::ZeroVector);
  const float w_default_middle_finger_length_m =
      hxFromUnrealLength((l_middle4_pos_cm - l_mcp3_pos_cm).Size());

  if (w_default_middle_finger_length_m <= 0.0f || std::isnan(w_default_middle_finger_length_m)) {
    AHxCoreActor::logError(FString::Printf(TEXT(
        "Failed to size the hand: sockets %s and %s are collocated or otherwise produce an unusable finger length."),
        *bone_names_.middle4.ToString(), *bone_names_.middle1.ToString()), true);
    hardDisable();
    return;
  }

  float w_uhp_middle_finger_length =
      user_profile_.finger_bone_lengths_m[HaptxApi::Finger::F_MIDDLE][HaptxApi::FingerBone::FB_PROXIMAL] +
      user_profile_.finger_bone_lengths_m[HaptxApi::Finger::F_MIDDLE][HaptxApi::FingerBone::FB_MEDIAL] +
      user_profile_.finger_bone_lengths_m[HaptxApi::Finger::F_MIDDLE][HaptxApi::FingerBone::FB_DISTAL];
  float w_uhp_hand_scale_factor = w_uhp_middle_finger_length / w_default_middle_finger_length_m;

  serverUserProfileUpdate(hand_material, mesh, w_uhp_hand_scale_factor);
}

bool AHxHandActor::serverUserProfileUpdate_Validate(UMaterialInterface*, USkeletalMesh*,
    float) {
  return true;
}

void AHxHandActor::serverUserProfileUpdate_Implementation(UMaterialInterface* hand_material,
    USkeletalMesh* hand_mesh, float w_hand_scale) {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  smc->SetMaterial(0, hand_material);
  ReplicatedMaterial0 = hand_material;

  smc->SetSkeletalMesh(hand_mesh);
  ReplicatedMesh = hand_mesh;

  w_uhp_hand_scale_factor_ = w_hand_scale;
  hand_needs_scale_update_ = true;
}

bool AHxHandActor::tryGetHandLocationAndRotation(FTransform& w_mcp3, FTransform& w_vive) {
  if (hx_core_ == nullptr || glove_ == nullptr) {
    return false;
  }

  if (!HaptxApi::OpenvrWrapper::isReady()) {
    return false;
  }

  bool success = false;
  HaptxApi::Transform ovr;
  if (!glove_->is_simulated) {
    if (glove_->vive_trackers.size() < 1) {
      return false;
    }
    const HaptxApi::ViveTracker vive_tracker = glove_->vive_trackers.front();
    last_openvr_wrapper_return_code_ = 
        HaptxApi::OpenvrWrapper::getTrackerTransform(vive_tracker.serial, &ovr);
    success = last_openvr_wrapper_return_code_ == HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS;
    if (success) {
      w_mcp3 = unrealFromHx(ovr * vive_tracker.transform.inverse());
    }
  } else {
    last_openvr_wrapper_return_code_ = 
        HaptxApi::OpenvrWrapper::getControllerTransform(HaptxApi::RelativeDirection(hand_), &ovr);
    success = last_openvr_wrapper_return_code_ == HaptxApi::OpenvrWrapper::ReturnCode::SUCCESS;
    if (success) {
      w_mcp3 = unrealFromHx(ovr);
    }
  }

  if (success) {
    w_vive = unrealFromHx(ovr);
    if (mocap_origin_ != nullptr) {
      w_mcp3 = UKismetMathLibrary::ComposeTransforms(w_mcp3,
          mocap_origin_->GetComponentTransform());
      w_vive = UKismetMathLibrary::ComposeTransforms(w_vive,
          mocap_origin_->GetComponentTransform());
    }
  }
  return success;
}

void AHxHandActor::warnAboutTrackingRefOff() {
  // If we haven't recently warned about the same thing
  if (!recently_warned_about_tracking_ref_being_off_) {
    if (glove_ == nullptr) {
      AHxCoreActor::logError("AHxHandActor::warnAboutTrackingRefOff(): Null glove.");
      return;
    }
    std::string error_code_string = (HaptxApi::OpenvrWrapper::toString(
        static_cast<HaptxApi::OpenvrWrapper::ReturnCode>(last_openvr_wrapper_return_code_)));
    std::wstring error_code_wstring(error_code_string.begin(), error_code_string.end());

    tracking_ref_off_debug_message_key_ = AHxCoreActor::logWarning(FString::Printf(
        TEXT("VIVE %s is off or not tracking for the %s hand: error code %s."),
        glove_->is_simulated ? TEXT("Tracker") : TEXT("controller"),
        HAND_AS_TEXT, error_code_wstring.c_str()), /*add_to_screen=*/true);

    recently_warned_about_tracking_ref_being_off_ = true;
  }
}

void AHxHandActor::visualizeForceFeedbackOutput() {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeForceFeedbackOutput)
  // The visual offset [cm] of force feedback information from respective bones in the
  // direction of the actuation normal.
  const float L_OFFSET_CM = 3.0f;
  // The color to draw actuation normals when the retractuator is disengaged.
  const FColor ACTUATION_NORMAL_DISENGAGED_COLOR = DEBUG_BLACK;
  // The color to draw actuation normals when the retractuator is engaged.
  const FColor ACTUATION_NORMAL_ENGAGED_COLOR = DEBUG_PURPLE_OR_TEAL;
  // The length [cm] to draw actuation normals.
  const float L_ACTUATION_NORMAL_LENGTH_CM = 2.0f;
  // The thickness [cm] to draw actuation normals.
  const float L_ACTUATION_NORMAL_THICKNESS_CM = 0.2f;
  // The color to draw actuation thresholds.
  const FColor ACTUATION_THRESHOLD_COLOR = DEBUG_GRAY;
  // The thickness [cm] to draw actuation thresholds.
  const float L_ACTUATION_THRESHOLD_THICKNESS_CM = 0.35f;
  // The color to draw contact forces.
  const FColor FORCE_COLOR = DEBUG_BLUE_OR_YELLOW;
  // The thickness [cm] to draw contact forces.
  const float L_FORCE_THICKNESS_CM = 0.355f;

  if (!IsValid(GetSkeletalMeshComponent()) || glove_ == nullptr) {
    return;
  }

  for (const HaptxApi::Retractuator& retractuator : glove_->retractuators) {
    float force_target_cn = 0.0f;
    if (hx_core_->getContactInterpreter().tryGetRetractuatorForceTargetN(glove_->id,
        retractuator.id, &force_target_cn)) {
      force_target_cn = unrealFromHxLength(force_target_cn);
    }

    HaptxApi::PassiveForceActuator::State state =
        HaptxApi::PassiveForceActuator::State::DISENGAGED;
    hx_core_->getContactInterpreter().tryGetRetractuatorStateTarget(glove_->id,
        retractuator.id, &state);

    for (const auto& restriction : retractuator.restrictions) {
      HaptxApi::Finger finger = HaptxApi::getFinger(restriction.first);
      HaptxApi::FingerBone finger_bone = HaptxApi::getFingerBone(restriction.first);
      if (finger == HaptxApi::Finger::F_LAST || finger_bone == HaptxApi::FingerBone::FB_LAST ||
          finger_bone != HaptxApi::FingerBone::FB_PROXIMAL) {  // Only draw on proximal segments.
        continue;
      }

      FTransform w_bone_transform =
          GetSkeletalMeshComponent()->GetSocketTransform(hand_bone_names_[finger][finger_bone]);
      for (const HaptxApi::Vector3D& actuation_normal : restriction.second) {
        const FVector l_actuation_normal = unrealFromHxVector(actuation_normal);
        const FVector w_actuation_normal = w_bone_transform.GetRotation() * l_actuation_normal;
        const FQuat w_actuation_normal_orientation = w_bone_transform.GetRotation() *
            l_actuation_normal.Rotation().Quaternion();
        const FQuat w_actuation_normal_rotation;
        FVector w_actuation_normal_base = w_bone_transform.GetLocation() +
            hand_scale_factor_ * L_OFFSET_CM * w_actuation_normal;

        // Draw the actuation normal.
        FVector w_actuation_normal_extent = hand_scale_factor_ / 2.0f *
            FVector(L_ACTUATION_NORMAL_LENGTH_CM, L_ACTUATION_NORMAL_THICKNESS_CM,
            L_ACTUATION_NORMAL_THICKNESS_CM);
        HxDebugDrawSystem::box(
            this,
            w_actuation_normal_base + w_actuation_normal_extent.X * w_actuation_normal,
            w_actuation_normal_extent,
            w_actuation_normal_orientation,
            state == HaptxApi::PassiveForceActuator::State::ENGAGED ?
            ACTUATION_NORMAL_ENGAGED_COLOR : ACTUATION_NORMAL_DISENGAGED_COLOR);

        // Is there a non-zero actuation threshold?
        const FRetractuatorParameters retractuator_parameters =
            retractuator_parameters_.getParametersForFinger(finger);
        if (retractuator_parameters.actuation_threshold_cn > 0.0f) {
          float l_actuation_threshold_length =
              force_feedback_visualization_parameters_.force_scale_cm_cn *
              retractuator_parameters.actuation_threshold_cn;
          FVector w_actuation_threshold_extent = hand_scale_factor_ / 2.0f *
              FVector(l_actuation_threshold_length, L_ACTUATION_THRESHOLD_THICKNESS_CM,
              L_ACTUATION_THRESHOLD_THICKNESS_CM);
          HxDebugDrawSystem::box(
              this,
              w_actuation_normal_base + w_actuation_threshold_extent.X * w_actuation_normal,
              w_actuation_threshold_extent,
              w_actuation_normal_orientation,
              state == HaptxApi::PassiveForceActuator::State::ENGAGED ?
              ACTUATION_NORMAL_ENGAGED_COLOR : ACTUATION_THRESHOLD_COLOR);
        }

        // Draw the force target.
        if (force_target_cn > 0.0f) {
          FVector w_force_extent = hand_scale_factor_ / 2.0f * FVector(
              force_feedback_visualization_parameters_.force_scale_cm_cn * force_target_cn,
              L_FORCE_THICKNESS_CM, L_FORCE_THICKNESS_CM);
          HxDebugDrawSystem::box(
              this,
              w_actuation_normal_base + w_force_extent.X * w_actuation_normal,
              w_force_extent,
              w_actuation_normal_orientation,
              state == HaptxApi::PassiveForceActuator::State::ENGAGED ?
              ACTUATION_NORMAL_ENGAGED_COLOR : FORCE_COLOR);
        }
      }
    }
  }
}

void AHxHandActor::visualizeContactDamping() {
  for (const auto& it : damping_constraint_from_object_id_) {
    // Alias for convenience.
    const UPhysicsConstraintComponent* constraint = it.Value;

    if (!IsValid(constraint)) {
      continue;
    }

    const UPrimitiveComponent* component = constraint->OverrideComponent2.Get();
    if (!IsValid(component)) {
      continue;
    }

    FBodyInstance* body = component->GetBodyInstance(
        constraint->ConstraintInstance.ConstraintBone2);
    if (body == nullptr) {
      continue;
    }

    FBox bounds = body->GetBodyBounds();
    DrawDebugBox(GetWorld(), bounds.GetCenter(), bounds.GetExtent(), DEBUG_PURPLE_OR_TEAL);
  }
}

void AHxHandActor::visualizeMocapData(const HaptxApi::MocapFrame mocap_frame,
    const FTransform w_mcp3, const FTransform w_vive) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeMocapData)
  // The scale to draw default-sized coordinate frames.
  const float COORD_SYS_SCALE = 3.0f;
  // The thickness to draw default-size coordinate frames.
  const float COORD_SYS_THICKNESS = 0.2f;
  // A scale factor applied to the MCP3 coordinate frame.
  const float MCP3_REL_SIZE = 1.6f;
  // A scale factor applied to the VIVE coordinate frame.
  const float VIVE_REL_SIZE = 1.4f;
  // A scale factor applied to the Hyleas source coordinate frame.
  const float HYLEAS_SOURCE_REL_SIZE = 1.2f;

  if (glove_ == nullptr) {
    return;
  }

  // Draw the world transform of MCP3.
  HxDebugDrawSystem::coordinateSystem(this, w_mcp3.GetTranslation(), w_mcp3.GetRotation(),
      MCP3_REL_SIZE * COORD_SYS_SCALE, MCP3_REL_SIZE * COORD_SYS_THICKNESS);

  // Draw the world transform of the tracked VIVE object.
  HxDebugDrawSystem::coordinateSystem(this, w_vive.GetLocation(),
      w_vive.GetRotation(), VIVE_REL_SIZE * COORD_SYS_SCALE, VIVE_REL_SIZE * COORD_SYS_THICKNESS);

  if (glove_->is_simulated) {
    // For simulated gloves we can draw where the VIVE tracker and Hyleas source would be, so why
    // not?
    if (glove_->vive_trackers.size() > 0) {
      const HaptxApi::ViveTracker vive_tracker = glove_->vive_trackers.front();
      FTransform w_vive_tracker = UKismetMathLibrary::ComposeTransforms(
          unrealFromHx(vive_tracker.transform), w_vive);
      HxDebugDrawSystem::coordinateSystem(this, w_vive_tracker.GetLocation(),
          w_vive_tracker.GetRotation(), VIVE_REL_SIZE * COORD_SYS_SCALE,
          VIVE_REL_SIZE * COORD_SYS_THICKNESS);
    }

    if (glove_->hyleas_sources.size() > 0) {
      const HaptxApi::HyleasSource hyleas_source = glove_->hyleas_sources.front();
      FTransform w_hyleas_source = UKismetMathLibrary::ComposeTransforms(
          unrealFromHx(hyleas_source.transform), w_vive);
      HxDebugDrawSystem::coordinateSystem(this, w_hyleas_source.GetLocation(),
          w_hyleas_source.GetRotation(), HYLEAS_SOURCE_REL_SIZE * COORD_SYS_SCALE,
          HYLEAS_SOURCE_REL_SIZE * COORD_SYS_THICKNESS);
    }
  } else if (glove_->vive_trackers.size() > 0 && glove_->hyleas_sources.size() > 0) {
    // For real gloves we only need to draw the Hyleas source, and we only know it's rigid
    // connection relative to the VIVE tracker.
    const HaptxApi::ViveTracker vive_tracker = glove_->vive_trackers.front();
    const HaptxApi::HyleasSource hyleas_source = glove_->hyleas_sources.front();
    FTransform l_hyleas_source = unrealFromHx(
        vive_tracker.transform.inverse() * hyleas_source.transform);
    FTransform w_hyleas_source = UKismetMathLibrary::ComposeTransforms(l_hyleas_source, w_vive);
    HxDebugDrawSystem::coordinateSystem(this, w_hyleas_source.GetLocation(),
        w_hyleas_source.GetRotation(), HYLEAS_SOURCE_REL_SIZE * COORD_SYS_SCALE,
        HYLEAS_SOURCE_REL_SIZE * COORD_SYS_THICKNESS);
  }

  // Draw all the mocap transforms that are relative to MCP3.
  HaptxApi::HaptxName mcp3_name = HaptxApi::getName(HaptxApi::getBodyPartJoint(
      HaptxApi::RelativeDirection(hand_), HaptxApi::F_MIDDLE, HaptxApi::FJ_JOINT1));
  for (auto& it : mocap_frame.transforms) {
    if (it.first.parent == mcp3_name) {
      FTransform w_datum = UKismetMathLibrary::ComposeTransforms(unrealFromHx(it.second), w_mcp3);

      HxDebugDrawSystem::coordinateSystem(this, w_datum.GetLocation(), w_datum.GetRotation(),
          COORD_SYS_SCALE, COORD_SYS_THICKNESS);
    }
  }
}

void AHxHandActor::visualizeHandAnimation(const HaptxApi::AnimFrame anim_frame,
    const HaptxApi::UserProfile profile, const FTransform w_mcp3) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeHandAnimation)
  const float COR_RADIUS_CM = 0.4f;
  const float BONE_THICKNESS_CM = 0.1f;

  // For aesthetic purposes only. Everything is actually located with respect to MCP3.
  FTransform w_palm = UKismetMathLibrary::ComposeTransforms(
      FTransform(FQuat::Identity, -l_middle1_cm_, FVector::OneVector), w_mcp3);
  DrawDebugSphere(GetWorld(), w_palm.GetLocation(), COR_RADIUS_CM, 8, HAPTX_ORANGE);

  for (int f_i = 0; f_i < HaptxApi::F_LAST; f_i++) {
    FTransform w_parent = UKismetMathLibrary::ComposeTransforms(FTransform(
        unrealFromHx(HaptxApi::JOINT1_ORIENT_CONVENTIONS[HaptxApi::RelativeDirection(hand_)][f_i]),
        unrealFromHxLength(profile.mcp3_joint1_pos_offsets_m[HaptxApi::RelativeDirection(hand_)][f_i]),
        FVector::OneVector), w_mcp3);
    // Draw a line to the joint1 (knuckle joint) location.
    DrawDebugLine(GetWorld(), w_palm.GetLocation(), w_parent.GetLocation(), HAPTX_GREEN, false,
        -1.f, 0u, BONE_THICKNESS_CM);

    for (int fj_i = 0; fj_i < HaptxApi::FJ_LAST; fj_i++) {
      // Draw a sphere at this joint location.
      DrawDebugSphere(GetWorld(), w_parent.GetLocation(), COR_RADIUS_CM, 8, HAPTX_ORANGE);
      auto l_joint = anim_frame.l_orientations.find(HaptxApi::getBodyPartJoint(
          HaptxApi::RelativeDirection(hand_), static_cast<HaptxApi::Finger>(f_i),
          static_cast<HaptxApi::FingerJoint>(fj_i)));
      if (l_joint == anim_frame.l_orientations.end()) {
        break;
      } else {
        FVector w_pos_next_cm = w_parent.TransformPosition(unrealFromHxLength(
            l_joint->second.rotate(HaptxApi::Vector3D::forward() *
            profile.finger_bone_lengths_m[f_i][fj_i])));
        // Draw a line between this joint location and the next.
        DrawDebugLine(GetWorld(), w_parent.GetLocation(), w_pos_next_cm, HAPTX_GREEN, false,
            -1.f, 0u, BONE_THICKNESS_CM);
        w_parent.SetLocation(w_pos_next_cm);
        w_parent.SetRotation(w_parent.GetRotation() * unrealFromHx(l_joint->second));
      }
    }
    // Draw a sphere at the fingertip.
    DrawDebugSphere(GetWorld(), w_parent.GetLocation(), COR_RADIUS_CM, 8, HAPTX_ORANGE);
  }
}

void AHxHandActor::visualizeHandAnimation2() {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeHandAnimation2)
  // The radius of the sphere drawn at joint centers of rotation.
  const float L_SOCKET_RADIUS = 0.3f;
  // The number of triangles in the joint center of rotation sphere.
  const int SOCKET_NUM_SEGMENTS = 12;
  // The thickness of the bones connecting joint centers of rotation.
  const float L_BONE_THICKNESS = 0.1f;

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    return;
  }

  for (const FName& name : smc->GetAllSocketNames()) {
    const FVector socket_loc = smc->GetSocketLocation(name);
    DrawDebugSphere(GetWorld(), socket_loc, L_SOCKET_RADIUS,
        hand_scale_factor_ * SOCKET_NUM_SEGMENTS, HAPTX_TEAL);

    const FName parent_name = smc->GetParentBone(name);
    if (parent_name != NAME_None) {
      const FVector parent_loc = smc->GetSocketLocation(parent_name);
      DrawDebugLine(GetWorld(), parent_loc, socket_loc, HAPTX_YELLOW, false, -1.f, 0u,
          hand_scale_factor_ * L_BONE_THICKNESS);
    }
  }
}

bool AHxHandActor::initializeDisplacementVisualizer() {
  // The name of the material property controlling the displacement visualizer's color.
  const FName COLOR_PARAM_NAME = FName(TEXT("Color"));

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    UE_LOG(HaptX, Error,
        TEXT("AHxHandActor::initializeDisplacementVisualizer(): Invalid skeletal mesh component."))
    return false;
  }

  if (!IsValid(smc->SkeletalMesh)) {
    UE_LOG(HaptX, Error,
        TEXT("AHxHandActor::initializeDisplacementVisualizer(): Invalid skeletal mesh."))
    return false;
  }

  // Copy the physically simulating skeletal mesh.
  dis_vis_pmc_ = NewObject<UPoseableMeshComponent>(this);
  if (!IsValid(smc->SkeletalMesh)) {
    UE_LOG(HaptX, Error, TEXT(
        "AHxHandActor::initializeDisplacementVisualizer(): Failed to create poseable mesh component."))
    return false;
  }
  dis_vis_pmc_->RegisterComponent();
  dis_vis_pmc_->SetSkeletalMesh(smc->SkeletalMesh, true);
  dis_vis_pmc_->SetWorldLocationAndRotation(smc->GetComponentLocation(), smc->GetComponentQuat());
  dis_vis_pmc_->SetWorldScale3D(smc->GetComponentScale());

  // Setup our custom material and sync relevant variables.
  dis_vis_mat_inst_ = UMaterialInstanceDynamic::Create(dis_vis_mat_, this);
  if (!IsValid(dis_vis_mat_inst_)) {
    UE_LOG(HaptX, Error, TEXT(
        "AHxHandActor::initializeDisplacementVisualizer(): Failed to create dynamic material instance."))
    return false;
  } else {
    dis_vis_mat_inst_->SetVectorParameterValue(COLOR_PARAM_NAME, dis_vis_parameters_.color);
    dis_vis_pmc_->SetMaterial(0, dis_vis_mat_inst_);
  }

  return true;
}

void AHxHandActor::updateDisplacementVisualizer(float delta_time_s) {
  // The name of the material property controlling the displacement visualizer's opacity.
  const FName OPACITY_PARAM_NAME = FName(TEXT("Opacity"));

  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc) || !IsValid(dis_vis_pmc_) || !IsValid(dis_vis_mat_inst_)) {
    return;
  }

  // Compute the greatest displacement between hand bones.
  TArray<FName> bone_names;
  smc->GetBoneNames(bone_names);
  float max_distance = 0.0f;
  for (const FName& bone_name : bone_names) {
    float distance = (smc->GetBoneLocation(bone_name, EBoneSpaces::WorldSpace) -
        dis_vis_pmc_->GetBoneLocation(bone_name, EBoneSpaces::WorldSpace)).Size();
    max_distance = fmaxf(distance, max_distance);
  }

  // Convert the distance to an interpolation alpha and apply it.
  float a = (max_distance - dis_vis_parameters_.min_displacement_cm) /
      (dis_vis_parameters_.max_displacement_cm - dis_vis_parameters_.min_displacement_cm);
  a = FMath::Clamp(a, 0.0f, 1.0f);
  float opacity = FMath::Lerp(0.0f, dis_vis_parameters_.max_opacity, a);
  dis_vis_mat_inst_->SetScalarParameterValue(OPACITY_PARAM_NAME, opacity);
}

void AHxHandActor::uninitializeDisplacementVisualizer() {
  if (IsValid(dis_vis_pmc_)) {
    dis_vis_pmc_->DestroyComponent();
    dis_vis_pmc_ = nullptr;
  }
  dis_vis_mat_inst_ = nullptr;  // Should be cleaned up by garbage collection.
  return;
}

void AHxHandActor::visualizeNetworkState() {
  // Added to color for hands that are instantiated on the server.
  FColor AUTHORITATIVE_COLOR = FColor::Blue;
  // Added to color for hands that are controlled by the local player.
  FColor LOCALLY_CONTROLLED_COLOR = FColor::Green;
  // Added to color for hands that have physics authority.
  FColor PHYSICS_AUTHORITATIVE_COLOR = FColor::Red;
  // For objects that are currently replicating movement.
  FColor MOVEMENT_REPLICATION_COLOR = FColor::Green;
  // For objects that have had movement replication paused.
  FColor MOVEMENT_REPLICATION_PAUSED_COLOR = FColor::Yellow;

  // The hands get a different color for each unique combination of server authoritative, locally
  // controlled, and physics authoritative.
  FColor network_state_color = FColor::Black;
  if (isAuthoritative()) {
    network_state_color += AUTHORITATIVE_COLOR;
  }
  if (isLocallyControlled()) {
    network_state_color += LOCALLY_CONTROLLED_COLOR;
  }
  if (isPhysicsAuthority()) {
    network_state_color += PHYSICS_AUTHORITATIVE_COLOR;
  }
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (IsValid(smc) && IsValid(PhysicsAuthorityZone)) {
    float w_radius_cm = PhysicsAuthorityZone->GetScaledSphereRadius();

    DrawDebugSphere(GetWorld(), smc->GetSocketLocation(bone_names_.middle1), w_radius_cm, 12,
        network_state_color);
  }

  // Objects that are currently under the purview of this networking system get highlighted.
  for (auto& it : objects_in_physics_authority_zone_) {
    UPrimitiveComponent* object = it.Key;
    if (!IsValid(object)) {
      continue;
    }

    FColor object_network_state_color = PHYSICS_AUTHORITATIVE_COLOR;
    auto global_data_it = global_physics_authority_data_from_comp_.Find(object);
    // On the server the highlight color changes when multiple players are interacting with an
    // object, indicating that the object is responsible for those players' hands having server
    // authority.
    if (isAuthoritative()) {
      if (global_data_it != nullptr &&
          global_data_it->physics_authority_zone_count_from_pawn.Num() > 1) {
        object_network_state_color = AUTHORITATIVE_COLOR;
      }
    // On clients object color depends on whether they are currently replicating movement or were
    // replicating movement before they came under our control.
    } else {
      AActor* actor = object->GetOwner();
      if (IsValid(actor) && object == actor->GetRootComponent()) {
        if (actor->bReplicateMovement) {
          object_network_state_color = MOVEMENT_REPLICATION_COLOR;
        } else if (global_data_it != nullptr && global_data_it->was_replicating_movement) {
          object_network_state_color = MOVEMENT_REPLICATION_PAUSED_COLOR;
        }
      }
    }

    FBox object_bounds = object->CalcBounds(object->GetComponentTransform()).GetBox();
    DrawDebugBox(GetWorld(), object_bounds.GetCenter(), object_bounds.GetExtent(),
        object_network_state_color);
  }

  int num_active_local_constraints = 0;
  for (UPhysicsConstraintComponent* constraint : local_constraints_) {
    if (IsValid(constraint) && constraint->ConstraintInstance.IsValidConstraintInstance()) {
      num_active_local_constraints++;
    }
  }
  AHxOnScreenLog::logToScreen(FString::Printf(TEXT("%d: %d active local constraints"),
      GetUniqueID(), num_active_local_constraints), 0.0f, FColor::Green);
  AHxOnScreenLog::logToScreen(FString::Printf(TEXT("%d: %d active replicated constraints"),
      GetUniqueID(), replicated_constraints_.Num()), 0.0f, FColor::Blue);
}

bool AHxHandActor::connectToCore() {
  if (IsValid(hx_core_)) {
    return true;
  }

  // Get pawn and enable input on this actor.
  pawn_ = getPawn(this);
  if (IsValid(pawn_)) {
    APlayerController* pc = Cast<APlayerController>(pawn_->Controller);
    if (IsValid(pc)) {
      EnableInput(pc);
    }
  }

  // Make sure there's a HaptxApi::Core in the level, and make sure it has tried to open
  hx_core_ = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(hx_core_)) {
    hardDisable();
    return false;
  }

  if (isLocallyControlled()) {
    // Find a mocap system that we can use, starting with gloves connected to a Dk2AirController.
    for (auto dk2_air_controller : hx_core_->getHaptxSystem().getDk2AirControllers()) {
      std::map<int, std::shared_ptr<HaptxApi::HyleasSystem>> hyleas_systems;
      if (dk2_air_controller != nullptr && dk2_air_controller->getHyleasSystems(&hyleas_systems) ==
          HaptxApi::AirController::ReturnCode::SUCCESS) {
        for (auto hs_it : hyleas_systems) {
          if (hs_it.second != nullptr &&
              hs_it.second->getRelativeDirection() == HaptxApi::RelativeDirection(hand_)) {
            mocap_system_ = hs_it.second;
            glove_ = hs_it.second->getGlove();
            break;
          }
        }

        if (mocap_system_.lock() != nullptr) {
          break;
        }
      }
    }

    // If we didn't find a glove connected to an Air Controller, look for a glove connected
    // elsewhere on the system.
    if (mocap_system_.lock() == nullptr) {
      for (auto hyleas_system : hx_core_->getHaptxSystem().getHyleasSystems()) {
        if (hyleas_system != nullptr && hyleas_system->getRelativeDirection() ==
            HaptxApi::RelativeDirection(hand_)) {
          mocap_system_ = hyleas_system;
          glove_ = hyleas_system->getGlove();
          break;
        }
      }
    }

    if (mocap_system_.lock() == nullptr) {
      UE_LOG(HaptX, Log, TEXT(
          "AHxHandActor::connectToCore(): No hardware detected for the %s hand. Using simulated peripheral instead."),
          HAND_AS_TEXT);
      const std::wstring& file_name = hand_ == ERelativeDirection::LEFT ?
          HaptxApi::SimulatedPeripheralDatabase::DK2_GLOVE_LARGE_LEFT_FILE_NAME :
          HaptxApi::SimulatedPeripheralDatabase::DK2_GLOVE_LARGE_RIGHT_FILE_NAME;
      std::shared_ptr<HaptxApi::Peripheral> peripheral;
      HaptxApi::SimulatedPeripheralDatabase::ReturnCode ret =
          HaptxApi::SimulatedPeripheralDatabase::getSimulatedPeripheral(file_name, &peripheral);
      if (ret != HaptxApi::SimulatedPeripheralDatabase::ReturnCode::SUCCESS) {
        AHxCoreActor::logError(FString::Printf(TEXT("Failed to load simulated peripheral for the %s hand."),
            HAND_AS_TEXT), true);
        UE_LOG(HaptX, Error, TEXT(
            "AHxHandActor::connectToCore(): HaptxApi::SimulatedPeripheralDatabase::getSimulatedPeripheral() failed with error code %d: %s."),
            ret, *STRING_TO_FSTRING(HaptxApi::SimulatedPeripheralDatabase::toString(ret)));
        hardDisable();
        return false;
      }

      glove_ = std::dynamic_pointer_cast<HaptxApi::Glove>(peripheral);
      if (glove_ == nullptr) {
        AHxCoreActor::logError(FString::Printf(TEXT(
            "Loaded a non-glove peripheral for the %s hand."), HAND_AS_TEXT), true);
        hardDisable();
        return false;
      }

      hx_core_->registerSimulatedPeripheral(*glove_);
    }
  }

  // We call this in OnGlobalCreatePhysics() as well in case our underlying physics bodies change.
  registerBones();
  return true;
}

void AHxHandActor::registerBones() {
  USkeletalMeshComponent* smc = GetSkeletalMeshComponent();
  if (!IsValid(smc)) {
    hardDisable();
    return;
  }

  if (!IsValid(hx_core_)) {
    // This will happen on the first OnGlobalCreatePhysics() before BeginPlay() has been called.
    return;
  }

  // Contact interpreter body registration.
  // palm
  bone_data_from_bone_name_.FindOrAdd(bone_names_.palm).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.palm));
  bone_data_from_bone_name_[bone_names_.palm].rigid_body_part = hand_ == ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // Proximal finger segments are treated as the same body as the palm.
  // thumb1
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb1).ci_body_id =
      bone_data_from_bone_name_[bone_names_.palm].ci_body_id;
  bone_data_from_bone_name_[bone_names_.thumb1].rigid_body_part =
      hand_ == ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // index1
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index1).ci_body_id =
      bone_data_from_bone_name_[bone_names_.palm].ci_body_id;
  bone_data_from_bone_name_[bone_names_.index1].rigid_body_part =
      hand_ == ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // middle1
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle1).ci_body_id =
      bone_data_from_bone_name_[bone_names_.palm].ci_body_id;
  bone_data_from_bone_name_[bone_names_.middle1].rigid_body_part = hand_ ==
      ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // ring1
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring1).ci_body_id =
      bone_data_from_bone_name_[bone_names_.palm].ci_body_id;
  bone_data_from_bone_name_[bone_names_.ring1].rigid_body_part = hand_ ==
      ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // pinky1
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky1).ci_body_id =
      bone_data_from_bone_name_[bone_names_.palm].ci_body_id;
  bone_data_from_bone_name_[bone_names_.pinky1].rigid_body_part = hand_ ==
      ERelativeDirection::LEFT ?
      HaptxApi::RigidBodyPart::LEFT_PALM : HaptxApi::RigidBodyPart::RIGHT_PALM;

  // Medial and distal segments are treated as the same body
  // thumb2 & thumb3
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb2).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.thumb2));
  bone_data_from_bone_name_[bone_names_.thumb2].rigid_body_part = HaptxApi::getRigidBodyPart(
      static_cast<HaptxApi::RelativeDirection>(hand_), HaptxApi::Finger::F_THUMB,
      HaptxApi::FingerBone::FB_MEDIAL);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb3).ci_body_id =
      bone_data_from_bone_name_[bone_names_.thumb2].ci_body_id;
  bone_data_from_bone_name_[bone_names_.thumb3].rigid_body_part =
      bone_data_from_bone_name_[bone_names_.thumb2].rigid_body_part;

  // index2 & index3
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index2).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.index2));
  bone_data_from_bone_name_[bone_names_.index2].rigid_body_part = HaptxApi::getRigidBodyPart(
      static_cast<HaptxApi::RelativeDirection>(hand_), HaptxApi::Finger::F_INDEX,
      HaptxApi::FingerBone::FB_MEDIAL);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index3).ci_body_id =
      bone_data_from_bone_name_[bone_names_.index2].ci_body_id;
  bone_data_from_bone_name_[bone_names_.index3].rigid_body_part =
      bone_data_from_bone_name_[bone_names_.index2].rigid_body_part;

  // middle2 & middle3
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle2).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.middle2));
  bone_data_from_bone_name_[bone_names_.middle2].rigid_body_part = HaptxApi::getRigidBodyPart(
      static_cast<HaptxApi::RelativeDirection>(hand_), HaptxApi::Finger::F_MIDDLE,
      HaptxApi::FingerBone::FB_MEDIAL);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle3).ci_body_id =
      bone_data_from_bone_name_[bone_names_.middle2].ci_body_id;
  bone_data_from_bone_name_[bone_names_.middle3].rigid_body_part =
      bone_data_from_bone_name_[bone_names_.middle2].rigid_body_part;

  // ring2 & ring3
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring2).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.ring2));
  bone_data_from_bone_name_[bone_names_.ring2].rigid_body_part = HaptxApi::getRigidBodyPart(
      static_cast<HaptxApi::RelativeDirection>(hand_), HaptxApi::Finger::F_RING,
      HaptxApi::FingerBone::FB_MEDIAL);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring3).ci_body_id =
      bone_data_from_bone_name_[bone_names_.ring2].ci_body_id;
  bone_data_from_bone_name_[bone_names_.ring3].rigid_body_part =
      bone_data_from_bone_name_[bone_names_.ring2].rigid_body_part;

  // pinky2 & pinky3
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky2).ci_body_id =
      getBodyInstanceId(smc->GetBodyInstance(bone_names_.pinky2));
  bone_data_from_bone_name_[bone_names_.pinky2].rigid_body_part = HaptxApi::getRigidBodyPart(
      static_cast<HaptxApi::RelativeDirection>(hand_), HaptxApi::Finger::F_PINKY,
      HaptxApi::FingerBone::FB_MEDIAL);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky3).ci_body_id =
      bone_data_from_bone_name_[bone_names_.pinky2].ci_body_id;
  bone_data_from_bone_name_[bone_names_.pinky3].rigid_body_part =
      bone_data_from_bone_name_[bone_names_.pinky2].rigid_body_part;

  for (auto& it : bone_data_from_bone_name_) {
    hx_core_->registerBodyWithCi(it.Value.ci_body_id, smc, it.Key, it.Key ==
        bone_names_.palm ? palm_body_parameters_ : finger_body_parameters_,
        it.Value.rigid_body_part);
    it.Value.has_ci_body_id = true;
  }

  // Define grasp detector interface body IDs.
  HaptxApi::GraspDetector& gd = hx_core_->getGraspDetector();
  whole_hand_gd_body_id_ = gd.registerBody();
  bone_data_from_bone_name_.FindOrAdd(bone_names_.palm).gd_body_id = gd.registerBody(
      whole_hand_gd_body_id_);
  // Thumb1 and the palm are treated as the same body to prevent objects from getting stuck
  // between them.
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb1).gd_body_id =
      bone_data_from_bone_name_.FindOrAdd(bone_names_.palm).gd_body_id;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb2).gd_body_id =
      gd.registerBody(whole_hand_gd_body_id_);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb3).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.thumb2].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index1).gd_body_id =
      gd.registerBody(whole_hand_gd_body_id_);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index2).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.index1].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index3).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.index2].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle1).gd_body_id =
      gd.registerBody(whole_hand_gd_body_id_);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle2).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.middle1].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle3).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.middle2].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring1).gd_body_id =
      gd.registerBody(whole_hand_gd_body_id_);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring2).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.ring1].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring3).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.ring2].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky1).gd_body_id =
      gd.registerBody(whole_hand_gd_body_id_);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky2).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.pinky1].gd_body_id);
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky3).gd_body_id =
      gd.registerBody(bone_data_from_bone_name_[bone_names_.pinky2].gd_body_id);
  hx_core_->registerGdBody(whole_hand_gd_body_id_, smc, bone_names_.palm, true);
  for (auto &key_value : bone_data_from_bone_name_) {
    hx_core_->registerGdBody(key_value.Value.gd_body_id, smc, key_value.Key);
    key_value.Value.has_gd_body_id = true;
  }
  // Make sure the palm's bone is registered under its GD body Id, not thumb1's.
  hx_core_->registerGdBody(bone_data_from_bone_name_[bone_names_.palm].gd_body_id, smc,
      bone_names_.palm);

  // Denote which bones are capable of engaging the contact damping algorithm.
  bone_data_from_bone_name_.FindOrAdd(bone_names_.palm).can_engage_contact_damping = true;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.thumb1).can_engage_contact_damping = true;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.index1).can_engage_contact_damping = true;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.middle1).can_engage_contact_damping = true;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.ring1).can_engage_contact_damping = true;
  bone_data_from_bone_name_.FindOrAdd(bone_names_.pinky1).can_engage_contact_damping = true;
}

void AHxHandActor::registerRetractuators() {
  if (!IsValid(hx_core_)) {
    AHxCoreActor::logWarning(
        "AHxHandActor::registerRetractuators(): Null core.");
    return;
  }

  if (glove_ == nullptr) {
    AHxCoreActor::logWarning(
        "AHxHandActor::registerRetractuators(): Null glove.");
    return;
  }

  for (const HaptxApi::Retractuator& retractuator : glove_->retractuators) {
    if (retractuator.restrictions.empty()) {
      continue;
    }

    HaptxApi::Finger finger = HaptxApi::getFinger(retractuator.restrictions.begin()->first);
    if (finger == HaptxApi::Finger::F_LAST) {
      continue;
    }

    bool multi_finger_retractuator = false;
    for (const auto& restriction : retractuator.restrictions) {
      if (finger != HaptxApi::getFinger(restriction.first)) {
        multi_finger_retractuator = true;
        break;
      }
    }
    if (multi_finger_retractuator) {
      AHxCoreActor::logWarning(
          "AHxHandActor::registerRetractuators(): Multi-finger retractuator not supported.");
      continue;
    }

    hx_core_->getContactInterpreter().registerRetractuator(glove_->id, retractuator,
        retractuator_parameters_.getParametersForFinger(finger).unwrap());
  }
}

void AHxHandActor::OnRep_w_uhp_hand_scale_factor_() {
  hand_needs_scale_update_ = true;
}

void AHxHandActor::OnRep_hand_scale_factor_() {
  hand_needs_scale_update_ = true;
}

void AHxHandActor::OnRep_is_client_physics_authority_() {
  if (isPhysicsAuthority()) {
    clearReplicatedConstraints();
    setLocalConstraintsPhysicallyEnabled(true);
    if (isAuthoritative()) {
      physics_targets_buffer_tail_i_ = 0;
      physics_targets_buffer_head_i_ = 0;
      physics_targets_buffer_started_ = false;
    }
  } else {
    if (isLocallyControlled()) {
      local_constraints_need_disabled_ = true;
      physics_state_buffer_tail_i_ = 0;
      physics_state_buffer_head_i_ = 0;
      physics_state_buffer_started_ = false;
    } else if (isAuthoritative()) {
      local_constraints_need_disabled_ = true;
    }
  }
}
