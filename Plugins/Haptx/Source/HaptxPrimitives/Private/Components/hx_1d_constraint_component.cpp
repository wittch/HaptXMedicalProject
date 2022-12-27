// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/Components/hx_1d_constraint_component.h>
#include <HaptxPrimitives/Public/haptx_primitives_shared.h>
#include <HaptxPrimitives/Public/ihaptx_primitives.h>

UHx1DConstraintComponent::UHx1DConstraintComponent(
    const FObjectInitializer &object_initializer) : Super(object_initializer),
    operating_domain_(EDofDomain::LAST), operating_axis_(EDofAxis::Z), initial_position_(0.0f),
    limit_motion_(false), scale_linear_limits_(false), lock_other_domain_(true), damping_(10.f),
    physical_behaviors_(), state_functions_() {}

void UHx1DConstraintComponent::BeginPlay() {
  Super::BeginPlay();

  if (limit_motion_ && lower_limit_ > upper_limit_) {
    UE_LOG(HaptxPrimitives, Warning,
        TEXT("Lower limit higher then upper limit in UHx1DConstraintComponent: low %f > high %f"),
        lower_limit_, upper_limit_);
  }

  if (!isConstraintFormed()) {
    return;
  }

  if (FMath::Abs(initial_position_) > 0.0f) {
    teleportAnchor1AlongDof(initial_position_, makeDegreeOfFreedomFromDomainAndAxis(
        operating_domain_, operating_axis_));
  }
}

void UHx1DConstraintComponent::InitializeComponent() {
  // Register behaviors and states.
  UHxDof* dof = getOperatingDof();
  if (dof != nullptr) {
    dof->force_update_ = true;
    for (UHxDofBehavior* behavior : physical_behaviors_) {
      dof->registerPhysicalBehavior(behavior);
    }
    for (UHxStateFunction* function : state_functions_) {
      dof->registerStateFunction(function);
    }
  }
  else {
    UE_LOG(HaptxPrimitives, Error, TEXT("Failed to find associated UHxDof."))
  }

  // Call parent's function (very necessary!)
  Super::InitializeComponent();
}

EDegreeOfFreedom UHx1DConstraintComponent::getOperatingDegreeOfFreedom() const {
  return makeDegreeOfFreedomFromDomainAndAxis(operating_domain_, operating_axis_);
}

UHxDof* UHx1DConstraintComponent::getOperatingDof() const {
  return getDof(makeDegreeOfFreedomFromDomainAndAxis(operating_domain_, operating_axis_));
}

void UHx1DConstraintComponent::teleportAnchor1AlongOperatingDof(float new_position) {
  teleportAnchor1AlongDof(new_position, getOperatingDegreeOfFreedom());
}

void UHx1DConstraintComponent::addForceAtAnchorAlongOperatingDof(float force, EAnchor anchor,
    bool accel_change, bool visualize) {
  FVector direction = directionOfDegreeOfFreedom(getOperatingDegreeOfFreedom());
  addForceAtAnchor(force * direction, anchor, EAnchorForceTorqueSpace::ANCHOR2, accel_change, 
      visualize);
}

void UHx1DConstraintComponent::addTorqueAtAnchorAlongOperatingDof(float torque,
    EAnchor anchor, bool accel_change, bool visualize) {
  FVector direction = directionOfDegreeOfFreedom(getOperatingDegreeOfFreedom());
  addTorqueAtAnchor(torque * direction, anchor, EAnchorForceTorqueSpace::ANCHOR2, accel_change, 
      visualize);
}

void UHx1DConstraintComponent::addImpulseAtAnchorAlongOperatingDof(float impulse, 
    EAnchor anchor, bool vel_change, bool visualize) {
  FVector direction = directionOfDegreeOfFreedom(getOperatingDegreeOfFreedom());
  addImpulseAtAnchor(impulse * direction, anchor, EAnchorForceTorqueSpace::ANCHOR2, vel_change, 
      visualize);
}

void UHx1DConstraintComponent::addAngularImpulseAtAnchorAlongOperatingDof(
    float angular_impulse, EAnchor anchor, bool vel_change, bool visualize) {
  FVector direction = directionOfDegreeOfFreedom(getOperatingDegreeOfFreedom());
  addAngularImpulseAtAnchor(angular_impulse * direction, anchor, EAnchorForceTorqueSpace::ANCHOR2, 
      vel_change, visualize);
}

void UHx1DConstraintComponent::setLimits(float lower_limit, float upper_limit) {
  lower_limit_ = lower_limit;
  upper_limit_ = upper_limit;

  if (!isConstraintFormed()) {
    return;
  }

  updateConstraint();
}

void UHx1DConstraintComponent::setDamping(float damping) {
  damping_ = damping;

  if (!isConstraintFormed()) {
    return;
  }

  updateConstraint();
}

void UHx1DConstraintComponent::configureConstraint(
    FConstraintInstance& in_constraint_instance) {
  in_constraint_instance.bScaleLinearLimits = scale_linear_limits_;

  // Lock degrees of freedom.
  in_constraint_instance.ProfileInstance.LinearLimit.XMotion =
      ((operating_domain_ == EDofDomain::LINEAR && operating_axis_ == EDofAxis::X) || 
      (operating_domain_ == EDofDomain::ANGULAR && !lock_other_domain_)) ? 
      ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Locked;
  in_constraint_instance.ProfileInstance.LinearLimit.YMotion =
      ((operating_domain_ == EDofDomain::LINEAR && operating_axis_ == EDofAxis::Y) ||
      (operating_domain_ == EDofDomain::ANGULAR && !lock_other_domain_)) ?
      ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Locked;
  in_constraint_instance.ProfileInstance.LinearLimit.ZMotion =
      ((operating_domain_ == EDofDomain::LINEAR && operating_axis_ == EDofAxis::Z) ||
      (operating_domain_ == EDofDomain::ANGULAR && !lock_other_domain_)) ?
      ELinearConstraintMotion::LCM_Free : ELinearConstraintMotion::LCM_Locked;
  in_constraint_instance.ProfileInstance.TwistLimit.TwistMotion =
      ((operating_domain_ == EDofDomain::ANGULAR && operating_axis_ == EDofAxis::X) ||
      (operating_domain_ == EDofDomain::LINEAR && !lock_other_domain_)) ?
      EAngularConstraintMotion::ACM_Free : EAngularConstraintMotion::ACM_Locked;
  in_constraint_instance.ProfileInstance.ConeLimit.Swing2Motion =
      ((operating_domain_ == EDofDomain::ANGULAR && operating_axis_ == EDofAxis::Y) ||
      (operating_domain_ == EDofDomain::LINEAR && !lock_other_domain_)) ?
      EAngularConstraintMotion::ACM_Free : EAngularConstraintMotion::ACM_Locked;
  in_constraint_instance.ProfileInstance.ConeLimit.Swing1Motion =
      ((operating_domain_ == EDofDomain::ANGULAR && operating_axis_ == EDofAxis::Z) ||
      (operating_domain_ == EDofDomain::LINEAR && !lock_other_domain_)) ?
      EAngularConstraintMotion::ACM_Free : EAngularConstraintMotion::ACM_Locked;

  // Apply damping.
  EDegreeOfFreedom degree_of_freedom = makeDegreeOfFreedomFromDomainAndAxis(operating_domain_, operating_axis_);
  in_constraint_instance.ProfileInstance.LinearDrive.VelocityTarget = operating_domain_ == EDofDomain::LINEAR ?
      FVector::ZeroVector : in_constraint_instance.ProfileInstance.LinearDrive.VelocityTarget;
  in_constraint_instance.ProfileInstance.LinearDrive.XDrive.bEnableVelocityDrive = degree_of_freedom == EDegreeOfFreedom::X_LIN;
  in_constraint_instance.ProfileInstance.LinearDrive.XDrive.Damping = degree_of_freedom == EDegreeOfFreedom::X_LIN ? damping_ : 0.0f;
  in_constraint_instance.ProfileInstance.LinearDrive.YDrive.bEnableVelocityDrive = degree_of_freedom == EDegreeOfFreedom::Y_LIN;
  in_constraint_instance.ProfileInstance.LinearDrive.YDrive.Damping = degree_of_freedom == EDegreeOfFreedom::Y_LIN ? damping_ : 0.0f;
  in_constraint_instance.ProfileInstance.LinearDrive.ZDrive.bEnableVelocityDrive = degree_of_freedom == EDegreeOfFreedom::Z_LIN;
  in_constraint_instance.ProfileInstance.LinearDrive.ZDrive.Damping = degree_of_freedom == EDegreeOfFreedom::Z_LIN ? damping_ : 0.0f;

  in_constraint_instance.ProfileInstance.AngularDrive.AngularVelocityTarget = domainOfDegreeOfFreedom(degree_of_freedom) == EDofDomain::ANGULAR ?
    FVector::ZeroVector : in_constraint_instance.ProfileInstance.AngularDrive.AngularVelocityTarget;
  in_constraint_instance.ProfileInstance.AngularDrive.AngularDriveMode = operating_domain_ == EDofDomain::ANGULAR ? 
      EAngularDriveMode::TwistAndSwing : 
      in_constraint_instance.ProfileInstance.AngularDrive.AngularDriveMode;
  in_constraint_instance.ProfileInstance.AngularDrive.TwistDrive.bEnableVelocityDrive = degree_of_freedom == EDegreeOfFreedom::X_ANG;
  in_constraint_instance.ProfileInstance.AngularDrive.TwistDrive.Damping = degree_of_freedom == EDegreeOfFreedom::X_ANG ? damping_ : 0.0f;
  in_constraint_instance.ProfileInstance.AngularDrive.SwingDrive.bEnableVelocityDrive = degree_of_freedom == EDegreeOfFreedom::Y_ANG || degree_of_freedom == EDegreeOfFreedom::Z_ANG;
  in_constraint_instance.ProfileInstance.AngularDrive.SwingDrive.Damping = (degree_of_freedom == EDegreeOfFreedom::Y_ANG || degree_of_freedom == EDegreeOfFreedom::Z_ANG) ? damping_ : 0.0f;
}
