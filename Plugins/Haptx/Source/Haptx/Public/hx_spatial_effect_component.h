// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Public/hx_bounding_volumes.h>
#include <Haptx/Public/hx_haptic_effect_component.h>
#include <Haptx/Public/hx_simulation_callbacks.h>
#include <HaptxApi/spatial_effect.h>
#include "hx_spatial_effect_component.generated.h"

//! @brief The abstract Haptic Effect class to override when creating effects that are
//! generated in free space.
//!
//! Extend this class and override the getForceN() function to create your own spatial haptic
//! effects. Position your effect and call play() when ready. Optionally define its bounding volume
//! using a child of UHxBoundingVolume.
//!
//! Note that this component's "Bounding Volume" property does not behave correctly under the
//! "Duplicate, Ctrl+W" operation in the editor. The original component's bounding volume, and the
//! duplicate component's bounding volume will refer to the same bounding volume object in memory,
//! and the duplicate component will fail to have a bounding volume at runtime. This can be avoided
//! by using copy-and-paste instead of duplicate, and it can be corrected by manually setting the
//! duplicated component's "Bounding Volume" property to "None" before proceeding as usual.

// The abstract Haptic Effect class to override when creating effects that are generated in free
// space.
UCLASS( ClassGroup=(Haptx), HideCategories = (ComponentReplication, Cooking, Physics, LOD,
    Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxSpatialEffectComponent : public UHxHapticEffectComponent {
  GENERATED_BODY()

public:
  //! Get the bounding volume this effect is operating in.
  //!
  //! @returns The bounding volume this effect is operating in.

  // Get the bounding volume this effect is operating in.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  UHxBoundingVolume* getBoundingVolume() const;

  //! Set the bounding volume this effect is operating in.
  //!
  //! @param bounding_volume The new bounding volume to use. Nullptr indicates an unbounded effect.

  // Set the bounding volume this effect is operating in. A value of nullptr indicates an unbounded
  // effect.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setBoundingVolume(UHxBoundingVolume* bounding_volume);

protected:
  //! Default constructor.
  UHxSpatialEffectComponent();

  virtual void BeginPlay() override;

  virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

  //! Override to define your Haptic Effect.
  //!
  //! @param spatial_info Information about a tactor in range of the effect.
  //! @returns The force [N] applied by the effect.
  virtual float getForceN(
      const HaptxApi::SpatialEffect::SpatialInfo& spatial_info) const {
    return 0.0f;
  }

  //! Get the underlying Haptic Effect.
  //!
  //! @returns The underlying Haptic Effect.
  virtual std::shared_ptr<HaptxApi::HapticEffect>
      getEffectInternal() const override {
    return spatial_effect_;
  }

  //! @brief The bounding volume this effect is operating in.
  //!
  //! A value of None indicates an unbounded effect.
  //!
  //! Note that this property does not behave correctly under the component "Duplicate, Ctrl+W"
  //! operation in the editor. Please use copy and paste instead.

  // The bounding volume this effect is operating in. A value of None indicates an unbounded
  // effect. Note that this property does not behave correctly under the component
  // "Duplicate, Ctrl+W" operation in the editor. Please use copy and paste instead.
  UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Haptic Effects")
  UHxBoundingVolume* bounding_volume_;

  //! The underlying object effect.
  std::shared_ptr<HaptxApi::SpatialEffect> spatial_effect_;

private:
  //! Callbacks for getting simulation information about this object.
  std::shared_ptr<WeldedComponentCallbacks> callbacks_;

  //! Wraps HaptxApi::SpatialEffect and associates it with an
  //! UHxSpatialEffectComponent.
  class HxUnrealSpatialEffect : public HaptxApi::SpatialEffect {

  public:
    //! Construct by association with a UObject.
    //!
    //! @param spatial_effect The UObject associated with this effect.
    HxUnrealSpatialEffect(UHxSpatialEffectComponent* spatial_effect);

    float getForceN(const HaptxApi::SpatialEffect::SpatialInfo& spatial_info) const override;

  private:
    //! The UObject associated with this effect.
    TWeakObjectPtr<UHxSpatialEffectComponent> spatial_effect_;
  };
};
