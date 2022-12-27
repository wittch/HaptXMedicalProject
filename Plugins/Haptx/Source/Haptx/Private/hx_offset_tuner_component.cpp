// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_offset_tuner_component.h>
#include <Runtime/Engine/Classes/Camera/PlayerCameraManager.h>
#include <Runtime/Engine/Classes/Engine/World.h>
#include <Runtime/Engine/Classes/GameFramework/Actor.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Haptx/Private/haptx_shared.h>

UHxOffsetSaveGame::UHxOffsetSaveGame() : 
    local_offset(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.0f, 1.0f, 1.0f)) {}

UHxOffsetTunerComponent::UHxOffsetTunerComponent(const FObjectInitializer& ObjectInitializer) :
    Super(ObjectInitializer),
    server_translation_frame_(ETranslationFrame::LOCAL), server_translation_speed_cm_s_(15.0f),
    server_rotation_frame_(ERotationFrame::LOCAL), server_rotation_speed_deg_s_(30.0f),
    client_forward_axis_(FName("HxOffsetForward")), client_right_axis_(FName("HxOffsetRight")), 
    client_up_axis_(FName("HxOffsetUp")), client_pitch_axis_(FName("HxOffsetPitch")),
    client_yaw_axis_(FName("HxOffsetYaw")), client_roll_axis_(FName("HxOffsetRoll")),
    client_reset_action_(FName("HxOffsetReset")), client_enable_level_specific_offsets_(true),
    server_initial_transform_(FTransform::Identity), server_local_offset_(FTransform::Identity),
    client_velocity_input_(FVector::ZeroVector), server_velocity_input_(FVector::ZeroVector),
    client_angular_velocity_input_(FVector::ZeroVector),
    server_angular_velocity_input_(FVector::ZeroVector), client_save_slot_(),
    client_save_game_(nullptr), server_camera_(nullptr),  client_has_saved_(false),
    is_authoritative_(false), is_locally_controlled_(false), client_is_input_bound_(false) {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  SetIsReplicated(true);
}

void UHxOffsetTunerComponent::BeginPlay() {
	Super::BeginPlay();

  is_authoritative_ = isActorAuthoritative(GetOwner());
  if (is_authoritative_) {
    server_camera_ = GetOwner()->FindComponentByClass<UCameraComponent>();
    server_initial_transform_ = GetOwner()->GetActorTransform();
  }

  is_locally_controlled_ = isPawnLocallyControlled(getPawn(GetOwner()));
  if (is_locally_controlled_) {
    FString default_offset_save_slot_ = UHxOffsetSaveGame::StaticClass()->GetFName().ToString();
  
    // Load our level specific offset (if we're using level specific offsets), otherwise load the 
    // default offset.
    if (client_enable_level_specific_offsets_) {
      client_save_slot_ = default_offset_save_slot_ + "-" + GetWorld()->GetMapName();

      if (UGameplayStatics::DoesSaveGameExist(client_save_slot_, 0)) {
        client_save_game_ = Cast<UHxOffsetSaveGame>(UGameplayStatics::LoadGameFromSlot(
            client_save_slot_, 0));
      } else {
        client_save_game_ = 
          Cast<UHxOffsetSaveGame>(UGameplayStatics::CreateSaveGameObject(
              UHxOffsetSaveGame::StaticClass()));
      }
    } else {
      client_save_slot_ = default_offset_save_slot_;
      if (UGameplayStatics::DoesSaveGameExist(client_save_slot_, 0)) {
        client_save_game_ = Cast<UHxOffsetSaveGame>(UGameplayStatics::LoadGameFromSlot(
            client_save_slot_, 0));
      } else {
        client_save_game_ = Cast<UHxOffsetSaveGame>(UGameplayStatics::CreateSaveGameObject(
            UHxOffsetSaveGame::StaticClass()));
      }
    }

    clientApplyLocalOffset();
  }
}

void UHxOffsetTunerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
    FActorComponentTickFunction * ThisTickFunction) {
  if (is_locally_controlled_ && clientBindInput() && !is_authoritative_) {
    PrimaryComponentTick.SetTickFunctionEnable(false);
  } 
  
  if (is_authoritative_) {
    if (!server_velocity_input_.IsZero()) {
      serverTranslate(serverGetCoordinateFrame(
          server_translation_frame_).GetRotation().RotateVector(
          server_velocity_input_ * server_translation_speed_cm_s_ * DeltaTime));
    }

    if (!server_angular_velocity_input_.IsZero()) {
      serverRotate(UKismetMathLibrary::RotatorFromAxisAndAngle(serverGetCoordinateFrame(
          server_rotation_frame_).GetRotation().RotateVector(server_angular_velocity_input_),
          server_rotation_speed_deg_s_ * DeltaTime).Quaternion());
    }
  }
}

void UHxOffsetTunerComponent::EndPlay(EEndPlayReason::Type EndPlayReason) {
  if (!is_locally_controlled_ || client_save_game_ == nullptr || client_has_saved_) {
    return;
  }

  UGameplayStatics::SaveGameToSlot(client_save_game_, client_save_slot_, 0);
  client_has_saved_ = true;
}

void UHxOffsetTunerComponent::clientTranslateForward(float axis) {
  if (client_velocity_input_.X != axis) {
    client_velocity_input_.X = axis;
    serverTranslateForward(axis);
  }
}

void UHxOffsetTunerComponent::clientTranslateRight(float axis) {
  if (client_velocity_input_.Y != axis) {
    client_velocity_input_.Y = axis;
    serverTranslateRight(axis);
  }
}

void UHxOffsetTunerComponent::clientTranslateUp(float axis) {
  if (client_velocity_input_.Z != axis) {
    client_velocity_input_.Z = axis;
    serverTranslateUp(axis);
  }
}

void UHxOffsetTunerComponent::clientRotatePitch(float axis) {
  if (client_angular_velocity_input_.Y != axis) {
    client_angular_velocity_input_.Y = axis;
    serverRotatePitch(axis);
  }
}

void UHxOffsetTunerComponent::clientRotateYaw(float axis) {
  if (client_angular_velocity_input_.Z != axis) {
    client_angular_velocity_input_.Z = axis;
    serverRotateYaw(axis);
  }
}

void UHxOffsetTunerComponent::clientRotateRoll(float axis) {
  if (client_angular_velocity_input_.X != axis) {
    client_angular_velocity_input_.X = axis;
    serverRotateRoll(axis);
  }
}

void UHxOffsetTunerComponent::clientResetOffset() {
  serverResetOffset();
}

bool UHxOffsetTunerComponent::serverTranslateForward_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverTranslateForward_Implementation(float axis) {
  server_velocity_input_.X = axis;
}

bool UHxOffsetTunerComponent::serverTranslateRight_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverTranslateRight_Implementation(float axis) {
  server_velocity_input_.Y = axis;
}

bool UHxOffsetTunerComponent::serverTranslateUp_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverTranslateUp_Implementation(float axis) {
  server_velocity_input_.Z = axis;
}

bool UHxOffsetTunerComponent::serverRotatePitch_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverRotatePitch_Implementation(float axis) {
  server_angular_velocity_input_.Y = axis;
}

bool UHxOffsetTunerComponent::serverRotateYaw_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverRotateYaw_Implementation(float axis) {
  server_angular_velocity_input_.Z = axis;
}

bool UHxOffsetTunerComponent::serverRotateRoll_Validate(float axis) {
  return true;
}

void UHxOffsetTunerComponent::serverRotateRoll_Implementation(float axis) {
  // Sign flip aligns axis to Unreal's weird convention for roll.
  server_angular_velocity_input_.X = -1.0f * axis;
}

bool UHxOffsetTunerComponent::serverResetOffset_Validate() {
  return true;
}

void UHxOffsetTunerComponent::serverResetOffset_Implementation() {
  AActor* owner = GetOwner();
  if (IsValid(owner)) {
    owner->SetActorTransform(server_initial_transform_);
  }

  server_local_offset_ = FTransform::Identity;
  clientUpdateSaveGameTranslation(FVector::ZeroVector);
  clientUpdateSaveGameRotation(FQuat::Identity);
}

FTransform UHxOffsetTunerComponent::serverGetCoordinateFrame(ETranslationFrame frame) {
  switch (frame) {
  case (ETranslationFrame::CAMERA):
    if (IsValid(server_camera_)) {
      return server_camera_->GetComponentTransform();
    }
    break;
  case (ETranslationFrame::GLOBAL):
    break;
  case (ETranslationFrame::LOCAL):
    AActor* owner = GetOwner();
    if (IsValid(owner)) {
      return owner->GetActorTransform();
    }
    break;
  }

  return FTransform::Identity;
}

FTransform UHxOffsetTunerComponent::serverGetCoordinateFrame(ERotationFrame frame) {
  // Use the value returned by the translation frame version of serverGetCoordinateFrame as the
  // logic is the same. The ERotationFrame struct only exists so that you can't select Camera as a
  // rotation frame.
  switch (frame) {
  case ERotationFrame::LOCAL:
    return serverGetCoordinateFrame(ETranslationFrame::LOCAL);
  case ERotationFrame::GLOBAL:
    return serverGetCoordinateFrame(ETranslationFrame::GLOBAL);
  default:
    return serverGetCoordinateFrame(ETranslationFrame::GLOBAL);
  }
}

void UHxOffsetTunerComponent::serverTranslate(const FVector& w_delta) {
  AActor* owner = GetOwner();
  if (IsValid(owner) && !w_delta.IsZero()) {
    owner->AddActorWorldOffset(w_delta);

    // Save the delta as if it were local to our initial transform.
    FTransform server_initial_transform_no_scale = server_initial_transform_;
    server_initial_transform_no_scale.RemoveScaling();
    server_local_offset_.AddToTranslation(
        server_initial_transform_no_scale.Inverse().TransformVector(w_delta));
    clientUpdateSaveGameTranslation(server_local_offset_.GetTranslation());
  }
}

void UHxOffsetTunerComponent::clientUpdateSaveGameTranslation_Implementation(
    const FVector& l_translation_cm) {
  if (IsValid(client_save_game_)) {
    client_save_game_->local_offset.SetTranslation(l_translation_cm);
  }
}

void UHxOffsetTunerComponent::serverRotate(const FQuat& w_delta) {
  AActor* owner = GetOwner();
  if (IsValid(owner) && !w_delta.IsIdentity()) {
    FTransform w_owner = owner->GetActorTransform();
    owner->SetActorRotation(w_delta * w_owner.GetRotation());

    // Save the delta rotation as if it were local to our initial transform.
    server_local_offset_.SetRotation(server_local_offset_.GetRotation() *
        w_owner.GetRotation().Inverse() * w_delta * w_owner.GetRotation());
    clientUpdateSaveGameRotation(server_local_offset_.GetRotation());
  }
}

void UHxOffsetTunerComponent::clientUpdateSaveGameRotation_Implementation(
    const FQuat& l_rotation) {
  if (IsValid(client_save_game_)) {
    client_save_game_->local_offset.SetRotation(l_rotation);
  }
}

void UHxOffsetTunerComponent::clientApplyLocalOffset() {
  if (IsValid(client_save_game_)) {
    serverApplyLocalOffset(client_save_game_->local_offset.GetLocation(),
        client_save_game_->local_offset.GetRotation());
  }
}

bool UHxOffsetTunerComponent::serverApplyLocalOffset_Validate(const FVector& l_translation_cm,
    const FQuat &l_rotation) {
  return true;
}

void UHxOffsetTunerComponent::serverApplyLocalOffset_Implementation(
    const FVector& l_translation_cm, const FQuat &l_rotation) {
  server_local_offset_.SetLocation(l_translation_cm);
  server_local_offset_.SetRotation(l_rotation);
  
  AActor* owner = GetOwner();
  if (IsValid(owner)) {
    owner->SetActorTransform(UKismetMathLibrary::ComposeTransforms(
        FTransform(l_rotation, l_translation_cm, FVector::OneVector), owner->GetTransform()));
  }
}

bool UHxOffsetTunerComponent::clientBindInput() {
  AActor* owner = GetOwner();
  UInputComponent* input_component = IsValid(GetOwner()) ? GetOwner()->InputComponent : nullptr;
  if (IsValid(input_component) && !client_is_input_bound_) {
    client_is_input_bound_ = true;
    input_component->BindAxis(client_forward_axis_, this,
        &UHxOffsetTunerComponent::clientTranslateForward);
    input_component->BindAxis(client_right_axis_, this,
        &UHxOffsetTunerComponent::clientTranslateRight);
    input_component->BindAxis(client_up_axis_, this,
        &UHxOffsetTunerComponent::clientTranslateUp);
    input_component->BindAxis(client_roll_axis_, this,
        &UHxOffsetTunerComponent::clientRotateRoll);
    input_component->BindAxis(client_pitch_axis_, this,
        &UHxOffsetTunerComponent::clientRotatePitch);
    input_component->BindAxis(client_yaw_axis_, this,
        &UHxOffsetTunerComponent::clientRotateYaw);
    input_component->BindAction(client_reset_action_, IE_Pressed, this,
        &UHxOffsetTunerComponent::serverResetOffset);
  }

  return client_is_input_bound_;
}
