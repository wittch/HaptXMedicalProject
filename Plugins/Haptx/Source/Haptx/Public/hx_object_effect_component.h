// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Public/hx_haptic_effect_component.h>
#include <HaptxApi/object_effect.h>
#include "hx_object_effect_component.generated.h"

//! @brief The Haptic Effect component to override when creating effects that are generated via
//! contact with objects.
//!
//! Extend this component and override the getForceN() function to create your own object-based
//! Haptic Effects. Add your child component to your GameObject and call play() when ready.

// The Haptic Effect component to override when creating effects that are generated via contact
// with objects.
UCLASS( ClassGroup=(Haptx), HideCategories = (ComponentReplication, Cooking, Physics, LOD,
    Collision, Lighting, Rendering, Mobile))
class HAPTX_API UHxObjectEffectComponent : public UHxHapticEffectComponent {
  GENERATED_BODY()

public:
  //! Adds this effect to an object.
  //!
  //! @param component The component representing the object.
  //! @param bone The bone representing the object.
  //! @param include_children Whether to also add this effect to child components/bones.
  //! @returns Whether the effect was successfully added to all objects that were capable of
  //! registration with the HaptX SDK.

  // Adds this effect to an object.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool addToObject(USceneComponent* component, FName bone = NAME_None,
      bool include_children = false);

  //! Removes this effect from an object.
  //!
  //! @param component The component representing the object.
  //! @param bone The bone representing the object.
  //! @param include_children Whether to also remove this effect from child components/bones.
  //! @returns Whether the effect was successfully removed from all objects that were capable of
  //! registration with the HaptX SDK.

  // Removes this effect from an object.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool removeFromObject(USceneComponent* component, FName bone = NAME_None,
      bool include_children = false);

  //! Whether this effect is on the object.
  //!
  //! @param component The component representing the object.
  //! @param bone The bone representing the object.
  //! @returns True if the effect is on the object.

  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool isOnObject(USceneComponent* component, FName bone = NAME_None) const;

  //! Get the objects this effect is attached to.
  //!
  //! @returns The objects this effect is attached to.
  TArray<int64> getAttachedObjects() const;

protected:
  //! Sets default values for this component's properties.
  UHxObjectEffectComponent();

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Override to define your Haptic Effect.
  //!
  //! @param contact_info Information about a contact generating the effect.
  //! @returns The force [N] applied by the effect against the body normal.
  virtual float getForceN(
    const HaptxApi::ObjectEffect::ContactInfo& contact_info) const { return 0.0f; }

  //! @brief Whether child objects attached to the component this effect is attached to are also
  //! affected.
  //!
  //! Set true if you want this effect to apply to bones or scene components that are attached to
  //! the object.

  // Whether child objects attached to the component this effect is attached to are also
  // affected.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  bool propagate_to_children_;

  //! Get the underlying Haptic Effect.
  //!
  //! @returns The underlying Haptic Effect.
  virtual std::shared_ptr<HaptxApi::HapticEffect>
      getEffectInternal() const override {
    return object_effect_;
  }

  //! The underlying object effect.
  std::shared_ptr<HaptxApi::ObjectEffect> object_effect_;

private:
  //! Get the ID's of an object and (optionally) all of its children.
  //!
  //! @param array The array to populate with the ID(s).
  //! @param component The component representing the object.
  //! @param bone The bone representing the object.
  //! @param include_children Whether to also get the ID's of child components/bones.
  static void getObjectIds(TArray<int64_t>& array, USceneComponent* component,
      FName bone = NAME_None, bool include_children = false);

  //! Wraps HaptxApi::ObjectEffect and associates it with an
  //! HxObjectEffect.
  class HxUnrealObjectEffect : public HaptxApi::ObjectEffect {

  public:
    //! Construct by association with a UObject.
    //!
    //! @param object_effect The UObject associated with this effect.
    HxUnrealObjectEffect(UHxObjectEffectComponent* object_effect);

    float getForceN(
      const HaptxApi::ObjectEffect::ContactInfo& contact_info) const override;

  private:
    //! The UObject associated with this effect.
    TWeakObjectPtr<UHxObjectEffectComponent> object_effect_;
  };
};
