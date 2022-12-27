// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/hx_dof_detent_behavior.h>

UHxDofDetentBehavior::UHxDofDetentBehavior(const FObjectInitializer& object_initializer) : 
    Super(object_initializer), detent_index_(INVALID_DETENT_INDEX) {}

float UHxDofDetentBehavior::getForceTorque(float position) {
  if (detents_.Num() > 0 && model_ != nullptr) {
    detent_index_ = getNearestDetentIndex(position, detent_index_);
    return model_->getOutput(position - detents_[detent_index_]);
  }
  else {
    return 0.f;
  }
}

bool UHxDofDetentBehavior::tryGetTarget(float& out_target) {
  if (detent_index_ > -1) {
    out_target = detents_[detent_index_];
    return true;
  }
  else {
    out_target = 0.0f;
    return false;
  }
}

void UHxDofDetentBehavior::initialize() {
  detents_.Sort();
  detent_index_ = getNearestDetentIndex(0.0f, 0);
}

int UHxDofDetentBehavior::getCurrentDetentIndex() const {
  return detent_index_;
}

void UHxDofDetentBehavior::setDetents(const TArray<float>& new_detents) {
  detents_ = new_detents;
  detents_.Sort();
}

const TArray<float>& UHxDofDetentBehavior::getDetentsSorted() {
  return detents_;
}

int UHxDofDetentBehavior::getNearestDetentIndex(float value, int starting_index) const {
  if (detents_.Num() < 1) {
    return INVALID_DETENT_INDEX;
  } else {
    if (starting_index < 0 || FMath::Abs(starting_index) > detents_.Num() - 1) {
      starting_index = 0;
    }
    int current_index = starting_index;
    int closest_index = starting_index;
    float smallest_distance = FMath::Abs(detents_[starting_index] - value);

    // Choose which direction to search.
    bool go_left = false;
    if (starting_index - 1 > -1 &&
      FMath::Abs(detents_[starting_index - 1] - value) < smallest_distance) {
      go_left = true;
    }

    if (go_left) {
      // Check to the left, stopping if the beginning is reached, or if the next index is
      // further then the current index.
      while (current_index > 0) {
        float next_distance = FMath::Abs(detents_[current_index - 1] - value);
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
      while (current_index < detents_.Num() - 1) {
        float next_distance = FMath::Abs(detents_[current_index + 1] - value);
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

    return closest_index;
  }
}
