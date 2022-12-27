// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_on_screen_log.h>
#include <Haptx/Private/haptx_shared.h>
#include <math.h>
#include <Runtime/Core/Public/Math/UnrealMathUtility.h>
#include <Runtime/Core/Public/Math/Vector.h>
#include <Runtime/Engine/Classes/Components/TextRenderComponent.h>
#include <Runtime/Engine/Classes/Engine/World.h>

AHxOnScreenLog* AHxOnScreenLog::instance_ = nullptr;
TArray<FOnScreenMessage> AHxOnScreenLog::queued_on_screen_messages_;

AHxOnScreenLog::AHxOnScreenLog(const FObjectInitializer& object_initializer) : 
    Super(object_initializer), display_on_screen_messages_(true), 
    min_severity_(EOnScreenMessageSeverity::INFO), text_size_(4.0f), max_line_length_(80u), 
    left_margin_(0.33f), top_margin_(0.33f), text_distance_cm_(100.0f), 
    debug_text_material_(nullptr), debug_text_material_color_param_(TEXT("color")) {
  PrimaryActorTick.bCanEverTick = true;
  SetTickGroup(ETickingGroup::TG_LastDemotable);
}

void AHxOnScreenLog::BeginPlay() {
  Super::BeginPlay();

  if (IsValid(instance_) && instance_ != this) {
    this->Destroy();
  }
  else {
    instance_ = this;
    debug_text_material_ = Cast<UMaterialInterface>(StaticLoadObject(
        UMaterialInterface::StaticClass(), nullptr, 
        TEXT("Material'/haptx/Misc/debug_text_material.debug_text_material'")));
    pcm_ = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
  }
}

void AHxOnScreenLog::Tick(float delta_seconds) {
  Super::Tick(delta_seconds);

  // This should never be true, but better safe than sorry.
  if (instance_ != this) {
    return;
  }

  // Process queued messages.
  for (auto& osm : queued_on_screen_messages_) {
    clearFromScreenInternal(osm.key);
    logToScreenInternal(osm);
  }
  queued_on_screen_messages_.Empty();

  // Update the log.
  if (on_screen_messages_.Num() > 0) {
    // Calculate where the log is located.
    FVector w_log_pos_cm = FVector::ZeroVector;
    FQuat w_log_rot = FQuat::Identity;
    if (IsValid(pcm_)) {
      float l_width_cm, l_height_cm = 0.0f;
      calculateWindowSize(l_width_cm, l_height_cm);
      FVector l_log_pos_cm = FVector(
          text_distance_cm_,
          (left_margin_ - 0.5f) * l_width_cm,
          (0.5f - top_margin_) * l_height_cm);

      FVector w_camera_pos_cm;
      FRotator w_camera_rot;
      pcm_->GetCameraViewPoint(w_camera_pos_cm, w_camera_rot);
      FTransform w_camera_transform = { w_camera_rot, w_camera_pos_cm, FVector::OneVector };
      w_log_pos_cm = w_camera_transform.TransformPosition(l_log_pos_cm);
      w_log_rot = w_camera_rot.Quaternion() * FQuat(FVector::UpVector, float(M_PI));
    }

    int num_lines = 0;
    std::vector<int32> keys_to_remove;
    for (auto& key_value : on_screen_messages_) {
      // Alias for convenience.
      FOnScreenMessage& osm = key_value.Value;

      // If our objects have become invalidated somehow, remove them.
      if (!IsValid(osm.render_actor) ||
          !IsValid(osm.render_actor->GetTextRender())) {
          keys_to_remove.push_back(osm.key);
          continue;
      }

      // Remove the message if it has expired.
      if (osm.duration >= 0.0f && GetWorld()->GetTimeSeconds() - osm.start_time > osm.duration) {
          keys_to_remove.push_back(osm.key);
          continue;
      }

      // Draw the message at the correct line in the log. The text doesn't get spaced properly 
      // without the space in front of the newline. This is probably due to a bug in 
      // UTextRenderComponent.
      // Skip the message if its severity is too low.
      if (display_on_screen_messages_ && min_severity_ <= osm.severity) {
        static const FString space_and_line_break = FString(TEXT(" \n"));
        osm.render_actor->GetTextRender()->SetText(FText::FromString(
            multiplyFString(space_and_line_break, num_lines) + osm.message_formatted));
        num_lines += countCharacter(osm.message_formatted, '\n') + 1;
      }
      else {
        osm.render_actor->GetTextRender()->SetText(FText());
      }

      // Position the actor at the log.
      osm.render_actor->SetActorLocationAndRotation(w_log_pos_cm, w_log_rot);
    }

    // Remove all messages destined to be removed.
    for (auto& key : keys_to_remove) {
      clearFromScreenInternal(key);
    }
  }
}

void AHxOnScreenLog::BeginDestroy() {
  if (instance_ == this) {
    close();
  }
  Super::BeginDestroy();
}

AHxOnScreenLog* AHxOnScreenLog::getInstance(UWorld* world) {
  open(world);
  return instance_;
}

void AHxOnScreenLog::open(UWorld* world) {
  if (!IsValid(instance_) && IsValid(world)) {
    instance_ = world->SpawnActor<AHxOnScreenLog>();
  }
}

void AHxOnScreenLog::close() {
  if (IsValid(instance_)) {
    instance_->Destroy();
  }
  instance_ = nullptr;
}

void AHxOnScreenLog::calculateWindowSize(float &l_width_cm, float &l_height_cm) const {
  if (IsValid(pcm_)) {
    float half_fov_tan = FMath::Tan(0.5f * pcm_->GetFOVAngle());
    l_width_cm = 2.0f * text_distance_cm_ * half_fov_tan;

    float inverse_aspect = pcm_->GetCameraCachePOV().AspectRatio > 0.0f ?
      1.0f / pcm_->GetCameraCachePOV().AspectRatio : 1.0f;
    l_height_cm = inverse_aspect * l_width_cm;
  }
}

FString AHxOnScreenLog::getFormattedOnScreenMessage(const FString& message) const {
  // Break the incoming message into its lines.
  TArray<FString> message_lines =
    UKismetStringLibrary::ParseIntoArray(message, FString(TEXT("\n")));
  TArray<FString> wrapped_lines;
  for (FString& line : message_lines) {
    FString wrapped_line = FString();
    TArray<FString> words = UKismetStringLibrary::ParseIntoArray(line, FString(TEXT(" ")));
    for (FString& word : words) {
      // If, when added, word causes the line to exceed its maximum length start a new line.
      // But only if the line is non-empty; otherwise, the word is solely responsible for the
      // breach.
      const int max_line_length = wrapped_lines.Num() == 0 ? max_line_length_ :
          (int)max_line_length_ - 1;  // Accounts for the space added at the beginning of new lines.
      if (wrapped_line.Len() + word.Len() > max_line_length && wrapped_line.Len() > 0) {
        wrapped_lines.Add(wrapped_line);
        wrapped_line = FString();
      }

      // Add word back to the line.
      wrapped_line += (wrapped_line.Len() > 0 ? TEXT(" ") : TEXT("")) + word;
    }

    // Make sure we don't miss the last line.
    if (wrapped_line.Len() > 0) {
      wrapped_lines.Add(wrapped_line);
    }
  }

  // Recombine lines, adding space for indents.
  return UKismetStringLibrary::JoinStringArray(wrapped_lines, FString(TEXT("\n ")));
}

int32 AHxOnScreenLog::getNextOnScreenMessageKey() {
  // Make sure on_screen_message_key stays between its min and max values (inclusive) to reduce the
  // chance of conflicts with other keys.
  static int32 on_screen_message_key = ON_SCREEN_MESSAGE_MIN_KEY - 1;
  on_screen_message_key++;

  on_screen_message_key =
    (on_screen_message_key - ON_SCREEN_MESSAGE_MIN_KEY) %
    (ON_SCREEN_MESSAGE_MAX_KEY - ON_SCREEN_MESSAGE_MIN_KEY + 1) +
    ON_SCREEN_MESSAGE_MIN_KEY;
  return on_screen_message_key;
}

int32 AHxOnScreenLog::logToScreen(const FString& message, float duration, FColor color, 
    int32 message_key) {
  return logToScreenWithSeverity(message, EOnScreenMessageSeverity::INFO, duration, color, 
      message_key);
}

int32 AHxOnScreenLog::logToScreenWithSeverity(const FString& message, 
    EOnScreenMessageSeverity severity, float duration, FColor color, int32 message_key) {
  // If a valid message key isn't provided, generate one.
  if (message_key < 0) {
    message_key = getNextOnScreenMessageKey();
  }
  
  // Queue the message.
  FOnScreenMessage osm;
  osm.message_raw = message;
  osm.severity = severity;
  osm.color = color;
  osm.duration = duration;
  osm.key = message_key;
  queued_on_screen_messages_.Add(osm);

  return message_key;
}

void AHxOnScreenLog::clearFromScreen(int32 message_key) {
  // Remove any queued messages that match this key.
  std::vector<int> indices_to_remove;
  for (int i = queued_on_screen_messages_.Num() - 1; i > -1; i--) {
    if (queued_on_screen_messages_[i].key == message_key) {
      indices_to_remove.push_back(i);
    }
  }
  for (auto& index : indices_to_remove) {
    queued_on_screen_messages_.RemoveAt(index);
  }

  // Remove any logged messages that match this key.
  if (IsValid(instance_)) {
    instance_->clearFromScreenInternal(message_key);
  }
}

void AHxOnScreenLog::logToScreenInternal(FOnScreenMessage& osm) {
  if (osm.message_raw.Len() > 0) {
    on_screen_messages_.Emplace(osm.key, osm);
    FOnScreenMessage* osm_found = on_screen_messages_.Find(osm.key);
    if (osm_found != nullptr) {
      osm_found->message_formatted = getFormattedOnScreenMessage(osm.message_raw);
      osm_found->start_time = GetWorld()->GetTimeSeconds();

      osm_found->render_actor = GetWorld()->SpawnActor<ATextRenderActor>();
      if (!IsValid(osm_found->render_actor)) {
        UE_LOG(LogTemp, Error, TEXT("Could not create render actor."))
        return;
      }

      UTextRenderComponent* component = osm_found->render_actor->GetTextRender();
      if (!IsValid(component)) {
        UE_LOG(LogTemp, Error, TEXT("Could not get render component from render actor."))
        return;
      }

      component->bAlwaysRenderAsText = true;
      component->SetVerticalAlignment(EVRTA_TextTop);
      component->SetWorldSize(text_size_);

      if (debug_text_material_ == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Could not find debug text material."));
        return;
      }

      UMaterialInstanceDynamic* mat_inst = UMaterialInstanceDynamic::Create(debug_text_material_,
        osm_found->render_actor);
      if (mat_inst == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Could not create dynamic instance of debug_text_material."));
        return;
      }
      mat_inst->SetVectorParameterValue(debug_text_material_color_param_, osm_found->color);
      component->SetMaterial(0, mat_inst);
    } 
  }
}

void AHxOnScreenLog::clearFromScreenInternal(int32 key) {
  FOnScreenMessage* existing_message = on_screen_messages_.Find(key);
  if (existing_message != nullptr) {
    if (IsValid(existing_message->render_actor)) {
      existing_message->render_actor->Destroy();
    }
    on_screen_messages_.Remove(key);
  }
}
