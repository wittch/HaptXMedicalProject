// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

namespace UnrealBuildTool.Rules {
  public class HaptX : ModuleRules {
    public HaptX(ReadOnlyTargetRules target) : base(target) {
      PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
      PrivatePCHHeaderFile = "Public/ihaptx.h";

      PublicDependencyModuleNames.AddRange(
        new string[] {
          "Core",
          "CoreUObject",
          "Engine",
          "HeadMountedDisplay",
          "SteamVR",
          "SteamVRController",
          "OpenVR"
        }
      );

      // Compiler options
      bUseRTTI = true;  // This is so we can use dynamic_casts

      if ((target.Platform == UnrealBuildTool.UnrealTargetPlatform.Win64)) {
        // Find the HaptX SDK environment variable.
        string haptx_directory = System.Environment.GetEnvironmentVariable("HAPTX_SDK_HOME");
        if (string.IsNullOrEmpty(haptx_directory)) {
          throw new System.Exception("ERROR: You need an environment variable called HAPTX_SDK_HOME that points to the root" +
            " directory of the HaptX SDK installation. Be sure to restart Visual Studio AND rebuild.");
        }

        // Add public libraries
        PublicLibraryPaths.AddRange(
          new string[] {
            haptx_directory + "/Lib/Release"
          }
        );

        PublicAdditionalLibraries.AddRange(
          new string[] {
            "HaptxApi.lib",
          }
        );

        // Add public includes
        PublicIncludePaths.AddRange(
          new string[] {
            haptx_directory + "/Include"
          }
        );

        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
      }
    }
  }
}
