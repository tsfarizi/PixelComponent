// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentEditorModule.h"
#include "PixelComponentFactory.h"
#include "PixelComponentAssetFactory.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeCategories.h"

#define LOCTEXT_NAMESPACE "FPixelComponentEditorModule"

EAssetTypeCategories::Type FPixelComponentEditorModule::PixelCategory = EAssetTypeCategories::None;

void FPixelComponentEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	PixelCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("PixelComponent")),
		LOCTEXT("PixelComponentCategory", "Pixel Component")
	);

	UE_LOG(LogTemp, Log, TEXT("PixelComponentEditor module loaded - Asset Category registered"));
}

void FPixelComponentEditorModule::ShutdownModule()
{
	PixelCategory = EAssetTypeCategories::None;
	UE_LOG(LogTemp, Log, TEXT("PixelComponentEditor module unloaded"));
}

uint32 FPixelComponentEditorModule::GetAssetCategory()
{
	return static_cast<uint32>(PixelCategory);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPixelComponentEditorModule, PixelComponentEditor)
