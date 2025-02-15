// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RigTutorial : ModuleRules
{
	public RigTutorial(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange([
			"Core", "CoreUObject", "Engine", "EnhancedInput", "ControlRig", "RigVM", "AnimationCore"
		]);
	}
}