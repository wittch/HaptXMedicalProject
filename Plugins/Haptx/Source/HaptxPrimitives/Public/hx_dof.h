// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/haptx_primitives_shared.h>
#include <HaptxPrimitives/Public/hx_dof_behavior.h>
#include <HaptxPrimitives/Public/hx_state_functions.h>
#include "hx_dof.generated.h"

#define DEG_PER_REV 360

//! @brief A struct that mirrors FTransform, except about a single cardinal direction.
//!
//! Rotation will be populated with the FTransform's rotation about the chosen direction, scale 
//! will be populated with the FTransform's scale in the chosen direction, and translation will be
//! populated with the FTransform's translation in the chosen direction.

// A struct that mirrors FTransform, except about a single cardinal direction.
USTRUCT(BlueprintType)
struct FTransform1D {
  GENERATED_BODY()

  //! The translation [cm] of a transform along a given EDofAxis.
  //!
  //! @param local_transform The transform of interest.
  //! @param axis The axis of interest.
  //!
  //! @returns The translation [cm] of a transform along a given DofAxis.
  static float GetTranslation(const FTransform& local_transform, EDofAxis axis);

  //! The rotation [deg] of a transform about a given EDofAxis.
  //!
  //! @param local_transform The transform of interest.
  //! @param axis The axis of interest.
  //!
  //! @returns The rotation [deg] of a transform about a given EDofAxis.
  static float GetRotation(const FTransform& local_transform, EDofAxis axis);

  //! The scale of a transform in a given EDofAxis.
  //!
  //! @param local_transform The transform of interest.
  //! @param axis The axis of interest.
  //!
  //! @returns The scale of a transform in a given EDofAxis.
  static float GetScale(const FTransform& local_transform, EDofAxis axis);
};

//! A rotation in degrees around a single axis relative to an original rotation on 
//! [-inf, inf].
class TotalRotation {

public:

  //! Default constructor.
  TotalRotation() : revolution_(0), partial_angle_(0.f) {}

  //! Constructs a total rotation from a given revolution and partial_rot.
  //!
  //! @param revolution Which revolution this total rotation is on.
  //! @param partial_rot Progress through the current rotation.
  TotalRotation(int revolution, float partial_rot) : revolution_(revolution),
    partial_angle_(partial_rot) {}

  //! Constructs a TotalRotation object from a given value [deg].
  //!
  //! @param total_rot The given total rotation [deg].
  explicit TotalRotation(float total_rot) {
    const float offset = total_rot < 0 ? -DEG_PER_REV / 2 : DEG_PER_REV / 2;
    revolution_ = (int)truncf((total_rot + offset) / DEG_PER_REV);
    partial_angle_ = total_rot - revolution_ * DEG_PER_REV;
  }

  //! Get the value of the total rotation on [-inf, inf].
  //!
  //! @returns The value of the total rotation on [-inf, inf].
  float getTotalRotation() const {
    return partial_angle_ + revolution_ * DEG_PER_REV;
  }

  //! Get a friendly string.
  //!
  //! @returns A friendly string.
  FString ToString() const {
    return FString::Printf(TEXT("Total rotation = %f, revolution = %d, partial angle = %f"),
      getTotalRotation(), revolution_, partial_angle_);
  }

  //! @brief Which revolution the total rotation is on.
  //!
  //! The 0th revolution is resting rotation +/- 180 degrees, the 1st revolution is
  //! CW 180 -> CW 540, the -1st rotation is CCW 180 -> CCW 540.
  int revolution_;

  //! @brief The partial rotation expressed as an angle in degrees on [-180,180].
  //!
  //! This + (revolution_ * 360) is the total rotation.
  float partial_angle_;
};

//! @brief A single degree of freedom that a UHxConstraintComponent can operate upon.
//!
//! See the @ref section_unreal_hx_dof "Unreal Haptic Primitive Guide" for a high level overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A single degree of freedom that a UHxConstraintComponent can operate upon.
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxDof : public UObject {
  GENERATED_BODY()

  friend class UHxConstraintComponent;
  friend class UHx1DConstraintComponent;
  friend class UHx1DTranslatorComponent;
  friend class UHx1DRotatorComponent;

public:

  //! Default constructor.
  //!
  //! @param object_initializer For registration with Unreal.
  UHxDof(const FObjectInitializer& object_initializer);

  //! Initializes this UHxDof for use. Called automatically by UHxConstraintComponent.
  void initialize();

  //! @brief Update this UHxDof with a new position.
  //!
  //! Generally this should only get called by UHxConstraintComponent or one of its child classes.
  //!
  //! @param position The new position.
  //! @param teleport If true the new position gets cleanly set and bypasses any logic.
  virtual void update(float position, bool teleport = false);

  //! Register a new, unique physical behavior.
  //!
  //! @param behavior The new behavior. Needs to have a name not already present in 
  //! #physical_behaviors_.

  // Register a new, unique physical behavior.
  UFUNCTION(BlueprintCallable)
  void registerPhysicalBehavior(UHxDofBehavior* behavior);

  //! Unregister a physical behavior.
  //!
  //! @param behavior The behavior to unregister.

  // Unregister a physical behavior.
  UFUNCTION(BlueprintCallable)
  void unregisterPhysicalBehavior(UHxDofBehavior* behavior);

  //! Register a new, unique state function.
  //!
  //! @param function The new state function. Needs to have a name not already present in 
  //! #state_functions_.

  // Register a new, unique state function.
  UFUNCTION(BlueprintCallable)
  void registerStateFunction(UHxStateFunction* function);

  //! Unregister a state function.
  //!
  //! @param function The state function to unregister.

  // Unregister a state function.
  UFUNCTION(BlueprintCallable)
  void unregisterStateFunction(UHxStateFunction* function);

  //! Find a registered UHxStateFunction by name.
  //!
  //! @param name The name of the state function to look for.
  //!
  //! @returns The state function, or null if not found.

  // Find a registered UHxStateFunction by name.
  UFUNCTION(BlueprintCallable)
  UHxStateFunction* findStateFunctionByName(FName name);

  //! Find a registered UHxDofBehavior by name.
  //!
  //! @param name The name of the behavior to look for.
  //!
  //! @returns The behavior, or null if not found.

  // Find a registered UHxDofBehavior by name.
  UFUNCTION(BlueprintCallable)
  UHxDofBehavior* findPhysicalBehaviorByName(FName name);

  //! @brief Get the current position or rotation around this degree of freedom.
  //!
  //! Rotations are returned in degrees. For rotations the absolute value might be > 180 if 
  //! UHxAngularDof::track_multiple_revolutions_ is true. For rotations, we measure this using a 
  //! vector projection onto the associated plane. This does not use any Euler angles. This means 
  //! this function may not do what you're expecting it to do if the constrained object can rotate
  //! around more than one axis.
  //!
  //! @returns The current position or rotation around this degree of freedom.

  // Get the current position or rotation around this degree of freedom.
  UFUNCTION(BlueprintCallable, BlueprintPure)
  float getCurrentPosition() const;

  //! Whether this UHxDof should update.
  //!
  //! @returns True if either #state_functions_ or #physical_behaviors_ is not empty or if 
  //! #force_update_ is true.

  // Whether this UHxDof should update.
  UFUNCTION(BlueprintCallable,  BlueprintPure)
  bool shouldUpdate() const;

protected:
  //! The position on this UHxDof that we were at on the last Tick() ([deg] for 
  //! UHxAngularDof, [cm] for UHxLinearDof).
  float current_position_;

  //! Update position even with no state functions or behaviors.

  // Update position even with no state functions or behaviors.
  UPROPERTY(BlueprintReadWrite, EditAnywhere)
  bool force_update_;

  //! The list of physical behaviors on this UHxDof.

  // The list of physical behaviors on this UHxDof.
  UPROPERTY(BlueprintReadOnly)
  TArray<UHxDofBehavior*> physical_behaviors_;

  //! The list of state functions on this UHxDof.

  // The list of state functions on this UHxDof.
  UPROPERTY(BlueprintReadOnly)
  TArray<UHxStateFunction*> state_functions_;

};

//! @brief A single linear degree of freedom that a UHxConstraintComponent can operate upon.
//!
//! See the @ref section_unreal_hx_dof "Unreal Haptic Primitive Guide" for a high level overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A single linear degree of freedom that a UHxConstraintComponent can operate upon.
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxLinearDof : public UHxDof {
  GENERATED_BODY()

  friend class UHxConstraintComponent;
  friend class UHx1DConstraintComponent;
  friend class UHx1DTranslatorComponent;
  friend class UHx1DRotatorComponent;
};

//! @brief A single angular degree of freedom that a UHxConstraintComponent can operate upon.
//!
//! See the @ref section_unreal_hx_dof "Unreal Haptic Primitive Guide" for a high level overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A single angular degree of freedom that a UHxConstraintComponent can operate upon.
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxAngularDof : public UHxDof {
  GENERATED_BODY()

  friend class UHxConstraintComponent;
  friend class UHx1DConstraintComponent;
  friend class UHx1DTranslatorComponent;
  friend class UHx1DRotatorComponent;

public:

  UHxAngularDof(const FObjectInitializer& object_initializer);

  void update(float position, bool teleport = false) override;

protected:

  //! @brief Track the rotation as more than just an angle between -180 and 180 degrees.
  //! 
  //! This changes how attached behaviors and state machines work.

  // Track the rotation as more than just an angle between -180 and 180 degrees.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool track_multiple_revolutions_;
};
