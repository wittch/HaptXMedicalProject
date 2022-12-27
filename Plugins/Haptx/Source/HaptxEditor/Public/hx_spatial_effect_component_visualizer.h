// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Editor/UnrealEd/Public/ComponentVisualizer.h>
#include <Runtime/Core/Public/CoreMinimal.h>
#include <Haptx/Public/hx_spatial_effect_component.h>

class FPrimitiveDrawInterface;
class FSceneView;

class FHxSpatialEffectComponentVisualizer : public FComponentVisualizer {
public:
  virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view,
    FPrimitiveDrawInterface* pdi) override;
};
