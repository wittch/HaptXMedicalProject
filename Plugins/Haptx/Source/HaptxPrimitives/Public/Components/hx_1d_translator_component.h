// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/Components/hx_1d_constraint_component.h>
#include "hx_1d_translator_component.generated.h"

//! @brief Uses features that can be added to @link UHxConstraintComponent UHxConstraintComponents 
//! @endlink that only need to operate in one translational degree of freedom.
//!
//! See the @ref section_unreal_hx_1d_components "Unreal Haptic Primitive Guide" for a high level 
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// Uses features that can be added to UHxConstraintComponents that only need to operate in one 
// translational degree of freedom.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew,
    meta = (BlueprintSpawnableComponent),
    HideCategories=(Tick, Tags, Collision, Rendering, HxConstraint))
class HAPTXPRIMITIVES_API UHx1DTranslatorComponent : public UHx1DConstraintComponent {
  GENERATED_BODY()

public:

  //! Default constructor.
  //!
  //! @param object_initializer For registration with Unreal.
  UHx1DTranslatorComponent(const FObjectInitializer& object_initializer);

protected:

  virtual void configureConstraint(FConstraintInstance& in_constraint_instance) override;
};
