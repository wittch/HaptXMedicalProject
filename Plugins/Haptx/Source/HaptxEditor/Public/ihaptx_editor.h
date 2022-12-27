// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Core/Public/Modules/ModuleInterface.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>

//! Manages the HaptxEditor module.
class IHaptxEditor : public IModuleInterface {

public:
  //! Getter for the HaptxEditor module.
  //!
  //! @returns The HaptxEditor module.
  static inline IHaptxEditor& Get() {
    return FModuleManager::LoadModuleChecked< IHaptxEditor >( "HaptxEditor" );
  }

  //! Checks whether the HaptxEditor module is loaded.
  //!
  //! @returns Whether the HaptxEditor module is loaded.
  static inline bool IsAvailable() {
    return FModuleManager::Get().IsModuleLoaded( "HaptxEditor" );
  }
};
