// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxEditor/Public/ihaptx_editor.h>
#include <Editor/UnrealEd/Classes/Editor/UnrealEdEngine.h>
#include <Editor/UnrealEd/Public/UnrealEdGlobals.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>
#include <HaptxEditor/Public/asset_type_actions_hx_physical_material.h>
#include <HaptxEditor/Public/hx_spatial_effect_component_visualizer.h>

class FHaptxEditor : public IHaptxEditor {
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FHaptxEditor, HaptxEditor )

void FHaptxEditor::StartupModule() {
  IHaptxEditor::StartupModule();

  // Register the UHxPhysicalMaterial action with the FAssetToolsModule.
  FAssetToolsModule& assetToolsModule = 
      FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
  assetToolsModule.Get().RegisterAssetTypeActions(
      MakeShareable(new FAssetTypeActions_HxPhysicalMaterial));

  // Register the FHxSpatialEffectComponentVisualizer visualizer with the engine.
  if (GUnrealEd != nullptr) {
    TSharedPtr<FComponentVisualizer> visualizer = 
        MakeShareable(new FHxSpatialEffectComponentVisualizer());

    if (visualizer.IsValid()) {
      GUnrealEd->RegisterComponentVisualizer(
          UHxSpatialEffectComponent::StaticClass()->GetFName(), visualizer);
      visualizer->OnRegister();
    }
  }
}

void FHaptxEditor::ShutdownModule() {
  IHaptxEditor::ShutdownModule();

  // Unregister the FHxSpatialEffectComponentVisualizer visualizer with the engine.
  if (GUnrealEd != nullptr) {
    GUnrealEd->UnregisterComponentVisualizer(UHxSpatialEffectComponent::StaticClass()->GetFName());
  }
}
