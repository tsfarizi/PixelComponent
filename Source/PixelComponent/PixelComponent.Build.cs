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
			"DeveloperSettings",  // For UPixelComponentSettings
			"UMG",                 // For UPixelImage widget
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Enable C++17/20 features
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("UnrealEd");
		}

		// Optimize string handling
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Off;
	}
}
