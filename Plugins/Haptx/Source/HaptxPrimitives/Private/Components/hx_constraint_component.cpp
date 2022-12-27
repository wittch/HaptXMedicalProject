// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/Components/hx_constraint_component.h>
#include <Runtime/Engine/Classes/Components/PrimitiveComponent.h>
#include <Runtime/Engine/Classes/Engine/World.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Private/PhysicsEngine/PhysXSupport.h>
#include <Runtime/Engine/Public/PhysicsPublic.h>
#include <Runtime/PhysicsCore/Public/PhysXIncludes.h>
#include <Haptx/Public/hx_debug_draw_system.h>
#include <HaptxPrimitives/Public/haptx_primitives_shared.h>
#include <HaptxPrimitives/Public/ihaptx_primitives.h>

UHxDof* FAllDofs::operator[](int index) const {
  switch (index) {
    case 0: {
      return static_cast<UHxDof*>(x_linear_dof.dof);
    }
    case 1: {
      return static_cast<UHxDof*>(y_linear_dof.dof);
    }
    case 2: {
      return static_cast<UHxDof*>(z_linear_dof.dof);
    }
    case 3: {
      return static_cast<UHxDof*>(x_angular_dof.dof);
    }
    case 4: {
      return static_cast<UHxDof*>(y_angular_dof.dof);
    }
    case 5: {
      return static_cast<UHxDof*>(z_angular_dof.dof);
    }
    default: {
      return nullptr;
    }
  }
}

void FHxConstraintSecondaryTickFunction::ExecuteTick(
    float DeltaTime,
    ELevelTick TickType,
    ENamedThreads::Type CurrentThread,
    const FGraphEventRef& CompletionGraphEvent) {
  if (Target && !Target->IsPendingKill() && !Target->IsUnreachable()) {
    FScopeCycleCounterUObject ActorScope(Target);
    Target->TickComponentSecondary(DeltaTime, TickType, *this);
  }
}

FString FHxConstraintSecondaryTickFunction::DiagnosticMessage() {
  return Target->GetFullName() + TEXT("UHxConstraintComponent Post-Physics Tick.");
}

UHxConstraintComponent::UHxConstraintComponent(const FObjectInitializer& object_initializer):
    Super(object_initializer), visualize_anchors_(false) {
  bEditableWhenInherited = true;
  bWantsInitializeComponent = true;
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;
  SecondaryComponentTick.bCanEverTick = true;
  SecondaryComponentTick.bStartWithTickEnabled = true;
  SecondaryComponentTick.TickGroup = TG_PostPhysics;
  ConstraintInstance.SetDisableCollision(true);
  ConstraintInstance.ProfileInstance.LinearLimit.bSoftConstraint = false;
  ConstraintInstance.ProfileInstance.ConeLimit.bSoftConstraint = false;
  ConstraintInstance.ProfileInstance.TwistLimit.bSoftConstraint = false;
  ConstraintInstance.ProfileInstance.ProjectionLinearTolerance = 0.1f;
  ConstraintInstance.ProfileInstance.ProjectionAngularTolerance = 1.0f;
  dofs_.x_linear_dof.dof = object_initializer.CreateDefaultSubobject<UHxLinearDof>(this, TEXT("X Linear DOF"));
  dofs_.y_linear_dof.dof = object_initializer.CreateDefaultSubobject<UHxLinearDof>(this, TEXT("Y Linear DOF"));
  dofs_.z_linear_dof.dof = object_initializer.CreateDefaultSubobject<UHxLinearDof>(this, TEXT("Z Linear DOF"));
  dofs_.x_angular_dof.dof = object_initializer.CreateDefaultSubobject<UHxAngularDof>(this, TEXT("X Angular DOF"));
  dofs_.y_angular_dof.dof = object_initializer.CreateDefaultSubobject<UHxAngularDof>(this, TEXT("Y Angular DOF"));
  dofs_.z_angular_dof.dof = object_initializer.CreateDefaultSubobject<UHxAngularDof>(this, TEXT("Z Angular DOF"));
}

void UHxConstraintComponent::InitializeComponent() {
  USceneComponent::InitializeComponent();

  // Initialize each UHxDof with the corresponding, correctly serialized UHxDofBehaviors and
  // UHxStateFunctions.
  FLinearDof* linear_dof_ptrs[] = 
    { &dofs_.x_linear_dof, &dofs_.y_linear_dof, &dofs_.z_linear_dof };
  for (FLinearDof* linear_dof_ptr : linear_dof_ptrs) {
    if (linear_dof_ptr->dof != nullptr) {
      for (UHxDofBehavior* behavior : linear_dof_ptr->physical_behaviors) {
        linear_dof_ptr->dof->registerPhysicalBehavior(behavior);
      }
      for (UHxStateFunction* function : linear_dof_ptr->state_functions) {
        linear_dof_ptr->dof->registerStateFunction(function);
      }
    }
  }
  FAngularDof* angular_dof_ptrs[] = 
    { &dofs_.x_angular_dof, &dofs_.y_angular_dof, &dofs_.z_angular_dof };
  for (FAngularDof* angular_dof_ptr : angular_dof_ptrs) {
    if (angular_dof_ptr->dof != nullptr) {
      for (UHxDofBehavior* behavior : angular_dof_ptr->physical_behaviors) {
        angular_dof_ptr->dof->registerPhysicalBehavior(behavior);
      }
      for (UHxStateFunction* function : angular_dof_ptr->state_functions) {
        angular_dof_ptr->dof->registerStateFunction(function);
      }
    }
  }

  // Copy initial UPhysicsConstraintComponent settings.
  hx_constraint_instance_ = ConstraintInstance;

  InitComponentConstraint();
}

#if WITH_EDITOR
void UHxConstraintComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
  configureConstraint(ConstraintInstance);

  Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UHxConstraintComponent::BeginPlay() {
  Super::BeginPlay();

  if (!IsTemplate() && SecondaryComponentTick.bCanEverTick) {
    SecondaryComponentTick.Target = this;
    SecondaryComponentTick.SetTickFunctionEnable(SecondaryComponentTick.bStartWithTickEnabled);
    SecondaryComponentTick.RegisterTickFunction(GetOwner()->GetLevel());
  }
}

void UHxConstraintComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  if (!isConstraintFormed()) {
    return;
  }

  if (!isSleeping()) {
    applyDofBehaviors();
  }
}

void UHxConstraintComponent::TickComponentSecondary(
    float DeltaTime,
    ELevelTick TickType,
    FHxConstraintSecondaryTickFunction& ThisTickFunction) {
  // The relative size to draw anchor coordinate systems.
  const float ANCHOR_THICKNESS_SCALE = 0.066f;
  // The color to draw anchor1's coordinate system.
  const FColor ANCHOR1_COLOR = FColor::Red;
  // The color to draw anchor2's coordinate system.
  const FColor ANCHOR2_COLOR = FColor::Blue;
  // The relative length to draw coordinate system axes.
  const float COORD_SYS_LENGTH_SCALE = 1.2f;
  // The relative thickness to draw coordinate system axes.
  const float COORD_SYS_THICKNESS_SCALE = 0.033f;

  if (!isConstraintFormed()) {
    return;
  }

  // Manage state functions.
  for (uint8 i = 0; i < uint8(EDegreeOfFreedom::LAST); i++) {
    UHxDof* dof = dofs_[i];
    if (dof == nullptr) {
      continue;
    }

    for (UHxStateFunction* function : dof->state_functions_) {
      function->update(dof->getCurrentPosition());
    }
  }

  // Visualize anchors.
  if (visualize_anchors_ &&
      (IsValid(component1_) || IsValid(component2_))) {
    // Draw anchor1 in the world.
    FTransform w_anchor1 = getAnchor1Transform(EAnchor1Space::WORLD);
    HxDebugDrawSystem::coordinateSystem(
        GetOwner(),
        w_anchor1.GetLocation(),
        w_anchor1.GetRotation(),
        COORD_SYS_LENGTH_SCALE * w_viz_scale_,
        COORD_SYS_THICKNESS_SCALE * w_viz_scale_);

    // Draw a line between component1's COM and its anchor.
    if (IsValid(component1_)) {
      HxDebugDrawSystem::line(
          GetOwner(),
          component1_->GetCenterOfMass(ConstraintInstance.ConstraintBone1),
          w_anchor1.GetLocation(),
          ANCHOR1_COLOR, ANCHOR_THICKNESS_SCALE * w_viz_scale_);
    }
    
    // Draw anchor2 in the world.
    FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);
    HxDebugDrawSystem::coordinateSystem(
        GetOwner(),
        w_anchor2.GetLocation(),
        w_anchor2.GetRotation(),
        COORD_SYS_LENGTH_SCALE * w_viz_scale_,
        COORD_SYS_THICKNESS_SCALE * w_viz_scale_);

    // Draw a line between component2's COM and its anchor.
    if (IsValid(component2_)) {
      HxDebugDrawSystem::line(
          GetOwner(),
          component2_->GetCenterOfMass(ConstraintInstance.ConstraintBone2),
          w_anchor2.GetLocation(),
          ANCHOR2_COLOR, ANCHOR_THICKNESS_SCALE * w_viz_scale_);
    }
  }
}

void UHxConstraintComponent::applyDofBehaviors() {
  if (!isConstraintFormed()) {
    return;
  }
  
  // Update UHxDofs with the current "constraint space" transform.
  updateDofs();

  // Apply physical behaviors.
  for (uint8 i = 0; i < uint8(EDegreeOfFreedom::LAST); i++) {
    UHxDof* dof = dofs_[i];
    if (dof == nullptr) {
      continue;
    }
    const FVector direction = directionOfDegreeOfFreedom(EDegreeOfFreedom(i));
    const float current_position = dof->getCurrentPosition();

    for (UHxDofBehavior* behavior : dof->physical_behaviors_) {
      if (behavior != nullptr && behavior->enabled_) {
        // Calculate the signed magnitude of the force or torque being applied to component1 at its
        // anchor.
        float a2_force_torque_smag1 = behavior->getForceTorque(current_position);

        // It's a force!
        if (domainOfDegreeOfFreedom(EDegreeOfFreedom(i)) == EDofDomain::LINEAR) {
          FVector a2_force1 = a2_force_torque_smag1 * direction *
              (ConstraintInstance.bScaleLinearLimits ? w_constraint_scale_ : FVector::OneVector);
          FVector a2_force2 = -a2_force1;
          addForceAtAnchor(a2_force1, EAnchor::ANCHOR1, EAnchorForceTorqueSpace::ANCHOR2,
              behavior->acceleration_, behavior->visualize_);
          addForceAtAnchor(a2_force2, EAnchor::ANCHOR2, EAnchorForceTorqueSpace::ANCHOR2,
              behavior->acceleration_, behavior->visualize_);
        }
        else {
          // It's a torque!
          FVector a2_torque1 = a2_force_torque_smag1 * direction;
          FVector a2_torque2 = -a2_torque1;
          addTorqueAtAnchor(a2_torque1, EAnchor::ANCHOR1, EAnchorForceTorqueSpace::ANCHOR2,
              behavior->acceleration_, behavior->visualize_);
          addTorqueAtAnchor(a2_torque2, EAnchor::ANCHOR2, EAnchorForceTorqueSpace::ANCHOR2,
              behavior->acceleration_, behavior->visualize_);
        }
      }
    }
  }
}

void UHxConstraintComponent::updateDofs() {
  // Calculate the current position of anchor1 in anchor2's frame. This is the "constraint space"
  // transform that drives behaviors and states.
  FTransform a2_anchor1 = calculateConstraintTransform();

  // Update each UHxDof with new positions.
  for (uint8 i = 0; i < uint8(EDegreeOfFreedom::LAST); i++) {
    UHxDof* dof = dofs_[i];
    if (dof == nullptr || !dof->shouldUpdate()) {
      continue;
    }
    float in_position;
    if (domainOfDegreeOfFreedom(EDegreeOfFreedom(i)) == EDofDomain::LINEAR) {
      in_position = 
          FTransform1D::GetTranslation(a2_anchor1, axisOfDegreeOfFreedom(EDegreeOfFreedom(i)));
    }
    else {
      in_position = 
          FTransform1D::GetRotation(a2_anchor1, axisOfDegreeOfFreedom(EDegreeOfFreedom(i)));
    }
    dof->update(in_position);
  }
}

void UHxConstraintComponent::hxSetConstrainedComponents(
    UPrimitiveComponent* component1, FName bone_name1,
    UPrimitiveComponent* component2, FName bone_name2) {
  if (IsValid(component1)) {
    ComponentName1.ComponentName = component1->GetFName();
    OverrideComponent1 = component1;
    ConstraintInstance.ConstraintBone1 = bone_name1;
  }

  if (IsValid(component2)) {
    ComponentName2.ComponentName = component2->GetFName();
    OverrideComponent2 = component2;
    ConstraintInstance.ConstraintBone2 = bone_name2;
  }

  // This is the UHxConstraintComponent version.
  InitComponentConstraint();
}

void UHxConstraintComponent::updateConstraint() {
  // Perform any configuration logic.
  configureConstraint(hx_constraint_instance_);

  // If this constraint is frozen, override limits offsets.
  FVector a2_linear_limits_offset = linear_limits_offset_;
  FRotator a2_angular_rotation_offset = hx_constraint_instance_.AngularRotationOffset;
  if (frozen_) {
    FTransform current_transform = calculateConstraintTransform();
    a2_linear_limits_offset = current_transform.GetLocation();
    a2_angular_rotation_offset = current_transform.GetRotation().Rotator();
  }

  // In order to apply the linear offset we teleport one of the components back to its initial 
  // position, then apply an offset to it, and then reform the constraint. Afterward the component 
  // gets teleported back to it's initial position.
  FTransform saved_transform = FTransform::Identity;
  if (component1_ != nullptr) {
    saved_transform = component1_->GetComponentTransform();

    FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);
    FTransform a1_component1 = getAnchor1Transform(EAnchor1Space::COMPONENT1).Inverse();
    FTransform w_component1_initial = 
        UKismetMathLibrary::ComposeTransforms(a1_component1, w_anchor2);
    component1_->SetWorldLocationAndRotation(
        w_component1_initial.GetLocation(), w_component1_initial.GetRotation(),
        false, nullptr, ETeleportType::TeleportPhysics
    );
    FVector w_linear_limits_offset = w_anchor2.GetRotation().RotateVector(a2_linear_limits_offset *
        (ConstraintInstance.bScaleLinearLimits ? GetComponentScale() : FVector::OneVector));
    component1_->AddWorldOffset(w_linear_limits_offset, false, nullptr, ETeleportType::TeleportPhysics);
    SetWorldLocationAndRotation(w_anchor2.GetLocation(), w_anchor2.GetRotation());
  }
  else if (component2_ != nullptr) {
    saved_transform = component2_->GetComponentTransform();

    FTransform w_anchor1 = getAnchor1Transform(EAnchor1Space::WORLD);
    FTransform a2_component2 = getAnchor2Transform(EAnchor2Space::COMPONENT2).Inverse();
    FTransform w_component2_initial = 
        UKismetMathLibrary::ComposeTransforms(a2_component2, w_anchor1);
    component2_->SetWorldLocationAndRotation(
        w_component2_initial.GetLocation(), w_component2_initial.GetRotation(),
        false, nullptr, ETeleportType::TeleportPhysics);
    FVector w_linear_limits_offset = w_anchor1.GetRotation().RotateVector(a2_linear_limits_offset *
        (ConstraintInstance.bScaleLinearLimits ? GetComponentScale() : FVector::OneVector));
    component2_->AddWorldOffset(-w_linear_limits_offset, false, nullptr, ETeleportType::TeleportPhysics);
    SetWorldLocationAndRotation(w_anchor1.GetLocation(), w_anchor1.GetRotation());
  }

  // Configure the underlying UPhysicsConstraintComponent.
  hx_constraint_instance_.ConstraintHandle = ConstraintInstance.ConstraintHandle;
  ConstraintInstance = hx_constraint_instance_;
  ConstraintInstance.AngularRotationOffset = a2_angular_rotation_offset;
  if (frozen_) {
    ConstraintInstance.SetLinearXLimit(LCM_Locked, 0);
    ConstraintInstance.SetLinearYLimit(LCM_Locked, 0);
    ConstraintInstance.SetLinearZLimit(LCM_Locked, 0);
    ConstraintInstance.SetAngularSwing1Limit(ACM_Locked, 0);
    ConstraintInstance.SetAngularSwing2Limit(ACM_Locked, 0);
    ConstraintInstance.SetAngularTwistLimit(ACM_Locked, 0);
  }
  UPhysicsConstraintComponent::InitComponentConstraint();
  hx_constraint_instance_.ConstraintHandle = ConstraintInstance.ConstraintHandle;

  // Restore the moved component to its initial transform.
  if (component1_ != nullptr) {
    component1_->SetWorldTransform(
        saved_transform, false, nullptr, ETeleportType::TeleportPhysics);
  }
  else if (component2_ != nullptr) {
    component2_->SetWorldTransform(
        saved_transform, false, nullptr, ETeleportType::TeleportPhysics);
  }
}

void UHxConstraintComponent::setLinearLimitsOffset(const FVector offset) {
  linear_limits_offset_ = offset;

  if (!isConstraintFormed()) {
    return;
  }

  updateConstraint();
}

void UHxConstraintComponent::setAngularLimitsOffset(const FRotator offset) {
  ConstraintInstance.AngularRotationOffset = offset;
  ConstraintInstance.AngularRotationOffset.Normalize();

  if (!isConstraintFormed()) {
    return;
  }

  updateConstraint();
}

void UHxConstraintComponent::teleportAnchor1(FVector new_location, FRotator new_rotation) {
  if (!IsValid(component1_)) {
    return;
  }

  // Construct a transform out of the new position and rotation.
  new_rotation.Normalize();
  FTransform a2_anchor1_target = FTransform(
      new_rotation, 
      new_location * (ConstraintInstance.bScaleLinearLimits ? w_constraint_scale_ : FVector::OneVector), 
      FVector::OneVector);

  // Calculate the world position of anchor2.
  FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);

  // Calculate component1's new world location and rotation.
  FTransform a1_component1 = getAnchor1Transform(EAnchor1Space::COMPONENT1).Inverse();
  FTransform w_component1 = UKismetMathLibrary::ComposeTransforms(a1_component1, 
      UKismetMathLibrary::ComposeTransforms(a2_anchor1_target, w_anchor2));

  // Do the teleport.
  component1_->SetWorldLocationAndRotation(w_component1.GetLocation(), w_component1.GetRotation(), 
      false, nullptr, ETeleportType::TeleportPhysics);

  // If we're frozen we have to update our constraint or we'll snap back to the previous position.
  if (frozen_) {
    updateConstraint();
  }

  // Update each UHxDof.
  for (uint8 i = 0; i < uint8(EDegreeOfFreedom::LAST); i++) {
    UHxDof* hx_dof = dofs_[i];
    if (hx_dof != nullptr) {
      EDegreeOfFreedom degree_of_freedom = static_cast<EDegreeOfFreedom>(i);
      if (domainOfDegreeOfFreedom(degree_of_freedom) == EDofDomain::LINEAR) {
        hx_dof->update(
            FTransform1D::GetTranslation(a2_anchor1_target, 
                axisOfDegreeOfFreedom(degree_of_freedom)), 
            true);
      }
      else {
        hx_dof->update(
            FTransform1D::GetRotation(a2_anchor1_target, 
                axisOfDegreeOfFreedom(degree_of_freedom)), 
            true);
      }
    }
  }
}

void UHxConstraintComponent::teleportAnchor1AlongDof(float new_position, 
    EDegreeOfFreedom degree_of_freedom) {
  if (!IsValid(component1_)) {
    return;
  }

  // The current location and rotation about the given direction.
  FTransform a2_anchor1 = calculateConstraintTransform();

  // Perform the teleportation.
  FVector direction = directionOfDegreeOfFreedom(degree_of_freedom);
  if (domainOfDegreeOfFreedom(degree_of_freedom) == EDofDomain::ANGULAR) {
    teleportAnchor1(
        a2_anchor1.GetLocation(),
        (a2_anchor1.GetRotation() *
            FQuat(direction, DEG_TO_RAD *
            (TotalRotation(new_position).partial_angle_ - FTransform1D::GetRotation(
                a2_anchor1, axisOfDegreeOfFreedom(degree_of_freedom))))).Rotator());
  } else {
    teleportAnchor1(
        a2_anchor1.GetLocation() + (new_position - FTransform1D::GetTranslation(
            a2_anchor1, axisOfDegreeOfFreedom(degree_of_freedom))) * direction,
        a2_anchor1.GetRotation().Rotator());
  }

  // Update the UHxDof.
  UHxDof* hx_dof = getDof(degree_of_freedom);
  if (IsValid(hx_dof)) {
    hx_dof->update(new_position, true);
  }
}

void UHxConstraintComponent::addForceAtAnchor(FVector force, EAnchor anchor,
    EAnchorForceTorqueSpace space, bool accel_change, bool visualize) {
  addForceAtAnchorInternal(force, anchor, space, accel_change, visualize, false);
}

void UHxConstraintComponent::addTorqueAtAnchor(FVector torque, EAnchor anchor,
    EAnchorForceTorqueSpace space, bool accel_change, bool visualize) {
  addTorqueAtAnchorInternal(torque, anchor, space, accel_change, visualize, false);
}

void UHxConstraintComponent::addImpulseAtAnchor(FVector impulse, EAnchor anchor,
  EAnchorForceTorqueSpace space, bool vel_change, bool visualize) {
  addForceAtAnchorInternal(impulse, anchor, space, vel_change, visualize, true);
}

void UHxConstraintComponent::addAngularImpulseAtAnchor(FVector angular_impulse, EAnchor anchor,
  EAnchorForceTorqueSpace space, bool vel_change, bool visualize) {
  addTorqueAtAnchorInternal(angular_impulse, anchor, space, vel_change, visualize, true);
}

// The relative length to draw torque arrow heads.
const float FORCE_TORQUE_ARROW_SIZE_SCALE = 0.05f;
// The relative thickness to draw torque arrows.
const float FORCE_TORQUE_THICKNESS_SCALE = 0.011f;
// The relative length to draw torque arrows.
const float TORQUE_SCALE = 1.0f;
// The color to draw torque arrows.
const FColor TORQUE_COLOR = FColor::Blue;

void UHxConstraintComponent::addForceAtAnchorInternal(FVector force, EAnchor anchor,
    EAnchorForceTorqueSpace space, bool accel_change, bool visualize, bool impulse) {
  // The relative length to draw force arrows.
  const float FORCE_SCALE = 0.01f;
  // The color to draw force arrows.
  const FColor FORCE_COLOR = FColor::Red;

  if (!isConstraintFormed()) {
    return;
  }

  if (anchor == EAnchor::ANCHOR1) {
    if (isValidAndSimulating(component1_)) {
      // Calculate the force in world space.
      FVector w_force1 = force;
      if (space == EAnchorForceTorqueSpace::ANCHOR2) {
        FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);
        w_force1 = w_anchor2.GetRotation().RotateVector(force);
      }

      if (w_force1.Size() > FLT_EPSILON) {
        // Calculate the torque generated by the force.
        FTransform w_anchor1 = getAnchor1Transform(EAnchor1Space::WORLD);
        FVector w_distance1 = w_anchor1.GetLocation() -
          component1_->GetCenterOfMass(ConstraintInstance.ConstraintBone1);
        FVector w_torque1 = FVector::CrossProduct(w_distance1, w_force1);

        // Apply as either a force or an impulse.
        if (impulse) {
          component1_->AddImpulse(w_force1, ConstraintInstance.ConstraintBone1, accel_change);
          component1_->AddAngularImpulseInRadians(w_torque1, ConstraintInstance.ConstraintBone1,
              accel_change);
        } else {
          component1_->AddForce(w_force1, ConstraintInstance.ConstraintBone1, accel_change);
          component1_->AddTorqueInRadians(w_torque1, ConstraintInstance.ConstraintBone1,
              accel_change);
        }

        // Visualize the force/impulse.
        if (visualize) {
          HxDebugDrawSystem::arrow(GetOwner(),
            w_anchor1.GetLocation(),
            w_anchor1.GetLocation() + FORCE_SCALE * w_force1,
            FORCE_TORQUE_ARROW_SIZE_SCALE * w_viz_scale_, FORCE_COLOR,
            FORCE_TORQUE_THICKNESS_SCALE * w_viz_scale_);
        }
      }
    }
  }
  else {
    if (isValidAndSimulating(component2_)) {
      // Calculate the force in world space.
      FVector w_force2 = force;
      FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);
      if (space == EAnchorForceTorqueSpace::ANCHOR2) {
          w_force2 = w_anchor2.GetRotation().RotateVector(w_force2);
      }

      if (w_force2.Size() > FLT_EPSILON) {
        // Calculate the torque generated by the force.
        FVector w_distance2 = w_anchor2.GetLocation() -
          component2_->GetCenterOfMass(ConstraintInstance.ConstraintBone2);
        FVector w_torque2 = FVector::CrossProduct(w_distance2, w_force2);

        // Apply as either a force or an impulse.
        if (impulse) {
          component2_->AddImpulse(w_force2, ConstraintInstance.ConstraintBone2, accel_change);
          component2_->AddAngularImpulseInRadians(w_torque2, ConstraintInstance.ConstraintBone2,
            accel_change);
        } else {
          component2_->AddForce(w_force2, ConstraintInstance.ConstraintBone2, accel_change);
          component2_->AddTorqueInRadians(w_torque2, ConstraintInstance.ConstraintBone2,
            accel_change);
        }

        // Visualize the force/impulse.
        if (visualize) {
          HxDebugDrawSystem::arrow(GetOwner(),
            w_anchor2.GetLocation(),
            w_anchor2.GetLocation() + FORCE_SCALE * w_force2,
            FORCE_TORQUE_ARROW_SIZE_SCALE * w_viz_scale_, FORCE_COLOR,
            FORCE_TORQUE_THICKNESS_SCALE * w_viz_scale_);
        }
      }
    }
  }
}

void UHxConstraintComponent::addTorqueAtAnchorInternal(FVector torque, EAnchor anchor,
  EAnchorForceTorqueSpace space, bool accel_change, bool visualize, bool impulse) {
  if (!isConstraintFormed()) {
    return;
  }
  
  FVector w_torque = torque;
  if (space == EAnchorForceTorqueSpace::ANCHOR2) {
    FTransform w_anchor2 = getAnchor2Transform(EAnchor2Space::WORLD);
    w_torque = w_anchor2.GetRotation().RotateVector(w_torque);
  }

  if (w_torque.Size() > FLT_EPSILON) {
    if (anchor == EAnchor::ANCHOR1) {
      if (isValidAndSimulating(component1_)) {
        if (impulse) {
          component1_->AddAngularImpulseInRadians(w_torque, ConstraintInstance.ConstraintBone1,
              accel_change);
        } else {
          component1_->AddTorqueInRadians(w_torque, ConstraintInstance.ConstraintBone1,
              accel_change);
        }

        // Visualize the applied torque/impulse.
        if (visualize) {
          HxDebugDrawSystem::arrow(GetOwner(),
            component1_->GetCenterOfMass(ConstraintInstance.ConstraintBone1),
            component1_->GetCenterOfMass(ConstraintInstance.ConstraintBone1) + 
              TORQUE_SCALE * w_torque,
            FORCE_TORQUE_ARROW_SIZE_SCALE * w_viz_scale_, TORQUE_COLOR,
            FORCE_TORQUE_THICKNESS_SCALE * w_viz_scale_);
        }
      }
    }
    else {
      if (isValidAndSimulating(component2_)) {
        if (impulse) {
          component2_->AddAngularImpulseInRadians(w_torque, ConstraintInstance.ConstraintBone2,
            accel_change);
        } else {
          component2_->AddTorqueInRadians(w_torque, ConstraintInstance.ConstraintBone2,
            accel_change);
        }

        // Visualize the applied torque/impulse.
        if (visualize) {
          HxDebugDrawSystem::arrow(GetOwner(),
            component2_->GetCenterOfMass(ConstraintInstance.ConstraintBone2),
            component2_->GetCenterOfMass(ConstraintInstance.ConstraintBone2) + 
              TORQUE_SCALE * w_torque,
            FORCE_TORQUE_ARROW_SIZE_SCALE * w_viz_scale_, TORQUE_COLOR,
            FORCE_TORQUE_THICKNESS_SCALE * w_viz_scale_);
        }
      }
    }
  }
}

FTransform UHxConstraintComponent::getAnchor1Transform(EAnchor1Space space) const {
  switch (space) {
  case EAnchor1Space::WORLD: {
    // Calculate the current position of anchor1 in the world's frame.
    FTransform w_anchor1 = b1_anchor1_;
    if (IsValid(component1_)) {
      FTransform w_bone1 =
          getScaleOne(component1_->GetSocketTransform(ConstraintInstance.ConstraintBone1));
      w_anchor1 = UKismetMathLibrary::ComposeTransforms(w_anchor1, w_bone1);
    }
    return w_anchor1;
  }
  case EAnchor1Space::COMPONENT1: {
    // Calculate the current position of anchor1 in component1's frame.
    FTransform l1_anchor1 = b1_anchor1_;
    if (IsValid(component1_)) {
      FTransform w_bone1 =
          getScaleOne(component1_->GetSocketTransform(ConstraintInstance.ConstraintBone1));
      FTransform l1_world = getScaleOne(component1_->GetComponentTransform()).Inverse();
      l1_anchor1 = UKismetMathLibrary::ComposeTransforms(b1_anchor1_, 
          UKismetMathLibrary::ComposeTransforms(w_bone1, l1_world));
    }
    return l1_anchor1;
  }
  case EAnchor1Space::BONE1:
    // The current position of anchor1 in bone1's frame is known.
    return b1_anchor1_;
  default:
    // This should never happen.
    UE_LOG(HaptxPrimitives, Error, TEXT("UHxConstraintComponent::getAnchor1Transform called with an invalid EAnchor1Space."))
    return FTransform::Identity;
  }
}

FTransform UHxConstraintComponent::getAnchor2Transform(EAnchor2Space space) const {
  switch (space) {
  case EAnchor2Space::WORLD: {
    // Calculate the current position of anchor2 in the world's frame.
    FTransform w_anchor2 = b2_anchor2_;
    if (IsValid(component2_)) {
      FTransform w_bone2 =
          getScaleOne(component2_->GetSocketTransform(ConstraintInstance.ConstraintBone2));
      w_anchor2 = UKismetMathLibrary::ComposeTransforms(w_anchor2, w_bone2);
    }
    return w_anchor2;
  }
  case EAnchor2Space::COMPONENT2: {
    // Calculate the current position of anchor2 in component2's frame.
    FTransform l2_anchor2 = b2_anchor2_;
    if (IsValid(component2_)) {
      FTransform w_bone2 =
          getScaleOne(component2_->GetSocketTransform(ConstraintInstance.ConstraintBone2));
      FTransform l2_world = getScaleOne(component2_->GetComponentTransform()).Inverse();
      l2_anchor2 = UKismetMathLibrary::ComposeTransforms(b2_anchor2_, 
          UKismetMathLibrary::ComposeTransforms(w_bone2, l2_world));
    }
    return l2_anchor2;
  }
  case EAnchor2Space::BONE2:
    // The current position of anchor2 in bone2's frame is known.
    return b2_anchor2_;
  default:
    // This should never happen.
    UE_LOG(HaptxPrimitives, Error, TEXT("UHxConstraintComponent::getAnchor2Transform called with an invalid EAnchor2Space."))
    return FTransform::Identity;
  }
}

FTransform UHxConstraintComponent::calculateConstraintTransform() const {
  // Calculate the inverse of the world position of anchor2.
  FTransform a2_world = getAnchor2Transform(EAnchor2Space::WORLD).Inverse();

  // Calculate the world position of anchor1.
  FTransform w_anchor1 = getAnchor1Transform(EAnchor1Space::WORLD);

  // Calculate the current position of anchor1 in anchor2's frame. This is the "constraint space"
  // transform that drives behaviors and states.
  FTransform a2_anchor1 = UKismetMathLibrary::ComposeTransforms(w_anchor1, a2_world);

  if (ConstraintInstance.bScaleLinearLimits) {
    a2_anchor1.ScaleTranslation(l_world_scale_);
  }

  return a2_anchor1;
}

UHxDof* UHxConstraintComponent::getDof(EDegreeOfFreedom degree_of_freedom) const {
  return dofs_[uint8(degree_of_freedom)];
}

float UHxConstraintComponent::getPositionAlongDof(EDegreeOfFreedom degree_of_freedom) {
  UHxDof* dof_ptr = getDof(degree_of_freedom);
  if (dof_ptr != nullptr) {
    return dof_ptr->getCurrentPosition();
  }
  return 0;
}

void UHxConstraintComponent::freeze() {
  if (!frozen_) {
    frozen_ = true;

    if (!isConstraintFormed()) {
      return;
    }

    updateConstraint();
  }
}

void UHxConstraintComponent::unfreeze() {
  if (frozen_) {
    frozen_ = false;

    if (!isConstraintFormed()) {
      return;
    }

    updateConstraint();
  }
}

bool UHxConstraintComponent::isSleeping() {
  return 
      (!IsValid(component1_) || 
          !component1_->RigidBodyIsAwake(ConstraintInstance.ConstraintBone1)) && 
      (!IsValid(component2_) || 
          !component2_->RigidBodyIsAwake(ConstraintInstance.ConstraintBone2));
}

bool UHxConstraintComponent::isConstraintFormed() {
  return ConstraintInstance.ConstraintHandle.IsValid();
}

void UHxConstraintComponent::InitComponentConstraint() {
  // A lower bound on the size of this visualizer.
  const float VIS_SCALE_MIN = 1.0f;

  // Initialize all behaviors.
  for (uint8 i = 0; i < uint8(EDegreeOfFreedom::LAST); i++) {
    UHxDof* dof = dofs_[i];
    if (dof != nullptr) {
      dof->initialize();
    }
  }

  // Update member variables. If either component is welded, replace it with its weld parent.
  component1_ = GetComponentInternal(EConstraintFrame::Frame1);
  if (IsValid(component1_) && component1_->BodyInstance.WeldParent != nullptr
    && component1_->BodyInstance.WeldParent->OwnerComponent.Get() != nullptr) {
    component1_ = component1_->BodyInstance.WeldParent->OwnerComponent.Get();
    ComponentName1.ComponentName = component1_->GetFName();
    OverrideComponent1 = component1_;
    ConstraintInstance.ConstraintBone1 = NAME_None;
  }

  component2_ = GetComponentInternal(EConstraintFrame::Frame2);
  if (IsValid(component2_) && component2_->BodyInstance.WeldParent != nullptr
      && component2_->BodyInstance.WeldParent->OwnerComponent.Get() != nullptr) {
    component2_ = component2_->BodyInstance.WeldParent->OwnerComponent.Get();
    ComponentName2.ComponentName = component2_->GetFName();
    OverrideComponent2 = component2_;
    ConstraintInstance.ConstraintBone2 = NAME_None;
  }

  // Calculate the transform of anchor1 in component1's bone's frame
  float w_body_size1 = 0.0f;
  b1_anchor1_ = getScaleOne(GetComponentTransform());
  if (component1_ != nullptr) {
    b1_anchor1_ = UKismetMathLibrary::ComposeTransforms(b1_anchor1_, getScaleOne(
        component1_->GetSocketTransform(ConstraintInstance.ConstraintBone1)).Inverse());

    w_body_size1 = component1_->Bounds.BoxExtent.Size();
  }
  a1_bone1_ = b1_anchor1_.Inverse();

  // Calculate the transform of anchor2 in component2's bone's frame, and the location of bone2's
  // center of mass in anchor2's frame.
  float w_body_size2 = 0.0f;
  b2_anchor2_ = getScaleOne(GetComponentTransform());
  if (component2_ != nullptr) {
    b2_anchor2_ = UKismetMathLibrary::ComposeTransforms(b2_anchor2_, getScaleOne(
        component2_->GetSocketTransform(ConstraintInstance.ConstraintBone2)).Inverse());

    w_body_size2 = component2_->Bounds.BoxExtent.Size();
  }
  a2_bone2_ = b2_anchor2_.Inverse();

  // Size the visualizers based on object extents and anchor lengths.
  float w_anchor_length_1 = b1_anchor1_.GetLocation().Size();
  float w_anchor_length_2 = b2_anchor2_.GetLocation().Size();
  if (FMath::Abs(w_body_size1) < FLT_EPSILON) {
    w_viz_scale_ = w_body_size2;
  }
  else if (FMath::Abs(w_body_size2) < FLT_EPSILON) {
    w_viz_scale_ = w_body_size1;
  }
  else if (FMath::Abs(w_anchor_length_1) < FLT_EPSILON) {
    w_viz_scale_ = w_body_size1;
  }
  else if (FMath::Abs(w_anchor_length_2) < FLT_EPSILON) {
    w_viz_scale_ = w_body_size2;
  }
  else {
    float anchor_length_ratio = w_anchor_length_1 / (w_anchor_length_1 + w_anchor_length_2);
    w_viz_scale_ = FMath::Lerp(w_body_size1, w_body_size2, anchor_length_ratio);
  }
  w_viz_scale_ = FMath::Max(w_viz_scale_, VIS_SCALE_MIN);

  // Record constraint scale for use if ConstraintInstance.bScaleLinearLimits is true.
  w_constraint_scale_ = GetComponentScale();
  l_world_scale_ = FVector(1.0f / w_constraint_scale_.X, 1.0f / w_constraint_scale_.Y, 1.0f /
      w_constraint_scale_.Z);

  updateConstraint();
}
