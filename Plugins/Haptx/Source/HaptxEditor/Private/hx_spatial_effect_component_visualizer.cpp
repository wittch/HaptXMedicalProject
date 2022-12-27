// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxEditor/Public/hx_spatial_effect_component_visualizer.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Public/SceneManagement.h>

static const FColor EFFECT_COLOR(0, 170, 188);

void FHxSpatialEffectComponentVisualizer::DrawVisualization(const UActorComponent* component, 
    const FSceneView* view, FPrimitiveDrawInterface* pdi) {
  const UHxSpatialEffectComponent* spatial_effect = 
      Cast<const UHxSpatialEffectComponent>(component);
  if (IsValid(spatial_effect)) {
    UHxBoundingVolume* bounding_volume = spatial_effect->getBoundingVolume();
    if (IsValid(bounding_volume)) {
      UClass* bounding_volume_class = bounding_volume->GetClass();
      if (bounding_volume_class == UHxSphereBoundingVolume::StaticClass()) {
        UHxSphereBoundingVolume* sphere_volume = Cast<UHxSphereBoundingVolume>(bounding_volume);
        if (sphere_volume->getRadiusCm() > 0.0f) {
          FTransform center_transform(sphere_volume->getCenterPositionCm());
          DrawWireSphere(pdi, UKismetMathLibrary::ComposeTransforms(center_transform,
              spatial_effect->GetComponentTransform()), EFFECT_COLOR,
              sphere_volume->getRadiusCm(), 20, 0);
        }
      }
      else if (bounding_volume_class == UHxBoxBoundingVolume::StaticClass()) {
        UHxBoxBoundingVolume* box_volume = Cast<UHxBoxBoundingVolume>(bounding_volume);
        if (IsValid(box_volume)) {
          FBox box(box_volume->getMinimaCm(), box_volume->getMaximaCm());
          DrawWireBox(pdi, spatial_effect->GetComponentTransform().ToMatrixWithScale(), box,
              EFFECT_COLOR, 0);
        }
      }
    }
  }
}
