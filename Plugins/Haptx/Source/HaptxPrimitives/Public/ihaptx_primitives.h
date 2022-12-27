// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Core/Public/Modules/ModuleInterface.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>

//! @defgroup group_unreal_haptic_primitives Unreal Haptic Primitives
//!
//! @brief Design advanced haptic interactions.
//!
//! See the @ref page_unreal_integration_guide to install and use these classes.
//!
//! See the @ref page_unreal_haptic_primitive_summary for a high level overview.

DECLARE_LOG_CATEGORY_EXTERN(HaptxPrimitives, Log, All);

class IHaptxPrimitives : public IModuleInterface {

public:
  static inline IHaptxPrimitives& Get() {
    return FModuleManager::LoadModuleChecked< IHaptxPrimitives >( "HaptxPrimitives" );
  }

  static inline bool IsAvailable() {
    return FModuleManager::Get().IsModuleLoaded( "HaptxPrimitives" );
  }
};
