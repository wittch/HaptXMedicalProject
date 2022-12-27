// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Curves/CurveFloat.h>
#include <Haptx/Public/hx_direct_effect_component.h>
#include "hx_curve_direct_effect_component.generated.h"

//!  An implementation of UHxDirectEffectComponent that defines the Haptic Effect using a
//! provided UCurveFloat.
//!
//! @ingroup group_unreal_plugin

// An implementation of UHxDirectEffectComponent that defines the Haptic Effect using a provided
// UCurveFloat.
UCLASS( ClassGroup=(Haptx), meta = (BlueprintSpawnableComponent), HideCategories = (
    ComponentReplication, Cooking, Physics, LOD, Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxCurveDirectEffectComponent : public UHxDirectEffectComponent {
  GENERATED_BODY()

public:
  //! Get the value of #input_time_scale_.
  //!
  //! @returns The value of #input_time_scale_.

  // Get the value of Input Time Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  float getInputTimeScale() const;

  //! Set the value of #input_time_scale_.
  //!
  //! @param time_scale The new value for #input_time_scale_.

  // Set the value of Input Time Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setInputTimeScale(float time_scale);

  //! Get the value of #displacement_curve_.
  //!
  //! @returns The value of #displacement_curve_.

  // Get the value of Displacement Curve.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  UCurveFloat* getDisplacementCurve() const;

  //! Set the value of #displacement_curve_.
  //!
  //! @param displacement_curve The new value for #displacement_curve_.

  // Set the value of Displacement Curve.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setDisplacementCurve(UCurveFloat* displacement_curve);

  //! Get the value of #output_displacement_scale_.
  //!
  //! @returns The value of #output_displacement_scale_.

  // Get the value of Output Force Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  float getOutputDisplacementScale() const;

  //! Set the value of #output_displacement_scale_.
  //!
  //! @param output_displacement_scale The new value for #output_displacement_scale_.

  // Set the value of Output Force Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setOutputDisplacementScale(float output_displacement_scale);

protected:
  //! Sets default values for this component's properties.

  UHxCurveDirectEffectComponent();

  //! Called when the game starts.

  virtual void BeginPlay() override;

  //! Returns #output_displacement_scale_ * #displacement_curve_(#input_time_scale_ * t).

  float getDisplacementM(
      const HaptxApi::DirectEffect::DirectInfo& direct_info) const override;

  //! Time values get multiplied by this value before they're evaluated in the curve.

  // Time values get multiplied by this value before they're evaluated in the curve.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  float input_time_scale_;

  //! The displacement curve [cm] that defines this effect. Accepts time [s] as an input.

  // The displacement curve [cm] that defines this effect. Accepts time [s] as an input.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  UCurveFloat* displacement_curve_;

  //! Displacement curve output values get multiplied by this value before they're applied
  //! as displacements.

  // Displacement curve output values get multiplied by this value before they're applied as
  // displacements.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  float output_displacement_scale_;

private:
  //! Updates values internally so that scaling time works as expected.

  void updateDuration();

  //! Updates private curve extrema.

  void updateDisplacementCurveExtrema();

  //! The smallest time value present in the curve.

  float min_time_s_;

  //! The largest time value present in the curve.

  float max_time_s_;
};
