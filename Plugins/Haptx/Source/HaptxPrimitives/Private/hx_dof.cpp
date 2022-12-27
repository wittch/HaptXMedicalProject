// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/hx_dof.h>

float FTransform1D::GetTranslation(const FTransform& local_transform, EDofAxis axis) {
  FVector direction = directionOfDegreeOfFreedom(
      makeDegreeOfFreedomFromDomainAndAxis(EDofDomain::LINEAR, axis));
  return FVector::DotProduct(local_transform.GetLocation(), direction);
}

float FTransform1D::GetRotation(const FTransform& local_transform, EDofAxis axis) {
  EDegreeOfFreedom degree_of_freedom =
      makeDegreeOfFreedomFromDomainAndAxis(EDofDomain::ANGULAR, axis);
  
  // Determine reference vectors.
  FVector direction = directionOfDegreeOfFreedom(degree_of_freedom);
  FVector ortho_first;
  FVector ortho_second;
  getOrthonormalDirectionsFromDegreeOfFreedom(degree_of_freedom, ortho_first, ortho_second);

  // Calculate the first, planar orthographic direction of local_transform.
  FQuat local_rotation = local_transform.GetRotation();
  FVector rot_ortho_first;
  FVector rot_ortho_second;
  getOrthonormalDirectionsFromDegreeOfFreedom(local_rotation, degree_of_freedom,
    rot_ortho_first, rot_ortho_second);
  FVector transform_first_ortho_planar = rot_ortho_first -
    FVector::DotProduct(rot_ortho_first, direction) * direction;  // Now planar.
  transform_first_ortho_planar.Normalize();

  // The rotation's magnitude is equal to the angle between the component of the transform's
  // first orthogonal vector that is in the plane formed by the degree of freedom's first and 
  // second orthogonal vectors, and the degree of freedom's first orthogonal vector. It's sign is 
  // determined by the sign of the dot product between the transform's planar vector the degree of 
  // freedom's second orthogonal vector.
  return
    RAD_TO_DEG * FMath::Sign(FVector::DotProduct(transform_first_ortho_planar, ortho_second))
    * FMath::Acos(FVector::DotProduct(transform_first_ortho_planar, ortho_first));
}

float FTransform1D::GetScale(const FTransform& local_transform, EDofAxis axis) {
  FVector direction = directionOfDegreeOfFreedom(
    makeDegreeOfFreedomFromDomainAndAxis(EDofDomain::LINEAR, axis));
  return FVector::DotProduct(local_transform.GetScale3D(), direction);
}

UHxDof::UHxDof(const FObjectInitializer& object_initializer) : Super(object_initializer),
    current_position_(0.f) {}

void UHxDof::initialize() {
  current_position_ = 0.0f;
  for (UHxDofBehavior* behavior : physical_behaviors_) {
    if (behavior != nullptr) {
      behavior->initialize();
    }
  }
  for (UHxStateFunction* function : state_functions_) {
    if (function != nullptr) {
      function->initialize();
    }
  }
}

void UHxDof::update(float position, bool teleport) {
  current_position_ = position;
}

void UHxDof::registerPhysicalBehavior(UHxDofBehavior* behavior) {
  if (behavior != nullptr) {
    physical_behaviors_.Add(behavior);
  }
}

void UHxDof::unregisterPhysicalBehavior(UHxDofBehavior* behavior) {
  if (behavior != nullptr) {
    physical_behaviors_.Remove(behavior);
  }
}

void UHxDof::registerStateFunction(UHxStateFunction* function) {
  if (function != nullptr) {
    state_functions_.Add(function);
  }
}

void UHxDof::unregisterStateFunction(UHxStateFunction* function) {
  if (function != nullptr) {
    state_functions_.Remove(function);
  }
}

UHxStateFunction* UHxDof::findStateFunctionByName(FName name) {
  for (UHxStateFunction* function : state_functions_) {
    if (function->name_ == name) {
      return function;
    }
  }
  return nullptr;
}

UHxDofBehavior* UHxDof::findPhysicalBehaviorByName(FName name) {
  for (UHxDofBehavior* behavior : physical_behaviors_) {
    if (behavior->name_ == name) {
      return behavior;
    }
  }
  return nullptr;
}

float UHxDof::getCurrentPosition() const {
  return current_position_;
}

bool UHxDof::shouldUpdate() const {
  return state_functions_.Num() > 0 || physical_behaviors_.Num() > 0 || force_update_;
}

UHxAngularDof::UHxAngularDof(const FObjectInitializer& object_initializer) :
    Super(object_initializer), track_multiple_revolutions_(true) {}

void UHxAngularDof::update(float position, bool teleport) {
  if (track_multiple_revolutions_ && !teleport) {
    TotalRotation previous_total_rot(current_position_);
    // Current rotation on axis in degrees
    int revolution = previous_total_rot.revolution_;

    // Check to see if the rotation has crossed onto another revolution
    if (fabsf(position) > 90 && fabsf(previous_total_rot.partial_angle_) > 90 &&
      (position * previous_total_rot.partial_angle_) < 0) {
      revolution += position < 0 ? 1 : -1;
    }
    // Save rotation so it can be checked against next frame
    current_position_ = TotalRotation(revolution, position).getTotalRotation();
  }
  else {
    current_position_ = position;
  }
}
