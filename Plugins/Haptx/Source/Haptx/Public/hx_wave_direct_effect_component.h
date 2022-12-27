// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/hx_direct_effect_component.h>
#include "hx_wave_direct_effect_component.generated.h"

//! An implementation of UHxDirectEffectComponent that defines the Haptic Effect using a
//! simple sine wave generator. The waveform starts at 0 then increases by default.
//!
//! @ingroup group_unreal_plugin

// An implementation of UHxDirectEffectComponent that defines the Haptic Effect using a simple sine
// wave generator. The waveform starts at 0 then increases by default.
UCLASS( ClassGroup=(Haptx), meta = (BlueprintSpawnableComponent), HideCategories = (
    ComponentReplication, Cooking, Physics, LOD, Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxWaveDirectEffectComponent : public UHxDirectEffectComponent {
  GENERATED_BODY()

public:
  //! @brief The frequency that the signal repeats itself at [Hz].
  //!
  //! Changing this while the effect is playing can cause discontinuities.

  // The frequency that the signal repeats itself at [Hz]. Changing this while the effect is playing
  // can cause discontinuities.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Haptic Effects", meta=(ClampMin=0.f,
      DisplayName="Frequency [Hz]"))
  float frequency_Hz_;

  //! The amplitude of the wave signal [cm].

  // The amplitude of the wave signal [cm].
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Haptic Effects", meta=(ClampMin=0.f,
      DisplayName="Amplitude [cm]"))
  float amplitude_cm_;

  //! Whether to invert (negate) the oscillation from the wave signal BEFORE adding it to
  //! #output_displacement_offset_cm_.

  // Whether to invert (negate) the oscillation from the wave signal BEFORE adding it to the output
  // displacement offset.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Haptic Effects")
  bool invert_oscillation_;

  //! @brief The phase that the wave starts at [deg], then continues from there.
  //!
  //! For example: an offset of 90 degrees turns a sine wave into a cosine wave.

  // The phase that the wave starts at [deg], then continues from there.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Haptic Effects", meta=(
      DisplayName="Input Phase Offset [deg]"))
  float input_phase_offset_deg_;

  //! A displacement amount [cm] to always add to the output (essentialy a DC offset for the
  //! signal).

  // A displacement amount [cm] to always add to the output (essentialy a DC offset for the signal).
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Haptic Effects", meta=(
      DisplayName="Output Displacement Offset [cm]"))
  float output_displacement_offset_cm_;

protected:
  //! Sets default values for this component's properties.
  UHxWaveDirectEffectComponent();

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Differs from parent by outputting the output from our sine wave generator.
  float getDisplacementM(
      const HaptxApi::DirectEffect::DirectInfo& direct_info) const override;

  //! The duration [s] with which to play the sine wave if #is_looping_ is false.

  // The duration [s] with which to play the sine wave if "Is Looping" is false.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects", meta=(ClampMin=0.f,
      DisplayName="Duration [s]"))
  float duration_s_;

private:
  //! Updates duration internally.

  void updateDuration();
};
