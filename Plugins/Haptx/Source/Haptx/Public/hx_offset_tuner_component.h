// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Camera/CameraComponent.h>
#include <Runtime/Engine/Classes/Components/ActorComponent.h>
#include <Runtime/Engine/Classes/Components/InputComponent.h>
#include <Runtime/Engine/Classes/GameFramework/SaveGame.h>
#include "hx_offset_tuner_component.generated.h"

//! An enumeration for the coordinate frame that this player translates with respect to.

// An enumeration for the coordinate frame that this player translates with respect to.
UENUM(BlueprintType)
enum class ETranslationFrame : uint8 {
  LOCAL     UMETA(DisplayName="Local"),   //!< Local space
  GLOBAL    UMETA(DisplayName="Global"),  //!< World space
  CAMERA    UMETA(DisplayName="Camera")   //!< Camera space
};

//! An enumeration for the coordinate frame that this player rotates with respect to.

// An enumeration for the coordinate frame that this player rotates with respect to.
UENUM(BlueprintType)
enum class ERotationFrame : uint8 {
  LOCAL     UMETA(DisplayName = "Local"),   //!< Local space
  GLOBAL    UMETA(DisplayName = "Global"),  //!< World space
};

//! A save game for storing persistent locomotion offsets.
UCLASS(ClassGroup = (Haptx))
class HAPTX_API UHxOffsetSaveGame : public USaveGame {
  GENERATED_BODY()

public:
  //! Default constructor.
  UHxOffsetSaveGame();

  //! Saved offset relative to initial transform.
  UPROPERTY()
  FTransform local_offset;
};

//! @brief Responsible for finely tuning the player's starting location and rotation in the game 
//! world.
//!
//! @ingroup group_unreal_plugin

// Responsible for finely tuning the player's starting location and rotation in the game world.
UCLASS( ClassGroup=(Haptx), meta=(BlueprintSpawnableComponent) )
class HAPTX_API UHxOffsetTunerComponent : public UActorComponent {
	GENERATED_BODY()

public:
  
  //! Initializes default values and grabs necessary references.
  UHxOffsetTunerComponent(const FObjectInitializer &ObjectInitializer);

  //! Applies the offset loaded by the client to the player.

  // Applies the offset loaded by the client to the player.
  UFUNCTION(BlueprintCallable)
  void clientApplyLocalOffset();

  //! The coordinate frame the player translates in.

  // The coordinate frame the player translates in.
  UPROPERTY(EditAnywhere, DisplayName = "Translation Frame")
  ETranslationFrame server_translation_frame_;

  //! The speed [cm/s] at which the player translates in each independent direction.

  // The speed [cm/s] at which the player translates in each independent direction.
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f, ClampMax = 1000.0f),
      DisplayName = "Translation Speed [cm/s]")
  float server_translation_speed_cm_s_;

  //! The coordinate frame the player rotates in.

  // The coordinate frame the player rotates in.
  UPROPERTY(EditAnywhere, DisplayName = "Rotation Frame")
  ERotationFrame server_rotation_frame_;

  //! The speed [deg/s] at which the player rotates in each independent direction.

  // The speed [deg/s] at which the player rotates in each independent direction.
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f, ClampMax = 360.0f),
      DisplayName = "Rotation Speed [deg/s]")
  float server_rotation_speed_deg_s_;

  //! The name of the input axis that controls forward motion.

  // The name of the input axis that controls forward motion.
  UPROPERTY(EditAnywhere, DisplayName = "Forward Axis")
  FName client_forward_axis_;

  //! The name of the input axis that controls rightward motion.

  // The name of the input axis that controls rightward motion.
  UPROPERTY(EditAnywhere, DisplayName = "Right Axis")
  FName client_right_axis_;

  //! The name of the input axis that controls upward motion.

  // The name of the input axis that controls upward motion.
  UPROPERTY(EditAnywhere, DisplayName = "Up Axis")
  FName client_up_axis_;

  //! The name of the input axis that controls pitch motion.

  // The name of the input axis that controls pitch motion.
  UPROPERTY(EditAnywhere, DisplayName = "Pitch Axis")
  FName client_pitch_axis_;

  //! The name of the input axis that controls yaw motion.

  // The name of the input axis that controls yaw motion.
  UPROPERTY(EditAnywhere, DisplayName = "Yaw Axis")
  FName client_yaw_axis_;

  //! The name of the input axis that controls roll motion.

  // The name of the input axis that controls roll motion.
  UPROPERTY(EditAnywhere, DisplayName = "Roll Axis")
  FName client_roll_axis_;

  //! The name of the input action that resets the offset.

  // The name of the input action that resets the offset.
  UPROPERTY(EditAnywhere, DisplayName = "Reset Action")
  FName client_reset_action_;

  //! Save a different offset for each level.

  // Save a different offset for each level.
  UPROPERTY(EditAnywhere, DisplayName = "Enable Level Specific Offsets")
  bool client_enable_level_specific_offsets_;

private:
  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Called every frame.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
      FActorComponentTickFunction * ThisTickFunction) override;

  //! Called when the game ends.
  //!
  //! @param EndPlayReason Why the game ended.
  virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

  //! Applies an offset received from the client.
  //!
  //! @param l_translation_cm The translation to apply.
  //! @param l_rotation The rotation to apply.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverApplyLocalOffset(const FVector& l_translation_cm, const FQuat& l_rotation);

  //! Binds input commands.
  //!
  //! @returns True if all input was successfully bound.
  bool clientBindInput();

  //! Moves the player forward.
  //!
  //! @param axis The current value of the forward axis.
  UFUNCTION(BlueprintCallable)
  void clientTranslateForward(float axis);

  //! Moves the player right.
  //!
  //! @param axis The current value of the right axis.
  UFUNCTION(BlueprintCallable)
  void clientTranslateRight(float axis);

  //! Moves the player up.
  //!
  //! @param axis The current value of the up axis.
  UFUNCTION(BlueprintCallable)
  void clientTranslateUp(float axis);

  //! Pitches the player.
  //!
  //! @param axis The current value of the pitch axis.
  UFUNCTION(BlueprintCallable)
  void clientRotatePitch(float axis);

  //! Yaws the player.
  //!
  //! @param axis The current value of the yaw axis.
  UFUNCTION(BlueprintCallable)
  void clientRotateYaw(float axis);

  //! Rolls the player.
  //!
  //! @param axis The current value of the roll axis.
  UFUNCTION(BlueprintCallable)
  void clientRotateRoll(float axis);

  //! Reset the player to its initial transform.
  UFUNCTION(BlueprintCallable)
    void clientResetOffset();

  //! @copydoc UHxOffsetTunerComponent::clientTranslateForward()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverTranslateForward(float axis);

  //! @copydoc UHxOffsetTunerComponent::clientTranslateRight()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverTranslateRight(float axis);

  //! @copydoc UHxOffsetTunerComponent::clientTranslateUp()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverTranslateUp(float axis);

  //! @copydoc UHxOffsetTunerComponent::clientRotatePitch()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverRotatePitch(float axis);

  //! @copydoc UHxOffsetTunerComponent::clientRotateYaw()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverRotateYaw(float axis);

  //! @copydoc UHxOffsetTunerComponent::clientRotateRoll()
  UFUNCTION(Server, Reliable, WithValidation)
  void serverRotateRoll(float axis);

  //! Reset our player to its initial transform.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverResetOffset();

  //! Gets the transform of a given coordinate frame.
  //!
  //! @param frame The frame.
  //! @returns The transform.
  FTransform serverGetCoordinateFrame(ETranslationFrame frame);

  //! @copydoc UHxOffsetTunerComponent::serverGetCoordinateFrame(ETranslationFrame)
  FTransform serverGetCoordinateFrame(ERotationFrame frame);

  //! Translates our player by a given delta.
  //!
  //! @param w_delta_cm The world space position delta.
  void serverTranslate(const FVector& w_delta_cm);

  //! Sets the offset that gets saved on-disk on the client's side.
  //!
  //! @param l_translation_cm The offset that gets saved.
  UFUNCTION(Client, Reliable)
  void clientUpdateSaveGameTranslation(const FVector& l_translation_cm);

  //! Rotates our player by a given delta.
  //!
  //! @param w_delta The world space rotation delta.
  void serverRotate(const FQuat& w_delta);

  //! Sets the offset that gets saved on-disk on the client's side.
  //!
  //! @param l_rotation The offset that gets saved.
  UFUNCTION(Client, Reliable)
  void clientUpdateSaveGameRotation(const FQuat& l_rotation);

  //! The initial transform of our owner.
  FTransform server_initial_transform_;

  //! The local offset.
  FTransform server_local_offset_;

  //! The input state for linear velocity (as known by the client).
  FVector client_velocity_input_;

  //! The input state for linear velocity (as known by the server).
  FVector server_velocity_input_;

  //! The input state for angular velocity (as known by the client).
  FVector client_angular_velocity_input_;

  //! The input state for angular velocity (as known by the server).
  FVector server_angular_velocity_input_;

  //! The name of the save slot where we put our local offset.
  FString client_save_slot_;

  //! The save game that holds our local offset.
  UPROPERTY()
  UHxOffsetSaveGame *client_save_game_;

  //! For locomotion based on camera frame.
  UPROPERTY()
  UCameraComponent *server_camera_;

  //! @brief Prevents the offset from being saved twice in subsequent calls to EndPlay().
  //!
  //! This was corrupting the save file for some reason.
  bool client_has_saved_;

  //! Whether this component is authoritative.
  bool is_authoritative_;

  //! Whether this component is locally controlled.
  bool is_locally_controlled_;

  //! Whether input has been successfully bound.
  bool client_is_input_bound_;
};
