// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Engine/TextRenderActor.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Classes/Kismet/KismetStringLibrary.h>
#include <Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h>
#include "hx_on_screen_log.generated.h"

//! The min value for on-screen warning and error message keys (arbitrarily chosen).
#define ON_SCREEN_MESSAGE_MIN_KEY (INT_MAX / 3)
//! The max value for on-screen warning and error message keys. Once this value is exceeded
//! keys loop back to the min value.
#define ON_SCREEN_MESSAGE_MAX_KEY (ON_SCREEN_MESSAGE_MIN_KEY + 1000)

//! @brief The severity level of an on-screen Log message.
//!
//! Must be enumerated in increasing order of severity.

// The severity level of an on-screen Log message.
UENUM(BlueprintType)
enum class EOnScreenMessageSeverity : uint8 {
  INFO        UMETA(DisplayName = "Info"),     //!< General message.
  WARNING     UMETA(DisplayName = "Warning"),  //!< Warning message.
  ERROR       UMETA(DisplayName = "Error")     //!< Error message.
};

//! @brief Represents an on-screen Log message.
//!
//! This struct only gets used inside AHxOnScreenLog.

// Represents an on-screen Log message.
USTRUCT()
struct FOnScreenMessage {
  GENERATED_BODY()

  //! The raw message that was logged.
  FString message_raw;

  //! The severity of the message being logged.
  EOnScreenMessageSeverity severity;

  //! The text being displayed on-screen.
  FString message_formatted;

  //! The color to display the text with.
  FColor color;

  //! The time [s] at which the message was generated.
  float start_time;

  //! The duration [s] of the message.
  float duration;

  //! The actor rendering this message.
  UPROPERTY()
  ATextRenderActor* render_actor;

  //! The key/ID for this message. Only one message with a given key can display at a time
  //! and will replace existing messages with the same key. If negative then a unique key will be
  //! chosen automatically.
  int32 key;
};

//! Responsible for creating and managing on-screen Log messages.
UCLASS(ClassGroup = (HaptX), Blueprintable)
class HAPTX_API AHxOnScreenLog : public AActor {
  GENERATED_UCLASS_BODY()

public:
  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Called every frame.
  //!
  //! @param delta_seconds The time that has passed since last frame in seconds.
  virtual void Tick(float delta_seconds) override;

  //! Called when game ends after everything has their EndPlay()s called.
  virtual void BeginDestroy() override;

  //! Get the singleton instance.
  static AHxOnScreenLog* getInstance(UWorld* world);

  //! Pass a world pointer to open the on-screen log.
  //!
  //! @param world The world to open in.
  static void open(UWorld* world);

  //! Close the on-screen log.
  static void close();

  //! @brief Logs an on-screen message.
  //!
  //! If the AHxCoreActor singleton's "Min Severity" property is not configured to display messages
  //! of severity EOnScreenMessageSeverity::INFO then this message will not be displayed. Call
  //! AHxOnScreenLog::logToScreenWithSeverity() instead.
  //!
  //! @param message The message text.
  //! @param duration The duration [s] the message is to be displayed on-screen. Durations of less
  //! than zero are treated as infinite.
  //! @param color The color to display this message on-screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key the message was logged with, or a negative number if something went wrong.

  // Logs an on-screen message. If the AHxCoreActor singleton's "Min Severity" property is not
  // configured to display messages of severity "INFO" then this message will not be displayed.
  // Call logToScreenWithSeverity() instead.
  UFUNCTION(BlueprintCallable)
  static int32 logToScreen(const FString& message, float duration, FColor color,
      int32 message_key = -1);

  //! Logs an on-screen message.
  //!
  //! @param message The message text.
  //! @param severity The severity of the message.
  //! @param duration The duration [s] the message is to be displayed on-screen. Durations of less
  //! than zero are treated as infinite.
  //! @param color The color to display this message on-screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key the message was logged with, or a negative number if something went wrong.

  // Logs an on-screen message.
  UFUNCTION(BlueprintCallable)
  static int32 logToScreenWithSeverity(const FString& message, EOnScreenMessageSeverity severity,
      float duration, FColor color, int32 message_key = -1);

  //! Clears an on-screen message by key.
  //!
  //! @param message_key The key of the message to clear.

  // Clears an on-screen message.
  UFUNCTION(BlueprintCallable)
  static void clearFromScreen(int32 message_key);

  //! @brief True to enable on-screen logging.
  //!
  //! Determines whether messages get rendered on-screen.
  bool display_on_screen_messages_;

  //! The minimum level of severity an on-screen message must have to be displayed.
  EOnScreenMessageSeverity min_severity_;

  //! The size of the text.
  float text_size_;

  //! The max number of characters in a line.
  uint8 max_line_length_;

  //! The fraction of horizontal screen space reserved for the left margin.
  float left_margin_;

  //! The fraction of vertical screen space reserved for the top margin.
  float top_margin_;

  //! The forward distance from the camera that the log renders.
  float text_distance_cm_;

private:
  //! Calculates the width and height of the view frustum at the distance the log occupies.
  //!
  //! @param [out] l_width_cm The calculated width of the screen.
  //! @param [out] l_height_cm The calculated height of the screen.
  void calculateWindowSize(float& l_width_cm, float & l_height_cm) const;

  //! Formats a message with line indents and wrapping.
  //!
  //! @param message The line to format.
  //!
  //! @returns The formatted line.
  FString getFormattedOnScreenMessage(const FString& message) const;

  //! @brief Gets the next key to use for displaying an on-screen message.
  //!
  //! Loops around once the max key is passed.
  //!
  //! @returns The key to use for the new on-screen message.
  static int32 getNextOnScreenMessageKey();

  //! Gets called by AHxOnScreenLog::logToScreen().
  //!
  //! @param osm The message to be logged.
  void logToScreenInternal(FOnScreenMessage& osm);

  //! Clears an existing message from the screen.
  //!
  //! @param key The key of the message to clear.
  void clearFromScreenInternal(int32 key);

  //! The player camera manager informing this class.
  UPROPERTY()
  APlayerCameraManager* pcm_;

  //! The material to use to draw on-screen text.
  UPROPERTY()
  UMaterialInterface* debug_text_material_;

  //! The name of the material parameter used to define text color.
  FName debug_text_material_color_param_;

  //! All the messages being displayed on-screen.
  UPROPERTY()
  TMap<int32, FOnScreenMessage> on_screen_messages_;

  //! A list of messages that have yet to be parsed by the singleton.
  static TArray<FOnScreenMessage> queued_on_screen_messages_;

  //! The singleton instance.
  static AHxOnScreenLog* instance_;

};
