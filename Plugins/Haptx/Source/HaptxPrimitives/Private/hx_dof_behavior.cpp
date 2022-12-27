// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/hx_dof_behavior.h>
#include <HaptxPrimitives/Public/hx_dof.h>
#include <HaptxPrimitives/Public/hx_physical_models.h>

UHxDofBehavior::UHxDofBehavior(const FObjectInitializer& object_initializer) :
    Super(object_initializer), acceleration_(false), enabled_(true), visualize_(false),
    name_(TEXT("Main")) {}

float UHxDofBehavior::getForceTorque(float position) {
  // Override me in child classes!
  return 0.0f;
}

bool UHxDofBehavior::tryGetTarget(float& out_target) {
  // Override me in child classes!
  out_target = 0.0f;
  return false;
}

void UHxDofBehavior::initialize() {
  // Override me in child classes!
}

UHxDofDefaultBehavior::UHxDofDefaultBehavior(const FObjectInitializer& object_initializer) :
  Super(object_initializer) {}

float UHxDofDefaultBehavior::getForceTorque(float position) {
  if (model_ == nullptr) {
    return 0.0f;
  }
  return model_->getOutput(position);
}

bool UHxDofDefaultBehavior::tryGetTarget(float& out_target) {
  out_target = 0.0f;
  if (model_ != nullptr && model_->GetClass() == UHxCurveModel::StaticClass()) {
    return false;
  }
  return true;
}

UHxDofTargetPositionBehavior::UHxDofTargetPositionBehavior(
    const FObjectInitializer& object_initializer) : Super(object_initializer),
    target_position_(0.0f) {}

float UHxDofTargetPositionBehavior::getForceTorque(float position) {
  if (model_ == nullptr) {
    return 0.0f;
  }
  return model_->getOutput(position - target_position_);
}

bool UHxDofTargetPositionBehavior::tryGetTarget(float& out_target) {
  out_target = target_position_;
  return true;
}
