// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/hx_physical_models.h>
#include "hx_dof_behavior.generated.h"

//! @brief The base class for all custom physical behaviors used in @link UHxConstraintComponent 
//! UHxConstraintComponents @endlink. 
//!
//! DO NOT INSTANTIATE THIS CLASS! Use a child class instead.
//!
//! To implement a custom behavior inherit from this class and override initialize(), and 
//! #getForceTorque(). Also override #tryGetTarget() if your behavior drives toward positions other 
//! than 0.
//!
//! See the @ref section_unreal_hx_dof_behavior "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// The base class for all custom physical behaviors used in UHxConstraintComponents. 
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxDofBehavior : public UObject {
  GENERATED_UCLASS_BODY()

  friend class UHxConstraintComponent;
  friend class UHxDof;

  public:

    //! Get the signed magnitude of the force or torque that is associated with @p position.
    //! 
    //! Defined in anchor2's frame.
    //!
    //! @param position The position of interest.
    //!
    //! @returns The signed magnitude of the force or torque.

    // Get the signed magnitude of the force or torque that is associated with position.
    UFUNCTION(BlueprintCallable)
    virtual float getForceTorque(float position);

    //! Get this behaviors current target location (if it has one).
    //!
    //! @param[out] out_target Populated with the target location (if it exists).
    //!
    //! @returns Whether a target location exists.

    // Get this behaviors current target location (if it has one).
    UFUNCTION(BlueprintCallable)
    virtual bool tryGetTarget(float& out_target);

    //! Whether forces or torques output by this behavior should be considered 
    //! accelerations.

    // Whether forces or torques output by this behavior should be considered accelerations.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    bool acceleration_;

    //! Whether this behavior is allowed to execute.

    // Whether this behavior is allowed to execute.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    bool enabled_;

    //! @brief The underlying model used to drive this behavior. 
    //!
    //! This can be any child class of UHxPhysicalModel.

    // The underlying model used to drive this behavior.
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Parameters")
    UHxPhysicalModel* model_;

    //! Whether to visualize the forces and torques being applied by this behavior.

    // Whether to visualize the forces and torques being applied by this behavior.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    bool visualize_;

    //! @brief The name of this behavior.
    //!
    //! Unique for a given UHxDof. See UHxDof.registerPhysicalBehavior().

    // The name of this behavior.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parameters")
    FName name_;

  private:

    //! Initializes the behavior for use. Gets called automatically by UHxDof.

    // Initializes the behavior for use. Gets called automatically by UHxDof.
    virtual void initialize();
};

//! @brief The default physical behavior.
//!
//! See the @ref section_unreal_hx_dof_behavior "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// The default physical behavior.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxDofDefaultBehavior : public UHxDofBehavior {
  GENERATED_UCLASS_BODY()

public:

  //! @brief Get the signed magnitude of the force or torque that is associated with @p position
  //! per the current #model_.
  //! 
  //! Defined in anchor2's frame.
  //!
  //! @param position The position of interest.
  //!
  //! @returns The signed magnitude of the force or torque.
  virtual float getForceTorque(float position) override;

  //! Gets the target position of 0 (if it exists).
  //!
  //! @param[out] out_target Populated with 0.
  //!
  //! @returns False if #model_ is of type UHxCurveModel, and true otherwise.
  virtual bool tryGetTarget(float& out_target) override;
};

//! @brief A physical behavior that always drives toward a target position.
//!
//! See the @ref section_unreal_hx_dof_behavior "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A physical behavior that always drives toward a target position.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxDofTargetPositionBehavior : public UHxDofBehavior {
  GENERATED_UCLASS_BODY()

public:

  //! @brief Get the signed magnitude of the force or torque that is associated with @p position
  //! relative to #target_position_.
  //! 
  //! Defined in anchor2's frame.
  //!
  //! @param position The position of interest.
  //!
  //! @returns The signed magnitude of the force or torque.
  virtual float getForceTorque(float position) override;

  //! Gets the current value of #target_position_.
  //!
  //! @param[out] out_target Populated with the current value of #target_position_.
  //!
  //! @returns True.
  virtual bool tryGetTarget(float& out_target) override;

protected:
  //! The position along the degree of freedom where the behavior is driving to.

  // The position along the degree of freedom where the behavior is driving to.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float target_position_;
};
