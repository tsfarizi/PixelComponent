// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

/**
 * FPixelComponentEditorModule
 *
 * Editor module for PixelComponent plugin.
 * Handles import factory registration, asset category registration,
 * and editor-specific functionality.
 */
class FPixelComponentEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the registered asset category bitmask.
	 * @return Asset category bitmask for Pixel Component assets
	 */
	static uint32 GetAssetCategory();

private:
	/** Registered asset category for Pixel Component assets */
	static EAssetTypeCategories::Type PixelCategory;
};
