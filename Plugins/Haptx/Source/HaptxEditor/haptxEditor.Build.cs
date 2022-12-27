// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

namespace UnrealBuildTool.Rules {
  public class HaptxEditor : ModuleRules {
    public HaptxEditor(ReadOnlyTargetRules target) : base(target) {
      PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
      PrivatePCHHeaderFile = "Public/ihaptx_editor.h";

      PrivateIncludePaths.AddRange(
        new string[] {
          "HaptxEditor/Private",
          "Haptx/Private"
        }
      );

      PrivateDependencyModuleNames.AddRange(
        new string[] {
          "Core",
          "CoreUObject",
          "UnrealEd",
          "HaptX",
          "ComponentVisualizers",
          "Engine"
        }
      );
    }
  }
}
