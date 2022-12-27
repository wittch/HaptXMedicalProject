// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_spatial_effect_component.h>
#include <Haptx/Public/hx_core_actor.h>

UHxSpatialEffectComponent::UHxSpatialEffectComponent() : spatial_effect_() {}

UHxBoundingVolume* UHxSpatialEffectComponent::getBoundingVolume() const {
  return bounding_volume_;
}

void UHxSpatialEffectComponent::setBoundingVolume(UHxBoundingVolume* bounding_volume) {
  bounding_volume_ = bounding_volume;
  if (spatial_effect_ != nullptr) {
    spatial_effect_->setBoundingVolume(
        IsValid(bounding_volume_) ? bounding_volume_->getBoundingVolume() : nullptr);
  }
}

void UHxSpatialEffectComponent::BeginPlay() {
  spatial_effect_ = std::make_shared<HxUnrealSpatialEffect>(this);
  callbacks_ = std::make_shared<WeldedComponentCallbacks>(this, NAME_None);
  spatial_effect_->setCallbacks(callbacks_);
  if (IsValid(bounding_volume_)) {
    spatial_effect_->setBoundingVolume(bounding_volume_->getBoundingVolume());
  }

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (IsValid(core)) {
    core->getContactInterpreter().registerSpatialEffect(spatial_effect_);
  } else {
    UE_LOG(HaptX, Error, TEXT(
        "UHxSpatialEffectComponent::BeginPlay(): Failed to get handle to core."))
  }

  Super::BeginPlay();
}

void UHxSpatialEffectComponent::EndPlay(EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (IsValid(core)) {
    if (spatial_effect_ != nullptr) {
      core->getContactInterpreter().unregisterSpatialEffect(spatial_effect_->getId());
    } else {
      UE_LOG(HaptX, Error, TEXT(
          "UHxSpatialEffectComponent::EndPlay(): Null internal effect."))
    }
  } else {
    UE_LOG(HaptX, Error, TEXT(
        "UHxSpatialEffectComponent::EndPlay(): Failed to get handle to core."))
  }
}

UHxSpatialEffectComponent::HxUnrealSpatialEffect::HxUnrealSpatialEffect(
    UHxSpatialEffectComponent* spatial_effect) : spatial_effect_(spatial_effect) {}

float UHxSpatialEffectComponent::HxUnrealSpatialEffect::getForceN(
    const HaptxApi::SpatialEffect::SpatialInfo& spatial_info) const {
  if (spatial_effect_.IsValid()) {
    return spatial_effect_->getForceN(spatial_info);
  }
  else {
    return 0.0f;
  }
}
