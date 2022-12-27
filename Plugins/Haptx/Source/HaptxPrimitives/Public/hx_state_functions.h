// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Curves/CurveFloat.h>
#include "hx_state_functions.generated.h"

//! Represents an invalid state.
#define STATE_FUNCTION_INVALID_STATE -100

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPrimitiveStateChange, int, new_state);

//! @brief The base class for all custom state functions used in @link UHxConstraintComponent 
//! UHxConstraintComponents @endlink. 
//!
//! DO NOT INSTANTIATE THIS CLASS! Use a child class instead.
//!
//! To implement a custom state function inherit from this class and override #initialize(), and 
//! #update().
//!
//! See the @ref section_unreal_hx_state_function "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// The base class for all custom state functions used in UHxConstraintComponents. 
UCLASS(ClassGroup = ("HaptX"), Blueprintable)
class HAPTXPRIMITIVES_API UHxStateFunction : public UObject {
  GENERATED_BODY()

  friend class UHxConstraintComponent;

  public:

    //! Default constructor.
    UHxStateFunction() : name_(TEXT("Main")), current_state_(STATE_FUNCTION_INVALID_STATE) {}

    //! Initializes the state function for use. Gets called automatically by UHxDof.

    // Initializes the state function for use. Gets called automatically by UHxDof.
    virtual void initialize() {}

    //! Get the state of this function as of the last call to #update().
    //!
    //! @returns The state of this function.

    // Get the state of this function as of the last call to update().
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int getCurrentState() const;

    //! @brief Event that fires when state changes.
    //!
    //! See @ref section_unreal_hx_state_function for an example of how to bind this event.

    // Event that fires when state changes.
    UPROPERTY(BlueprintAssignable, Category = Events)
    FOnPrimitiveStateChange on_state_change_;

    //! @brief The name of this state function.
    //!
    //! Unique for a given HxDof. See UHxDof.registerStateFunction().

    // The name of this state function.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parameters")
    FName name_;

  protected:

    //! Updates the underlying state machine with a new position.
    //!
    //! @param input_position The new position.
    //!
    //! @returns The state at the provided position.
    virtual int update(float input_position);

    //! Changes the current state and fires an event if the given state differs from the 
    //! current one.
    //!
    //! @param new_state The new state.
    //!
    //! Do not call this from friend classes!
    void changeState(int new_state);

    //! The current state of the function as of the last call to #update().
    int current_state_;
};

//! @brief A state function with two states that change about a transition position.
//!
//! See the @ref section_unreal_hx_state_function "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A state functions with two states that change about a transition position.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHx2StateFunction : public UHxStateFunction {
  GENERATED_BODY()

  public:

    //! Default constructor.
    UHx2StateFunction() : UHxStateFunction(), transition_position_(0.f),
        invert_state_order_(false) {}

    //! @brief Updates the state.
    //!
    //! 0 is the most negative portion of positions unless #invert_state_order_ is true.
    //!
    //! @param input_position The new position.
    //!
    //! @returns 0 or 1. 

    // Updates the state.
    virtual int update(float input_position) override;

    //! The position along the degree of freedom where the transition from 0 to 1 happens.

    // The position along the degree of freedom where the transition from 0 to 1 happens.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    float transition_position_;

    //! By default, state values increase as position increases. Set this to true to invert
    //! that ordering.

    // By default, state values increase as position increases. Set this to true to invert that 
    // ordering.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    bool invert_state_order_;
};

//! @brief A state function with three states that change about high and low transition positions.
//!
//! See the @ref section_unreal_hx_state_function "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A state function with three states that change about high and low transition positions.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHx3StateFunction : public UHxStateFunction {
  GENERATED_BODY()

  public:

    //! Default constructor.
    UHx3StateFunction() : UHxStateFunction(), low_transition_position_(-1.f),
        high_transition_position_(1.f), invert_state_order_(false) {}

    //! @brief Updates the state.
    //!
    //! 0 is the most negative portion of positions unless #invert_state_order_ is true.
    //!
    //! @param input_position The new position.
    //!
    //! @returns 0-2 normally. Will always return STATE_FUNCTION_INVALID_STATE if 
    //! #low_transition_position_ >= #high_transition_position_.

    // Updates the state.
    virtual int update(float input_position) override;

    //! The position along the degree of freedom where the transition from low to mid 
    //! happens.

    // The position along the degree of freedom where the transition from low to mid happens.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    float low_transition_position_;

    //! The position along the degree of freedom where the transition from mid to high
    //! happens.

    // The position along the degree of freedom where the transition from mid to high happens.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    float high_transition_position_;

    //! By default, state values increase as position increases. Set this to true to invert
    //! that ordering.

    // By default, state values increase as position increases. Set this to true to invert that 
    // ordering.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    bool invert_state_order_;
};

//! @brief A state function with N states that change based on N positions.
//!
//! See the @ref section_unreal_hx_state_function "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A state function with N states that change based on N positions.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxNStateFunction : public UHxStateFunction {
  GENERATED_BODY()

public:
  UHxNStateFunction() : UHxStateFunction(), state_positions_(), invert_state_order_(false) {}

  //! Sorts state positions from smallest to largest unless invert_state_order_ is true,
  //! in which case sorts from largest to smallest.

  // Sorts state positions from smallest to largest unless invert_state_order_ is true,
  // in which case sorts from largest to smallest.
  void initialize() override;

  //! @brief Updates the state.
  //!
  //! State positions are sorted, so 0 always corresponds to positions lower than the smallest 
  //! position, N-1 corresponds to positions higher than the largest position, and the rest of the
  //! states are sorted in-between. If invert_state_order_ is true, that is flipped. Will always 
  //! output STATE_FUNCTION_INVALID_STATE if the array is empty.
  //!
  //! @param input_position The new position.
  //!
  //! @returns The index of the state position that is nearest the input position.

  // Updates the state.
  int update(float input_position) override;

  //! @brief Sets new state positions.
  //!
  //! The new state positions will be sorted after being copied.
  //!
  //! @param state_positions The new state positions.

  // Sets new state positions.
  UFUNCTION(BlueprintCallable, Category = "Parameters")
  void setStatePositions(const TArray<float>& state_positions);

protected:

  //! The list of state positions.

  // The list of state positions.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
  TArray<float> state_positions_;

  //! By default, state values increase as position increases. Set this to true to invert 
  //! that ordering.

  // By default, state values increase as position increases. Set this to true to invert that 
  // ordering.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
  bool invert_state_order_;

private:
  //! Sorts state positions from least-to-greatest or greatest-to-least depending on 
  //! #invert_state_order_.
  void sortStatePositions();
};

//! @brief A state function whose states correspond to the output of a UCurveFloat.
//!
//! See the @ref section_unreal_hx_state_function "Unreal Haptic Primitive Guide" for a high level
//! overview.
//!
//! @ingroup group_unreal_haptic_primitives

// A state function whose states correspond to the output of a UCurveFloat.
UCLASS(ClassGroup = ("HaptX"), Blueprintable, DefaultToInstanced, EditInlineNew)
class HAPTXPRIMITIVES_API UHxCurveStateFunction : public UHxStateFunction {
  GENERATED_BODY()

  public:
    UHxCurveStateFunction() : UHxStateFunction(), input_scale_(1.f), input_offset_(0.f), 
        curve_(nullptr) {}

    //! Updates the state.
    //!
    //! @param input_position The new position.
    //!
    //! @returns A state extracted from the output of #curve_ rounded to the nearest integer. If 
    //! the curve is not set, returns STATE_FUNCTION_INVALID_STATE.

    // Updates the state.
    virtual int update(float input_position) override;

    //! @brief The amount by which to scale the input to the curve.
    //!
    //! It is 'a' in: y = f(ax + b). The units of 'x' are [cm] on 
    //! @link UHxLinearDof UHxLinearDofs @endlink and [deg] on 
    //! @link UHxAngularDof UHxAngularDofs @endlink.

    // The amount by which to scale the input to the curve.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    float input_scale_;

    //! @brief The amount by which to offset the input to the curve. 
    //!
    //! It is 'b' in: y = f(ax + b). The units of 'x' are [cm] on 
    //! @link UHxLinearDof UHxLinearDofs @endlink and [deg] on 
    //! @link UHxAngularDof UHxAngularDofs @endlink. 

    // The amount by which to offset the input to the curve
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    float input_offset_;

    //! The curve that defines this state function.

    // The curve that defines this state function.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    UCurveFloat* curve_;
};
