// Copyright (C) 2019-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_direct_effect_component.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/peripheral_link.h>
#include <HaptxApi/names.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>

UHxDirectEffectComponent::UHxDirectEffectComponent() : direct_effect_() {
  for (int i = 0; i < static_cast<int>(HaptxApi::CoverageRegion::LAST); i++) {
    coverage_regions_.Add(STRING_TO_FNAME(
        HaptxApi::getName(static_cast<HaptxApi::CoverageRegion>(i)).getText()));
  }
}

void UHxDirectEffectComponent::BeginPlay() {
  direct_effect_ = std::make_shared<HxUnrealDirectEffect>(this);
  AHxHandActor::on_left_hand_initialized.AddUObject(this, 
      &UHxDirectEffectComponent::onHandInitialized);
  AHxHandActor::on_right_hand_initialized.AddUObject(this,
      &UHxDirectEffectComponent::onHandInitialized);
  addToTactors();
  Super::BeginPlay();
}

bool UHxDirectEffectComponent::addToTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id) {
  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {   
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::addToTactor(): Failed to get handle to core."))
    return false;
  }

  return core->getContactInterpreter().addEffectToTactor(peripheral_id, tactor_id, direct_effect_);
}

bool UHxDirectEffectComponent::removeFromTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id) {
  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {   
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::removeFromTactor(): Failed to get handle to core."))
    return false;
  }

  if (direct_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::removeFromTactor(): Null internal effect."))
    return false;
  }

  return core->getContactInterpreter().removeEffectFromTactor(peripheral_id, tactor_id,
      direct_effect_->getId());
}

bool UHxDirectEffectComponent::isOnTactor(HaptxApi::HaptxUuid peripheral_id, int tactor_id) const {
  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {   
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::isOnTactor(): Failed to get handle to core."))
    return false;
  }

  if (direct_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::isOnTactor(): Null internal effect."))
    return false;
  }

  return core->getContactInterpreter().isEffectOnTactor(peripheral_id, tactor_id,
      direct_effect_->getId());
}

std::unordered_map<HaptxApi::HaptxUuid, std::unordered_set<int>>
    UHxDirectEffectComponent::getAttachedTactors() const {
  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {   
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::getAttachedTactors(): Failed to get handle to core."))
    return {};
  }

  if (direct_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxDirectEffectComponent::getAttachedTactors(): Null internal effect."))
    return {};
  }

  return core->getContactInterpreter().getEffectAttachedTactors(direct_effect_->getId());
}

bool UHxDirectEffectComponent::addToCoverageRegion(FName coverage_region) {
  coverage_regions_.Emplace(coverage_region);
  HaptxApi::HaptxName hx_coverage_region(FNAME_TO_CSTR(coverage_region));

  // Add all tactors associated with the given coverage region.
  TArray<AActor*> peripheral_link_actors;
  UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UPeripheralLink::StaticClass(),
      peripheral_link_actors);
  bool a_tactor_has_failed = false;
  for (AActor* it : peripheral_link_actors) {
    IPeripheralLink* peripheral_link = Cast<IPeripheralLink>(it);
    if (peripheral_link != nullptr && peripheral_link->getPeripheral() != nullptr) {
      const auto& peripheral = peripheral_link->getPeripheral();
      for (const HaptxApi::Tactor& tactor : peripheral->tactors) {
        if (tactor.coverage_region == hx_coverage_region &&
            !addToTactor(peripheral->id, tactor.id)) {
          a_tactor_has_failed = true;
        }
      }
    }
  }
  return !a_tactor_has_failed;
}

bool UHxDirectEffectComponent::removeFromCoverageRegion(FName coverage_region) {  
  HaptxApi::HaptxName hx_coverage_region(FNAME_TO_CSTR(coverage_region));

  if (coverage_regions_.Remove(coverage_region) > 0) {
    // Remove all tactors associated with the given coverage region.
    TArray<AActor*> peripheral_link_actors;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UPeripheralLink::StaticClass(),
        peripheral_link_actors);
    for (AActor* it : peripheral_link_actors) {
      IPeripheralLink* peripheral_link = Cast<IPeripheralLink>(it);
      if (peripheral_link != nullptr && peripheral_link->getPeripheral() != nullptr) {
        const auto& peripheral = peripheral_link->getPeripheral();
        for (const HaptxApi::Tactor& tactor : peripheral->tactors) {
          if (tactor.coverage_region == hx_coverage_region) {
            removeFromTactor(peripheral->id, tactor.id);
          }
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

bool UHxDirectEffectComponent::isOnCoverageRegion(FName coverage_region) const {
  return coverage_regions_.Contains(coverage_region);
}

void UHxDirectEffectComponent::addToTactors() {
  // Find all the tactors in the world that are associated with our rigid body parts.
  TArray<AActor*> peripheral_link_actors;
  UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UPeripheralLink::StaticClass(),
      peripheral_link_actors);
  for (AActor* it : peripheral_link_actors) {
    IPeripheralLink* peripheral_link = Cast<IPeripheralLink>(it);
    if (peripheral_link != nullptr && peripheral_link->getPeripheral() != nullptr) {
      const auto& peripheral = peripheral_link->getPeripheral();
      for (const HaptxApi::Tactor& tactor : peripheral->tactors) {
        if (coverage_regions_.Contains(STRING_TO_FNAME(tactor.coverage_region.getText()))) {
          addToTactor(peripheral->id, tactor.id);
        }
      }
    }
  }
}

void UHxDirectEffectComponent::onHandInitialized(AHxHandActor* hand) {
  addToTactors();
}

UHxDirectEffectComponent::HxUnrealDirectEffect::HxUnrealDirectEffect(
    UHxDirectEffectComponent* direct_effect) : direct_effect_(direct_effect) {}

float UHxDirectEffectComponent::HxUnrealDirectEffect::getDisplacementM(
    const HaptxApi::DirectEffect::DirectInfo& direct_info) const {
  if (direct_effect_.IsValid()) {
    return direct_effect_->getDisplacementM(direct_info);
  }
  else {
    return 0.0f;
  }
}
