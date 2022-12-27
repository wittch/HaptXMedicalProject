// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class HaptXMedicalProject1Target : TargetRules
{
	public HaptXMedicalProject1Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "HaptXMedicalProject1" } );
	}
}
