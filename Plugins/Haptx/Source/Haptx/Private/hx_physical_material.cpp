// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Haptx/Public/hx_physical_material.h>

UHxPhysicalMaterial::UHxPhysicalMaterial(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	disable_tactile_feedback_(false), override_force_feedback_enabled_(false),
	force_feedback_enabled_(true), override_base_contact_tolerance_(false),
	base_contact_tolerance_cm_(0.0f), override_compliance_(false), compliance_cm_cn_(0.0f),
	override_grasping_enabled_(false), grasping_enabled_(true), override_grasp_threshold_(false),
	grasp_threshold_(0.f), override_grasp_drives_(false), override_grasp_linear_limit_(false), override_grasp_cone_limit_(false),
	override_grasp_twist_limit_(false), override_contact_damping_enabled_(false),
	contact_damping_enabled_(true), linear_contact_damping_(0.0f),
	angular_contact_damping_(0.0f) {
	grasp_drives_.bEnablePositionDrive = true;
	grasp_drives_.bEnableVelocityDrive = true;
	grasp_drives_.Stiffness = 1.0e6f;
	grasp_drives_.Damping = 1.0e3f;
	grasp_drives_.MaxForce = 3.0e4f;
}
