// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_haptic_effect_component.h>
#include <Haptx/Private/haptx_shared.h>

UHxHapticEffectComponent::UHxHapticEffectComponent() : begin_playing_(true), is_looping_(false) {}

void UHxHapticEffectComponent::BeginPlay() {
	Super::BeginPlay();

  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->setIsLooping(is_looping_);
    if (begin_playing_) {
      effect->play();
    }
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::BeginPlay(): Underlying effect is null."))
  }
}

void UHxHapticEffectComponent::play() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->play();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::play(): Underlying effect is null."))
  }
}

bool UHxHapticEffectComponent::isPlaying() const {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    return effect->isPlaying();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::isPlaying(): Underlying effect is null."))
  }

  return false;
}

void UHxHapticEffectComponent::pause() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->pause();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::pause(): Underlying effect is null."))
  }
}

bool UHxHapticEffectComponent::isPaused() const {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    return effect->isPaused();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::isPaused(): Underlying effect is null."))
  }

  return false;
}

void UHxHapticEffectComponent::unpause() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->unpause();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::unpause(): Underlying effect is null."))
  }
}

bool UHxHapticEffectComponent::isLooping() const {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    return effect->isLooping();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::isLooping(): Underlying effect is null."))
  }

  return is_looping_;
}

void UHxHapticEffectComponent::setIsLooping(bool is_looping) {
  is_looping_ = is_looping;
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->setIsLooping(is_looping);
  }
  else {
    UE_LOG(HaptX, Error, 
        TEXT("UHxHapticEffectComponent::setIsLooping(): Underlying effect is null."))
  }
}

void UHxHapticEffectComponent::stop() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->stop();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::stop(): Underlying effect is null."))
  }
}

void UHxHapticEffectComponent::restart() {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->restart();
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::restart(): Underlying effect is null."))
  }
}

void UHxHapticEffectComponent::advance(float delta_time_s) {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->advance(delta_time_s);
  }
  else {
    UE_LOG(HaptX, Error, TEXT("UHxHapticEffectComponent::advance(): Underlying effect is null."))
  }
}

float UHxHapticEffectComponent::getPlayTimeS() const {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    return effect->getPlayTimeS();
  }
  else {
    UE_LOG(HaptX, Error, 
        TEXT("UHxHapticEffectComponent::getPlayTimeS(): Underlying effect is null."))
  }

  return 0.0f;
}

float UHxHapticEffectComponent::getDurationS() const {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    return effect->getDurationS();
  }
  else {
    UE_LOG(HaptX, Error, 
        TEXT("UHxHapticEffectComponent::getDurationS(): Underlying effect is null."))
  }

  return 0.0f;
}

void UHxHapticEffectComponent::setDurationS(float duration_s) {
  std::shared_ptr<HaptxApi::HapticEffect> effect = getEffectInternal();
  if (effect != nullptr) {
    effect->setDurationS(duration_s);
  }
  else {
    UE_LOG(HaptX, Error, 
        TEXT("UHxHapticEffectComponent::setDurationS(): Underlying effect is null."))
  }
}
