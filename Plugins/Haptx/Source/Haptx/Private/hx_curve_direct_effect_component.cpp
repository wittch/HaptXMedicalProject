// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_curve_direct_effect_component.h>
#include <Haptx/Private/haptx_shared.h>

UHxCurveDirectEffectComponent::UHxCurveDirectEffectComponent() : input_time_scale_(1.0f), 
    displacement_curve_(nullptr), output_displacement_scale_(1.0f), min_time_s_(0.0f), 
    max_time_s_(0.0f) {}

float UHxCurveDirectEffectComponent::getInputTimeScale() const {
  return input_time_scale_;
}

void UHxCurveDirectEffectComponent::setInputTimeScale(float time_scale) {
  input_time_scale_ = time_scale;
  updateDuration();
}

UCurveFloat* UHxCurveDirectEffectComponent::getDisplacementCurve() const {
  return displacement_curve_;
}

void UHxCurveDirectEffectComponent::setDisplacementCurve(UCurveFloat* displacement_curve) {
  displacement_curve_ = displacement_curve;
  updateDisplacementCurveExtrema();
}

float UHxCurveDirectEffectComponent::getOutputDisplacementScale() const {
  return output_displacement_scale_;
}

void UHxCurveDirectEffectComponent::setOutputDisplacementScale(float output_displacement_scale) {
  output_displacement_scale_ = output_displacement_scale;
}

void UHxCurveDirectEffectComponent::BeginPlay() {
  Super::BeginPlay();
  updateDisplacementCurveExtrema();
}

float UHxCurveDirectEffectComponent::getDisplacementM(
    const HaptxApi::DirectEffect::DirectInfo& direct_info) const {
  if (displacement_curve_ == nullptr) {
    return 0.0f;
  }
  else {
    float curve_output_cm = displacement_curve_->GetFloatValue(
        (input_time_scale_ > 0.0f ? min_time_s_ : max_time_s_) + 
        input_time_scale_ * direct_info.time_s);

    return hxFromUnrealLength(output_displacement_scale_ * curve_output_cm);
  }
}

void UHxCurveDirectEffectComponent::updateDuration() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->setDurationS(input_time_scale_ != 0.0f ?
        (max_time_s_ - min_time_s_) / FMath::Abs(input_time_scale_) : 0.0f);
  }
}

void UHxCurveDirectEffectComponent::updateDisplacementCurveExtrema() {
  if (!IsValid(displacement_curve_) || displacement_curve_->FloatCurve.Keys.Num() == 0) {
    min_time_s_ = 0.0f;
    max_time_s_ = 0.0f;
  }
  else {
    min_time_s_ = displacement_curve_->FloatCurve.Keys[0].Time;
    max_time_s_ = displacement_curve_->FloatCurve.Keys[0].Time;
    for (FRichCurveKey& key_value : displacement_curve_->FloatCurve.Keys) {
      if (key_value.Time < min_time_s_) {
        min_time_s_ = key_value.Time;
      }
      else if (key_value.Time > max_time_s_) {
        max_time_s_ = key_value.Time;
      }
    }
  }
  updateDuration();
}
