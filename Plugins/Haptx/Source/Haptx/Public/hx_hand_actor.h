// Copyright (C) 2017-2021 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Animation/SkeletalMeshActor.h>
#include <Runtime/Engine/Classes/Components/PoseableMeshComponent.h>
#include <Runtime/Engine/Classes/Components/SphereComponent.h>
#include <Runtime/Core/Public/Containers/CircularBuffer.h>
#include <HaptxApi/anim_frame.h>
#include <HaptxApi/contact_interpreter.h>
#include <HaptxApi/glove_slip_compensator.h>
#include <HaptxApi/simulated_gestures.h>
#include <Haptx/Public/hx_core_actor.h>
#include <Haptx/Public/hx_hand_actor_structs.h>
#include <Haptx/Public/hx_patch_socket.h>
#include <Haptx/Public/peripheral_link.h>
#include "hx_hand_actor.generated.h"

class AHxHandActor;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeftHandInitialized, AHxHandActor*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRightHandInitialized, AHxHandActor*);
DECLARE_STATS_GROUP_IF_PROFILING(TEXT("AHxHandActor"), STATGROUP_AHxHandActor, STATCAT_Advanced)

//! @brief Represents one HaptX Glove.
//!
//! See the @ref section_hx_hand_actor "Unreal Plugin Guide" for a high level overview.
//!
//! @ingroup group_unreal_plugin

// Represents one HaptX Glove.
UCLASS(ClassGroup = (Haptx), meta = (BlueprintSpawnableComponent))
class HAPTX_API AHxHandActor : public ASkeletalMeshActor, public IPeripheralLink,
    public IHxPatchSocket {
  GENERATED_UCLASS_BODY()

public:
  //! Returns the properties used for network replication. This needs to be overridden by all actor
  //! classes with native replicated properties.
  //!
  //! @param [out] OutLifetimeProps The properties used for network replication.
  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty>& OutLifetimeProps) const override;

  //! Called whenever a component has new physics data.
  //!
  //! @param component The component with new physics data.
  void OnGlobalCreatePhysics(UActorComponent* component);

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Called every frame pre-physics.
  //!
  //! @param DeltaTime The time since the last tick.
  virtual void Tick(float DeltaTime) override;

  //! Called every frame post-update-work.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickSecondary(
    float DeltaTime,
    ELevelTick TickType,
    FHxHandSecondaryTickFunction& ThisTickFunction);

  //! Settings for this component's second tick function.
  struct FHxHandSecondaryTickFunction SecondaryTick;

  //! Called when hit by an object.
  //!
  //! @param MyComp The component on this actor that was hit.
  //! @param Other The other actor in the collision.
  //! @param OtherComp The component on the other actor that was hit.
  //! @param bSelfMoved When receiving a hit from another object's movement (bSelfMoved is false),
  //! the directions of 'Hit.Normal' and 'Hit.ImpactNormal' will be adjusted to indicate force from
  //! the other object against this object.
  //! @param HitLocation The world location where contact took place.
  //! @param HitNormal The world normal of the contact.
  //! @param NormalImpulse The impulse applied to resolve the contact.
  //! @param Hit Additional information about this collision.

  // Called when hit by an object.
  virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
      UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal,
      FVector NormalImpulse, const FHitResult& Hit) override;

  //! Get the name of the currently loaded user profile.
  //!
  //! @returns The name of the currently loaded user profile, or "No user profile loaded"

  // Get the name of the loaded user profile
  UFUNCTION(BlueprintPure, Category = "UserProfile")
  static FString GetUserProfileName();

  //! Get the length of the currently loaded user profile.
  //!
  //! @returns The length [m] of the currently loaded user profile, returns 0 if no profile

  // Get the hand lenght of the loaded user profile
  UFUNCTION(BlueprintPure, Category = "UserProfile")
  static float GetUserHandLength();

  //! Get the hand width of the currently loaded user profile.
  //!
  //! @returns The width [m] of the currently loaded user profile, returns 0 if no profile

  // Get the hand width of the loaded user profile
  UFUNCTION(BlueprintPure, Category = "UserProfile")
  static float GetUserHandWidth();

  //! Get whether this is a left or right hand.
  //!
  //! @returns Whether this is a left or right hand.

  // Get whether this is a left or right hand.
  UFUNCTION(BlueprintCallable)
  ERelativeDirection getHand() const;

  //! Binds various inputs to this actor's UInputComponent (if it has one).

  // Binds various inputs to this actor's UInputComponent (if it has one).
  UFUNCTION(BlueprintCallable)
  void bindInput();

  //! Get the currently active left hand.
  //!
  //! @returns The currently active left hand. Nullptr if there isn't an active left hand.

  // Get the currently active left hand. Nullptr if there isn't an active left hand.
  UFUNCTION(BlueprintCallable)
  static AHxHandActor* getLeftHand();

  //! Get the currently active right hand.
  //!
  //! @returns The currently active right hand. Nullptr if there isn't an active right hand.

  // Get the currently active right hand. Nullptr if there isn't an active right hand.
  UFUNCTION(BlueprintCallable)
  static AHxHandActor* getRightHand();

  //! Gets whether this hand is controlled by the local player.
  //!
  //! @returns True if this hand is controlled by the local player.

  // Gets whether this hand is controlled by the local player.
  UFUNCTION(BlueprintCallable)
  bool isLocallyControlled() const;

  //! Gets whether this function is being called on the server.
  //!
  //! @returns True if this function is being called on the server.

  // Gets whether this function is being called on the server.
  UFUNCTION(BlueprintCallable)
  bool isAuthoritative() const;

  //! Event that fires when the left hand is initialized.

  static FOnLeftHandInitialized on_left_hand_initialized;

  //! Event that fires when the right hand is initialized.

  static FOnRightHandInitialized on_right_hand_initialized;

  //! Set a new value for the "Hand Scale" property.
  //!
  //! @note This invalidates any physics handles to the hand.
  //!
  //! @param hand_scale_factor The new value.

  // Set a new value for the "Hand Scale" property.
  UFUNCTION(BlueprintCallable)
  void setHandScaleFactor(float hand_scale_factor);

  //! Teleports the root bone of the hand (the palm) to a given world location and rotation.
  //!
  //! @param new_location The new palm location.
  //! @param new_rotation The new palm rotation.

  // Teleports the root bone of the hand (the palm) to a given world location and rotation.
  UFUNCTION(BlueprintCallable)
  void teleportPalm(FVector new_location, FRotator new_rotation);

  //! Teleports the tracked bone of the hand (middle1) to a given world location and rotation.
  //!
  //! @param new_location The new middle1 location.
  //! @param new_rotation The new middle1 rotation.

  // Teleports the tracked bone of the hand (middle1) to a given world location and rotation.
  UFUNCTION(BlueprintCallable)
  void teleportMiddle1(FVector new_location, FRotator new_rotation);

  //! Adds a local constraint to get replicated with this hand.
  //!
  //! @param constraint The constraint to replicate with this hand.
  void notifyLocalConstraintCreated(UPhysicsConstraintComponent* constraint);

  //! Removes a physics constraint from being replicated with this hand.
  //!
  //! @param constraint The constraint to remove.
  void notifyLocalConstraintDestroyed(UPhysicsConstraintComponent* constraint);

  //! Toggle the mocap visualizer.

  // Toggle the mocap visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleMocapVisualizer();

  //! Toggle the trace visualizer.

  // Toggle the trace visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleTraceVisualizer();

  //! Toggle the tactile feedback visualizer.

  // Toggle the tactile feedback visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleTactileFeedbackVisualizer();

  //! Toggle the force feedback visualizer.

  // Toggle the force feedback visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleForceFeedbackVisualizer();

  //! Toggle the hand animation visualizer.

  // Toggle the hand animation visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleHandAnimationVisualizer();

  //! @brief Toggle the second hand animation visualizer.
  //!
  //! Note: this function exists for debugging purposes only and is not guaranteed to exist in
  // future releases.
  //!
  //! Shows the state of the avatar hand itself.

  // Toggle the second hand animation visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleHandAnimationVisualizer2();

  //! Toggle the displacement visualizer.

  // Toggle the displacement visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleDisplacementVisualizer();

  //! Toggle the contact damping visualizer.

  // Toggle the contact damping visualizer.
  UFUNCTION(BlueprintCallable)
  void toggleContactDampingVisualizer();

  //! Set whether the displacement visualizer is active.
  //!
  //! @param active True to enable the displacement visualizer.

  //! Set whether the displacement visualizer is active.
  UFUNCTION(BlueprintCallable)
  void setDisplacementVisualizerActive(bool active);

  //! Whether the displacement visualizer is enabled.

  // Whether the displacement visualizer is enabled.
  UFUNCTION(BlueprintCallable)
  bool isDisplacementVisualizerActive();

  //! Gets the displacement vector between the root bone of the physics hand and the same spot on
  //! the tracked hand.
  //!
  //! @returns The displacement vector between the root bone of the physics hand and the same spot on
  //! the tracked hand (in world space), or the zero vector if the displacement could not be
  //! calculated.

  // Gets the displacement vector between the root bone of the physics hand and the same spot on
  // the tracked hand.
  UFUNCTION(BlueprintCallable)
  FVector getTrackedDisplacement() const;

  virtual std::shared_ptr<HaptxApi::Peripheral> getPeripheral() override;

  virtual bool tryGetLocatingFeatureTransform(FName locating_feature,
      FTransform* w_transform) override;

  virtual bool tryGetRelativeDirection(ERelativeDirection* rel_dir) override;

  virtual bool tryGetCiBodyId(const UHxPatchComponent &patch, int64_t *ci_body_id) override;

// This section is here because we want these properties to show up first in the editor
protected:
  //! Whether this is a a left or right hand.

  // Whether this is a a left or right hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn="true"), Replicated)
  ERelativeDirection hand_;

  //! A scale factor that gets applied to the hand after user hand profile scaling is applied.

  // A scale factor that gets applied to the hand after user hand profile scaling is applied.
  UPROPERTY(EditAnywhere, BlueprintReadonly, meta = (DisplayName = "Hand Scale",
      ClampMin = "0.01", UIMin = "0.01"), ReplicatedUsing = OnRep_hand_scale_factor_)
  float hand_scale_factor_;

  //! Linear drive parameters used in the physics constraint driving the palm.

  // Linear drive parameters used in the physics constraint driving the palm.
  UPROPERTY(EditAnywhere)
  FConstraintDrive linear_drive_;

  //! Angular drive parameters used in the physics constraint driving the palm.

  // Angular drive parameters used in the physics constraint driving the palm.
  UPROPERTY(EditAnywhere)
  FConstraintDrive angular_drive_;

  //! Whether damping constraints can form between the palm and objects contacting it.

  // Whether damping constraints can form between the palm and objects contacting it.
  UPROPERTY(EditAnywhere)
  bool enable_contact_damping_;

  //! Linear damping used in the physics constraint that forms between the palm and objects
  //! contacting it.

  // Linear damping used in the physics constraint that forms between the palm and objects
  // contacting it.
  UPROPERTY(EditAnywhere, meta = (editcondition = "enable_contact_damping_", ClampMin = "0.0",
      UIMin = "0.0"))
  float linear_contact_damping_;

  //! Angular damping used in the physics constraint that forms between the palm and objects
  //! contacting it.

  // Angular damping used in the physics constraint that forms between the palm and objects
  // contacting it.
  UPROPERTY(EditAnywhere, meta = (editcondition = "enable_contact_damping_", ClampMin = "0.0",
      UIMin = "0.0"))
  float angular_contact_damping_;

public:
  //! @brief Which hand animation optimization mode to use.

  // Which hand animation optimization mode to use.
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  EHandAnimationOptimizationMode hand_anim_optimization_mode_{
      EHandAnimationOptimizationMode::DYNAMIC};

  //! @brief The relative distance threshold to use if #hand_anim_optimization_mode_ is set to
  //! EHandAnimationOptimizationMode::DYNAMIC.
  //!
  //! A value of 0 effectively disables dynamic hand animation optimization and a value of 1 makes
  //! it engage as early as possible.

  // The relative distance threshold to use if hand_anim_optimization_mode_ is set to
  // EHandAnimationOptimizationMode::DYNAMIC. A value of 0 effectively disables dynamic hand
  // animation optimization and a value of 1 makes it engage as early as possible.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0",
      UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
  float dynamic_hand_anim_rel_dist_threshold_{0.5f};

  //! @brief Whether to automatically compensate for Glove slippage.
  //!
  //! This subroutine kicks in when the user's fingers are out flat, and incrementally offsets the
  //! avatar's palm until its fingers are also out flat.

  // Whether to automatically compensate for Glove slippage. This subroutine kicks in when the
  // user's fingers are out flat, and incrementally offsets the avatar's palm until its fingers are
  // also out flat.
  UPROPERTY(EditAnywhere)
  bool enable_glove_slip_compensation_;

  //! Parameters that characterize Glove slip compensation.

  // Parameters that characterize Glove slip compensation.
  UPROPERTY(EditAnywhere, meta = (editcondition = "enable_glove_slip_compensation_"))
  FGloveSlipCompensationParameters glove_slip_compensation_parameters_;

  //! @brief Whether to automatically compensate for thimble thickness.
  //!
  //! This subroutine kicks in when the user's fingertips are close to the thumb tip and
  //! linearly reduces the gaps to zero.

  // Whether to automatically compensate for thimble thickness. This subroutine kicks in when the
  // user's fingertips are close to the thumb tip and linearly reduces the gaps to zero.
  UPROPERTY(EditAnywhere)
  bool enable_thimble_compensation_;

  //! Parameters that characterize thimble compensation.

  // Parameters that characterize thimble compensation.
  UPROPERTY(EditAnywhere, meta = (editcondition = "enable_thimble_compensation_"))
  FThimbleCompensationParameters thimble_compensation_parameters_;

  //! Whether to teleport the hand to its tracked location and rotation if it deviates by a
  //! specified distance.

  // Whether to teleport the hand to its tracked location and rotation if it deviates by a
  // specified distance.
  UPROPERTY(EditAnywhere)
  bool enable_corrective_teleportation_;

  //! @brief The max distance the hand can deviate from its tracked location before being
  //! teleported to it.
  //!
  //! We automatically scale this when hand_scale_factor_ is greater than 1. Does not occur if
  //! #enable_corrective_teleportation_ is false.

  // The max distance the hand can deviate from its tracked location before being teleported to it.
  UPROPERTY(EditAnywhere, meta = (editcondition = "enable_corrective_teleportation_",
      ClampMin = "0.0", UIMin = "0.0"))
  float corrective_teleportation_distance_;

  //! The USceneComponent we currently treat as the origin for the motion capture system.

  // The USceneComponent we currently treat as the origin for the motion capture system.
  UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn="true"), Replicated)
  USceneComponent* mocap_origin_;

  //! How quickly simulated poses change.

  // How quickly simulated poses change.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
  float simulated_animation_aggressiveness_1_s;

  //! This inline flag toggles force feedback visualization.

  // This inline flag toggles force feedback visualization.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Visualization")
  bool visualize_force_feedback_;

  //! @brief Visualize force feedback.
  //!
  //! Elements include:
  //! - Black bars: actuation normals
  //! - Gray bars: actuation thresholds
  //! - Blue bars: forces
  //! - Teal: represents actuation

  // Visualize force feedback.
  UPROPERTY(EditAnywhere, meta = (editcondition = "visualize_force_feedback_",
      DisplayName = "Visualize Force Feedback"), Category = "Visualization")
  FForceFeedbackVisualizationParameters force_feedback_visualization_parameters_;

  //! Whether to visualize locations and rotations of mocap tracked segments.

  // Whether to visualize locations and rotations of mocap tracked segments.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_motion_capture_;

  //! Whether to visualize hand animation intermediate steps and data.

  // Whether to visualize hand animation intermediate steps and data.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_hand_animation_;

  //! Whether to visualize the avatar hand animation state.

  // Whether to visualize the avatar hand animation state.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_hand_animation_2_;

  //! Whether to visualize which objects are being damped to better sit in the palm.

  // Whether to visualize which objects are being damped to better sit in the palm.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_contact_damping_{false};

  //! Whether to visualize displacement of the virtual hand from motion capture and hand
  //! animation targets.

  // Whether to visualize displacement of the virtual hand from motion capture and hand animation
  // targets.
  UPROPERTY(EditAnywhere, meta = (editcondition = "visualize_displacement_",
      DisplayName = "Visualize Displacement"), Category = "Visualization")
  FDisplacementVisualizationParameters dis_vis_parameters_;

protected:
  //! @brief The frequency [Hz] at which to transmit physics targets for multiplayer.
  //!
  //! Increasing this value reduces lag, but increases the volume of network traffic.

  // The frequency [Hz] at which to transmit physics targets for multiplayer.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "1.0", UIMin = "1.0"), Replicated)
  float physics_targets_transmission_frequency_hz_;

  //! @brief The target amount of time [s] frames are buffered.
  //!
  //! Increasing this value increases lag, but improves the smoothness and stability of physics
  //! networking.

  // The target amount of time [s] frames are buffered.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
  float physics_targets_buffer_duration_s_;

  //! @brief The frequency [Hz] at which to transmit physics state for multiplayer.
  //!
  //! Increasing this value reduces lag, but increases the volume of network traffic.

  // The frequency [Hz] at which to transmit physics state for multiplayer.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "1.0", UIMin = "1.0"), Replicated)
  float physics_state_transmission_frequency_hz_;

  //! @brief The target amount of time [s] frames are buffered.
  //!
  //! Increasing this value increases lag, but improves the smoothness and stability of physics
  //! networking.

  // The target amount of time [s] frames are buffered.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
  float physics_state_buffer_duration_s_;

  //! @brief As soon as the physics authority zone overlaps another physics authority zone the
  //! radius will increase by a multiplier equal to 1 plus this value. As soon as it is no longer
  //! overlapping any physics authority zones it will go back to its original size.
  //!
  //! This prevents fluttering of state if physics authority zones are oscillating in and out of
  //! each other.

  // As soon as the physics authority zone overlaps another physics authority zone the
  // radius will increase by a multiplier equal to 1 plus this value. As soon as it is no longer
  // overlapping any physics authority zones it will go back to its original size. This prevents
  // fluttering of state if physics authority zones are oscillating in and out of each other.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0", UIMin = "0", ClampMax = "1", UIMax = "1"))
  float physics_authority_zone_radius_hysteresis_;

  //! The mesh to use if this is a female left hand.

  // The mesh to use if this is a female left hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  USkeletalMesh* female_left_hand_skeletal_mesh_;

  //! The static mesh version of the skeletal female left hand.
  //!
  //! See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html

  // The static mesh version of the skeletal female left hand.
  // See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UStaticMesh* female_left_hand_static_mesh_;

  //! The mesh to use if this is a female right hand.

  // The mesh to use if this is a female right hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  USkeletalMesh* female_right_hand_skeletal_mesh_;

  //! The static mesh version of the skeletal female right hand.
  //!
  //! See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html

  // The static mesh version of the skeletal female right hand.
  // See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UStaticMesh* female_right_hand_static_mesh_;

  //! The mesh to use if this is a male left hand.

  // The mesh to use if this is a male left hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  USkeletalMesh* male_left_hand_skeletal_mesh_;

  //! The static mesh version of the skeletal male left hand.
  //!
  //! See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html

  // The static mesh version of the skeletal male left hand.
  // See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UStaticMesh* male_left_hand_static_mesh_;

  //! The mesh to use if this is a male right hand.

  // The mesh to use if this is a male right hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  USkeletalMesh* male_right_hand_skeletal_mesh_;

  //! The static mesh version of the skeletal male right hand.
  //!
  //! See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html

  // The static mesh version of the skeletal male right hand.
  // See https://docs.unrealengine.com/en-US/Engine/Content/Types/SkeletalMeshes/SkeletalMeshConversion/index.html
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UStaticMesh* male_right_hand_static_mesh_;

  //! The material to use on the female hand for light skin.

  // The material to use on the female hand for light skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* light_female_hand_material_;

  //! The material to use on the female hand for medium skin.

  // The material to use on the female hand for medium skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* medium_female_hand_material_;

  //! The material to use on the female hand for dark skin.

  // The material to use on the female hand for dark skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* dark_female_hand_material_;

  //! The material to use on the female hand for neutral skin.

  // The material to use on the female hand for neutral skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* neutral_female_hand_material_;

  //! The material to use on the male hand for light skin.

  // The material to use on the male hand for light skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* light_male_hand_material_;

  //! The material to use on the male hand for medium skin.

  // The material to use on the male hand for medium skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* medium_male_hand_material_;

  //! The material to use on the male hand for dark skin.

  // The material to use on the male hand for dark skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* dark_male_hand_material_;

  //! The material to use on the male hand for neutral skin.

  // The material to use on the male hand for neutral skin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
  UMaterialInterface* neutral_male_hand_material_;

  //! This inline flag toggles displacement visualization.

  // This inline flag toggles displacement visualization.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta=(InlineEditConditionToggle),
      Category = "Visualization")
  bool visualize_displacement_;

  //! The name of the input action that controls toggling the displacement visualizer.

  // The name of the input action that controls toggling the displacement visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization",
      Meta = (DisplayName = "Toggle Displacement Vis Action"))
  FName toggle_dis_vis_action_;

  //! The material to use on the displacement visualizer hand.

  // The material to use on the displacement visualizer hand.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visualization",
      Meta=(DisplayName = "Displacement Visualizer Material"))
  UMaterialInterface* dis_vis_mat_;

  //! The name of the input action that controls toggling the mocap visualizer.

  // The name of the input action that controls toggling the mocap visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_mocap_vis_action_;

  //! The name of the input action that controls toggling the trace visualizer.

  // The name of the input action that controls toggling the trace visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_trace_vis_action_;

  //! The name of the input action that controls toggling the tactor output visualizer.

  // The name of the input action that controls toggling the tactor output visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_tactile_feedback_vis_action_;

  //! The name of the input action that controls toggling the force feedback output
  //! visualizer.

  // The name of the input action that controls toggling the force feedback output visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_force_feedback_vis_action_;

  //! The name of the input action that controls toggling the hand animation visualizer.

  // The name of the input action that controls toggling the hand animation visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_hand_animation_vis_action_;

  //! The name of the input action that controls toggling the second hand animation visualizer.

  // The name of the input action that controls toggling the second hand animation visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_hand_animation_vis_2_action_;

  //! The name of the input action that controls toggling the contact damping visualizer.

  // The name of the input action that controls toggling the contact damping visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_contact_damping_vis_action_{TEXT("HxToggleContactDampingVis")};

private:
  //! Disable the functionality of this class.
  void hardDisable();

  //! Checks to see if there's another AHxHandActor in the level right now with the same #hand_.
  //!
  //! @returns True if there is at least one duplicate.
  bool checkForDuplicateHands() const;

  //! Map hand segments to bone names.

  // Map hand segments to bone names.
  UPROPERTY(EditAnywhere, AdvancedDisplay)
  FHandBoneNames bone_names_;

  //! Whether the AHxHandActor should have any functionality enabled.
  bool is_enabled_;

  //! Whether #bindInput() has executed successfully.
  bool is_input_bound_;

  //! Whether hand physics is authoritative on the client or the server.
  UPROPERTY(ReplicatedUsing = OnRep_is_client_physics_authority_)
  bool is_client_physics_authority_;

  //! Whether local constraints need to be disabled because we no longer have physics authority.
  bool local_constraints_need_disabled_;

  //! The active left hand. Nullptr if not initialized.
  static TWeakObjectPtr<AHxHandActor> left_hand_;

  //! The active right hand. Nullptr if not initialized.
  static TWeakObjectPtr<AHxHandActor> right_hand_;

  //! Adds a new physics targets frame to the buffer.
  //!
  //! @param time_s The world time that the targets were generated.
  //! @param targets New physics targets.
  void pushPhysicsTargets(float time_s, const FHandPhysicsTargets& targets);

  //! Interpolates physics targets received across the network.
  //!
  //! @param delta_time_s The delta time from our physics simulation.
  void interpolatePhysicsTargets(float delta_time_s);

  //! Adds a new physics state frame to the buffer.
  //!
  //! @param time_s The world time that the state was generated.
  //! @param state The hand physics state being sent.
  void pushPhysicsState(float time_s, const FHandPhysicsState& state);

  //! Interpolates physics state received across the network.
  //!
  //! @param delta_time_s The delta time from our physics simulation.
  void interpolatePhysicsState(float delta_time_s);

  //! Updates the physics targets of all constraints driving the hand.
  //!
  //! @param targets New physics targets.
  void updatePhysicsTargets(const FHandPhysicsTargets& targets);

  //! Sends physics targets to the server.
  //!
  //! @param time_s The world time that the targets were generated.
  //! @param targets New physics targets.
  UFUNCTION(Server, Unreliable, WithValidation)
  void serverUpdatePhysicsTargets(float time_s, const FHandPhysicsTargets& targets);

  //! Updates the physics state of the hand.
  //!
  //! @param state The new physics state.
  void updatePhysicsState(const FHandPhysicsState& state);

  //! Sends hand physics state to the server.
  //!
  //! @param time_s The world time that the state was generated.
  //! @param state The hand physics state being sent.
  UFUNCTION(Server, Unreliable, WithValidation)
  void serverUpdatePhysicsState(float time_s, const FHandPhysicsState& state);

  //! Sends hand physics state to the server and all connected clients.
  //!
  //! @param time_s The world time that the state was generated.
  //! @param state The hand physics state being sent.
  UFUNCTION(NetMulticast, Unreliable, WithValidation)
  void multicastUpdatePhysicsState(float time_s, const FHandPhysicsState& state);

  //! Updates replicated constraints to match given constraint states.
  //!
  //! @param states The new constraint states to use.
  void updateReplicatedConstraints(const TArray<FConstraintPhysicsState>& states);

  //! Teleports the hand to a new world position and orientation.
  //!
  //! @param w_position_cm The new world position.
  //! @param w_orient The new world orientation.
  void teleportHand(const FVector& w_position_cm, const FQuat& w_orient);

  //! Teleports the authoritative hand to a new world position and orientation.
  //!
  //! @param w_position_cm The new world position.
  //! @param w_orient The new world orientation.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverTeleportHand(const FVector& w_position_cm, const FQuat& w_orient);

  //! Updates the server with a new value for #hand_scale_factor_.
  //!
  //! @param hand_scale_factor The new value.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverSetHandScaleFactor(float hand_scale_factor);

  //! Resizes the hand based on the current values of #w_uhp_hand_scale_factor_ and
  //! #hand_scale_factor_.
  void updateHandScale();

  //! Configure the constraint driving the hand in the level.
  void initPalmConstraint();

  //! @brief Create a constraint between the given object and the palm that dampens the object's
  //! motion.
  //!
  //! The intent is to make the object easier to hold.
  //!
  //! @param other_comp The component whose motion to damp.
  //! @param other_bone The bone whose motion to damp.
  //!
  //! @returns The new constraint.
  UPhysicsConstraintComponent* createContactDampingConstraint(UPrimitiveComponent* other_comp,
      FName other_bone);

  //! Called when #PhysicsAuthorityZone begins overlapping another physical component.
  //!
  //! @param overlapped_component The component on this actor that overlapped.
  //! @param other_actor The other actor being overlapped.
  //! @param other_comp The component on the other actor being overlapped.
  //! @param other_body_index The index of the other component's FBodyInstance being overlapped.
  //! @param from_sweep Whether this overlap was generated as the result of a sweep.
  //! @param sweep_result The result of the sweep (if relevant).
  UFUNCTION()
  void onPhysicsAuthorityZoneBeginOverlap(UPrimitiveComponent* overlapped_component,
      AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index,
      bool from_sweep, const FHitResult& sweep_result);

  //! Called when #PhysicsAuthorityZone stops overlapping another physical component.
  //!
  //! @param overlapped_component The component on this actor that overlapped.
  //! @param other_actor The other actor being overlapped.
  //! @param other_comp The component on the other actor being overlapped.
  //! @param other_body_index The index of the other component's FBodyInstance being overlapped.
  UFUNCTION()
  void onPhysicsAuthorityZoneEndOverlap(UPrimitiveComponent* overlapped_component,
      AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index);

  //! Evaluates whether the client should have physics authority. Should only be called by the
  //! server.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverUpdatePhysicsAuthority();

  //! Whether this hand currently has physics authority.
  bool isPhysicsAuthority() const;

  //! Gets the physics states of all objects in our physics authority zone.
  //!
  //! @param [out] object_states Populated with object states.
  void getPhysicsStatesOfObjectsInAuthorityZone(TArray<FObjectPhysicsState>& object_states);

  //! Gets the states of all constraints managed by the hand when it has physics authority.
  //!
  //! @param [out] constraint_states Populated with constraint states.
  void getLocalConstraintStates(TArray<FConstraintPhysicsState>& constraint_states);

  //! Returns true if the given constraint should be replicated.
  //!
  //! A constraint should be replicated if one of the components is the hand, and the other is
  //! either null or a replicated object.
  //!
  //! @param constraint The constraint to check.
  //! @returns True if the given constraint should be replicated.
  bool isConstraintValidForReplication(UPhysicsConstraintComponent* constraint) const;

  //! Sets whether constraints managed locally by the hand are physically active.
  //!
  //! @param enabled Whether the constraints should be physically active.
  void setLocalConstraintsPhysicallyEnabled(bool enabled);

  //! Clears any constraints that were created as the result of replication.
  void clearReplicatedConstraints();

  //! @brief The location of middle1 relative to the palm (unrotated, and ignoring scale).
  //!
  //! Apply the palm's world rotation then add its world position to find the world location of
  //! middle1.
  FVector l_middle1_cm_;

  //! The physics constraint responsible for moving the hand around the level.
  UPROPERTY()
  UPhysicsConstraintComponent* palm_constraint_;

  //! A list of constraints indexed by the HaptxApi::Finger and HaptxApi::FingerJoint values that
  //! they correspond to.
  FConstraintInstance* joints_[HaptxApi::F_LAST][HaptxApi::FJ_LAST];

  //! The set of all objects that hit the hand this frame.
  TSet<int64> contacting_object_ids_;

  //! A map of existing contact damping constraints from their associated object id's.

  // A map to existing contact damping constraints from their associated object id's.
  UPROPERTY()
  TMap<int64, UPhysicsConstraintComponent*> damping_constraint_from_object_id_;

  //! The magnitude of the palm's compound bounding box extents.
  float palm_extent_;

  //! Collisions with these actors will be ignored.
  UPROPERTY()
  TSet<AActor*> actors_to_ignore_;

  //! @brief Current physics state. Constructed piece-meal to be communicated over the network at
  //! regular intervals.
  //!
  //! Only hands with physics authority use this data member.
  UPROPERTY()
  FHandPhysicsState physics_state_;

  //! Constraints created due to replicated physics state. Keyed by id from originating
  //! FConstraintPhysicsState.
  UPROPERTY()
  TMap<int32, UPhysicsConstraintComponent*> replicated_constraints_;

  //! A local constraint is one that is key to the physics state of the hand when the local
  //! computer has physics authority.
  UPROPERTY()
  TSet<UPhysicsConstraintComponent*> local_constraints_;

  //! The zone that contains objects which are evaluated for physics authority.

  // The zone that contains objects which are evaluated for physics authority.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,meta = (AllowPrivateAccess = "true"))
	class USphereComponent* PhysicsAuthorityZone;

  //! All the objects currently detected by #PhysicsAuthorityZone mapped to the number of overlap
  //! events from that object.
  UPROPERTY()
  TMap<UPrimitiveComponent*, int> objects_in_physics_authority_zone_;

  //! Components that have just had their movement replication paused and thus need their
  //! replication targets cleared ASAP.
  UPROPERTY()
  TSet<UPrimitiveComponent*> comps_that_need_replication_targets_removed_;

  //! How many other physics authority zones are currently being overlapped.
  int num_physics_authority_zone_overlaps_;

  //! @brief A static map tracking global, inter-hand data about objects that are in at least one
  //! physics authority zone.
  //!
  //! If more than one client would claim authority over an object, the server claims it instead.
  static TMap<UPrimitiveComponent*, FGlobalPhysicsAuthorityObjectData>
      global_physics_authority_data_from_comp_;

  //! Buffer of physics targets frames.
  TCircularBuffer<FHandPhysicsTargetsFrame> physics_targets_buffer_ =
      TCircularBuffer<FHandPhysicsTargetsFrame>(128);

  //! Where sequential data in #physics_targets_buffer_ begins.
  int physics_targets_buffer_tail_i_;

  //! Where sequential data in #physics_targets_buffer_ ends.
  int physics_targets_buffer_head_i_;

  //! Whether data insertion into #physics_targets_buffer_ has started.
  bool physics_targets_buffer_started_;

  //! Buffer of physics state frames.
  TCircularBuffer<FHandPhysicsStateFrame> physics_state_buffer_ =
      TCircularBuffer<FHandPhysicsStateFrame>(128);

  //! Where sequential data in #physics_state_buffer_ begins.
  int physics_state_buffer_tail_i_;

  //! Where sequential data in #physics_state_buffer_ ends.
  int physics_state_buffer_head_i_;

  //! Whether data insertion into #physics_state_buffer_ has started.
  bool physics_state_buffer_started_;

  //! The last time that this hand transmitted a physics update (relative to the beginning of the
  //! game).
  float time_of_last_physics_transmission_s_;

  //! The effective world time from the simulation on the other end of the network that we're using
  //! to interpolate values.
  float follow_time_s_;

  //! Cached to prevent floating point loss.
  float physics_authority_zone_radius_enlarged_cm_;

  //! Cached to prevent floating point loss.
  float physics_authority_zone_radius_nominal_cm_;

  //! Updates hand pose with mocap data from the HaptxApi::HandAnimationInterface.
  //!
  //! @param delta_time The time [s] since the last hand animation update.
  void updateHandAnimation(float delta_time);

  //! Load the correctly-sized hand mesh based on configuration and user profile settings.
  void loadUserProfile();

  //! Updates the server with information derived from a user hand profile.
  //!
  //! @param hand_material New hand material.
  //! @param hand_mesh New hand mesh.
  //! @param w_hand_scale New hand size.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverUserProfileUpdate(UMaterialInterface* hand_material, USkeletalMesh* hand_mesh,
      float w_hand_scale);

  //! Attempt to figure out the hand's location and rotation from SteamVR.
  //!
  //! @param [out] w_mcp3 The world transform of MCP3.
  //! @param [out] w_vive The world transform of the tracked VIVE object.
  //!
  //! @returns Whether the hands location and rotation were successfully populated.
  bool tryGetHandLocationAndRotation(FTransform& w_mcp3, FTransform& w_vive);

  //! Warn the user that the tracking reference is off for this hand as long as we haven't done it
  //! too recently.
  void warnAboutTrackingRefOff();

  //! The hand's default world scale calculated from user hand profile information.
  UPROPERTY(ReplicatedUsing = OnRep_w_uhp_hand_scale_factor_)
  float w_uhp_hand_scale_factor_;

  //! Whether AHxHandActor::updateHandScale() should be called before the next physics tick.
  bool hand_needs_scale_update_;

  //! Whether we've warned about the Vive tracking reference being off recently.
  bool recently_warned_about_tracking_ref_being_off_;

  //! Whether we've already configured the controllers for simulated mocap on the first
  //! TickComponent().
  bool first_tick_has_happened_;

  //! True if the palm has never been teleported.
  bool palm_needs_first_teleport_;

  //! The message key used to log the tracking ref warning message for this hand.
  int32 tracking_ref_off_debug_message_key_ = -1;

  //! Draw force feedback visualization information.
  void visualizeForceFeedbackOutput();

  //! Draw contact damping visualization for one frame.
  void visualizeContactDamping();

  //! Draw mocap data in VR.
  //!
  //! @param mocap_frame The mocap data to draw.
  //! @param w_mcp3 The world transform of MCP3.
  //! @param w_vive The world transform of the tracked VIVE device.
  void visualizeMocapData(HaptxApi::MocapFrame mocap_frame, FTransform w_mcp3, FTransform w_vive);

  //! Draw hand animation state.
  //!
  //! @param anim_frame The anim frame to draw.
  //! @param profile The user of the anim frame to draw.
  //! @param w_mcp3 The world transform of MCP3.
  void visualizeHandAnimation(HaptxApi::AnimFrame anim_frame, HaptxApi::UserProfile profile,
      FTransform w_mcp3);

  //! Draw the second hand animation visualizer, showing the animation state of the avatar hand.
  void visualizeHandAnimation2();

  //! Initializes the displacement visualizer.
  //!
  //! @returns True if the displacement visualizer was successfully initialized.
  bool initializeDisplacementVisualizer();

  //! Updates the displacement visualizer with the most recent bone displacements.
  //!
  //! @param delta_time_s Frame time.
  void updateDisplacementVisualizer(float delta_time_s);

  //! Tears down the displacement visualizer.
  void uninitializeDisplacementVisualizer();

  //! Visualize network state for one frame.
  void visualizeNetworkState();

  //! @brief The skinned mesh rendering the displacement visualizer.
  UPROPERTY()
  UPoseableMeshComponent* dis_vis_pmc_;

  //! @brief This hand's dynamic instance of the displacement visualizer material.
  UPROPERTY()
  UMaterialInstanceDynamic* dis_vis_mat_inst_;

  //! Try to connect to the AHxCoreActor and disable ourselves if we fail.
  //!
  //! @returns Whether the AHxCoreActor is connected.
  bool connectToCore();

  //! Builds #bone_data_from_bone_name_, registering bones with the CI and GD as necessary. Must
  //! be called immediately after physics has initialized.
  void registerBones();

  //! Register retractuators on this hand's peripheral with the HaptxApi::ContactInterpreter.
  void registerRetractuators();

  //! Parameters controlling how fingers are physically modeled in the
  //! HaptxApi::ContactInterpreter.

  // Parameters controlling how fingers are physically modeled in the
  // HaptxApi::ContactInterpreter.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  FBodyParameters finger_body_parameters_;

  //! Parameters controlling how the palm is physically modeled in the
  //! HaptxApi::ContactInterpreter.

  // Parameters controlling how the palm is physically modeled in the
  // HaptxApi::ContactInterpreter.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  FBodyParameters palm_body_parameters_;

  //! Force feedback parameters for each finger.

  // Force feedback parameters for each finger.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  FAllRetractuatorParameters retractuator_parameters_;

  //! The HaptxApi::GraspDetector body ID referring to the hand as a whole.
  int64_t whole_hand_gd_body_id_;

  //! Mapping from bone name to extra information associated with said bone.
  TMap<FName, FHxHandActorBoneData> bone_data_from_bone_name_;

  //! Reference to the AHxCoreActor pseudo-singleton.
  UPROPERTY()
  AHxCoreActor *hx_core_;

  //! Which gesture we're using (if simulating hand animation).
  HaptxApi::Gesture gesture_;

  //! The last anim frame we used (if simulating hand animation).
  HaptxApi::AnimFrame last_simulated_anim_frame_;

  //! The mocap system driving the fingers on this hand.
  std::weak_ptr<HaptxApi::HyleasSystem> mocap_system_;

  //! The Glove slip compensator working on this hand.
  std::unique_ptr<HaptxApi::GloveSlipCompensator> glove_slip_compensator_;

  //! The peripheral driving this hand.
  std::shared_ptr<HaptxApi::Glove> glove_;

  //! The user of this hand.
  HaptxApi::UserProfile user_profile_;

  //! A profile derived from the dimensions of the avatar hand. The overall size of this hand is
  //! based on the real user profile.
  HaptxApi::UserProfile avatar_profile_;

  //! The profile whose dimensions slide between #user_profile_ and #avatar_profile_ based on
  //! avatar animation optimization.
  HaptxApi::UserProfile avatar_anim_optimized_profile_;

  //! Bone names indexed by HaptxApi::Finger and HaptxApi::FingerJoint values.
  FName hand_joint_bone_names_[HaptxApi::F_LAST][HaptxApi::FJ_LAST];

  //! Bone names indexed by HaptxApi::Finger and HaptxApi::FingerBone values.
  FName hand_bone_names_[HaptxApi::F_LAST][HaptxApi::FJ_LAST];

  //! Fingertip names indexed by HaptxApi::Finger.
  FName fingertip_names_[HaptxApi::F_LAST];

  //! Whether the touch pad button on this controller was pressed last frame.
  bool touch_pad_was_pressed_;

  //! Called when #hand_scale_factor_ is replicated.
  UFUNCTION()
  virtual void OnRep_hand_scale_factor_();

  //! Called when #w_uhp_hand_scale_factor_ is replicated.
  UFUNCTION()
  virtual void OnRep_w_uhp_hand_scale_factor_();

  //! Called when #OnRep_is_client_physics_authority_ is replicated.
  UFUNCTION()
  virtual void OnRep_is_client_physics_authority_();

  //! Cached return value of getPawn().
  APawn* pawn_;

  //! The static mesh version of this skeletal mesh.
  UStaticMesh* static_mesh_{nullptr};

  //! The return code of the last call to openvr wrapper tracking functions.
  int last_openvr_wrapper_return_code_;
};
