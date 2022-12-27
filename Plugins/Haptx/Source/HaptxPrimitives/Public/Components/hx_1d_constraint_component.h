// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/Components/hx_constraint_component.h>
#include <HaptxPrimitives/Public/hx_dof_detent_behavior.h>
#include <HaptxPrimitives/Public/hx_state_functions.h>
#include "hx_1d_constraint_component.generated.h"

//! @brief Uses features that can be added to @link UHxConstraintComponent UHxConstraintComponents 
//! @endlink that only need to operate in one degree of freedom.
//!
//! DO NOT INSTANTIATE THIS CLASS! Use UHx1DRotatorComponent or UHx1DTranslatorComponent instead.
//!
//! See the @ref section_unreal_hx_1d_components "Unreal Haptic Primitive Guide" for a high level 
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// Uses features that can be added to UHxConstraintComponents that only need to operate in one 
// degree of freedom.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew, HideCategories=(Tick, Tags,
    Collision, Rendering, HxConstraint))
class HAPTXPRIMITIVES_API UHx1DConstraintComponent : public UHxConstraintComponent {
  GENERATED_BODY()

public:
  //! Default constructor.
  //!
  //! @param object_initializer For registration with Unreal.
  UHx1DConstraintComponent(const FObjectInitializer& object_initializer);

  //! Called when the game starts.
  void BeginPlay();

  //! Called when the game starts, but before #BeginPlay().
  virtual void InitializeComponent() override;

  //! Get the EDegreeOfFreedom that this component is operating with.
  //!
  //! @returns The degree of freedom that this component is operating with.

  // Get the EDegreeOfFreedom that this component is operating with.
  UFUNCTION(BlueprintCallable, BlueprintPure)
  EDegreeOfFreedom getOperatingDegreeOfFreedom() const;

  //! Get the UHxDof that this component is operating with.
  //!
  //! @returns The UHxDof that this component is operating with.

  // Get the UHxDof that this component is operating with.
  UFUNCTION(BlueprintCallable)
  UHxDof* getOperatingDof() const;

  //! @brief Set the lower and upper limits of this constraint.
  //!
  //! If you expect this to take immediate effect, make sure #limit_motion_ is true.
  //!
  //! @param lower_limit The new lower limit.
  //! @param upper_limit The new upper limit.

  // Set the lower and upper limits of this constraint.
  UFUNCTION(BlueprintCallable)
  void setLimits(float lower_limit, float upper_limit);

  //! Set the damping of this constraint.
  //!
  //! @param damping The new damping.

  // Set the damping of this constraint.
  UFUNCTION(BlueprintCallable)
      void setDamping(float damping);

  //! Teleport anchor1 to the new position along the operating UHxDof.
  //!
  //! @param new_position The new position along the operating UHxDof.

  // Teleport anchor1 to the new position along the operating UHxDof.
  UFUNCTION(BlueprintCallable)
  void teleportAnchor1AlongOperatingDof(float new_position);

  //! Apply force at anchor along operating UHxDof.
  //!
  //! @param force The force value to apply.
  //! @param anchor Which anchor to apply @p force to.
  //! @param accel_change Whether to apply @p force as an acceleration or a force.
  //! @param visualize Whether to visualize @p force. Lasts for one frame.

  // Apply force at anchor along operating UHxDof.
  UFUNCTION(BlueprintCallable)
  void addForceAtAnchorAlongOperatingDof(float force, EAnchor anchor, bool accel_change = false, 
      bool visualize = false);

  //! Apply torque at anchor along operating UHxDof.
  //!
  //! @param torque The torque value to apply.
  //! @param anchor Which anchor to apply @p torque to.
  //! @param accel_change Whether to apply @p torque as an angular acceleration or a torque.
  //! @param visualize Whether to visualize @p torque. Lasts for one frame.

  // Apply torque at anchor along operating UHxDof.
  UFUNCTION(BlueprintCallable)
  void addTorqueAtAnchorAlongOperatingDof(float torque, EAnchor anchor, 
      bool accel_change = false, bool visualize = false);

  //! Apply impulse at anchor along operating UHxDof.
  //!
  //! @param impulse The impulse value to apply.
  //! @param anchor Which anchor to apply @p impulse to.
  //! @param vel_change Whether to apply @p impulse as a velocity change or a impulse.
  //! @param visualize Whether to visualize @p impulse. Lasts for one frame.

  // Apply impulse at anchor along operating UHxDof.
  UFUNCTION(BlueprintCallable)
  void addImpulseAtAnchorAlongOperatingDof(float impulse, EAnchor anchor, 
      bool vel_change = false, bool visualize = false);

  //! Apply angular impulse at anchor along operating UHxDof.
  //!
  //! @param angular_impulse The angular impulse value to apply.
  //! @param anchor Which anchor to apply @p angular_impulse to.
  //! @param vel_change Whether to apply @p angular_impulse as an angular velocity change or an 
  //! angular impulse.
  //! @param visualize Whether to visualize @p angular_impulse. Lasts for one frame.

  // Apply angular impulse at anchor along operating UHxDof.
  UFUNCTION(BlueprintCallable)
  void addAngularImpulseAtAnchorAlongOperatingDof(float angular_impulse, EAnchor anchor,
      bool vel_change = false, bool visualize = false);

protected:

  virtual void configureConstraint(FConstraintInstance& in_constraint_instance) override;

  //! Which domain this constraint operates in.
  EDofDomain operating_domain_;

  //! Which axis the constraint operates along.

  // Which axis the constraint operates along.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  EDofAxis operating_axis_;

  //! @brief The initial position to teleport the constrained object to after the constraint has 
  //! formed.
  //!
  //! This happens only once in #BeginPlay().

  // The initial position to teleport the constrained object to after the constraint has formed.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  float initial_position_;

  //! @brief Whether to put limits on this constraint's motion.
  //!
  //! See #lower_limit_ and #upper_limit_.

  // Whether to put limits on this constraint's motion.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool limit_motion_;

  //! The lower bound on this constraint's position limits.

  // The lower bound on this constraint's position limits.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "limit_motion_"))
  float lower_limit_;

  //! The upper bound on this constraint's position limits.

  // The upper bound on this constraint's position limits.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "limit_motion_"))
  float upper_limit_;

  //! @brief Whether to scale linear limits.
  //!
  //! This will also scale the linear limits offset field of UHx1DRotatorComponents.
  //!
  //! This gets passed directly to the relevant UPhysicsConstraintComponent property.

  // Whether to scale linear limits.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool scale_linear_limits_;

  //! Whether to lock the other domain (linear/angular).

  // Whether to lock the other domain (linear/angular).
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool lock_other_domain_;

  //! @brief How much the constraint's motion gets damped.
  //!
  //! This gets passed directly to the relevant UPhysicsConstraintComponent property.

  // How much the constraint's motion gets damped.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
  float damping_;

  //! The list of physical behaviors affecting the primitive on this UHxDof.

  // The list of physical behaviors affecting the primitive on this UHxDof.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced)
  TArray<UHxDofBehavior*> physical_behaviors_;

  //! The list of state functions defining the primitive's state on this UHxDof.

  // The list of state functions defining the primitive's state on this UHxDof.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced)
  TArray<UHxStateFunction*> state_functions_;

};
