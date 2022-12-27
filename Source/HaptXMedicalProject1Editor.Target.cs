// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class HaptXMedicalProject1EditorTarget : TargetRules
{
	public HaptXMedicalProject1EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "HaptXMedicalProject1" } );
	}
}
