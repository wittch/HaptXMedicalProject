// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_bounding_volumes.h>
#include <Haptx/Private/haptx_shared.h>

std::shared_ptr<const HaptxApi::BoundingVolume> UHxBoundingVolume::getBoundingVolume() const {
  return bounding_volume_;
}

UHxSphereBoundingVolume* UHxSphereBoundingVolume::newSphereBoundingVolume(UObject* world_context,
    float radius_cm, FVector center_position_cm) {
  UHxSphereBoundingVolume* sphere = NewObject<UHxSphereBoundingVolume>(world_context);
  sphere->setRadiusCm(radius_cm);
  sphere->setCenterPosition(center_position_cm);
  return sphere;
}

UHxSphereBoundingVolume::UHxSphereBoundingVolume() : radius_cm_(10.0f), 
    center_position_cm_(FVector::ZeroVector) {
  bounding_volume_ = std::make_shared<HaptxApi::SphereBoundingVolume>(
      hxFromUnrealLength(radius_cm_), hxFromUnrealLength(center_position_cm_));
}

void UHxSphereBoundingVolume::PostLoad() {
  Super::PostLoad();
  setRadiusCm(radius_cm_);
  setCenterPosition(center_position_cm_);
}

float UHxSphereBoundingVolume::getRadiusCm() const {
  return radius_cm_;
}

void UHxSphereBoundingVolume::setRadiusCm(float radius_cm) {
  radius_cm_ = radius_cm;
  if (bounding_volume_ != nullptr) {
    std::static_pointer_cast<HaptxApi::SphereBoundingVolume>(bounding_volume_)->setRadiusM(
        hxFromUnrealLength(radius_cm_));
  }
  else {
    UE_LOG(HaptX, Error, TEXT(
      "UHxSphereBoundingVolume::setRadiusCm(): Underlying bounding volume is null."))
  }
}

FVector UHxSphereBoundingVolume::getCenterPositionCm() const {
  return center_position_cm_;
};

void UHxSphereBoundingVolume::setCenterPosition(FVector center_position_cm) {
  center_position_cm_ = center_position_cm;
  if (bounding_volume_ != nullptr) {
    std::static_pointer_cast<HaptxApi::SphereBoundingVolume>(bounding_volume_)->setCenterPositionM(
        hxFromUnrealLength(center_position_cm_));
  }
  else {
    UE_LOG(HaptX, Error, TEXT(
        "UHxSphereBoundingVolume::setCenterPositionCm(): Underlying bounding volume is null."))
  }
}

UHxBoxBoundingVolume* UHxBoxBoundingVolume::newBoxBoundingVolume(UObject* world_context,
    FVector minima_cm, FVector maxima_cm) {
  UHxBoxBoundingVolume* box = NewObject<UHxBoxBoundingVolume>(world_context);
  box->setExtremaCm(minima_cm, maxima_cm);
  return box;
}

UHxBoxBoundingVolume::UHxBoxBoundingVolume() : minima_cm_(-10.0f, -10.0f, -10.0f), 
    maxima_cm_(10.0f, 10.0f, 10.0f) {
  HaptxApi::Vector3D minima_m = hxFromUnrealLength(minima_cm_);
  HaptxApi::Vector3D maxima_m = hxFromUnrealLength(maxima_cm_);
  bounding_volume_ = std::make_shared<HaptxApi::BoxBoundingVolume>(minima_m.x_, maxima_m.x_, 
      minima_m.y_, maxima_m.y_, minima_m.z_, maxima_m.z_);
}

void UHxBoxBoundingVolume::PostLoad() {
  Super::PostLoad();
  setExtremaCm(minima_cm_, maxima_cm_);
}

FVector UHxBoxBoundingVolume::getMinimaCm() const {
  return minima_cm_;
}

FVector UHxBoxBoundingVolume::getMaximaCm() const {
  return maxima_cm_;
}

void UHxBoxBoundingVolume::setExtremaCm(const FVector& minima_cm, const FVector& maxima_cm) {
  minima_cm_ = minima_cm;
  maxima_cm_ = maxima_cm;
  if (bounding_volume_ != nullptr) {
    HaptxApi::Vector3D minima_m = hxFromUnrealLength(minima_cm_);
    HaptxApi::Vector3D maxima_m = hxFromUnrealLength(maxima_cm_);
    std::static_pointer_cast<HaptxApi::BoxBoundingVolume>(bounding_volume_)->setExtrema(
        minima_m.x_, maxima_m.x_, minima_m.y_, maxima_m.y_, minima_m.z_, maxima_m.z_);
  }
  else {
    UE_LOG(HaptX, Error, TEXT(
        "UHxBoxBoundingVolume::setExtremaCm(): Underlying bounding volume is null."))
  }
}
