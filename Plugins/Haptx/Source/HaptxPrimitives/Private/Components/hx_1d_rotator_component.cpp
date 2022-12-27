// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/Components/hx_1d_rotator_component.h>

UHx1DRotatorComponent::UHx1DRotatorComponent(const FObjectInitializer &object_initializer) :
    Super(object_initializer), track_multiple_revolutions_(true), in_low_safety_sector_(false), 
    in_high_safety_sector_(false), safety_sector_active_(false) {
  operating_domain_ = EDofDomain::ANGULAR;
  lower_limit_ = -90.0f;
  upper_limit_ = 90.0f;
  damping_ = 0.1f;
}

void UHx1DRotatorComponent::InitializeComponent() {
  Super::InitializeComponent();

  // Track multiple revolutions.
  UHxAngularDof* angular_dof = dynamic_cast<UHxAngularDof*>(getOperatingDof());
  if (angular_dof != nullptr) {
    angular_dof->track_multiple_revolutions_ = track_multiple_revolutions_;
  }
  else {
    UE_LOG(HaptxPrimitives, Error, TEXT("Failed to find associated UHxAngularDof."))
  }
}

void UHx1DRotatorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!isConstraintFormed()) {
    return;
  }

  // If we have dynamic limit checking and handling to do.
  if (limit_motion_ && shouldUseSafetySector() && checkLimits()) {
    updateConstraint();
  }
}

bool UHx1DRotatorComponent::checkLimits() {
  UHxDof* dof = getDof(makeDegreeOfFreedomFromDomainAndAxis(operating_domain_, operating_axis_));
  if (dof != nullptr) {
    float rotation = dof->getCurrentPosition();
    in_low_safety_sector_ = isBetween(rotation,
        lower_limit_ - MIN_DEAD_ZONE_WIDTH,
        lower_limit_ + SAFETY_SECTOR_WIDTH);
    in_high_safety_sector_ = isBetween(rotation,
        upper_limit_ - SAFETY_SECTOR_WIDTH,
        upper_limit_ + MIN_DEAD_ZONE_WIDTH);

    if (!in_low_safety_sector_ && !in_high_safety_sector_) {
      // Verify that the constraint hasn't been teleported past its limits, and that its limits
      // haven't been moved past it.
      if (rotation < lower_limit_) {
        teleportAnchor1AlongDof(lower_limit_, makeDegreeOfFreedomFromDomainAndAxis(
            operating_domain_, operating_axis_));
        rotation = lower_limit_;
        in_low_safety_sector_ = true;
      }
      else if (rotation > upper_limit_) {
        teleportAnchor1AlongDof(upper_limit_, makeDegreeOfFreedomFromDomainAndAxis(
            operating_domain_, operating_axis_));
        rotation = upper_limit_;
        in_high_safety_sector_ = true;
      }
    }

    bool in_safety_sector = in_low_safety_sector_ || in_high_safety_sector_;
    if (safety_sector_active_) {
      // Check for movement out from the safety sector.
      if (!in_safety_sector){
        return true;
      }
    } else {
      // Check for movement into any safety sector.
      if (in_safety_sector) {
        return true;
      }
    }
  }

  return false;
}

void UHx1DRotatorComponent::configureConstraint(
    FConstraintInstance& in_constraint_instance) {
  Super::configureConstraint(in_constraint_instance);

  // Assume parameters have default values unless otherwise informed.
  safety_sector_active_ = false;
  in_constraint_instance.AngularRotationOffset = FRotator::ZeroRotator;

  // If we'll be using range of motion limits.
  if (limit_motion_) {
    // If we don't need the safety sector algorithm we can use limited motion.
    if (!shouldUseSafetySector()) {
      float angle_offset = (upper_limit_ + lower_limit_) / 2;
      in_constraint_instance.AngularRotationOffset = {
          operating_axis_ == EDofAxis::Y ? angle_offset : 0,
          operating_axis_ == EDofAxis::Z ? angle_offset : 0,
          operating_axis_ == EDofAxis::X ? angle_offset : 0
      };
      const float half_rom_range = (upper_limit_ - lower_limit_) / 2;
      switch (operating_axis_) {
        case EDofAxis::X:
          in_constraint_instance.ProfileInstance.TwistLimit.TwistMotion = ACM_Limited;
          in_constraint_instance.ProfileInstance.TwistLimit.TwistLimitDegrees = half_rom_range;
          break;
        case EDofAxis::Y:
          in_constraint_instance.ProfileInstance.ConeLimit.Swing2Motion = ACM_Limited;
          in_constraint_instance.ProfileInstance.ConeLimit.Swing2LimitDegrees = half_rom_range;
          break;
        case EDofAxis::Z:
          in_constraint_instance.ProfileInstance.ConeLimit.Swing1Motion = ACM_Limited;
          in_constraint_instance.ProfileInstance.ConeLimit.Swing1LimitDegrees = half_rom_range;
          break;
      }
    } else {
      if (!in_low_safety_sector_ && !in_high_safety_sector_) {
        // Use free limits when not in a safety sector.
        safety_sector_active_ = false;
      } else {
        // Use limited motion when in a safety sector.
        TotalRotation total_rot_offset;
        if (in_low_safety_sector_) {
          total_rot_offset = TotalRotation(lower_limit_ +
            SAFETY_SECTOR_WIDTH);
        }
        else {  // if (in_high_safety_sector) {
          total_rot_offset = TotalRotation(upper_limit_ - SAFETY_SECTOR_WIDTH);
        }
        in_constraint_instance.AngularRotationOffset = { 
            operating_axis_ == EDofAxis::Y ? total_rot_offset.partial_angle_ : 0.0f,
            operating_axis_ == EDofAxis::Z ? total_rot_offset.partial_angle_ : 0.0f,
            operating_axis_ == EDofAxis::X ? total_rot_offset.partial_angle_ : 0.0f
        };
        switch (operating_axis_) {
        case EDofAxis::X:
          in_constraint_instance.ProfileInstance.TwistLimit.TwistMotion = ACM_Limited;
          in_constraint_instance.ProfileInstance.TwistLimit.TwistLimitDegrees = 
              SAFETY_SECTOR_WIDTH;
          break;
        case EDofAxis::Y:
          in_constraint_instance.ProfileInstance.ConeLimit.Swing2Motion = ACM_Limited;
          in_constraint_instance.ProfileInstance.ConeLimit.Swing2LimitDegrees =  
              SAFETY_SECTOR_WIDTH;
          break;
        case EDofAxis::Z:
          in_constraint_instance.ProfileInstance.ConeLimit.Swing1Motion = ACM_Limited;
          in_constraint_instance.ProfileInstance.ConeLimit.Swing1LimitDegrees = 
              SAFETY_SECTOR_WIDTH;
          break;
        }
        
        safety_sector_active_ = true;
      }
    }
  }
}

bool UHx1DRotatorComponent::shouldUseSafetySector() {
  return upper_limit_ - lower_limit_ >= DEG_PER_REV - MIN_DEAD_ZONE_WIDTH;
}
