// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Curves/CurveFloat.h>
#include <Haptx/Public/hx_spatial_effect_component.h>
#include "hx_curve_spatial_effect_component.generated.h"

//!  An implementation of UHxSpatialEffectComponent that defines the Haptic Effect using a
//! provided UCurveFloat.
//!
//! @ingroup group_unreal_plugin

// An implementation of UHxSpatialEffectComponent that defines the Haptic Effect using a provided
// UCurveFloat.
UCLASS( ClassGroup=(Haptx), meta = (BlueprintSpawnableComponent), HideCategories = (
    ComponentReplication, Cooking, Physics, LOD, Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxCurveSpatialEffectComponent : public UHxSpatialEffectComponent {
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

  //! Get the value of #force_curve_.
  //!
  //! @returns The value of #force_curve_.

  // Get the value of Force Curve.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  UCurveFloat* getForceCurve() const;

  //! Set the value of #force_curve_.
  //!
  //! @param force_curve The new value for #force_curve_.

  // Set the value of Force Curve.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setForceCurve(UCurveFloat* force_curve);

  //! Get the value of #output_force_scale_.
  //!
  //! @returns The value of #output_force_scale_.

  // Get the value of Output Force Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  float getOutputForceScale() const;

  //! Set the value of #output_force_scale_.
  //!
  //! @param output_force_scale The new value for #output_force_scale_.

  // Set the value of Output Force Scale.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setOutputForceScale(float output_force_scale);

protected:
  //! Sets default values for this component's properties.

  UHxCurveSpatialEffectComponent();

  //! Called when the game starts.

  virtual void BeginPlay() override;

  //! Returns #output_force_scale_ * #force_curve_(#input_time_scale_ * t).

  float getForceN(
      const HaptxApi::SpatialEffect::SpatialInfo& spatial_info) const override;

  //! Time values get multiplied by this value before they're evaluated in the curve.

  // Time values get multiplied by this value before they're evaluated in the curve.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  float input_time_scale_;

  //! The force curve [cN] that defines this effect. Accepts time [s] as an input.

  // The force curve [cN] that defines this effect. Accepts time [s] as an input.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  UCurveFloat* force_curve_;

  //! Force curve output values get multiplied by this value before they're applied as
  //! forces.

  // Force curve output values get multiplied by this value before they're applied as forces.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  float output_force_scale_;

private:
  //! Updates values internally so that scaling time works as expected.

  void updateDuration();

  //! Updates private curve extrema.

  void updateForceCurveExtrema();

  //! The smallest time value present in the curve.

  float min_time_s_;

  //! The largest time value present in the curve.

  float max_time_s_;
};
