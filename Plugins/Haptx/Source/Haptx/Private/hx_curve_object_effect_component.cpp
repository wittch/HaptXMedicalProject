// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_curve_object_effect_component.h>

UHxCurveObjectEffectComponent::UHxCurveObjectEffectComponent() : input_time_scale_(1.0f), 
    force_curve_(nullptr), output_force_scale_(1.0f), min_time_s_(0.0f), max_time_s_(0.0f) {}

void UHxCurveObjectEffectComponent::BeginPlay() {
  Super::BeginPlay();
  updateForceCurveExtrema();
}

float UHxCurveObjectEffectComponent::getForceN(
    const HaptxApi::ObjectEffect::ContactInfo& contact_info) const {
  if (force_curve_ == nullptr) {
    return 0.0f;
  }
  else {
    float curve_output_cn = force_curve_->GetFloatValue(
        (input_time_scale_ > 0.0f ? min_time_s_ : max_time_s_) + 
        input_time_scale_ * contact_info.time_s);

    return hxFromUnrealLength(-contact_info.object_normal.dot(contact_info.body_normal) *
        output_force_scale_ * curve_output_cn);
  }
}

void UHxCurveObjectEffectComponent::updateDuration() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->setDurationS(input_time_scale_ != 0.0f ?
        (max_time_s_ - min_time_s_) / FMath::Abs(input_time_scale_) : 0.0f);
  }
}

void UHxCurveObjectEffectComponent::updateForceCurveExtrema() {
  if (!IsValid(force_curve_) || force_curve_->FloatCurve.Keys.Num() == 0) {
    min_time_s_ = 0.0f;
    max_time_s_ = 0.0f;
  }
  else {
    min_time_s_ = force_curve_->FloatCurve.Keys[0].Time;
    max_time_s_ = force_curve_->FloatCurve.Keys[0].Time;
    for (FRichCurveKey& key_value : force_curve_->FloatCurve.Keys) {
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

float UHxCurveObjectEffectComponent::getInputTimeScale() const {
  return input_time_scale_;
}

void UHxCurveObjectEffectComponent::setInputTimeScale(float time_scale) {
  input_time_scale_ = time_scale;
  updateDuration();
}

UCurveFloat* UHxCurveObjectEffectComponent::getForceCurve() const {
  return force_curve_;
}

void UHxCurveObjectEffectComponent::setForceCurve(UCurveFloat* force_curve) {
  force_curve_ = force_curve;
  updateForceCurveExtrema();
}

float UHxCurveObjectEffectComponent::getOutputForceScale() const {
  return output_force_scale_;
}

void UHxCurveObjectEffectComponent::setOutputForceScale(float output_force_scale) {
  output_force_scale_ = output_force_scale;
}
