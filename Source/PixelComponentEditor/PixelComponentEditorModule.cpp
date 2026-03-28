// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentEditorModule.h"
#include "PixelComponentFactory.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FPixelComponentEditorModule"

void FPixelComponentEditorModule::StartupModule()
{
	// Register asset factory for Aseprite JSON import
	UE_LOG(LogTemp, Log, TEXT("PixelComponentEditor module loaded"));
}

void FPixelComponentEditorModule::ShutdownModule()
{
	// Unregister factories if needed
	UE_LOG(LogTemp, Log, TEXT("PixelComponentEditor module unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPixelComponentEditorModule, PixelComponentEditor)
