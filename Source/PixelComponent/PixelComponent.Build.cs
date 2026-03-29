// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PixelComponent : ModuleRules
{
	public PixelComponent(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities",
			"DeveloperSettings",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		bLegacyPublicIncludePaths = false;
	}
}
