// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * FPixelComponentEditorModule
 * 
 * Editor module for PixelComponent plugin.
 * Handles import factory registration and editor-specific functionality.
 */
class FPixelComponentEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
