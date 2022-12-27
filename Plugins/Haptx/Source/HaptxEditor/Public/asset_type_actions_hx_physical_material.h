// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Developer/AssetTools/Public/AssetTypeActions_Base.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/hx_physical_material.h>

//! A class that allows @link UHxPhysicalMaterial UHxPhysicalMaterials @endlink to be 
//! created in the editor. See @ref page_unreal_plugin_summary.
class FAssetTypeActions_HxPhysicalMaterial : public FAssetTypeActions_Base {
public:
  //! The name to display.
  //!
  //! @returns The name to display.
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions",
      "AssetTypeActions_HxPhysicalMaterial", "HaptX Physical Material"); }

  //! The color to display.
  //!
  //! @returns The color to display.
	virtual FColor GetTypeColor() const override { return HAPTX_TEAL; }

  //! The class to create.
  //!
  //! @returns The class to create.
  virtual UClass* GetSupportedClass() const override { return UHxPhysicalMaterial::StaticClass(); }

  //! The category to place the create option in.
  //!
  //! @returns The category to place the create option in.
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Physics; }
};
