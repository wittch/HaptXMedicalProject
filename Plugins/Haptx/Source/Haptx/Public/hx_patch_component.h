// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <memory>
#include <Runtime/Engine/Classes/Components/PrimitiveComponent.h>
#include <Runtime/Engine/Classes/Components/SkeletalMeshComponent.h>
#include <Runtime/Engine/Public/CollisionQueryParams.h>
#include <HaptxApi/contact_interpreter.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/contact_interpreter_parameters.h>
#include <Haptx/Public/hx_core_actor.h>
#include <Haptx/Public/hx_simulation_callbacks.h>
#include <Haptx/Public/ihaptx.h>
#include <Haptx/Public/peripheral_link.h>
#include "hx_patch_component.generated.h"

DECLARE_STATS_GROUP_IF_PROFILING(TEXT("HxPatch"), STATGROUP_HxPatch, STATCAT_Advanced)

//! @brief Represents parameters used in the tactile feedback visualizer.
//!
//! This struct only exists for organizational purposes in the details panel.

// Represents parameters used in the tactile feedback visualizer.
USTRUCT(BlueprintType)
struct FTactileFeedbackVisualizationParameters {
  GENERATED_BODY()

  //! @brief The amount by which to scale tactor height targets.
  //!
  //! Increase to make tactile feedback visualizations move more when tactors actuate.

  // The amount by which to scale tactor height targets.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  float height_target_scale = 5.f;

  //! The distance [cm] which tactor output visualizations are offset from tactor positions.

  // The distance [cm] which tactor output visualizations are offset from tactor positions.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  float draw_offset = 3.f;
};

//! Everything a patch needs to cache about a HaptxApi::Tactor.
struct HxPatchComponentTactorData {
  
  //! Complete constructor.
  //!
  //! @param tactor See #tactor
  //! @param callbacks See #callbacks
  HxPatchComponentTactorData(HaptxApi::Tactor tactor,
      std::shared_ptr<WeldedComponentCallbacks> callbacks) : tactor(tactor),
      callbacks(callbacks) {}

  //! The tactor itself.
  HaptxApi::Tactor tactor{};

  //! The tactor's simulation callbacks.
  std::shared_ptr<WeldedComponentCallbacks> callbacks{nullptr};

  //! Whether the following trace origin is valid.
  bool trace_origin_valid{false};

  //! The trace origin relative to this component.
  FTransform l_trace_origin{FTransform::Identity};
};

//! A custom FTickFunction so @link UHxPatchComponent UHxPatchComponents @endlink can tick 
//! before and after physics.
USTRUCT()
struct FHxPatchSecondaryTickFunction : public FTickFunction {
  GENERATED_BODY()

  //! The UHxPatchComponent that is ticking.
  class UHxPatchComponent* Target;

  //! Abstract function. Actually execute the tick.
  //!
  //! @param DeltaTime Frame time to advance [s].
  //! @param TickType Kind of tick for this frame.
  //! @param CurrentThread Thread we are executing on, useful to pass along as new tasks are 
  //! created.
  //! @param CompletionGraphEvent Completion event for this task. Useful for holding the 
  //! completion of this task until certain child tasks are complete.
  HAPTX_API virtual void ExecuteTick(
      float DeltaTime,
      ELevelTick TickType,
      ENamedThreads::Type CurrentThread,
      const FGraphEventRef& CompletionGraphEvent) override;

  //! Abstract function to describe this tick. Used to print messages about illegal cycles 
  //! in the dependency graph.
  HAPTX_API virtual FString DiagnosticMessage() override;
};
template<>
struct TStructOpsTypeTraits<FHxPatchSecondaryTickFunction> :
    public TStructOpsTypeTraitsBase2<FHxPatchSecondaryTickFunction> {
  enum {
    WithCopy = false
  };
};

//! @brief Represents a patch of Tactors. Responsible for tactile feedback and sampling the game
//! world for surface characteristics.
//!
//! See the @ref section_hx_patch_component "Unreal Plugin Guide" for a high level overview.
//!
//! @note UHxPatchComponent must have a uniform world scale. Non-uniform world scales will produce
//! undefined behavior.
//!
//! @ingroup group_unreal_plugin

// Represents a patch of Tactors.
UCLASS(ClassGroup = (Haptx), meta = (BlueprintSpawnableComponent))
class HAPTX_API UHxPatchComponent : public UPrimitiveComponent {
  GENERATED_UCLASS_BODY()

public:
  //! Called whenever a component has new physics data.
  //!
  //! @param component The component with new physics data.
  void OnGlobalCreatePhysics(UActorComponent* component);

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Called every frame post-physics.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  //! Called every frame post-update-work.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponentSecondary(
    float DeltaTime,
    ELevelTick TickType,
    FHxPatchSecondaryTickFunction& ThisTickFunction);

  //! Settings for this component's second tick function.
  struct FHxPatchSecondaryTickFunction SecondaryComponentTick;

  //! Ignores a given actor with all ray traces.
  //!
  //! @param actor The actor to ignore.

  // Ignores a given actor with all ray traces.
  UFUNCTION(BlueprintCallable)
  void ignoreActor(AActor* actor);

  //! Computes and caches trace origins for the skeletal mesh of the hand by tracing against a
  //! static mesh representation of the hand in its base pose.
  //!
  //! @param skeletal_mesh The skeletal mesh representing the physically simulated hand.
  //! @param static_mesh The static mesh version to trace against.
  void updateTraceOrigins(USkeletalMeshComponent* skeletal_mesh, UStaticMesh* static_mesh);

  //! The locating feature this patch gets aligned with.

  // The locating feature this patch gets aligned with.
  UPROPERTY(EditAnywhere, Category = "Sockets")
  FString locating_feature_;

  //! An offset applied to this patch in #locating_feature_'s frame.

  // An offset applied to this patch in the locating feature's frame.
  UPROPERTY(EditAnywhere, Category = "Sockets")
  FTransform locating_feature_offset_;

  //! The set of coverage regions this patch represents in the HaptxApi.
  
  // The set of coverage regions this patch represents in the HaptxApi.
  UPROPERTY(EditAnywhere, Category = "Sockets")
  TSet<FString> coverage_regions_;

  //! Parameters to be sent to the contact interpreter to configure individual tactor
  //! behavior.

  // Parameters to be sent to the contact interpreter to configure individual tactor behavior.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  FTactorParameters tactor_parameters_;

  //! The distance to trace outward from each sample point [cm].

  // The distance to trace outward from each sample point [cm].
  UPROPERTY(EditAnywhere, Category = "Tracing", meta=(UIMin="0.0", ClampMin="0.0"))
  float object_trace_distance_cm_{2.0f};

  //! Amount by which a raytrace begins "inside" of an object to account for positional 
  //! tolerance stack-up and object interpenetration [cm].

  // Amount by which a raytrace begins "inside" of an object to account for positional tolerance
  // stack-up and object interpenetration [cm].
  UPROPERTY(EditAnywhere, Category = "Tracing")
  float object_trace_offset_cm_{0.5f};

  //! Whether we trace against complex geometry on objects.

  // Whether we trace against complex geometry on objects.
  UPROPERTY(EditAnywhere, Category = "Tracing")
  bool object_trace_complex_{true};

  //! Which UV channels to query when tracing.

  // Which UV channels to query when tracing.
  UPROPERTY(EditAnywhere, Category = "Tracing")
  TArray<int32> object_trace_uv_channels_{};

  //! Which directions this patch emits ray traces in tactor space.

  // Which directions this patch emits ray traces in tactor space.
  UPROPERTY(EditAnywhere, Category = "Tracing", DisplayName = "Tactor Object Trace Directions")
  TArray<FVector> t_object_trace_directions_;

  //! Which directions this patch emits ray traces in component space.

  // Which directions this patch emits ray traces in component space.
  UPROPERTY(EditAnywhere, Category = "Tracing", DisplayName = "Component Object Trace Directions")
  TArray<FVector> l_object_trace_directions_;

  //! The amount by which to increment self traces if they fail [cm].

  // The amount by which to increment self traces if they fail [cm].
  UPROPERTY(EditAnywhere, Category = "Tracing", meta=(UIMin="0.0", ClampMin="0.0"))
  float self_trace_increment_cm_{0.1f};

  //! The maximum amount of self trace offset before giving up [cm].

  // The maximum amount of self trace offset before giving up [cm].
  UPROPERTY(EditAnywhere, Category = "Tracing", meta=(UIMin="0.0", ClampMin="0.0"))
  float max_self_trace_offset_cm_{3.0f};

  //! Visualize trace geometry.

  // Visualize trace geometry.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_traces_;

  //! This inline flag toggles tactile feedback visualization.

  // This inline flag toggles tactile feedback visualization.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization",
      meta = (InlineEditConditionToggle))
  bool visualize_tactile_feedback_;

  //! @brief Visualize tactile feedback state.
  //! 
  //! Visualizer displacements match hardware displacements multiplied by a configurable scale 
  //! factor.

  // Visualize tactile feedback state.
  UPROPERTY(EditAnywhere, Category = "Visualization", meta =
      (editcondition = "visualize_tactile_feedback_", DisplayName = "Visualize Tactile Feedback"))
  FTactileFeedbackVisualizationParameters tactile_feedback_visualization_parameters_;

private:
  //! Disable the functionality of this class.
  void hardDisable();

  //! Load the tactor list with information provided by the HaptX API.
  void registerTactors();

  //! Generates trace results and sends them to the HaptxApi::ContactInterpreter.
  void updateTraces();

  //! Sets raytrace blacklists and white lists.
  void configureTraceParameters();

  //! Draw tactile feedback visualization information.
  void visualizeTactileFeedbackOutput();

  //! Try to connect to the core, disable ourselves and return false if we fail.
  //!
  //! @returns Whether all connections succeeded.
  bool connectToCore();

  //! Updates patch transform according to #locating_feature_ and #locating_feature_offset_.
  void alignWithLocatingSocket();

  //! Object parameters for heightfield trace onto objects (WHITELIST).
  TArray<TEnumAsByte<EObjectTypeQuery>> object_trace_collision_type_whitelist_;

  //! Which actors to ignore during object traces (BLACKLIST).
  TArray<AActor*> object_trace_ignore_actors_;

  //! Reference to the AHxCoreActor pseudo-singleton.

  // Reference to the AHxCoreActor pseudo-singleton.
  UPROPERTY()
  AHxCoreActor *hx_core_;

  //! The UUID of the peripheral this patch is on.
  HaptxApi::HaptxUuid peripheral_id_{};

  //! Whether or not this HaptX component is disabled.
  bool is_enabled_;

  //! The list of tactors this patch represents.
  std::list<HxPatchComponentTactorData> tactor_data_{};
};
