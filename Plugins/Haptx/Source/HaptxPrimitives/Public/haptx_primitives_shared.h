// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Components/PrimitiveComponent.h>
#include <Runtime/Engine/Classes/GameFramework/Actor.h>
#include <Haptx/Private/haptx_shared.h>
#include <HaptxPrimitives/Public/ihaptx_primitives.h>

//! Multiply to convert radians to degrees.
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.2957795131f
#endif
//! Multiply to convert degrees to radians.
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.01745329251f
#endif

DECLARE_STATS_GROUP_IF_PROFILING(TEXT("HaptxPrimitivesShared"), STATGROUP_HaptxPrimitivesShared, 
    STATCAT_Advanced)

//! The linear and angular domains of 3-dimensional space.
UENUM(BlueprintType)
enum class EDofDomain : uint8 {
  LINEAR    UMETA(DisplayName = "Linear"),   //!< Linear domain.
  ANGULAR   UMETA(DisplayName = "Angular"),  //!< Angular domain.
  LAST      UMETA(Hidden)                    //!< Reserved, mainly for iteration purposes.
};

//! Basis vectors that describe 3-dimensional space.
UENUM(BlueprintType)
enum class EDofAxis : uint8 {
  X       UMETA(DisplayName = "X"),  //!< X basis vector.
  Y       UMETA(DisplayName = "Y"),  //!< Y basis vector.
  Z       UMETA(DisplayName = "Z"),  //!< Z basis vector.
  LAST    UMETA(Hidden)              //!< Reserved, mainly for iteration purposes.
};

//! The 6 degrees of freedom of 3-dimensional space.
UENUM(BlueprintType)
enum class EDegreeOfFreedom : uint8 {
  X_LIN   UMETA(DisplayName = "X Linear"),   //!< X linear degree of freedom.
  Y_LIN   UMETA(DisplayName = "Y Linear"),   //!< Y linear degree of freedom.
  Z_LIN   UMETA(DisplayName = "Z Linear"),   //!< Z linear degree of freedom.
  X_ANG   UMETA(DisplayName = "X Angular"),  //!< X angular degree of freedom.
  Y_ANG   UMETA(DisplayName = "Y Angular"),  //!< Y angular degree of freedom.
  Z_ANG   UMETA(DisplayName = "Z Angular"),  //!< Z angular degree of freedom.
  LAST    UMETA(Hidden)                      //!< Reserved, mainly for iteration purposes.
};

//! Make an EDegreeOfFreedom from an EDofDomain and an EDofAxis.
//!
//! @param domain The domain of the degree of freedom.
//! @param axis The axis of the degree of freedom.
//!
//! @returns The matching degree of freedom.
static EDegreeOfFreedom makeDegreeOfFreedomFromDomainAndAxis(EDofDomain domain, EDofAxis axis) {
  switch (axis) {
    case EDofAxis::X: {
      switch (domain) {
        case EDofDomain::LINEAR: {
          return EDegreeOfFreedom::X_LIN;
        }
        case EDofDomain::ANGULAR: {
          return EDegreeOfFreedom::X_ANG;
        }
        default: {
          return EDegreeOfFreedom::LAST;
        }
      }
    }
    case EDofAxis::Y: {
      switch (domain) {
        case EDofDomain::LINEAR: {
          return EDegreeOfFreedom::Y_LIN;
        }
        case EDofDomain::ANGULAR: {
          return EDegreeOfFreedom::Y_ANG;
        }
        default: {
          return EDegreeOfFreedom::LAST;
        }
      }
    }
    case EDofAxis::Z: {
      switch (domain) {
        case EDofDomain::LINEAR: {
          return EDegreeOfFreedom::Z_LIN;
        }
        case EDofDomain::ANGULAR: {
          return EDegreeOfFreedom::Z_ANG;
        }
        default: {
          return EDegreeOfFreedom::LAST;
        }
      }
    }
    default: {
      return EDegreeOfFreedom::LAST;
    }
  }
}

//! Get the EDofAxis of an EDegreeOfFreedom.
//!
//! @param degree_of_freedom The degree of freedom.
//!
//! @returns The axis of @p degree_of_freedom.
static const EDofAxis axisOfDegreeOfFreedom(EDegreeOfFreedom degree_of_freedom) {
  return static_cast<EDofAxis>(static_cast<uint8>(degree_of_freedom) % 3);
}

//! Get the EDofDomain of an EDegreeOfFreedom.
//!
//! @param degree_of_freedom The degree of freedom.
//!
//! @returns The domain of @p degree_of_freedom.
static EDofDomain domainOfDegreeOfFreedom(EDegreeOfFreedom degree_of_freedom) {
  return static_cast<EDofDomain>(static_cast<uint8>(degree_of_freedom) / 3);
}

//! @brief Get the local space direction of an EDegreeOfFreedom.
//!
//! These values match the behavior of the transform tools in the editor.
//!
//! @param degree_of_freedom The degree of freedom.
//!
//! @returns The local space direction of the degree of freedom.
static const FVector& directionOfDegreeOfFreedom(EDegreeOfFreedom degree_of_freedom) {
  static FVector directions[static_cast<uint8>(EDegreeOfFreedom::LAST)] = {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
    { -1, 0, 0 },
    { 0, -1, 0 },
    { 0, 0, 1 }
  };
  return directions[static_cast<uint8>(degree_of_freedom)];
}

//! @brief Get the orthonormal basis vectors of an EDegreeOfFreedom.
//!
//! For example, if provided EDegreeOfFreedom::X_LIN @p first and @p second will be populated with
//! the directions of EDegreeOfFreedom::Y_LIN and EDegreeOfFreedom::Z_LIN respectively.
//!
//! @param degree_of_freedom The degree of freedom.
//! @param[out] first The first orthonormal basis vector.
//! @param[out] second The second orthonormal basis vector.
static const void getOrthonormalDirectionsFromDegreeOfFreedom(
    EDegreeOfFreedom degree_of_freedom, FVector& first, FVector& second) {
  switch (degree_of_freedom) {
    case EDegreeOfFreedom::X_LIN: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::Y_LIN);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::Z_LIN);
      return;
    }
    case EDegreeOfFreedom::Y_LIN: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::Z_LIN);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::X_LIN);
      return;
    }
    case EDegreeOfFreedom::Z_LIN: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::X_LIN);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::Y_LIN);
      return;
    }
    case EDegreeOfFreedom::X_ANG: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::Y_ANG);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::Z_ANG);
      return;
    }
    case EDegreeOfFreedom::Y_ANG: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::Z_ANG);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::X_ANG);
      return;
    }
    case EDegreeOfFreedom::Z_ANG: {
      first = directionOfDegreeOfFreedom(EDegreeOfFreedom::X_ANG);
      second = directionOfDegreeOfFreedom(EDegreeOfFreedom::Y_ANG);
      return;
    }
    default: {
      first = FVector::OneVector;
      second = FVector::OneVector;
      return;
    }
  }
}

//! @brief Get the local space direction of a rotation's EDegreeOfFreedom.
//!
//! These values match the behavior of the transform tools in the editor.
//!
//! @param rotation The rotation of interest.
//! @param degree_of_freedom The degree of freedom.
//!
//! @returns The local space direction of the rotation's degree of freedom.
static const FVector directionOfDegreeOfFreedom(FQuat& rotation, 
    EDegreeOfFreedom degree_of_freedom) {
  switch (degree_of_freedom) {
    case EDegreeOfFreedom::X_LIN:
      return rotation.GetForwardVector();
    case EDegreeOfFreedom::Y_LIN:
      return rotation.GetRightVector();
    case EDegreeOfFreedom::Z_LIN:
      return rotation.GetUpVector();
    case EDegreeOfFreedom::X_ANG:
      return -rotation.GetForwardVector();
    case EDegreeOfFreedom::Y_ANG:
      return -rotation.GetRightVector();
    case EDegreeOfFreedom::Z_ANG:
      return rotation.GetUpVector();
    default:
      return FVector::OneVector;
  }
}

//! @brief Get the orthonormal basis vectors of a rotation's EDegreeOfFreedom.
//!
//! For example, if provided EDegreeOfFreedom::X_LIN @p first and @p second will be populated with
//! the rotation's directions of EDegreeOfFreedom::Y_LIN and EDegreeOfFreedom::Z_LIN respectively.
//!
//! @param rotation The rotation of interest.
//! @param degree_of_freedom The degree of freedom.
//! @param[out] first The first orthonormal basis vector.
//! @param[out] second The second orthonormal basis vector.
static const void getOrthonormalDirectionsFromDegreeOfFreedom(
    FQuat& rotation, EDegreeOfFreedom degree_of_freedom, FVector& first, FVector& second) {
  getOrthonormalDirectionsFromDegreeOfFreedom(degree_of_freedom, first, second);
  first = rotation * first;
  second = rotation * second;
}

//! Whether x is between a and b.
//!
//! @param x The value of interest.
//! @param a The lower bound.
//! @param b The upper bound.
//! @param inclusive Whether the bounds are inclusive.
//!
//! @returns Whether x is between a and b.
static inline bool isBetween(float x, float a, float b, bool inclusive = true) {
  if (inclusive) {
    return x >= a && x <= b;
  }
  else {
    return x > a && x < b;
  }
}

//! Get a copy of a transform with its scale set to FVector::OneVector.
//!
//! @param in_transform The transform to scale and copy.
//!
//! @returns A copy of @p in_transform with a scale of one.
static inline FTransform getScaleOne(const FTransform& in_transform) {
  FTransform out_transform = in_transform;
  out_transform.SetScale3D(FVector::OneVector);
  return out_transform;
}

//! @brief Get the world space COM transform of a single bone.
//!
//! The transform is composed of the COM's position, the body's rotation, and a scale of 
//! FVector::OneVector.
//!
//! @param component The component that contains the bone.
//! @param bone_name The name of the bone.
//!
//! @returns The world space COM transform.
static inline FTransform getCenterOfMassTransform(UPrimitiveComponent* component,
    FName bone_name = NAME_None) {
  if (IsValid(component)) {
    return FTransform(
      component->GetSocketRotation(bone_name),
      component->GetCenterOfMass(bone_name),
      FVector::OneVector);
  }
  else {
    return FTransform::Identity;
  }
}

//! Whether a bone is both valid and simulating physics.
//!
//! @param component The component of interest.
//! @param bone_name The bone of interest.
//!
//! @returns True if the bone is valid and simulating physics.
static bool isValidAndSimulating(UPrimitiveComponent* component, FName bone_name = NAME_None) {
  return IsValid(component) && component->IsSimulatingPhysics(bone_name);
}
