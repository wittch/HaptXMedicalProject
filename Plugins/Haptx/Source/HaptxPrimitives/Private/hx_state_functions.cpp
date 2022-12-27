// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/hx_state_functions.h>

int UHxStateFunction::getCurrentState() const {
  return current_state_;
};

int UHxStateFunction::update(float input_position) {
  changeState(STATE_FUNCTION_INVALID_STATE);
  return current_state_;
};

void UHxStateFunction::changeState(int new_state) {
  if (current_state_ != new_state) {
    // Fire event
    on_state_change_.Broadcast(new_state);
  }
  current_state_ = new_state;
}

int UHx2StateFunction::update(float input_position) {
  int new_state = invert_state_order_ == (input_position < transition_position_) ? 1 : 0;
  changeState(new_state);
  return current_state_;
};

int UHx3StateFunction::update(float input_position) {
  int new_state = STATE_FUNCTION_INVALID_STATE;
  if (low_transition_position_ >= high_transition_position_) {
    UE_LOG(HaptxPrimitives, Warning, TEXT(
        "Low transition position higher then high transition position in UHx3StateFunction: low %f > high %f"),
        low_transition_position_, high_transition_position_);
  }
  else if (input_position < low_transition_position_) {
    new_state = invert_state_order_ ? 2 : 0;
  }
  else if (input_position < high_transition_position_) {
    new_state = 1;
  }
  else {
    new_state = invert_state_order_ ? 0 : 2;
  }
  changeState(new_state);
  return current_state_;
};

void UHxNStateFunction::initialize() {
  sortStatePositions();
}

int UHxNStateFunction::update(float input_position) {
  if (state_positions_.Num() < 1) {
    changeState(STATE_FUNCTION_INVALID_STATE);
    return -1;
  }
  else {
    // Find a valid place to start searching for our nearest state.
    int starting_index = current_state_ - 1;
    if (starting_index < 0) {
      starting_index = 0;
    }
    else if (starting_index > state_positions_.Num() - 1) {
      starting_index = state_positions_.Num() - 1;
    }
    
    // Initialize search variables.
    int current_index = starting_index;
    int closest_index = starting_index;
    float smallest_distance = FMath::Abs(state_positions_[current_index] - input_position);

    // Choose which direction to search.
    bool go_left = false;
    if (starting_index - 1 > -1 &&
      FMath::Abs(state_positions_[starting_index - 1] - input_position) < smallest_distance) {
      go_left = true;
    }

    if (go_left) {
      // Check to the left, stopping if the beginning is reached, or if the next index is
      // further then the current index.
      while (current_index > 0) {
        float next_distance = FMath::Abs(state_positions_[current_index - 1] - input_position);
        if (smallest_distance > next_distance) {
          smallest_distance = next_distance;
          closest_index = current_index - 1;
          current_index = closest_index;
        }
        else {
          break;
        }
      }
    }
    else {
      // Check to the right, stopping as soon as a value exceeds smallest distance.
      current_index = starting_index;
      while (current_index < state_positions_.Num() - 1) {
        float next_distance = FMath::Abs(state_positions_[current_index + 1] - input_position);
        if (smallest_distance > next_distance) {
          smallest_distance = next_distance;
          closest_index = current_index + 1;
          current_index += 1;
        }
        else {
          break;
        }
      }
    }

    changeState(closest_index);
    return closest_index;
  }
}

void UHxNStateFunction::setStatePositions(const TArray<float>& state_positions) {
  state_positions_ = state_positions;
  sortStatePositions();
}

void UHxNStateFunction::sortStatePositions() {
  state_positions_.Sort();
  if (invert_state_order_) {
    TArray<float> sorted = state_positions_;
    for (int i = 0; i < sorted.Num(); i++) {
      state_positions_[sorted.Num() - 1 - i] = sorted[i];
    }
  }
}

int UHxCurveStateFunction::update(float input_position) {
  int new_state;
  if (curve_ == nullptr) {
    new_state = STATE_FUNCTION_INVALID_STATE;
  }
  else {
    new_state = (int)roundf(curve_->GetFloatValue(input_position * input_scale_ + input_offset_));
  }
  changeState(new_state);
  return current_state_;
};
