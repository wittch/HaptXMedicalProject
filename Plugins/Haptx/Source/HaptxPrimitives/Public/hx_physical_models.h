// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Curves/CurveFloat.h>
#include "hx_physical_models.generated.h"

//! @brief The base class for all custom physical models used in @link UHxDofBehavior 
//! UHxDofBehaviors @endlink. 
//!
//! DO NOT INSTANTIATE THIS CLASS! Use a child class instead.
//!
//! To implement a custom model inherit from this class and override #getOutput().
//!
//! See the @ref section_unreal_hx_physical_model "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// The base class for all custom physical models used in UHxDofBehaviors. 
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxPhysicalModel : public UObject {
  GENERATED_BODY()

public:

  //! Default constructor.
  UHxPhysicalModel() {}

  //! @brief Get the signed magnitude of the output corresponding to @p input.
  //!
  //! Override to define a new model. 
  //!
  //! @param input The input to the model.
  //!
  //! @returns The signed magnitude of the output.

  // Get the signed magnitude of the output corresponding to input.
  virtual float getOutput(float input) const { return 0.0f; };
};

//! @brief A model that provides a constant restoring force toward the "0" position.
//!
//! This model is useful for providing a constant source of tension against a limit or another 
//! object, but otherwise tends to be unstable.
//!
//! See the @ref section_unreal_hx_physical_model "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A model that provides a constant restoring force toward the "0" position.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxConstantModel : public UHxPhysicalModel {
  GENERATED_BODY()

public:

  //! Default constructor.
  UHxConstantModel() : UHxPhysicalModel(), constant_(10.0f) {}

  //! Returns a value with a magnitude of #constant_ and sign opposite of @p input.
  //!
  //! @param input The input to the model.
  //!
  //! @returns A value with a magnitude of #constant_ and sign opposite of @p input.

  // Returns a value with a magnitude of "constant_" and sign opposite of "input".
  UFUNCTION(BlueprintCallable)
  virtual float getOutput(float input) const override { 
    return -1.0f * FMath::Sign(input) * constant_; 
  };

  //! @brief The magnitude of the value that gets returned by #getOutput().
  //!
  //! Units of Unreal force (or acceleration) on @link UHxLinearDof UHxLinearDofs @endlink and 
  //! Unreal torque (or angular acceleration) on @link UHxAngularDof UHxAngularDofs @endlink.

  // The magnitude of the value that gets returned by getOutput().
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float constant_;
};

//! @brief A model that provides a linearly scaled force toward the "0" position.
//!
//! This is a good general-purpose model as it tends to be stable with reasonable damping. It may
//! be thought of as the "default" model.
//!
//! See the @ref section_unreal_hx_physical_model "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A model that provides a linearly scaled force toward the "0" position.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxSpringModel : public UHxPhysicalModel {
  GENERATED_BODY()

public:

  //! Default constructor.
  UHxSpringModel() : UHxPhysicalModel(), stiffness_(1.0f) {}

  //! Returns a value with magnitude that scales linearly with @p input and #stiffness_, and
  //! sign opposite of input.
  //!
  //! @param input The input to the model.
  //!
  //! @returns A value with magnitude that scales linearly with @p input and #stiffness_, and
  //! sign opposite of input.

  // Returns a value with magnitude that scales linearly with input and stiffness, and sign 
  // opposite of input.
  UFUNCTION(BlueprintCallable)
  float getOutput(float input) const override { return -stiffness_ * input;  };

  //! @brief The stiffness of the spring.
  //!
  //! Units of Unreal force (or acceleration) per [cm] on @link UHxLinearDof UHxLinearDofs @endlink
  //! and Unreal torque (or angular acceleration) per [deg] on 
  //! @link UHxAngularDof UHxAngularDofs @endlink.

  // The stiffness of the spring.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float stiffness_;

};

//! @brief A model that maps outputs (scaled and offset) to inputs (scaled and offset) as defined 
//! by an arbitrary UCurveFloat.
//!
//! This is a very flexible model that can be used to describe almost any continuous function 
//! (in theory). In practice its usefulness is constrained by the editor tool used to create 
//! UCurveFloats.
//!
//! When designing curves that you intend to re-use, it is highly recommended that the functional
//! region be described across -1 and 1 on both axes. This makes scale parameters very intuitive to
//! use.
//!
//! See the @ref section_unreal_hx_physical_model "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A model that maps outputs (scaled and offset) to inputs (scaled and offset) as defined by an 
// arbitrary UCurveFloat.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxCurveModel : public UHxPhysicalModel {
  GENERATED_BODY()

public:

  //! Default constructor.
  UHxCurveModel() : curve_(nullptr), input_scale_(1.0f), input_offset_(0.0f), 
      output_scale_(1.0f), output_offset_(0.0f) {}

  //! @brief Returns the output value of #curve_ (scaled and offset) matching @p input
  //! (scaled and offset).
  //!
  //! It is 'y' in: y = a * f(bx + c) + d. Units of Unreal force (or acceleration) on 
  //! @link UHxLinearDof UHxLinearDofs @endlink and Unreal torque (or angular acceleration) on 
  //! @link UHxAngularDof UHxAngularDofs @endlink.
  //!
  //! @param input The input to the model.
  //!
  //! @returns The output value of #curve_ (scaled and offset) matching @p input 
  //! (scaled and offset).

  // Returns the output value of curve (scaled and offset) matching input (scaled and offset).
  UFUNCTION(BlueprintCallable)
  float getOutput(float input) const override {
    if (curve_ != nullptr) {
      return output_offset_ + output_scale_ * 
          (curve_->GetFloatValue(input_offset_ + input_scale_ * input));
    }
    else {
      return 0.0f;
    }
  }

  //! @brief The curve that defines this model.
  //!
  //! It is 'f(x)' in: y = a * f(bx + c) + d. The output is in units of Unreal force 
  //! (or acceleration) on @link UHxLinearDof UHxLinearDofs @endlink and Unreal torque 
  //! (or angular acceleration) on @link UHxAngularDof UHxAngularDofs @endlink.

  // The curve that defines this model. 
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  UCurveFloat* curve_;

  //! @brief The amount by which to scale the input to the curve.
  //!
  //! It is 'b' in: y = a * f(bx + c) + d. The units of 'x' are [cm] on 
  //! @link UHxLinearDof UHxLinearDofs @endlink and [deg] on 
  //! @link UHxAngularDof UHxAngularDofs @endlink.

  // The amount by which to scale the input to the curve.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float input_scale_;

  //! @brief The amount by which to offset the input to the curve. 
  //!
  //! It is 'c' in: y = a * f(bx + c) + d. Units of [cm] on 
  //! @link UHxLinearDof UHxLinearDofs @endlink and [deg] on 
  //! @link UHxAngularDof UHxAngularDofs @endlink. 

  // The amount by which to offset the input to the curve
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float input_offset_;

  //! @brief The amount by which to scale the output of the curve.
  //!
  //! It is 'a' in: y = a * f(bx + c) + d.

  // The amount by which to scale the output of the curve.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float output_scale_;

  //! @brief The amount by which to offset the output of the curve.
  //!
  //! It is 'd' in: y = a * f(bx + c) + d. Units of Unreal force (or acceleration) on 
  //! @link UHxLinearDof UHxLinearDofs @endlink and Unreal torque (or angular acceleration) on 
  //! @link UHxAngularDof UHxAngularDofs @endlink. 

  // The amount by which to offset the output of the curve.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  float output_offset_;

};
