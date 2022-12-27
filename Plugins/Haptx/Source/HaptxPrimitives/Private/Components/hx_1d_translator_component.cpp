// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/Components/hx_1d_translator_component.h>
#include <HaptxPrimitives/Public/haptx_primitives_shared.h>
#include <HaptxPrimitives/Public/ihaptx_primitives.h>

UHx1DTranslatorComponent::UHx1DTranslatorComponent(
    const FObjectInitializer &object_initializer) : Super(object_initializer) {
  operating_domain_ = EDofDomain::LINEAR;
  lower_limit_ = 0.0f;
  upper_limit_ = 100.0f;
  damping_ = 10.f;
}

void UHx1DTranslatorComponent::configureConstraint(
    FConstraintInstance& in_constraint_instance) {
  Super::configureConstraint(in_constraint_instance);

  if (limit_motion_ && upper_limit_ > lower_limit_) {
    float mid_point = (lower_limit_ + upper_limit_) / 2.0f;
    FVector direction = directionOfDegreeOfFreedom(makeDegreeOfFreedomFromDomainAndAxis(
        operating_domain_, operating_axis_));
    linear_limits_offset_ = mid_point * direction;

    float limit = (upper_limit_ - lower_limit_) / 2.0f;
    in_constraint_instance.ProfileInstance.LinearLimit.Limit = limit;
    switch (operating_axis_) {
      case EDofAxis::X:
        in_constraint_instance.ProfileInstance.LinearLimit.XMotion = LCM_Limited;
        break;
      case EDofAxis::Y:
        in_constraint_instance.ProfileInstance.LinearLimit.YMotion = LCM_Limited;
        break;
      case EDofAxis::Z:
        in_constraint_instance.ProfileInstance.LinearLimit.ZMotion = LCM_Limited;
        break;
    }
  }
}
