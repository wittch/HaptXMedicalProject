// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_wave_direct_effect_component.h>

UHxWaveDirectEffectComponent::UHxWaveDirectEffectComponent() : frequency_Hz_(10.f),
    amplitude_cm_(0.5f), invert_oscillation_(false), input_phase_offset_deg_(0.f),
    output_displacement_offset_cm_(0.f), duration_s_(1.f) {
  is_looping_ = true;
}

void UHxWaveDirectEffectComponent::BeginPlay() {
  Super::BeginPlay();
  updateDuration();
}

float UHxWaveDirectEffectComponent::getDisplacementM(
    const HaptxApi::DirectEffect::DirectInfo& direct_info) const {
  float wave_output_cm = evaluateSineWave(direct_info.time_s, frequency_Hz_,
      input_phase_offset_deg_);
  wave_output_cm *= amplitude_cm_;
  if (invert_oscillation_) {
    wave_output_cm *= -1.f;
  }
  wave_output_cm += output_displacement_offset_cm_;

  return hxFromUnrealLength(wave_output_cm);
}

void UHxWaveDirectEffectComponent::updateDuration() {
  // If we can actually set the duration
  if (direct_effect_ != nullptr) {
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
    direct_effect_->setDurationS(duration);
  }
}
