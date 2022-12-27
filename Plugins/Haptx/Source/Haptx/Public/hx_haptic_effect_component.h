// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Components/PrimitiveComponent.h>
#include <HaptxApi/haptic_effect.h>
#include <memory>
#include "hx_haptic_effect_component.generated.h"

//! @brief Defines the basic interface for Haptic Effects.
//!
//! See HaptxApi::HapticEffect.

// Defines the basic interface for Haptic Effects.
UCLASS( ClassGroup=(Haptx),
    HideCategories = (ComponentReplication, Cooking, Physics, LOD, Collision))
class HAPTX_API UHxHapticEffectComponent : public UPrimitiveComponent {
  GENERATED_BODY()

public:
  //! Play the Haptic Effect.

  // Play the Haptic Effect.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void play();

  //! Get whether the effect is playing and not paused.
  //!
  //! @returns Whether the effect is playing and not paused.

  // Whether the effect is playing and not paused.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool isPlaying() const;

  //! Pause the Haptic Effect.

  // Pause the Haptic Effect.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void pause();

  //! Get whether the effect is paused.
  //!
  //! @returns Whether the effect is paused.

  // Get whether the effect is paused.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool isPaused() const;

  //! Unpause the effect.

  // Unpause the effect.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void unpause();

  //! Whether the effect is looping.
  //!
  //! @returns Whether the effect is looping.

  // Whether the effect is looping.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  bool isLooping() const;

  //! Set whether the effect is looping.
  //!
  //! @param is_looping Whether the effect is looping.

  // Set whether the effect is looping.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setIsLooping(bool is_looping);

  //! Stop the Haptic Effect.

  // Stop the Haptic Effect.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void stop();

  //! Restart the effect back to the beginning.

  // Restart the effect back to the beginning.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void restart();

  //! Advance the effect by some delta time [s].
  //!
  //! @param delta_time_s The amount of time [s] by which to advance the effect.

  // Advance the effect by some delta time [s].
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void advance(float delta_time_s);

  //! @brief Get the amount of time [s] the effect has spent playing.
  //!
  //! If looping this will cycle between 0 and duration.
  //!
  //! @returns The amount of time [s] the effect has spent playing.

  // Get the amount of time [s] the effect has spent playing.
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  float getPlayTimeS() const;

  //! @brief Get the amount of play time after which the effect automatically stops [s].
  //!
  //! Values less than 0 represent infinite duration.
  //!
  //! If the effect is set to loop it will instead reset back to the beginning.
  //!
  //! @returns The amount of play time after which the effect automatically stops [s].

  // Get the amount of play time after which the effect automatically stops [s].
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  float getDurationS() const;

  //! @brief Set the amount of play time after which the effect automatically stops [s].
  //!
  //! Values less than 0 represent infinite duration.
  //!
  //! If the effect is set to loop it will instead reset back to the beginning.
  //!
  //! @param duration_s The new amount of play time after which the effect automatically
  //! stops [s].

  // Set the amount of play time after which the effect automatically stops [s].
  UFUNCTION(BlueprintCallable, Category = "Haptic Effects")
  void setDurationS(float duration_s);

protected:
  //! Sets default values for this component's properties.

  UHxHapticEffectComponent();

  //! Called when the game starts.

  virtual void BeginPlay() override;

  //! @brief Get the underlying Haptic Effect.
  //!
  //! Defined by child classes.
  //!
  //! @returns The underlying Haptic Effect.

  virtual std::shared_ptr<HaptxApi::HapticEffect> getEffectInternal() const { 
      return nullptr; }

  //! Whether this effect begins the game already playing.

  // Whether this effect begins the game already playing.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  bool begin_playing_;

  //! Whether this effect stops once its duration is exceeded, or begins anew.

  // Whether this effect stops once its duration is exceeded, or begins anew.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Haptic Effects")
  bool is_looping_;
};
