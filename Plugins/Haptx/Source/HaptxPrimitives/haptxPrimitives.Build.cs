// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

namespace UnrealBuildTool.Rules {
  public class HaptxPrimitives : ModuleRules {
    public HaptxPrimitives(ReadOnlyTargetRules target) : base(target) {
      PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
      PrivatePCHHeaderFile = "Public/ihaptx_primitives.h";

      PublicDependencyModuleNames.AddRange(
        new string[] {
          "Core",
          "CoreUObject",
          "Engine",
          "PhysX",
          "APEX"
        }
      );

      PrivateDependencyModuleNames.Add("HaptX");
    }
  }
}
