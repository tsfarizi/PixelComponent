// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponent.h"

#define LOCTEXT_NAMESPACE "FPixelComponentModule"

void FPixelComponentModule::StartupModule()
{
	// Module loaded - PixelComponent is ready for Aseprite JSON imports
	UE_LOG(LogTemp, Log, TEXT("PixelComponent module loaded"));
}

void FPixelComponentModule::ShutdownModule()
{
	// Module shutdown - clean up if needed
	UE_LOG(LogTemp, Log, TEXT("PixelComponent module unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPixelComponentModule, PixelComponent)