// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_wave_object_effect_component.h>

UHxWaveObjectEffectComponent::UHxWaveObjectEffectComponent() : frequency_Hz_(10.f),
    amplitude_cN_(10000.f), invert_oscillation_(false), input_phase_offset_deg_(0.f),
    output_force_offset_cN_(0.f), duration_s_(1.f) {
  is_looping_ = true;
}

void UHxWaveObjectEffectComponent::BeginPlay() {
  Super::BeginPlay();
  updateDuration();
}

float UHxWaveObjectEffectComponent::getForceN(
    const HaptxApi::ObjectEffect::ContactInfo& contact_info) const {
  float wave_output_cN = evaluateSineWave(contact_info.time_s, frequency_Hz_,
      input_phase_offset_deg_);
  wave_output_cN *= amplitude_cN_;
  if (invert_oscillation_) {
    wave_output_cN *= -1.f;
  }
  wave_output_cN += output_force_offset_cN_;

  return hxFromUnrealLength(-contact_info.object_normal.dot(contact_info.body_normal) *
      wave_output_cN);
}

void UHxWaveObjectEffectComponent::updateDuration() {
  // If we can actually set the duration
  if (object_effect_ != nullptr) {
    float duration = 0.f;
    // If we can actually play the effect
    if (frequency_Hz_ > 0.f) {
      // If we intend for the effect to loop cleanly
      if (is_looping_) {
        duration = 1 / frequency_Hz_;
      }
      // If we intend to use duration_s_ and only play the effect once
      else if (duration_s_ > 0.f) {
        duration = duration_s_;
      }
    }
    object_effect_->setDurationS(duration);
  }
}
