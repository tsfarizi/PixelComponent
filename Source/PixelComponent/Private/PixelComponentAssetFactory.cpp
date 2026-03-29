// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentAssetFactory.h"
#include "PixelComponentAsset.h"
#include "Engine/Texture2D.h"
#include "Misc/MessageDialog.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentAssetFactory, Log, All);

UPixelComponentAssetFactory::UPixelComponentAssetFactory()
{
	SupportedClass = UPixelComponentAsset::StaticClass();
	bEditorImport = false;
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UPixelComponentAssetFactory::ConfigureProperties()
{
	// Optional: Could show a dialog to select initial texture
	// For now, we'll let users assign texture in the asset editor
	return true;
}

UObject* UPixelComponentAssetFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	// Create the new PixelComponentAsset
	UPixelComponentAsset* NewAsset = NewObject<UPixelComponentAsset>(InParent, InName, Flags | RF_Transactional);

	if (NewAsset)
	{
		// Set default asset name
		NewAsset->SetAssetName(InName.ToString());

		// If a texture was selected, assign it
		if (SourceTexture)
		{
			NewAsset->SetSourceTexture(SourceTexture);
			UE_LOG(LogPixelComponentAssetFactory, Log, TEXT("Created new PixelComponentAsset with texture: %s"), *SourceTexture->GetName());
		}
		else
		{
			UE_LOG(LogPixelComponentAssetFactory, Log, TEXT("Created new empty PixelComponentAsset"));
		}

		// Mark package dirty for saving
		InParent->MarkPackageDirty();

		return NewAsset;
	}

	return nullptr;
}
