// Copyright (C) 2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <PhysicsEngine/ConstraintInstance.h>
#include "constraint_instance_net_quanitze.generated.h"

//! A FLinearDriveConstraint with quantized FVectors.
USTRUCT()
struct FLinearDriveConstraint_NetQuantize100 {
  GENERATED_BODY()

  //! Default constructor.
  FLinearDriveConstraint_NetQuantize100() = default;

  //! Construct from a FLinearDriveConstraint.
  //!
  //! @param other Which object to construct from.
  FLinearDriveConstraint_NetQuantize100(const FLinearDriveConstraint& other) : 
      PositionTarget(other.PositionTarget), VelocityTarget(other.VelocityTarget),
      XDrive(other.XDrive), YDrive(other.YDrive), ZDrive(other.ZDrive),
      bEnablePositionDrive(other.bEnablePositionDrive) {}

  //! Convert to a FLinearDriveConstraint.
  //!
  //! @returns An equivalent FLinearDriveConstraint.
  FLinearDriveConstraint deserialize() const {
    FLinearDriveConstraint out;
    out.PositionTarget = PositionTarget;
    out.VelocityTarget = VelocityTarget;
    out.XDrive = XDrive;
    out.YDrive = YDrive;
    out.ZDrive = ZDrive;
    out.bEnablePositionDrive = bEnablePositionDrive;
    return out;
  }

  //! See FLinearDriveConstraint::PositionTarget.
  UPROPERTY()
  FVector_NetQuantize100 PositionTarget;

  //! See FLinearDriveConstraint::VelocityTarget.
  UPROPERTY()
  FVector_NetQuantize100 VelocityTarget;
   
  //! See FLinearDriveConstraint::XDrive.
  UPROPERTY()
  FConstraintDrive XDrive;
    
  //! See FLinearDriveConstraint::YDrive.
  UPROPERTY()
  FConstraintDrive YDrive;
    
  //! See FLinearDriveConstraint::ZDrive.
  UPROPERTY()
  FConstraintDrive ZDrive;

  //! See FLinearDriveConstraint::bEnablePositionDrive.
  UPROPERTY()
  uint8 bEnablePositionDrive;
};

//! A FAngularDriveConstraint with quantized FVectors.
USTRUCT()
struct FAngularDriveConstraint_NetQuantize100 {
  GENERATED_BODY()

  //! Default constructor.
  FAngularDriveConstraint_NetQuantize100() = default;

  //! Construct from a FAngularDriveConstraint.
  //!
  //! @param other Which object to construct from.
  FAngularDriveConstraint_NetQuantize100(const FAngularDriveConstraint& other) : 
      TwistDrive(other.TwistDrive), SwingDrive(other.SwingDrive), SlerpDrive(other.SlerpDrive),
      OrientationTarget(other.OrientationTarget),
      AngularVelocityTarget(other.AngularVelocityTarget), 
      AngularDriveMode(other.AngularDriveMode) {}

  //! Convert to a FAngularDriveConstraint.
  //!
  //! @returns An equivalent FAngularDriveConstraint.
  FAngularDriveConstraint deserialize() const {
    FAngularDriveConstraint out;
    out.TwistDrive = TwistDrive;
    out.SwingDrive = SwingDrive;
    out.SlerpDrive = SlerpDrive;
    out.OrientationTarget = OrientationTarget;
    out.AngularVelocityTarget = AngularVelocityTarget;
    out.AngularDriveMode = AngularDriveMode;
    return out;
  }

  //! See FAngularDriveConstraint::TwistDrive.
  UPROPERTY()
  FConstraintDrive TwistDrive;

  //! See FAngularDriveConstraint::SwingDrive.
  UPROPERTY()
  FConstraintDrive SwingDrive;

  //! See FAngularDriveConstraint::SlerpDrive.
  UPROPERTY()
  FConstraintDrive SlerpDrive;

  //! See FAngularDriveConstraint::OrientationTarget.
  UPROPERTY()
	FRotator OrientationTarget;

  //! See FAngularDriveConstraint::AngularVelocityTarget.
  UPROPERTY()
	FVector_NetQuantize100 AngularVelocityTarget;

  //! See FAngularDriveConstraint::AngularDriveMode.
  UPROPERTY()
  TEnumAsByte<enum EAngularDriveMode::Type> AngularDriveMode;
};

//! A FConstraintProfileProperties with quantized FVectors.

// A FConstraintProfileProperties with quantized FVectors.
USTRUCT()
struct FConstraintProfileProperties_NetQuantize100 {
  GENERATED_BODY()

  //! Default constructor.
  FConstraintProfileProperties_NetQuantize100() = default;

  //! Construct from a FConstraintProfileProperties.
  //!
  //! @param other Which object to construct from.
  FConstraintProfileProperties_NetQuantize100(const FConstraintProfileProperties& other) : 
      ProjectionLinearTolerance(other.ProjectionLinearTolerance),
      ProjectionAngularTolerance(other.ProjectionAngularTolerance),
      LinearBreakThreshold(other.LinearBreakThreshold),
      AngularBreakThreshold(other.AngularBreakThreshold), LinearLimit(other.LinearLimit),
      ConeLimit(other.ConeLimit), TwistLimit(other.TwistLimit), LinearDrive(other.LinearDrive),
      AngularDrive(other.AngularDrive), bDisableCollision(other.bDisableCollision),
      bParentDominates(other.bParentDominates), bEnableProjection(other.bEnableProjection),
      bAngularBreakable(other.bAngularBreakable), bLinearBreakable(other.bLinearBreakable) {}

  //! Convert to a FConstraintProfileProperties.
  //!
  //! @returns An equivalent FConstraintProfileProperties.
  FConstraintProfileProperties deserialize() const {
    FConstraintProfileProperties out;
    out.ProjectionLinearTolerance = ProjectionLinearTolerance;
    out.ProjectionAngularTolerance = ProjectionAngularTolerance;
    out.LinearBreakThreshold = LinearBreakThreshold;
    out.AngularBreakThreshold = AngularBreakThreshold;
    out.LinearLimit = LinearLimit;
    out.ConeLimit = ConeLimit;
    out.TwistLimit = TwistLimit;
    out.LinearDrive = LinearDrive.deserialize();
    out.AngularDrive = AngularDrive.deserialize();
    out.bDisableCollision = bDisableCollision;
    out.bParentDominates = bParentDominates;
    out.bEnableProjection = bEnableProjection;
    out.bAngularBreakable = bAngularBreakable;
    out.bLinearBreakable = bLinearBreakable;
    return out;
  }

  //! See FConstraintProfileProperties::ProjectionLinearTolerance.
  UPROPERTY()
  float ProjectionLinearTolerance;

  //! See FConstraintProfileProperties::ProjectionAngularTolerance.
  UPROPERTY()
  float ProjectionAngularTolerance;

  //! See FConstraintProfileProperties::LinearBreakThreshold.
  UPROPERTY()
  float LinearBreakThreshold;

  //! See FConstraintProfileProperties::AngularBreakThreshold.
  UPROPERTY()
  float AngularBreakThreshold;

  //! See FConstraintProfileProperties::LinearLimit.
  UPROPERTY()
  FLinearConstraint LinearLimit;
    
  //! See FConstraintProfileProperties::ConeLimit.
  UPROPERTY()
  FConeConstraint ConeLimit;

  //! See FConstraintProfileProperties::TwistLimit.
  UPROPERTY()
  FTwistConstraint TwistLimit;

  //! See FConstraintProfileProperties::LinearDrive.
  UPROPERTY()
  FLinearDriveConstraint_NetQuantize100 LinearDrive;
    
  //! See FConstraintProfileProperties::AngularDrive.
  UPROPERTY()
  FAngularDriveConstraint_NetQuantize100 AngularDrive;
    
  //! See FConstraintProfileProperties::bDisableCollision.
  UPROPERTY()
  uint8 bDisableCollision;

  //! See FConstraintProfileProperties::bParentDominates.
  UPROPERTY()
  uint8 bParentDominates;

  //! See FConstraintProfileProperties::bEnableProjection.
  UPROPERTY()
  uint8 bEnableProjection;

  //! See FConstraintProfileProperties::bAngularBreakable.
  UPROPERTY()
  uint8 bAngularBreakable;

  //! See FConstraintProfileProperties::bLinearBreakable.
  UPROPERTY()
  uint8 bLinearBreakable;
};

//! A FConstraintInstance with quantized FVectors.
USTRUCT()
struct FConstraintInstance_NetQuantize100 {
  GENERATED_BODY()

  //! Default constructor.
  FConstraintInstance_NetQuantize100() = default;

  //! Construct from a FConstraintInstance.
  //!
  //! @param other Which object to construct from.
  FConstraintInstance_NetQuantize100(const FConstraintInstance& other) : 
      JointName(other.JointName), ConstraintBone1(other.ConstraintBone1),
      ConstraintBone2(other.ConstraintBone2), Pos1(other.Pos1), PriAxis1(other.PriAxis1),
      SecAxis1(other.SecAxis1), Pos2(other.Pos2), PriAxis2(other.PriAxis2),
      SecAxis2(other.SecAxis2), AngularRotationOffset(other.AngularRotationOffset),
      bScaleLinearLimits(other.bScaleLinearLimits), ProfileInstance(other.ProfileInstance) {}

  //! Convert to a FConstraintInstance.
  //!
  //! @returns An equivalent FConstraintInstance.
  FConstraintInstance deserialize() const {
    FConstraintInstance out;
    out.JointName = JointName;
    out.ConstraintBone1 = ConstraintBone1;
    out.ConstraintBone2 = ConstraintBone2;
    out.Pos1 = Pos1;
    out.PriAxis1 = PriAxis1;
    out.SecAxis1 = SecAxis1;
    out.SecAxis1 = SecAxis1;
    out.Pos2 = Pos2;
    out.PriAxis2 = PriAxis2;
    out.SecAxis2 = SecAxis2;
    out.AngularRotationOffset = AngularRotationOffset;
    out.bScaleLinearLimits = bScaleLinearLimits;
    out.ProfileInstance = ProfileInstance.deserialize();
    // This prevents a crash since this variable doesn't initialize to nullptr.
    out.ConstraintHandle.ConstraintData = nullptr;
    return out;
  }

  //! See FConstraintInstance::JointName.
  UPROPERTY()
  FName JointName;

  //! See FConstraintInstance::ConstraintBone1.
  UPROPERTY()
  FName ConstraintBone1;

  //! See FConstraintInstance::ConstraintBone2.
  UPROPERTY()
  FName ConstraintBone2;

  //! See FConstraintInstance::Pos1.
  UPROPERTY()
  FVector_NetQuantize100 Pos1;

  //! See FConstraintInstance::PriAxis1.
  UPROPERTY()
  FVector_NetQuantize100 PriAxis1;

  //! See FConstraintInstance::SecAxis1.
  UPROPERTY()
  FVector_NetQuantize100 SecAxis1;

  //! See FConstraintInstance::Pos2.
  UPROPERTY()
  FVector_NetQuantize100 Pos2;

  //! See FConstraintInstance::PriAxis2.
  UPROPERTY()
  FVector_NetQuantize100 PriAxis2;

  //! See FConstraintInstance::SecAxis2.
  UPROPERTY()
  FVector_NetQuantize100 SecAxis2;

  //! See FConstraintInstance::AngularRotationOffset.
  UPROPERTY()
  FRotator AngularRotationOffset;

  //! See FConstraintInstance::bScaleLinearLimits.
  UPROPERTY()
  uint32 bScaleLinearLimits;

  //! See FConstraintInstance::ProfileInstance.
  UPROPERTY()
  FConstraintProfileProperties_NetQuantize100 ProfileInstance;
};
