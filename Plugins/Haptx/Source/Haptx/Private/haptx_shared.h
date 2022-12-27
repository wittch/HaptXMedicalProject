// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <list>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <Runtime/Engine/Classes/Components/SkeletalMeshComponent.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>
#include <Runtime/Engine/Classes/Engine/World.h>
#include <Runtime/Engine/Classes/GameFramework/Pawn.h>
#include <Runtime/Engine/Classes/GameFramework/PlayerController.h>
#include <Runtime/Engine/Classes/GameFramework/PlayerState.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h>
#include <Runtime/Engine/Classes/PhysicalMaterials/PhysicalMaterial.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsSettings.h>
#include <HaptxApi/enum.h>
#include <HaptxApi/transform.h>
#include <Haptx/Public/ihaptx.h>

DECLARE_STATS_GROUP_IF_PROFILING(TEXT("HaptxShared"), STATGROUP_HaptxShared, STATCAT_Advanced)

//! Home environment variable.
#define HOME_ENV_VAR "HAPTX_SDK_HOME"
//! Fallback home directory.
#define FALLBACK_HOME "C:/Program Files/HaptX/SDK"
//! HaptX brand palette.
#define HAPTX_CHARCOAL FColor(68, 68, 68)
//! HaptX brand palette.
#define HAPTX_ORANGE FColor(255, 121, 0)
//! HaptX brand palette.
#define HAPTX_GREEN FColor(166, 183, 0)
//! HaptX brand palette.
#define HAPTX_TEAL FColor(0, 170, 188)
//! HaptX brand palette.
#define HAPTX_YELLOW FColor(242, 161, 0)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_BLACK FColor(26, 26, 26)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_WHITE FColor(229, 229, 229)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_GRAY FColor(127, 127, 127)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_PURPLE_OR_TEAL FColor(0, 255, 255)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_RED_OR_GREEN FColor(255, 0, 0)
//! @brief Debug color palette.
//!
//! Designed to be color blind friendly. For colors that include an "or" the palette is valid if
//! the value matches either of the colors.
#define DEBUG_BLUE_OR_YELLOW FColor(0, 0, 255)

//! Convert a std::string to an FName.
#define STRING_TO_FNAME(string) (FName(*FString(UTF8_TO_TCHAR((string).c_str()))))
//! Convert a char array to an FName.
#define CSTR_TO_FNAME(cstr) (FName(*FString(UTF8_TO_TCHAR(cstr))))
//! Convert an FName to a char array.
#define FNAME_TO_CSTR(fname) (TCHAR_TO_UTF8(*(fname).ToString().ToLower()))
//! Convert a std::string to an FString.
#define STRING_TO_FSTRING(string) (FString(UTF8_TO_TCHAR((string).c_str())))
//! Convert a char array to an FString.
#define CSTR_TO_FSTRING(cstr) (FString(UTF8_TO_TCHAR(cstr)))
//! Convert an FString to a char array.
#define FSTRING_TO_CSTR(fstring) (TCHAR_TO_UTF8(*fstring))
//! Convert a HaptxApi::HaptxName to an FName.
#define HAPTX_NAME_TO_FNAME(haptx_name) STRING_TO_FNAME(haptx_name.getText())

//! Multiply to convert radians to revolutions.
#define RAD_TO_REV 0.15915494f
//! Multiply to convert radians to revolutions.
#define REV_TO_RAD (2.f * float(M_PI))
//! Multiply to convert degrees to revolutions.
#define DEG_TO_REV 0.00277778f
//! Multiply to convert degrees to revolutions.
#define DEG_TO_RAD 0.0174533f

//! Gestures in the SimulatedAnimator (matches HaptxApi::Gestures enum).

// Gestures in the SimulatedAnimator (matches Gestures enum).
UENUM(BlueprintType)
enum class EGesture : uint8 {
  POKE                UMETA(DisplayName = "Poke"),             //!< Poke.
  PRECISION_GRASP     UMETA(DisplayName = "Precision Grasp"),  //!< Precision grasp (pinch).
  POWER_GRASP         UMETA(DisplayName = "Power Grasp"),      //!< Power grasp.
  FIST                UMETA(DisplayName = "Fist")              //!< Fist.
};

//! Left or right?

// Left or right?
UENUM(BlueprintType)
enum class ERelativeDirection : uint8 {
  LEFT     UMETA(DisplayName = "Left"),   //!< Left.
  RIGHT    UMETA(DisplayName = "Right"),  //!< Right.
};

//! For enum values being used as array indices or lengths.
typedef unsigned long long EnumIdx;

//! Convert a HaptxApi::Vector2D to an FVector2D.
#define VECTOR2D_TO_FVECTOR2D(v2d) (FVector2D((v2d).x_, (v2d).y_))
//! Convert an FVector2D to a HaptxApi::Vector2D.
#define FVECTOR2D_TO_VECTOR2D(fv) (HaptxApi::Vector2D((fv).X, (fv).Y))
//! Convert a HaptxApi::Vector3D to an FVector.
#define VECTOR3D_TO_FVECTOR(v3d) (FVector((v3d).x_, (v3d).y_, (v3d).z_))
//! Convert an FVector to a HaptxApi::Vector3D.
#define FVECTOR_TO_VECTOR3D(fv) (HaptxApi::Vector3D((fv).X, (fv).Y, (fv).Z))
//! Convert a HaptxApi::Quaternion to an FQuat.
#define QUATERNION_TO_FQUAT(q) (FQuat((q).i_, (q).j_, (q).k_, (q).r_))
//! Convert an FQuat to a HaptxApi::Quaternion.
#define FQUAT_TO_QUATERNION(fq) (HaptxApi::Quaternion((fq).W, (fq).X, (fq).Y, (fq).Z))

DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HaptxShared: Checking rotation normalization"),
    STAT_normalizedRotationChecks, STATGROUP_HaptxShared)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HaptxShared: Normalizing rotations"),
    STAT_normalizeRotations, STATGROUP_HaptxShared)

//! Relative direction substitution pattern.
#define REL_DIR_PATTERN L"{rel_dir}"

//! @brief Convert a left-handed FVector to a right-handed HaptxApi::Vector3D.
//!
//! Does not assume any units and does not do any unit conversions.
//!
//! @param unreal_vector The vector to convert.
//!
//! @returns The converted vector.
static inline HaptxApi::Vector3D hxFromUnrealVector(const FVector& unreal_vector) {
  return FVECTOR_TO_VECTOR3D(unreal_vector).switchHandedness(HaptxApi::Axis::Y);
}

//! @brief Convert a right-handed HaptxApi::Vector3D to a left-handed FVector.
//!
//! Does not assume any units and does not do any unit conversions.
//!
//! @param hx_vector The vector to convert.
//!
//! @returns The converted vector.
static inline FVector unrealFromHxVector(const HaptxApi::Vector3D& hx_vector) {
  return VECTOR3D_TO_FVECTOR(hx_vector.switchHandedness(HaptxApi::Axis::Y));
}

//! Convert a value with units that are order one in length from Unreal units [cm.../...]
//! to HaptX units [m.../...].
//!
//! @param unreal_value The Unreal value [cm.../...] to convert.
//!
//! @returns The converted value [m.../...].
static inline float hxFromUnrealLength(float unreal_value) {
  return unreal_value / 100.0f;
}

//! Convert a value with units that are order one in length from HaptX units [m.../...]
//! to Unreal units [cm.../...].
//!
//! @param hx_value The HaptX value [m.../...] to convert.
//!
//! @returns The converted value [cm.../...].
static inline float unrealFromHxLength(float hx_value) {
  return 100.0f * hx_value;
}

//! Convert a left-handed FVector with units that are order one in length [cm.../...][3]
//! to a right-handed HaptxApi::Vector3D [m.../...][3].
//!
//! @param unreal_value The left-handed vector [cm.../...][3] to convert.
//!
//! @returns The converted right-hand vector [m.../...][3].
static inline HaptxApi::Vector3D hxFromUnrealLength(const FVector& unreal_value) {
  return hxFromUnrealVector(unreal_value / 100.0f);
}

//! Convert a right-handed HaptxApi::Vector3D with units that are order one in 
//! length [m.../...][3] to a left-handed FVector [cm.../...][3].
//!
//! @param hx_value The right-handed vector [m.../...][3] to convert.
//!
//! @returns The converted left-hand vector [cm.../...][3].
static inline FVector unrealFromHxLength(const HaptxApi::Vector3D& hx_value) {
  return 100.0f * unrealFromHxVector(hx_value);
}

//! Convert a left-handed FQuat to a right-handed HaptxApi::Quaternion.
//!
//! @param unreal_quaternion The quaternion to convert.
//!
//! @returns The converted quaternion.
static inline HaptxApi::Quaternion hxFromUnreal(const FQuat& unreal_quaternion) {
  return FQUAT_TO_QUATERNION(unreal_quaternion).switchHandedness(HaptxApi::Axis::Y);
}

//! Convert a right-handed HaptxApi::Quaternion to a left-handed FQuat.
//!
//! @param hx_quaternion The quaternion to convert.
//!
//! @returns The converted quaternion.
static inline FQuat unrealFromHx(const HaptxApi::Quaternion& hx_quaternion) {
  FQuat unreal_quaternion = QUATERNION_TO_FQUAT(hx_quaternion.switchHandedness(HaptxApi::Axis::Y));
  return unreal_quaternion;
}

//! Convert a left-handed FRotator to a right-handed HaptxApi::Quaternion.
//!
//! @param unreal_rotation The rotation to convert.
//!
//! @returns The converted quaternion.
static inline HaptxApi::Quaternion hxFromUnrealRotation(const FRotator& unreal_rotation) {
  return hxFromUnreal(unreal_rotation.Quaternion());
}

//! Convert a right-handed HaptxApi::Quaternion to a left-handed FRotator.
//!
//! @param hx_quaternion The quaternion to convert.
//!
//! @returns The converted rotation.
static inline FRotator unrealFromHxRotation(const HaptxApi::Quaternion& hx_quaternion) {
  return FRotator(unrealFromHx(hx_quaternion));
}

//! Convert an Unreal 3D scale vector to a HaptX 3D scale vector.
//!
//! @param unreal_scale The scale to convert.
//!
//! @returns The converted scale.

static inline HaptxApi::Vector3D hxFromUnrealScale(const FVector& unreal_scale) {
  return FVECTOR_TO_VECTOR3D(unreal_scale);
}

//! Convert an HaptX 3D scale vector to an Unreal scale vector.
//!
//! @param hx_scale The scale to convert.
//!
//! @returns The converted scale.

static inline FVector unrealFromHxScale(const HaptxApi::Vector3D& hx_scale) {
  return VECTOR3D_TO_FVECTOR(hx_scale);
}

//! Convert a HaptxApi::Transform to an FTransform.
//!
//! @param hx_transform The transform to convert.
//!
//! @returns The converted transform.
static inline FTransform unrealFromHx(const HaptxApi::Transform hx_transform) {
  return FTransform(
      unrealFromHxRotation(hx_transform.getRotation()),
      unrealFromHxLength(hx_transform.getTranslation()),
      unrealFromHxScale(hx_transform.getScale()));
}

//! Convert an FTransform to a HaptxApi::Transform.
//!
//! @param unreal_transform The transform to convert.
//!
//! @returns The converted transform.
static inline HaptxApi::Transform hxFromUnreal(const FTransform unreal_transform) {
  return HaptxApi::Transform(
      hxFromUnrealLength(unreal_transform.GetLocation()),
      hxFromUnreal(unreal_transform.GetRotation()),
      hxFromUnrealScale(unreal_transform.GetScale3D()));
}

//! Convert a left-handed angular velocity FVector (rotation axis scaled by angular speed in
//! deg/s) to a right-handed angular velocity HaptxApi::Vector3D (rotation axis scaled by angular
//! speed in rad/s).
//!
//! @param unreal_angular_velocity The left-handed angular velocity vector to convert.
//!
//! @returns The converted right-handed angular velocity vector.
static inline HaptxApi::Vector3D hxFromUnrealAngularVelocity(
    const FVector& unreal_angular_velocity) {
  // The negation on the following line is very necessary for angular velocities!
  return -hxFromUnrealVector(FVector::DegreesToRadians(unreal_angular_velocity));
}

//! Convert a right-handed angular velocity HaptxApi::Vector3D (rotation axis scaled by
//! angular speed in rad/s) to a left-handed angular velocity FVector (rotation axis scaled by
//! angular speed in deg/s).
//!
//! @param hx_angular_velocity The right-handed angular velocity vector to convert.
//!
//! @returns The converted left-handed angular velocity vector.
static inline FVector unrealFromHxAngularVelocity(const HaptxApi::Vector3D& hx_angular_velocity) {
  // The negation on the following line is very necessary for angular velocities!
  return -FVector::RadiansToDegrees(unrealFromHxVector(hx_angular_velocity));
}

//! Substitutes all instances of REL_DIR_PATTERN with the given relative direction.
//!
//! @param input The string to perform substitutions on.
//! @param rel_dir The relative direction to substitute.
//! @returns A copy of @p input with substitutions in place.
static inline FString substituteRelDir(const FString input, ERelativeDirection rel_dir) {
  return input.Replace(REL_DIR_PATTERN, rel_dir == ERelativeDirection::LEFT ? L"LEFT" : L"RIGHT",
      ESearchCase::IgnoreCase);
}

//! Get the average of an FVector's X, Y, and Z components.
//!
//! @param v The vector whose components to average.
//!
//! @returns The average of @p v's components.
static inline float componentAverage(FVector v) {
  return (v.X + v.Y + v.Z) / 3.0f;
}

//! Copies UPhysicalMaterial settings from one instance to another.
//!
//! @param from The instance to copy from.
//! @param to The instance to copy to.
static inline void copyPhysicalMaterialSettings(UPhysicalMaterial* from, UPhysicalMaterial* to) {
  if (from && to) {
    to->Friction = from->Friction;
    to->FrictionCombineMode = from->FrictionCombineMode;
    to->bOverrideFrictionCombineMode = from->bOverrideFrictionCombineMode;
    to->Restitution = from->Restitution;
    to->RestitutionCombineMode = from->RestitutionCombineMode;
    to->bOverrideRestitutionCombineMode = from->bOverrideRestitutionCombineMode;
    to->Density = from->Density;
    to->RaiseMassToPower = from->RaiseMassToPower;
    to->DestructibleDamageThresholdScale = from->DestructibleDamageThresholdScale;
    to->SurfaceType = from->SurfaceType;
    to->TireFrictionScale = from->TireFrictionScale;
    to->TireFrictionScales = from->TireFrictionScales;
  }
}

//! Performs all the steps necessary to properly destroy a physics constraint.
//!
//! @param pcc The physics constraint to destroy.
static inline void destroyPhysicsConstraintComponent(UPhysicsConstraintComponent* pcc) {
  if (IsValid(pcc) && pcc->ConstraintInstance.ConstraintHandle.IsValid()) {
    pcc->BreakConstraint();
    pcc->DestroyComponent();
  }
}

//! Gets the FBodyInstance matching a given component/bone.
//!
//! @param comp The component whose FBodyInstance to get.
//! @param bone The bone whose FBodyInstance to get.
//! @returns The FBodyInstance matching a given component/bone combination. If a weld parent exists
//! it gets returned instead.
static inline FBodyInstance* getBodyInstance(UPrimitiveComponent* comp, FName bone) {
  if (!IsValid(comp)) return nullptr;

  FBodyInstance* body_inst = comp->GetBodyInstance(bone, false);
  if (body_inst != nullptr) {
    return body_inst->WeldParent != nullptr ? body_inst->WeldParent : body_inst;
  }
  else {
    return nullptr;
  }
}

//! Gets the physical material associated with the given component/bone.
//!
//! @param comp The component whose physical material to get.
//! @param bone The bone whose physical material to get.
//! @returns The physical material associated with the given component/bone.
static inline UPhysicalMaterial* getPhysicalMaterial(UPrimitiveComponent* comp, FName bone) {
  if (!IsValid(comp)) return nullptr;

  FBodyInstance* body_inst = comp->GetBodyInstance(bone, false);
  if (body_inst != nullptr) {
    return body_inst->GetSimplePhysicalMaterial();
  }
  else {
    return nullptr;
  }
}

//! Converts an ERelativeDirection to an HaptxApi::RelativeDirection.
//!
//! @param in_dir The direction to convert.
//!
//! @returns The converted direction.
static HaptxApi::RelativeDirection relDirFromERelDir(ERelativeDirection in_dir) {
  switch (in_dir) {
    case ERelativeDirection::LEFT: {
      return HaptxApi::RD_LEFT;
    }
    case ERelativeDirection::RIGHT: {
      return HaptxApi::RD_RIGHT;
    }
    default: {
      return HaptxApi::RD_LAST;
    }
  }
}

//! Counts the occurrences of a character in a string.
//!
//! @param in_string The string to count in.
//! @param char_to_count The character whose occurrences to count.
//! @returns The number of occurrences in the string.
static inline uint32 countCharacter(FString& in_string, TCHAR char_to_count) {
  uint32 count = 0;
  for (int i = 0; i < in_string.Len(); i++) {
    if (in_string[i] == char_to_count) {
      count++;
    }
  }
  return count;
}

//! Creates an FString consisting of N copies of the given character.
//!
//! @param in_char The character to construct with.
//! @param count The number of times the character is repeated.
//! @returns An FString consisting of N copies of the given character.
static inline FString multiplyTChar(TCHAR in_char, int32 count) {
  FString string;
  string.Reset(count);
  for (int i = 0; i < count; i++) {
    string.AppendChar(in_char);
  }
  return string;
}

//! Creates an FString consisting of N copies of the given string.
//!
//! @param in_string The string to construct with.
//! @param count The number of times the string is repeated.
//! @returns An FString consisting of N copies of the given string.
static inline FString multiplyFString(const FString& in_string, int32 count) {
  FString string;
  string.Reset(count * in_string.Len());
  for (int i = 0; i < count; i++) {
    string.Append(in_string);
  }
  return string;
}

#define INVALID_BODY_INSTANCE_ID INT64_MAX
//! @brief Get an ID that is unique to a given FBodyInstance.
//!
//! The least significant 32 bits store the FBodyInstance's index relative to its parent component,
//! and the most significant 32 bits store the parent component's unique ID.
//!
//! @param body_inst A pointer to the FBodyInstance whose ID to get.
//! @returns The ID of the FBodyInstance.
static inline int64_t getBodyInstanceId(FBodyInstance* body_inst){
  if (body_inst == nullptr || !IsValid(body_inst->OwnerComponent.Get())) {
    return INVALID_BODY_INSTANCE_ID;
  }

  return static_cast<int64_t>(
      static_cast<uint64_t>(body_inst->OwnerComponent.Get()->GetUniqueID()) << 32 |
      static_cast<uint32_t>(body_inst->InstanceBodyIndex));
}

//! Get the pawn using an actor (if there is one).
//!
//! @param actor The actor to check.
//! @returns The pawn using the actor (if there is one), nullptr otherwise.
static inline APawn* getPawn(AActor* actor) {
  TSet<AActor*> actors_checked;  // Infinite loop guard. Some actors are their own owner.
  AActor* actor_it = actor;
  while (IsValid(actor_it) && !actors_checked.Contains(actor_it)) {
    APawn* pawn = Cast<APawn>(actor_it);
    if (IsValid(pawn)) {
      return pawn;
    }

    actors_checked.Add(actor_it);
    actor_it = actor->GetOwner();
  }
  
  return nullptr;
}

//! Determine whether the pawn in question is controlled by the person at the calling
//! machine.
//!
//! @param pawn The pawn in question.
//! @returns True if the pawn is controlled by the local player.
static inline bool isPawnLocallyControlled(const APawn* pawn) {
  if (!IsValid(pawn)) {
    return false;
  }
  
  return pawn->IsLocallyControlled();
}

//! Determine whether the actor in question is instantiated on the server.
//!
//! @param actor The actor in question.
//! @returns True if the actor is instantiated on the server.
static inline bool isActorAuthoritative(const AActor* actor) {
  if (!IsValid(actor)) {
    return false;
  }

  return actor->GetLocalRole() == ENetRole::ROLE_Authority;
}

//! Gets the FRigidBodyStates from all bones in a given skeletal mesh.
//!
//! @param smc The mesh to get the FRigidBodyStates from.
//! @param states [out] Populated with rigid body states.
static void getRigidBodyStates(const USkeletalMeshComponent* smc,
    TArray<FRigidBodyState>& states) {
  if (!IsValid(smc)) {
    return;
  }

  states.SetNum(smc->Bodies.Num());
  for (int i = 0; i < smc->Bodies.Num(); i++) {
    FBodyInstance* body = smc->Bodies[i];
    if (body == nullptr || !body->IsValidBodyInstance()) {
      states[i] = FRigidBodyState();
      continue;
    }
    body->GetRigidBodyState(states[i]);
  }
}

//! @brief Interpolate between two rigid body states.
//!
//! FVectors are linearly interpolated and FQuats are spherically interpolated.
//!
//! @param a Physics state for @p alpha = 0.
//! @param b Physics state for @p alpha = 1.
//! @param alpha Interpolation alpha.
static FRigidBodyState interpolateRigidBodyState(FRigidBodyState a, FRigidBodyState b,
    float alpha) {
  FRigidBodyState c;
  c.Flags = a.Flags & b.Flags;
  c.Position = UKismetMathLibrary::VLerp(a.Position, b.Position, alpha);
  c.Quaternion = FQuat::Slerp(a.Quaternion, b.Quaternion, alpha);
  c.LinVel = UKismetMathLibrary::VLerp(a.LinVel, b.LinVel, alpha);
  c.AngVel = UKismetMathLibrary::VLerp(a.AngVel, b.AngVel, alpha);
  return c;
}

//! Gets a player's ping.
//!
//! @param player_controller The player's controller.
//! @returns The player's ping.
static float getPlayerPingMs(const APlayerController* player_controller) {
  if (IsValid(player_controller)) {
    if (player_controller->PlayerState != nullptr) {
      return player_controller->PlayerState->ExactPing;
    }
  }
  return 0.0f;
}

//! Works like UPhysicsConstraintComponent::InitComponentConstraint(), only updating constraint
//! frames is optional.
//!
//! @param constraint The constraint to initialize.
//! @param update_frames Whether to update the constraint's frames.
static void initPhysicsConstraintComponent(UPhysicsConstraintComponent* constraint,
    bool update_frames = true) {
  if (!IsValid(constraint)) {
    return;
  }

  if (update_frames) {
    constraint->UpdateConstraintFrames();
  }

  FBodyInstance* body1 = nullptr;
  UPrimitiveComponent* component1 = constraint->OverrideComponent1.Get();
  if (IsValid(component1)) {
    body1 = component1->GetBodyInstance(constraint->ConstraintInstance.ConstraintBone1);
  }
  FBodyInstance* body2 = nullptr;
  UPrimitiveComponent* component2 = constraint->OverrideComponent2.Get();
  if (IsValid(component2)) {
    body2 = component2->GetBodyInstance(constraint->ConstraintInstance.ConstraintBone2);
  }

  if (body1 != nullptr || body2 != nullptr) {
    constraint->ConstraintInstance.InitConstraint(body1, body2,
        constraint->GetComponentScale().GetAbsMin(), constraint);
  }
}

//! Set whether the constraint is active in the physics simulation.
//!
//! @param constraint The constraint whose state to change.
//! @param enabled Whether the constraint should be active in the physics simulation.
static inline void setConstraintPhysicallyEnabled(UPhysicsConstraintComponent* constraint,
    bool enabled) {
  if (IsValid(constraint)) {
    if (enabled) {
      if (constraint->ConstraintInstance.IsTerminated() ||
          constraint->ConstraintInstance.IsBroken()) {
        initPhysicsConstraintComponent(constraint, false);
      }
    } else {
      if (constraint->ConstraintInstance.IsValidConstraintInstance()) {
        constraint->BreakConstraint();
      }
    }
  }
}

//! @brief Returns true if @p component is replicating movement.
//!
//! A component's movement is only replicated if it's the root component of its actor.
//!
//! @param component The component to check.
//! @returns True if @p component is replicating movement.
static inline bool isMovementReplicated(const UPrimitiveComponent* component) {
  if (!IsValid(component)) {
    return false;
  }

  AActor* owner = component->GetOwner();
  if (!IsValid(owner)) {
    return false;
  }

  return owner->GetIsReplicated() && owner->bReplicateMovement && component->GetIsReplicated() &&
      component == owner->GetRootComponent();
}

//! @brief Returns true if @p component is replicated.
//!
//! A component is replicated if both it and its actor are replicated.
//!
//! @param component The component to check.
//! @returns True if @p component is replicated.
static inline bool isReplicated(const UPrimitiveComponent* component) {
  if (!IsValid(component)) {
    return false;
  }

  AActor* owner = component->GetOwner();
  if (!IsValid(owner)) {
    return false;
  }

  return owner->GetIsReplicated() && component->GetIsReplicated();
}

//! Gets the transform of a bone relative to the root in the base pose.
//!
//! @param comp The skeletal mesh.
//! @param bone_name Which bone.
//! @param l_transform [out] Populated with the transform of the bone relative to the root.
//! @returns True if the transform was successfully populated.
static inline bool getRefPoseTransformRelativeToRoot(const USkinnedMeshComponent* const comp,
    const FName bone_name, FTransform& l_transform) {
  if (!IsValid(comp) || !IsValid(comp->SkeletalMesh)) {
    return false;
  }

  const auto& ref_pose = comp->SkeletalMesh->RefSkeleton.GetRefBonePose();
  const auto& bone_info = comp->SkeletalMesh->RefSkeleton.GetRefBoneInfo();

  int32 i = comp->GetBoneIndex(bone_name);
  if (i == INDEX_NONE) {
    return false;
  }

  l_transform = FTransform::Identity;
  while (i > 0 &&  // i = 0 means the root bone and is intentionally disallowed.
      i < ref_pose.Num() && i < bone_info.Num()) {
    l_transform = UKismetMathLibrary::ComposeTransforms(l_transform, ref_pose[i]);
    i = bone_info[i].ParentIndex;
  }

  return true;
}

//! Produces output from a sine wave given the below parameters and an amplitude of 1.
//!
//! @param time_s The time that the wave has been playing for [s].
//! @param frequency_Hz The frequency of the wave [Hz].
//! @param input_phase_offset_deg The phase offset to give the wave [deg].
static float evaluateSineWave(float time_s, float frequency_Hz, float input_phase_offset_deg) {
  // IF YOU CHANGE THIS CODE PLEASE RUN runEvaluateSineWaveTests() BELOW //
  const float wave_input = time_s * frequency_Hz * REV_TO_RAD +
      input_phase_offset_deg * DEG_TO_RAD;
  // A value between -1 and 1 fitting the timing criteria for the sine wave
  return sinf(wave_input);
}
