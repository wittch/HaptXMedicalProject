// Copyright (C) 2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h>
#include <HaptxApi/retractuator.h>
#include <Haptx/Public/constraint_instance_net_quanitze.h>
#include <Haptx/Public/contact_interpreter_parameters.h>
#include "hx_hand_actor_structs.generated.h"

//! @brief All of the segments expected in a complete hand.
//!
//! Each value represents the bone name that corresponds to that segment.

// All of the segments expected in a complete hand.
USTRUCT()
struct FHandBoneNames {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere)
  FName thumb1 = FName(TEXT("thumb1"));
  UPROPERTY(EditAnywhere)
  FName thumb2 = FName(TEXT("thumb2"));
  UPROPERTY(EditAnywhere)
  FName thumb3 = FName(TEXT("thumb3"));
  UPROPERTY(EditAnywhere)
  FName thumb4 = FName(TEXT("thumb4"));
  UPROPERTY(EditAnywhere)
  FName index1 = FName(TEXT("index1"));
  UPROPERTY(EditAnywhere)
  FName index2 = FName(TEXT("index2"));
  UPROPERTY(EditAnywhere)
  FName index3 = FName(TEXT("index3"));
  UPROPERTY(EditAnywhere)
  FName index4 = FName(TEXT("index4"));
  UPROPERTY(EditAnywhere)
  FName middle1 = FName(TEXT("middle1"));
  UPROPERTY(EditAnywhere)
  FName middle2 = FName(TEXT("middle2"));
  UPROPERTY(EditAnywhere)
  FName middle3 = FName(TEXT("middle3"));
  UPROPERTY(EditAnywhere)
  FName middle4 = FName(TEXT("middle4"));
  UPROPERTY(EditAnywhere)
  FName ring1 = FName(TEXT("ring1"));
  UPROPERTY(EditAnywhere)
  FName ring2 = FName(TEXT("ring2"));
  UPROPERTY(EditAnywhere)
  FName ring3 = FName(TEXT("ring3"));
  UPROPERTY(EditAnywhere)
  FName ring4 = FName(TEXT("ring4"));
  UPROPERTY(EditAnywhere)
  FName pinky1 = FName(TEXT("pinky1"));
  UPROPERTY(EditAnywhere)
  FName pinky2 = FName(TEXT("pinky2"));
  UPROPERTY(EditAnywhere)
  FName pinky3 = FName(TEXT("pinky3"));
  UPROPERTY(EditAnywhere)
  FName pinky4 = FName(TEXT("pinky4"));
  UPROPERTY(EditAnywhere)
  FName palm = FName(TEXT("root"));
};

//! Holds the retractuator parameters that get sent to the HaptxApi::ContactInterpreter for
//! each finger.

// Holds the retractuator parameters that get sent to the ContactInterpreter for each finger.
USTRUCT()
struct FAllRetractuatorParameters {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere)
  FRetractuatorParameters thumb;
  UPROPERTY(EditAnywhere)
  FRetractuatorParameters index;
  UPROPERTY(EditAnywhere)
  FRetractuatorParameters middle;
  UPROPERTY(EditAnywhere)
  FRetractuatorParameters ring;
  UPROPERTY(EditAnywhere)
  FRetractuatorParameters pinky;

  //! Get the retractuator parameters corresponding to a given finger.
  //!
  //! @param finger Which finger.
  //! @returns The corresponding parameters.
  FRetractuatorParameters getParametersForFinger(HaptxApi::Finger finger) {
    switch (finger) {
    case HaptxApi::Finger::F_THUMB:
      return thumb;
    case HaptxApi::Finger::F_INDEX:
      return index;
    case HaptxApi::Finger::F_MIDDLE:
      return middle;
    case HaptxApi::Finger::F_RING:
      return ring;
    case HaptxApi::Finger::F_PINKY:
      return pinky;
    default:
      return FRetractuatorParameters();
    }
  }
};

//! Holds the parameters that characterize thimble compensation.

// Holds the parameters that characterize thimble compensation.
USTRUCT(BlueprintType)
struct FThimbleCompensationParameters {
  GENERATED_BODY()

  //! The distance to the thumb tip within which we correct the position of other fingertips [cm].
  //! Should always be >= max_correction_dist_cm.

  // The distance to the thumb tip within which we correct the position of other fingertips [cm].
  // Should always be >= max_correction_dist_cm.
  UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "10.0"))
  float correction_dist_threshold_cm = 5.0f;

  //! The distance to the thumb tip at which we apply max_correction_amount_cm [cm]. Should always
  //! be >= max_correction_amount_cm and <= correction_dist_threshold_cm.

  // The distance to the thumb tip at which we apply max_correction_amount_cm [cm]. Should always
  // be >= max_correction_amount_cm and <= correction_dist_threshold_cm.
  UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "5.0"))
  float max_correction_dist_cm = 1.0f;

  //! The maximum correction we can apply to fingertip positions [cm]. Should always be <=
  //! max_correction_dist_cm and >= 0.

  // The maximum correction we can apply to fingertip positions [cm]. Should always be <=
  // max_correction_dist_cm and >= 0.
  UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "5.0"))
  float max_correction_amount_cm = 1.0f;
};

//! @brief Represents parameters used in the force feedback visualizer.
//!
//! This struct only exists for organizational purposes in the details panel.

// Represents parameters used in the force feedback visualizer.
USTRUCT(BlueprintType)
struct FForceFeedbackVisualizationParameters {
  GENERATED_BODY()

    //! How much forces get scaled in the force feedback visualizer. Increase to make force
    //! feedback visualizations taller.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Force Scale cm/cN", UIMin="0.0",
      ClampMin="0.0"))
  float force_scale_cm_cn = 6e-5f;
};

//! Holds the parameters that characterize Glove slip compensation.

// Holds the parameters that characterize Glove slip compensation.
USTRUCT(BlueprintType)
struct FGloveSlipCompensationParameters {
  GENERATED_BODY()

  //! Multiplied by delta time [s] to get LERP alpha. Increase to make slip compensation happen
  //! faster.

  // Multiplied by delta time [s] to get LERP alpha. Increase to make slip compensation happen
  // faster.
  UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "10.0"))
  float aggressiveness_1_s = 1.0f;

  //! How straight the user's fingers need to be for Glove slip compensation to
  //! engage. Values should range between [0, 1]. A value of 1 indicates perfectly straight
  //! (such that Glove slip compensation will likely never engage), and a value of 0 indicates that
  //! Glove slip compensation is always engaged.

  // How straight the user's fingers need to be for Glove slip compensation to
  // engage. Values should range between [0, 1]. A value of 1 indicates perfectly straight
  // (such that Glove slip compensation will likely never engage), and a value of 0 indicates that
  // Glove slip compensation is always engaged.
  UPROPERTY(EditAnywhere,
      meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
  float on_threshold = 0.95f;
};

//! @brief Represents parameters used in the displacement visualizer.
//!
//! This struct only exists for organizational purposes in the details panel.

// Represents parameters used in the displacement visualizer.
USTRUCT(BlueprintType)
struct FDisplacementVisualizationParameters {
  GENERATED_BODY()

  //! The color of the visualizer hand.

  // The color of the visualizer hand.
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FColor color = FColor::White;

  //! The displacement at which the hand starts to fade in.

  // The displacement at which the hand starts to fade in.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Min Displacement [cm]",
      UIMin = "0.0", ClampMin = "0.0"))
  float min_displacement_cm = 5.0f;

  //! The displacement at which the hand is fully faded in.

  // The displacement at which the hand is fully faded in.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Displacement [cm]",
      UIMin = "0.0", ClampMin = "0.0"))
  float max_displacement_cm = 10.0f;

  //! The opacity of the hand at max displacement.

  // The opacity of the hand at max displacement.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0.0", ClampMin = "0.0",
      UIMax = "1.0", ClampMax = "1.0"))
  float max_opacity = 0.05f;
};

//! The physics information about a constraint that AHxHandActor needs to synchronize interactions
//! over a network.
USTRUCT()
struct FConstraintPhysicsState {
  GENERATED_BODY()

  //! The unique ID of this constraint.
  UPROPERTY()
  int32 id;

  //! The value for UPhysicsConstraintComponent::OverrideComponent1.
  UPROPERTY()
  UPrimitiveComponent* component1;

  //! The value for UPhysicsConstraintComponent::OverrideComponent2.
  UPROPERTY()
  UPrimitiveComponent* component2;

  //! The world scale of the UPhysicsConstraintComponent.
  UPROPERTY()
  FVector w_scale;

  //! The value for UPhysicsConstraintComponent::ConstraintInstance.
  UPROPERTY()
  FConstraintInstance_NetQuantize100 constraint_instance;
};

//! The physics information about an FBodyInstance that AHxHandActor needs to synchronize
//! interactions over a network.
USTRUCT()
struct FObjectPhysicsState {
  GENERATED_BODY()

  //! The component that owns the FBodyInstance.
  UPROPERTY()
  UPrimitiveComponent* component;

  //! The index of the FBodyInstance in the component.
  UPROPERTY()
  int32 body_index;

  //! The FBodyInstance's state.
  UPROPERTY()
  FRigidBodyState state;
};

//! The targets of all constraints driving the hand.
USTRUCT()
struct FHandPhysicsTargets {
  GENERATED_BODY()

  //! The position target of the constraint driving the palm.
  UPROPERTY()
  FVector_NetQuantize100 w_middle1_pos_cm;

  //! The orientation target of the constraint driving the palm.
  UPROPERTY()
  FQuat w_middle1_orient;

  //! @brief The orientation targets of the constraints driving the finger segments.
  //!
  //! Indexed by HaptxApi::HandJoint.
  UPROPERTY()
  TArray<FQuat> l_joint_orients;

  //! @brief Interpolate between two physics targets.
  //!
  //! FVectors are linearly interpolated and FQuats are spherically interpolated.
  //!
  //! @param a Physics targets for @p alpha = 0.
  //! @param b Physics targets for @p alpha = 1.
  //! @param alpha Interpolation alpha.
  static inline FHandPhysicsTargets interpolate(const FHandPhysicsTargets& a,
      const FHandPhysicsTargets& b, float alpha) {
    FHandPhysicsTargets c;
    c.w_middle1_pos_cm = UKismetMathLibrary::VLerp(a.w_middle1_pos_cm, b.w_middle1_pos_cm, alpha);
    c.w_middle1_orient = FQuat::Slerp(a.w_middle1_orient, b.w_middle1_orient, alpha);
    int min_num = FMath::Min(a.l_joint_orients.Num(), b.l_joint_orients.Num());
    c.l_joint_orients.SetNum(min_num);
    for (int i = 0; i < min_num; i++) {
      c.l_joint_orients[i] = FQuat::Slerp(a.l_joint_orients[i], b.l_joint_orients[i], alpha);
    }
    return c;
  }
};

//! A timestamped FHandPhysicsTargets.
USTRUCT()
struct FHandPhysicsTargetsFrame {
  GENERATED_BODY()

  //! The time stamp of the frame.
  UPROPERTY()
  float time_s;

  //! The frame itself.
  UPROPERTY()
  FHandPhysicsTargets targets;
};

//! The physics targets for each constraint driving the hand, the physics state of each
//! FBodyInstance in the hand, and the physics state of each object near the hand.
USTRUCT()
struct FHandPhysicsState {
  GENERATED_BODY()

  //! The physics state of each FBodyInstance in the hand.
  UPROPERTY()
  TArray<FRigidBodyState> w_body_states;

  //! The physics targets for each constraint driving the hand.
  UPROPERTY()
  FHandPhysicsTargets targets;

  //! The physics state of each object near the hand.
  UPROPERTY()
  TArray<FObjectPhysicsState> w_object_states;

  //! The physics state of any constraints involving the hand.
  UPROPERTY()
  TArray<FConstraintPhysicsState> constraint_states;

  //! @brief Interpolate between two hand physics states.
  //!
  //! FVectors are linearly interpolated and FQuats are spherically interpolated.
  //!
  //! @param a Physics state for @p alpha = 0.
  //! @param b Physics state for @p alpha = 1.
  //! @param alpha Interpolation alpha.
  static inline FHandPhysicsState interpolate(const FHandPhysicsState& a,
      const FHandPhysicsState& b, float alpha) {
    FHandPhysicsState c;
    int min_num = FMath::Min(a.w_body_states.Num(), b.w_body_states.Num());
    c.w_body_states.SetNum(min_num);
    for (int i = 0; i < min_num; i++) {
      c.w_body_states[i] =
          interpolateRigidBodyState(a.w_body_states[i], b.w_body_states[i], alpha);
    }

    c.targets = FHandPhysicsTargets::interpolate(a.targets, b.targets, alpha);

    c.w_object_states.SetNum(a.w_object_states.Num());
    int c_i = 0;
    for (int a_i = 0; a_i < a.w_object_states.Num(); a_i++) {
      for (int b_i = 0; b_i < b.w_object_states.Num(); b_i++) {
        if (a.w_object_states[a_i].component == b.w_object_states[b_i].component &&
            a.w_object_states[a_i].body_index == b.w_object_states[b_i].body_index) {
          c.w_object_states[c_i].component = a.w_object_states[a_i].component;
          c.w_object_states[c_i].body_index = a.w_object_states[a_i].body_index;
          c.w_object_states[c_i].state = interpolateRigidBodyState(a.w_object_states[a_i].state,
              b.w_object_states[b_i].state, alpha);
          c_i++;
          break;
        }
      }
    }
    c.w_object_states.SetNum(c_i, true);

    // TODO: Figure out how to interpolate constraint states.
    c.constraint_states = a.constraint_states;
    return c;
  }
};

//! A timestamped FHandPhysicsState.
USTRUCT()
struct FHandPhysicsStateFrame {
  GENERATED_BODY()

  //! The time stamp of the frame.
  UPROPERTY()
  float time_s;

  //! The frame itself.
  UPROPERTY()
  FHandPhysicsState state;
};

//! Information about an object inside at least one hand's physics authority zone.
USTRUCT()
struct FGlobalPhysicsAuthorityObjectData {
  GENERATED_BODY()

  //! Whether this object was replicating movement when it was first encountered.
  UPROPERTY()
  bool was_replicating_movement;

  //! A map from each pawn interacting with the object to how many physics authority zones from
  //! that pawn the object is currently inside.
  UPROPERTY()
  TMap<const APawn*, int> physics_authority_zone_count_from_pawn;
};

//! A custom FTickFunction so @link AHxHandActor UHxHandComponents @endlink can tick before and
//! after physics.

// A custom FTickFunction so UHxHandComponents can tick before and after physics.
USTRUCT()
struct FHxHandSecondaryTickFunction : public FTickFunction {
  GENERATED_BODY()

  //! The AHxHandActor that is ticking.
  class AHxHandActor* Target;

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

  //! Abstract function to describe this tick. Used to print messages about illegal cycles in the
  //! dependency graph.
  HAPTX_API virtual FString DiagnosticMessage() override;
};
template<>
struct TStructOpsTypeTraits<FHxHandSecondaryTickFunction> : public TStructOpsTypeTraitsBase2<FHxHandSecondaryTickFunction> {
  enum {
    WithCopy = false
  };
};

//! Information that AHxHandActor stores on a per-bone basis.
USTRUCT()
struct FHxHandActorBoneData {
  GENERATED_BODY()

  //! Whether #ci_body_id has been defined for this bone.
  bool has_ci_body_id = false;

  //! The ID of this bone to the contact interpreter interface.
  int64_t ci_body_id = 0u;

  //! The rigid body part associated with this bone.
  HaptxApi::RigidBodyPart rigid_body_part = HaptxApi::RigidBodyPart::LAST;

  //! Whether #gd_body_id has been defined for this bone.
  bool has_gd_body_id = false;

  //! The ID of this bone to the grasp detector interface.
  int64_t gd_body_id = 0u;

  //! Whether this bone can engage contact damping.
  bool can_engage_contact_damping = false;
};

//! Different ways hand animation can be optimized.

// Different ways hand animation can be optimized.
UENUM(BlueprintType)
enum class EHandAnimationOptimizationMode : uint8 {
  //! Optimizes for fingertip position when a finger is near the thumb, and joint angles otherwise.
  DYNAMIC               UMETA(DisplayName = "Dynamic"),
  //! Always optimizes for joint angles. Users may find that the avatar fingers don't touch when
  //! their real fingers do.
  JOINT_ANGLES          UMETA(DisplayName = "Joint Angles"),
  //! Always optimizes for fingertip positions. Users may find their knuckles scrunched when they
  //! should be straight.
  FINGERTIP_POSITIONS   UMETA(DisplayName = "Fingertip Positions")
};
