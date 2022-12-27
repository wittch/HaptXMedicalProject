// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Private/haptx_shared.h>
#include <HaptxApi/contact_interpreter.h>
#include "contact_interpreter_parameters.generated.h"

//! @brief Holds rendering information relevant to @link HaptxApi::Tactor Tactors @endlink.
//!
//! Wraps HaptxApi::ContactInterpreter::TactorParameters.

// Holds rendering information relevant to Tactors. Wraps
// HaptxApi::ContactInterpreter::TactorParameters.
USTRUCT()
struct FTactorParameters {
  GENERATED_BODY()

  //! Default constructor.
  FTactorParameters() {
    HaptxApi::ContactInterpreter::TactorParameters params;
    dynamic_scaling = params.dynamic_scaling;
    max_height_target_cm = unrealFromHxLength(params.max_height_target_m);
  }

  //! Unwraps this instance.
  //!
  //! @returns An unwrapped instance.
  HaptxApi::ContactInterpreter::TactorParameters unwrap() const {
    HaptxApi::ContactInterpreter::TactorParameters params;
    params.dynamic_scaling = dynamic_scaling;
    params.max_height_target_m = hxFromUnrealLength(max_height_target_cm);
    return params;
  }

  //! How much the target Tactor height should vary as a ratio of the height suggested by the
  //! physical model [cm/cm].

  // How much the target Tactor height should vary as a ratio of the height suggested by the
  // physical model [cm/cm].
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin="0.0", ClampMin="0.0",
      UIMax="1.0"))
  float dynamic_scaling;

  //! Maximum height target the Tactor will be allowed to attain [cm].
  //!
  //! The actual maximum is dependent on the particular Tactor and is limited in hardware.

  // Maximum height target the Tactor will be allowed to attain [cm]. The actual maximum is
  // dependent on the particular Tactor and is limited in hardware.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin="0.0", ClampMin="0.0",
      UIMax="0.3", DisplayName="Max Height Target [cm]"))
  float max_height_target_cm;
};

//! @brief Retractuator-specific rendering settings.
//!
//! Wraps HaptxApi::ContactInterpreter::RetractuatorParameters.

// Retractuator-specific rendering settings. Wraps
// HaptxApi::ContactInterpreter::RetractuatorParameters.
USTRUCT()
struct FRetractuatorParameters {
  GENERATED_BODY()

  //! Default constructor.
  FRetractuatorParameters() {
    HaptxApi::ContactInterpreter::RetractuatorParameters params;
    actuation_threshold_cn = unrealFromHxLength(params.actuation_threshold_n);
    filter_strength_s = params.filter_strength_s;
    release_threshold_cn_s = unrealFromHxLength(params.release_threshold_n_s);
  }

  //! Unwraps this instance.
  //!
  //! @returns An unwrapped instance.
  HaptxApi::ContactInterpreter::RetractuatorParameters unwrap() const {
    HaptxApi::ContactInterpreter::RetractuatorParameters params;
    params.actuation_threshold_n = hxFromUnrealLength(actuation_threshold_cn);
    params.filter_strength_s = filter_strength_s;
    params.release_threshold_n_s = hxFromUnrealLength(release_threshold_cn_s);
    return params;
  }

  //! The minimum sum of scalar projections of contact forces [cN] along actuation directions
  //! required to engage a Retractuator.

  // The minimum sum of scalar projections of contact forces [cN] along actuation directions
  // required to engage a Retractuator.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin="0.0", ClampMin="0.0",
      DisplayName="Actuation Threshold [cN]"))
  float actuation_threshold_cn;

  //! How aggressively nominal Retractuator forces will be filtered when comparing against the
  //! release threshold (similar to time constant) [s].
  //!
  //! Smaller values lead to faster responses but more jitter; larger values lead to slower but
  //! smoother responses.

  // How aggressively nominal Retractuator forces will be filtered when comparing against the
  // release threshold (similar to time constant) [s]. Smaller values lead to faster responses but
  // more jitter; larger values lead to slower but smoother responses.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin="0.0", ClampMin="0.0",
      UIMax="5.0", DisplayName="Filter Strength [s]"))
  float filter_strength_s;

  //! The force rate required to release a Retractuator when a contact is still present [cN/s].
  //!
  //! Smaller values lead to more aggressive release behavior but more jitter; larger values
  //! lead to more conservative and slower responses, potentially increasing the perception of
  //! stickiness in the force feedback system.

  // The force rate required to release a Retractuator when a contact is still present [cN/s].
  // Smaller values lead to more aggressive release behavior but more jitter; larger values
  // lead to more conservative and slower responses, potentially increasing the perception of
  // stickiness in the force feedback system.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin="0.0", ClampMin="0.0",
      DisplayName="Release Threshold [cN/s]"))
  float release_threshold_cn_s;
};

//! Parameters for haptically enabled bodies controlled by a user.
//!
//! Wraps HaptxApi::ContactInterpreter::BodyParameters.

// Parameters for haptically enabled bodies controlled by a user. Wraps
// HaptxApi::ContactInterpreter::BodyParameters.
USTRUCT()
struct FBodyParameters {
  GENERATED_BODY()

  //! Default constructor.
  FBodyParameters() {
    HaptxApi::ContactInterpreter::BodyParameters params;
    base_contact_tolerance_cm = unrealFromHxLength(params.base_contact_tolerance_m);
    compliance_cm_cn = params.compliance_m_n;
  }

  //! Unwraps this instance.
  //!
  //! @returns An unwrapped instance.
  HaptxApi::ContactInterpreter::BodyParameters unwrap() const {
    HaptxApi::ContactInterpreter::BodyParameters params;
    params.base_contact_tolerance_m = hxFromUnrealLength(base_contact_tolerance_cm);
    params.compliance_m_n = compliance_cm_cn;

    return params;
  }

  //! Base contact distance tolerance [cm].
  //!
  //! The maximum separation [cm] measured along the ray trace vector that is sufficient to
  //! consider two objects as inter-penetrating in the absence of contact force.  Larger contact
  //! tolerances will cause more haptic rendering when two rigid bodies are close but not
  //! visually in contact.

  // Base contact distance tolerance [cm]. The maximum separation [cm] measured along the ray trace
  // vector that is sufficient to consider two objects as inter-penetrating in the absence of
  // contact force.  Larger contact tolerances will cause more haptic rendering when two rigid
  // bodies are close but not visually in contact.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin = "-1.0", UIMax = "1.0",
      DisplayName="Base Contact Tolerance [cm]"))
  float base_contact_tolerance_cm;

  //! Compliance [cm/cN].
  //!
  //! Contact distance tolerance increase per unit force [cm/cN]. Larger values imply softer
  //! haptically enabled bodies and more distributed contact regions.

  //! Compliance [cm/cN]. Contact distance tolerance increase per unit force [cm/cN]. Larger values
  //! imply softer haptically enabled bodies and more distributed contact regions.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(ClampMin="0.0", UIMin = "0.0",
      UIMax = "0.01", DisplayName="Compliance [cm/cN]"))
  float compliance_cm_cn;
};

//! Properties related to individual objects from the simulation.
//!
//! Wraps HaptxApi::ContactInterpreter::ObjectParameters.

// Properties related to individual objects from the simulation. Wraps
// HaptxApi::ContactInterpreter::ObjectParameters.
USTRUCT()
struct FObjectParameters {
  GENERATED_BODY()

  //! Default constructor.
  FObjectParameters() {
    HaptxApi::ContactInterpreter::ObjectParameters params;
    triggers_tactile_feedback = params.triggers_tactile_feedback;
    triggers_force_feedback = params.triggers_force_feedback;
    base_contact_tolerance_cm = unrealFromHxLength(params.base_contact_tolerance_m);
    compliance_cm_cn = params.compliance_m_n;
  }

  //! Unwraps this instance.
  //!
  //! @returns An unwrapped instance.
  HaptxApi::ContactInterpreter::ObjectParameters unwrap() const {
    HaptxApi::ContactInterpreter::ObjectParameters params;
    params.triggers_tactile_feedback = triggers_tactile_feedback;
    params.triggers_force_feedback = triggers_force_feedback;
    params.base_contact_tolerance_m = hxFromUnrealLength(base_contact_tolerance_cm);
    params.compliance_m_n = compliance_cm_cn;
    return params;
  }

  //! Whether the object can elicit a tactile response.
  //!
  //! This must be true in order for the user to feel the object's contour.

  // Whether the object can elicit a tactile response. This must be true in order for the user to
  // feel the object's contour.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  bool triggers_tactile_feedback;

  //! Whether the object can elicit a force-feedback response.
  //!
  //! This must be true in order for the user's motion to be restricted when contacting the
  //! object.

  // Whether the object can elicit a force-feedback response. This must be true in order for the
  // user's motion to be restricted when contacting the object.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter")
  bool triggers_force_feedback;

  //! Base contact distance tolerance [cm].
  //!
  //! The maximum separation [cm] measured along the ray trace vector that is sufficient to
  //! consider two objects as inter-penetrating in the absence of contact force. Larger contact
  //! tolerances will cause more haptic rendering when two rigid bodies are close but not
  //! visually in contact.

  // Base contact distance tolerance [cm]. The maximum separation [cm] measured along the ray trace
  // vector that is sufficient to consider two objects as inter-penetrating in the absence of
  // contact force. Larger contact tolerances will cause more haptic rendering when two rigid
  // bodies are close but not visually in contact.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(UIMin = "-1.0", UIMax = "1.0",
      DisplayName="Base Contact Tolerance [cm]"))
  float base_contact_tolerance_cm;

  //! Compliance [cm/cN].
  //!
  //! Contact distance tolerance increase per unit force [cm/cN]. Larger values imply softer
  //! haptically enabled objects that produce more distributed contact regions.

  // Compliance [cm/cN]. Contact distance tolerance increase per unit force [cm/cN]. Larger values
  // imply softer haptically enabled objects that produce more distributed contact regions.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(ClampMin="0.0", UIMin = "0.0",
      UIMax = "0.01", DisplayName="Compliance [cm/cN]"))
  float compliance_cm_cn;
};
