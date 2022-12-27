// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/Components/hx_1d_constraint_component.h>
#include "hx_1d_rotator_component.generated.h"

//! @brief Uses features that can be added to @link UHxConstraintComponent UHxConstraintComponents 
//! @endlink that only need to operate in one rotational degree of freedom.
//!
//! See the @ref section_unreal_hx_1d_components "Unreal Haptic Primitive Guide" for a high level 
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// Uses features that can be added to UHxConstraintComponents that only need to operate in one 
// rotational degree of freedom.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew, 
    meta = (BlueprintSpawnableComponent),
    HideCategories=(Tick, Tags, Collision, Rendering, HxConstraint))
class HAPTXPRIMITIVES_API UHx1DRotatorComponent : public UHx1DConstraintComponent {
  GENERATED_BODY()

public:

  //! Default constructor.
  //!
  //! @param object_initializer For registration with Unreal.
  UHx1DRotatorComponent(const FObjectInitializer& object_initializer);

  //! Called when the game starts.
  void InitializeComponent() override;

  //! Called every frame.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

protected:
  //! @brief Track the rotation as more than just an angle between -180 and 180 degrees. 
  //!
  //! This changes how attached behaviors and state machines work.

  // Track the rotation as more than just an angle between -180 and 180 degrees. 
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool track_multiple_revolutions_;

  virtual void configureConstraint(FConstraintInstance& in_constraint_instance) override;

private:
  //! Whether limits need to be updated.
  //!
  //! @returns True if limits need to be updated.
  bool checkLimits();

  //! Whether the current configuration demands the safety sector algorithm. 
  //!
  //! @returns True if the constraint has more than 360 - MIN_DEAD_ZONE_WIDTH degrees of allowed 
  //! motion.
  bool shouldUseSafetySector();

  //! Whether we're currently in the low safety sector and using the safety sector 
  //! algorithm.
  bool in_low_safety_sector_;

  //! Whether we're currently in the high safety sector and using the safety sector 
  //! algorithm.
  bool in_high_safety_sector_;

  //! Whether the safety sector is currently active.
  bool safety_sector_active_;

  //! @brief The smallest width [deg] of the "dead zone" in the dial's range of motion where we 
  //! don't need the safety net approach. 
  //!
  //! If the dead zone [deg] is smaller than this or is nonexistent (> 360 range of motion) then we 
  //! should use the safety net approach when we get close to our range of motion limits.
  static constexpr float MIN_DEAD_ZONE_WIDTH = 20.f;

  //! @brief The angle (degrees) of the safety sector of the circular range of motion of this 
  //! constraint.
  //!
  //! When we are within SAFETY_SECTOR_WIDTH degrees of a range of motion limit, we reform the
  //! underlying physics constraint with new symmetrical limits of size SAFETY_SECTOR_WIDTH around
  //! that threshold position. When that is not the case, the constraint has no limits. We track
  //! revolutions and angles manually to detect this change in state.
  static constexpr float SAFETY_SECTOR_WIDTH = 30.f;
};
