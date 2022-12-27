// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_object_effect_component.h>
#include <Haptx/Public/hx_core_actor.h>

bool UHxObjectEffectComponent::addToObject(USceneComponent* component, FName bone,
    bool include_children) {
  if (!IsValid(component)) {
    UE_LOG(HaptX, Warning, 
        TEXT("UHxObjectEffectComponent::addToObject(): Null component provided."))
    return false;
  }

  TArray<int64> object_ids;
  getObjectIds(object_ids, component, bone, include_children);
  if (object_ids.Num() == 0) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::addToObject(): Component %s and bone %s could not be registered with the HaptX SDK."),
        *component->GetName(), *bone.ToString())
    return false;
  }

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::addToObject(): Failed to get handle to core."))
    return false;
  }

  bool failed = false;
  for (int64_t object_id : object_ids) {
    if (!core->getContactInterpreter().addEffectToObject(object_id, object_effect_)) {
      UE_LOG(HaptX, Error,
          TEXT("UHxObjectEffectComponent::addToObject(): Failed to add effect to object %d."), 
          object_id)
      failed = true;
    }
  }
  return !failed;
}

bool UHxObjectEffectComponent::removeFromObject(USceneComponent* component, FName bone,
    bool include_children) {
  if (!IsValid(component)) {
    UE_LOG(HaptX, Warning,
        TEXT("UHxObjectEffectComponent::removeFromObject(): Null component provided."))
    return false;
  }

  TArray<int64> object_ids;
  getObjectIds(object_ids, component, bone, include_children);
  if (object_ids.Num() == 0) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::removeFromObject(): Component %s and bone %s could not be registered with the HaptX SDK."),
        *component->GetName(), *bone.ToString())
    return false;
  }

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::removeFromObject(): Failed to get handle to core."))
    return false;
  }

  if (object_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::removeFromObject(): Null internal effect."))
    return false;
  }

  bool failed = false;
  for (int64_t object_id : object_ids) {
    if (!core->getContactInterpreter().removeEffectFromObject(object_id,
        object_effect_->getId())) {
      UE_LOG(HaptX, Error, TEXT(
          "UHxObjectEffectComponent::removeFromObject(): Failed to remove effect from object %d."),
          object_id)
      failed = true;
    }
  }
  return !failed;
}

bool UHxObjectEffectComponent::isOnObject(USceneComponent* component, FName bone) const {
  if (!IsValid(component)) {
    UE_LOG(HaptX, Warning,
        TEXT("UHxObjectEffectComponent::isOnObject(): Null component provided."))
    return false;
  }

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::isOnObject(): Failed to get handle to core."))
    return false;
  }

  UPrimitiveComponent* primitive_component = Cast<UPrimitiveComponent>(component);
  int64_t object_id;
  if (!IsValid(primitive_component) || !core->tryRegisterObjectWithCi(primitive_component, bone,
      false, object_id)) {
    UE_LOG(HaptX, Warning, TEXT(
        "UHxObjectEffectComponent::isOnObject(): Component %s and bone %s could not be registered with the HaptX SDK."),
        *component->GetName(), *bone.ToString())
    return false;
  }

  if (object_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::isOnObject(): Null internal effect."))
    return false;
  }

  return core->getContactInterpreter().isEffectOnObject(object_id, object_effect_->getId());
}

TArray<int64> UHxObjectEffectComponent::getAttachedObjects() const {
  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(GetWorld());
  if (!IsValid(core)) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::getAttachedObjects(): Failed to get handle to core."))
    return {};
  }

  if (object_effect_ == nullptr) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::getAttachedObjects(): Null internal effect."))
    return {};
  }

  if (object_effect_ != nullptr) {
    std::unordered_set<int64_t> attached_objects =
        core->getContactInterpreter().getEffectAttachedObjects(object_effect_->getId());
    TArray<int64_t> array;
    array.Reserve(attached_objects.size());
    for (const auto& object_id : attached_objects) {
      array.Add(object_id);
    }
    return array;
  }
  else {
    UE_LOG(HaptX, Error,
        TEXT("UHxObjectEffectComponent::getAttachedObjects(): Underlying effect is null."))
    return TArray<int64>();
  }
}

UHxObjectEffectComponent::UHxObjectEffectComponent() : propagate_to_children_(false), 
    object_effect_() {}

void UHxObjectEffectComponent::BeginPlay() {
  object_effect_ = std::make_shared<HxUnrealObjectEffect>(this);
  if (IsValid(GetAttachParent())) {
    addToObject(GetAttachParent(), GetAttachSocketName(), propagate_to_children_);
  }
  Super::BeginPlay();
}

void UHxObjectEffectComponent::getObjectIds(TArray<int64_t>& array, 
    USceneComponent* component, FName bone, bool include_children) {
  if (!IsValid(component)) {
    return;
  }

  AHxCoreActor* core = AHxCoreActor::getAndMaintainPseudoSingleton(component->GetWorld());
  if (!IsValid(core)) {
    UE_LOG(HaptX, Error, TEXT(
        "UHxObjectEffectComponent::getObjectIds(): Failed to get handle to core."))
    return;
  }

  // Add this component to the array.
  UPrimitiveComponent* primitive_component = Cast<UPrimitiveComponent>(component);
  int64_t ci_object_id;
  if (IsValid(primitive_component) && primitive_component->GetBodyInstance(bone) != nullptr && 
      primitive_component->GetBodyInstance(bone)->IsValidBodyInstance() &&
      core->tryRegisterObjectWithCi(primitive_component, bone, false, ci_object_id)) {
    array.Add(ci_object_id);
  }

  if (include_children) {
    // Add all child components that are attached to the parent bone.
    TArray<USceneComponent*> direct_children;
    component->GetChildrenComponents(false, direct_children);
    for (USceneComponent* child : direct_children) {
      if (IsValid(child) && child->GetAttachSocketName() == bone) {
        getObjectIds(array, child, NAME_None, include_children);
      }
    }

    // Add all child bones as well.
    USkinnedMeshComponent* skinned_mesh = Cast<USkinnedMeshComponent>(component);
    if (IsValid(skinned_mesh)) {
      TArray<FName> bone_names;
      skinned_mesh->GetBoneNames(bone_names);
      if (bone == NAME_None) {
        for (FName bone_name : bone_names) {
          if (skinned_mesh->GetBodyInstance(bone_name) != nullptr &&
            core->tryRegisterObjectWithCi(skinned_mesh, bone_name, false, ci_object_id)) {
            array.Add(ci_object_id);
          }
        }
      }
      else {
        for (FName bone_name : bone_names) {
          if (skinned_mesh->BoneIsChildOf(bone_name, bone)) {
            getObjectIds(array, component, bone_name, include_children);
          }
        }
      }
    }
  }
}

UHxObjectEffectComponent::HxUnrealObjectEffect::HxUnrealObjectEffect(
    UHxObjectEffectComponent* object_effect) : object_effect_(object_effect) {}

float UHxObjectEffectComponent::HxUnrealObjectEffect::getForceN(
    const HaptxApi::ObjectEffect::ContactInfo& contact_info) const {
  if (object_effect_.IsValid()) {
    return object_effect_->getForceN(contact_info);
  }
  else {
    return 0.0f;
  }
}
