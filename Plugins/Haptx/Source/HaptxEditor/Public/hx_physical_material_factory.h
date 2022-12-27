// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Editor/UnrealEd/Classes/Factories/Factory.h>
#include <Haptx/Public/hx_physical_material.h>
#include "hx_physical_material_factory.generated.h"

//! A class that allows @link UHxPhysicalMaterial UHxPhysicalMaterials @endlink to be 
//! created in the editor. See @ref page_unreal_plugin_summary.
UCLASS(HideCategories = Object)
class UHxPhysicalMaterialFactory : public UFactory {
  GENERATED_UCLASS_BODY()

    //! Create a new UHaptxPhysicalMaterial.
    //!
    //! @returns The new UHaptxPhysicalMaterial.
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
