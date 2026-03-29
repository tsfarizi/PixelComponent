// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Include public headers for convenience
#include "PixelComponentTypes.h"
#include "PixelComponentAsset.h"
#include "PixelComponentMaterialLibrary.h"
#include "PixelComponentSettings.h"
#include "PixelImage.h"
#include "PixelAssetPicker.h"

/**
 * FPixelComponentModule
 * 
 * Main module class for the PixelComponent plugin.
 * This plugin provides Aseprite pixel art importing and data management.
 */
class FPixelComponentModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
