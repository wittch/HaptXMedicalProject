// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/PhysicalMaterials/PhysicalMaterial.h>
#include <Runtime/Engine/Classes/PhysicsEngine/ConstraintInstance.h>
#include "hx_physical_material.generated.h"

//! @brief Allows for the association of haptic properties on a per-object basis.
//!
//! See the @ref section_unreal_physical_material "Unreal Plugin Guide" for a high level overview.
//!
//! @note Note that while most of these parameters can be changed at runtime, some of them will 
//! not be reflected on previously registered objects unless 
//! AHxCoreActor::registerObjectWithCi() and AHxCoreActorregisterObjectWithGd() are called again
//! with the register_again flag set to true. This is something we plan to streamline in a future 
//! release.
//!
//! @ingroup group_unreal_plugin

// Allows for the association of haptic properties on a per-object basis.
UCLASS(BlueprintType, Blueprintable, CollapseCategories, HideCategories = Object)
class HAPTX_API UHxPhysicalMaterial : public UPhysicalMaterial {
  GENERATED_UCLASS_BODY()
  
public:

  //! Disable this object's ability to produce tactile feedback.

  // Disable this object's ability to produce tactile feedback.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Interpreter")
  bool disable_tactile_feedback_;

  //! Override the default settings for #force_feedback_enabled_.

  // Override the default settings for force feedback enabled.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Interpreter")
  bool override_force_feedback_enabled_;

  //! Control this object's ability to produce force feedback.

  // Control this object's ability to produce force feedback.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Interpreter",
      meta = (editcondition = "override_force_feedback_enabled_"))
  bool force_feedback_enabled_;

  //! Override the default settings for #base_contact_tolerance_cm_.

  // Override the default settings for base contact tolerance.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Interpreter")
  bool override_base_contact_tolerance_;

  //! Maximum distance to cause haptic actuation [cm].

  // Maximum distance to cause haptic actuation [cm].
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Interpreter",
      meta = (editcondition = "override_base_contact_tolerance_", UIMin = "-1.0", UIMax = "1.0"))
  float base_contact_tolerance_cm_;

  //! Override the default settings for #compliance_cm_cn_.

  // Override the default settings for compliance.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Interpreter")
  bool override_compliance_;

  //! Contact tolerance increase per unit force [cm/cN].

  // Contact tolerance increase per unit force [cm/cN].
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Interpreter",
      meta = (editcondition = "override_compliance_", UIMin = "0.0",
      UIMax = "0.01"))
  float compliance_cm_cn_;

  //! @brief Override the default settings for #grasping_enabled_.
  //! 
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for grasping enabled.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Grasping")
  bool override_grasping_enabled_;

  //! @brief Control this object's ability to be grasped.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Control this object's ability to be grasped.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasping",
      meta = (editcondition = "override_grasping_enabled_"))
  bool grasping_enabled_;

  //! @brief Override the default settings for #grasp_threshold_. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for grasp threshold.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Grasping")
  bool override_grasp_threshold_;

  //! @brief The grasp threshold to use if overriding the default. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The grasp threshold to use if overriding the default.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasping",
      meta = (editcondition = "override_grasp_threshold_", ClampMin = "0.0", UIMin = "0.0"))
  float grasp_threshold_;

  //! @brief Override the default settings for #release_hysteresis_. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for release hysteresis.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Grasping")
  bool override_release_hysteresis_;

  //! @brief The release hysteresis to use if overriding the default. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The release hysteresis to use if overriding the default.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasping",
      meta = (editcondition = "override_release_hysteresis_", ClampMin = "0.0", UIMin = "0.0", 
      ClampMax = "1.0", UIMax = "1.0"))
  float release_hysteresis_;

  //! @brief Override the default settings for both linear drive parameters (StickConstraint) and angular drive parameters (PinchConstraint) with #grasp_drives_. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // Override the default settings for linear drive parameters and angular drive parameters.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle), Category = "Grasping")
  bool override_grasp_drives_;

  //! @brief The drive parameters that assist when grasping an object to use if overriding the default. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The drive parameters that assist when grasping an object to use if overriding the default.
  UPROPERTY(EditAnywhere, meta = (editcondition = "override_grasp_drives_"), Category = "Grasping")
  FConstraintDrive grasp_drives_;

  //! @brief Override the default settings for #grasp_linear_limit_. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for grasp linear limit.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Grasping")
  bool override_grasp_linear_limit_;

  //! @brief The linear limit settings to use if overriding the default.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The linear limit settings to use if overriding the default.
  UPROPERTY(EditAnywhere, Category = "Grasping", 
      meta = (editcondition = "override_grasp_linear_limit_"))
  FLinearConstraint grasp_linear_limit_;

  //! @brief Override the default settings for #grasp_cone_limit_.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for grasp cone limit.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, 
      meta = (InlineEditConditionToggle), Category = "Grasping")
  bool override_grasp_cone_limit_;

  //! @brief The swing1 and swing2 limit settings to use if overriding the default.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The swing1 and swing2 limit settings to use if overriding the default.
  UPROPERTY(EditAnywhere, Category = "Grasping", 
      meta = (editcondition = "override_grasp_cone_limit_"))
  FConeConstraint grasp_cone_limit_;

  //! @brief Override the default settings for #grasp_twist_limit_. 
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for grasp twist limit.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Grasping")
  bool override_grasp_twist_limit_;

  //! @brief The twist limit settings to use if overriding the default.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting 
  //! will be overridden by the value from the weld parent.

  // The twist limit settings to use if overriding the default.
  UPROPERTY(EditAnywhere, Category = "Grasping", 
      meta = (editcondition = "override_grasp_twist_limit_"))
  FTwistConstraint grasp_twist_limit_;

  //! @brief Override the default settings for #contact_damping_enabled_.
  //! 
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for contact damping enabled.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Damping")  
    bool override_contact_damping_enabled_;

  //! @brief Whether this object's motion gets damped when in contact with the palm. This makes it 
  //! considerably easier to hold.
  //! 
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Whether this object's motion gets damped when in contact with the palm. This makes it 
  // considerably easier to hold.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Damping",
      meta = (editcondition = "override_contact_damping_enabled_"))
  bool contact_damping_enabled_;

  //! @brief Override the default settings for #linear_contact_damping_.
  //! 
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for linear contact damping.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Damping")
  bool override_linear_contact_damping_;

  //! @brief Linear damping used in the physics constraint that forms between the palm and objects 
  //! contacting it.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Linear damping used in the physics constraint that forms between the palm and objects
  // contacting it.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Damping",
      meta = (editcondition = "override_linear_contact_damping_", ClampMin = "0.0", UIMin = "0.0"))
  float linear_contact_damping_;

  //! @brief Override the default settings for #angular_contact_damping_.
  //! 
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Override the default settings for angular contact damping.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle),
      Category = "Contact Damping")
  bool override_angular_contact_damping_;

  //! @brief Angular damping used in the physics constraint that forms between the palm and objects 
  //! contacting it.
  //!
  //! Note: this property applies to rigidly grouped objects. If an object is welded, the setting
  //! will be overridden by the value from the weld parent.

  // Angular damping used in the physics constraint that forms between the palm and objects
  // contacting it.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Damping",
      meta = (editcondition = "override_angular_contact_damping_", ClampMin = "0.0",
      UIMin = "0.0"))
  float angular_contact_damping_;
};
