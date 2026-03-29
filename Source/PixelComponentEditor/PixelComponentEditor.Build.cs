// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PixelComponentEditor : ModuleRules
{
	public PixelComponentEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"PixelComponent",
			"AssetTools"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"ToolMenus",
			"EditorStyle"
		});

		bLegacyPublicIncludePaths = false;
	}
}
