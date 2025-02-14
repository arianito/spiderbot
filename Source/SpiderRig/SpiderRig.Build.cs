// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpiderRig : ModuleRules
{
	public SpiderRig(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange([
			"Core", "CoreUObject", "Engine", "EnhancedInput", "ControlRig", "RigVM", "AnimationCore", "Niagara"
		]);
	}
}