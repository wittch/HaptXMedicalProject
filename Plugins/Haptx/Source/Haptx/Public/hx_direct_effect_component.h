// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/hx_hand_actor.h>
#include <Haptx/Public/hx_haptic_effect_component.h>
#include <HaptxApi/direct_effect.h>
#include "hx_direct_effect_component.generated.h"

//! @brief The Haptic Effect component to override when creating effects that aren't convenient via
//! the other base effect classes.
//!
//! Extend this class and override the getDisplacementM() function to create your own direct haptic
//! effects. Add tactors to the effect using addToTactor() or addToCoverageRegion() and call play
//! when ready.

// The Haptic Effect component to override when creating effects that aren't convenient via the
// other base effect classes.
UCLASS(ClassGroup=(Haptx), HideCategories = (ComponentReplication, Cooking, Physics, LOD,
    Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxDirectEffectComponent : public UHxHapticEffectComponent {
  GENERATED_BODY()

public:
  //! Adds this effect to a tactor.
  //!
  //! @param peripheral_id Which peripheral.
  //! @param tactor_id Which tactor.
  //! @returns True if this is the function call that adds the effect to the tactor.
  bool addToTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id);

  //! Removes this effect from a tactor.
  //!
  //! @param peripheral_id Which peripheral.
  //! @param tactor_id Which tactor.
  //! @returns True if this is the function call that removes the effect from the tactor.
  bool removeFromTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id);

  //! Checks whether this effect is on a tactor.
  //!
  //! @param peripheral_id Which peripheral.
  //! @param tactor_id Which tactor.
  //! @returns True if the effect is on the tactor.
  bool isOnTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id) const;

  //! Get the tactors that this effect is attached to.
  //!
  //! @returns A map from peripheral ID to the set of tactors on that peripheral the effect is
  //! attached too.
  std::unordered_map<HaptxApi::HaptxUuid, std::unordered_set<int>> getAttachedTactors() const;

  //! Adds this effect to all tactors that are associated with the given coverage region.
  //!
  //! @param coverage_region Which coverage region.
  //! @returns True unless the effect failed to be added to one of the coverage region's tactors.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool addToCoverageRegion(FName coverage_region);

  //! Remove this effect from all tactors that are associated with the given coverage region.
  //!
  //! @param coverage_region Which coverage region.
  //! @returns True if the effect was removed from the coverage region.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool removeFromCoverageRegion(FName coverage_region);

  //! Whether this effect is on a given coverage region.
  //!
  //! @param coverage_region Which coverage region to search for.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool isOnCoverageRegion(FName coverage_region) const;

protected:
  //! Sets default values for this component's properties.
  UHxDirectEffectComponent();

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Override to define your Haptic Effect.
  //!
  //! @param direct_info Information about a tactor the effect is playing on.
  //! @returns The displacement [m] resulting from the effect.
  virtual float getDisplacementM(const HaptxApi::DirectEffect::DirectInfo& direct_info) const {
    return 0.0f;
  }

  //! Get the underlying Haptic Effect.
  //!
  //! @returns The underlying Haptic Effect.
  virtual std::shared_ptr<HaptxApi::HapticEffect>
      getEffectInternal() const override {
    return direct_effect_;
  }

  //! This effect operates on all tactors that are associated with these coverage regions.

  // This effect operates on all tactors that are associated with these coverage regions.
  UPROPERTY(EditAnywhere, Category = "Haptic Effects")
  TSet<FName> coverage_regions_;

  //! The underlying direct effect.
  std::shared_ptr<HaptxApi::DirectEffect> direct_effect_;

private:
  //! Adds this effect to all tactors.
  void addToTactors();

  //! Gets bound to AHxHandActor::on_right_hand_initialized and
  //! AHxHandActor::on_left_hand_initialized.
  void onHandInitialized(AHxHandActor* hand);

  //! Wraps HaptxApi::Directffect and associates it with an
  //! UHxDirectEffectComponent.
  class HxUnrealDirectEffect : public HaptxApi::DirectEffect {

  public:
    //! Construct by association with a UObject.
    //!
    //! @param direct_effect The UObject associated with this effect.
    HxUnrealDirectEffect(UHxDirectEffectComponent* direct_effect);

    float getDisplacementM(const HaptxApi::DirectEffect::DirectInfo& direct_info) const override;

  private:
    //! The UObject associated with this effect.
    TWeakObjectPtr<UHxDirectEffectComponent> direct_effect_;
  };
};
