// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <memory>
#include <HaptxApi/bounding_volume.h>
#include "hx_bounding_volumes.generated.h"

//! @brief Wraps HaptxApi::BoundingVolume.
//!
//! DO NOT INSTANTIATE THIS CLASS! Use a child class instead.

// Wraps HaptxApi::BoundingVolume.
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTX_API UHxBoundingVolume : public UObject {
  GENERATED_BODY()

public:
  //! Default constructor.
  
  UHxBoundingVolume() : bounding_volume_() {}
  
  //! Get a pointer to the underlying bounding volume.
  //!
  //! @returns A pointer to the underlying bounding volume.

  std::shared_ptr<const HaptxApi::BoundingVolume> getBoundingVolume() const;
  
protected:
  
  //! The underlying bounding volume.
  //!
  //! Set by child classes.
  
  std::shared_ptr<HaptxApi::BoundingVolume> bounding_volume_;
};

//! Wraps HaptxApi::SphereBoundingVolume.

// Wraps HaptxApi::SphereBoundingVolume.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew)
class HAPTX_API UHxSphereBoundingVolume : public UHxBoundingVolume {
  GENERATED_BODY()

public:
  //! Blueprint constructor.
  //!
  //! @param world_context World context object.
  //! @param radius_cm The sphere radius [cm].
  //! @param center_position_cm The center position [cm].
  //! @returns A newly constructed UHxSphereBoundingVolume.

  UFUNCTION(BlueprintCallable, meta = (WorldContext = "world_context"))
  static UHxSphereBoundingVolume* newSphereBoundingVolume(UObject* world_context, 
      float radius_cm = 10.0f, FVector center_position_cm = FVector::ZeroVector);

  //! Default constructor.

  UHxSphereBoundingVolume();

  //! Do any object-specific cleanup required immediately after loading an object, and immediately
  //! after any undo/redo.

  virtual void PostLoad() override;
  
  //! Get the sphere radius [cm].
  //!
  //! @returns The sphere radius [cm].

  // Get the sphere radius [cm].
  UFUNCTION(BlueprintCallable)
  float getRadiusCm() const;

  //! Set the radius [cm].
  //!
  //! @param radius_cm The radius [cm] to use.

  // Set the radius [cm].
  UFUNCTION(BlueprintCallable)
  void setRadiusCm(float radius_cm);

  //! Get the center position [cm].
  //!
  //! @returns The center position [cm].

  // The center position [cm].
  UFUNCTION(BlueprintCallable)
  FVector getCenterPositionCm() const;

  //! Set the center position [cm].
  //!
  //! @param center_position_cm The center position [cm] to use.

  // Set the center position [cm].
  UFUNCTION(BlueprintCallable)
  void setCenterPosition(FVector center_position_cm);

protected:
  //! The radius [cm] of the sphere volume.

  // The radius [cm] of the sphere volume.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
  float radius_cm_;

  //! The center position [cm] of the sphere volume.

  // The center position [cm] of the sphere volume.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  FVector center_position_cm_;
};

//! Wraps HaptxApi::BoxBoundingVolume.

// Wraps HaptxApi::BoxBoundingVolume.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew)
class HAPTX_API UHxBoxBoundingVolume : public UHxBoundingVolume {
  GENERATED_BODY()

public:
  //! Blueprint constructor.
  //!
  //! @param world_context World context object.
  //! @param minima_cm The minima [cm] of the box volume.
  //! @param maxima_cm The maxima [cm] of the box volume.
  //! @returns A newly constructed UHxBoxBoundingVolume.

  UFUNCTION(BlueprintCallable, meta = (WorldContext = "world_context"))
  static UHxBoxBoundingVolume* newBoxBoundingVolume(UObject* world_context,
      FVector minima_cm = FVector(-10.0f, -10.0f, -10.0f), 
      FVector maxima_cm = FVector(10.0f, 10.0f, 10.0f));

  //! Default constructor.

  UHxBoxBoundingVolume();

  //! Do any object-specific cleanup required immediately after loading an object, and immediately
  //! after any undo/redo.

  virtual void PostLoad() override;

  //! Get the minima [cm] of the box volume.
  //!
  //! @returns The minima [cm] of the box volume.

  // Get the minima [cm] of the box volume.
  UFUNCTION(BlueprintCallable)
  FVector getMinimaCm() const;

  //! Get the maxima [cm] of the box volume.
  //!
  //! @returns The maxima [cm] of the box volume.

  // Get the maxima [cm] of the box volume.
  UFUNCTION(BlueprintCallable)
  FVector getMaximaCm() const;

  //! Set the extrema [cm] to use.
  //!
  //! @param minima_cm The minima [cm] to use.
  //! @param maxima_cm The maxima [cm] to use.

  // Set the extrema [cm] to use.
  UFUNCTION(BlueprintCallable)
  void setExtremaCm(const FVector& minima_cm, const FVector& maxima_cm);

protected:
  //! The minima [cm] of the box volume.

  // The minima [cm] of the box volume.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  FVector minima_cm_;

  //! The maxima [cm] of the box volume.

  // The maxima [cm] of the box volume.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  FVector maxima_cm_;
};
