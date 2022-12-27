// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/hx_dof_behavior.h>
#include "hx_dof_detent_behavior.generated.h"

#define INVALID_DETENT_INDEX -1

//! @brief A physical behavior that always drives toward the nearest detent.
//!
//! See the @ref section_unreal_hx_dof_detent_behavior "Unreal Haptic Primitive Guide" for a high 
//! level overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A physical behavior that always drives toward the nearest detent.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxDofDetentBehavior : public UHxDofBehavior {
  GENERATED_UCLASS_BODY()

public:

  //! Get the signed magnitude of the force or torque that is associated with @p position
  //! relative to the nearest detent.
  //! 
  //! Defined in anchor2's frame.
  //!
  //! @param position The position of interest.
  //!
  //! @returns The signed magnitude of the force or torque.
  virtual float getForceTorque(float position) override;

  //! Gets the value of nearest detent.
  //!
  //! @param[out] out_target Populated with the value of the nearest detent.
  //!
  //! @returns True.
  virtual bool tryGetTarget(float& out_target) override;

  //! @brief Get the index of the nearest detent as of the last call to #getForceTorque().
  //!
  //! A return value of INVALID_DETENT_INDEX represents an invalid detent index.
  //!
  //! @returns The index of the nearest detent.

  // Get the index of the nearest detent as of the last call to getForceTorque().
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Behavior")
  int getCurrentDetentIndex() const;

  //! @brief Sets new detent values.
  //!
  //! The internal copy of this list gets sorted.
  //!
  //! @param new_detents The new list of detents.

  // Sets new detent values.
  UFUNCTION(BlueprintCallable, Category = "Behavior")
  void setDetents(const TArray<float>& new_detents);

  //! Get the current list of detents (sorted).
  //!
  //! @returns The current list of detents (sorted).

  // Get the current list of detents (sorted).
  UFUNCTION(BlueprintCallable, Category = "Behavior")
  const TArray<float>& getDetentsSorted();

protected:
  //! The internal list of detent positions (sorted).

  // The internal list of detent positions (sorted).
  UPROPERTY(EditAnywhere, Category = "Parameters")
  TArray<float> detents_;

private:
  //! Sorts detents and configures initial values.
  void initialize() override;

  //! @brief Get the index of the nearest detent to @p value. 
  //!
  //! Starting at @p starting_index and work outward.
  //!
  //! @returns The index of the nearest detent to @p value, or -1 if no valid index exists.
  int getNearestDetentIndex(float value, int starting_index) const;

  //! @brief The index of the last targeted detent.
  //!
  //! Invalid targets represented with -1.
  int detent_index_;
};
