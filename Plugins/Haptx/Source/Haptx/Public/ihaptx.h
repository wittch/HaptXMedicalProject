// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Core/Public/Modules/ModuleInterface.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>

//! @defgroup group_unreal_plugin Unreal Plugin
//!
//! @brief Integration with the Unreal development environment.
//!
//! See the @ref page_unreal_integration_guide to install and use these classes.
//!
//! See the @ref page_unreal_plugin_summary for a high level overview.

DECLARE_LOG_CATEGORY_EXTERN(HaptX, Log, All);

// Macros designed to enable/disable deep profiling in this plugin by commenting-out/uncommenting
// the following #define.
//#define PROFILING
#ifdef PROFILING
#define DECLARE_STATS_GROUP_IF_PROFILING(name, group, stat_cat) DECLARE_STATS_GROUP(name, group, stat_cat)
#define DECLARE_CYCLE_STAT_IF_PROFILING(name, stat, group) DECLARE_CYCLE_STAT(name, stat, group)
#define SCOPE_CYCLE_COUNTER_IF_PROFILING(stat) SCOPE_CYCLE_COUNTER(stat)
#else
#define DECLARE_STATS_GROUP_IF_PROFILING(name, group, stat_cat)
#define DECLARE_CYCLE_STAT_IF_PROFILING(name, stat, group)
#define SCOPE_CYCLE_COUNTER_IF_PROFILING(stat)
#endif

//! Manages the HaptX module.
class IHaptx : public IModuleInterface {

public:

  //! Getter for the HaptX module.
  //!
  //! @returns The HaptX module.
  static inline IHaptx& Get() {
    return FModuleManager::LoadModuleChecked< IHaptx >( "HaptX" );
  }

  //! Checks whether the HaptX module is loaded.
  //!
  //! @returns Whether the HaptX module is loaded.
  static inline bool IsAvailable() {
    return FModuleManager::Get().IsModuleLoaded( "HaptX" );
  }
};
