// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelImage.h"
#include "PixelComponentMaterialLibrary.h"
#include "Components/Widget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Types/SlateEnums.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelImage, Log, All);

UPixelImage::UPixelImage()
	: PixelAsset(nullptr)
	, bAutoInitializeMaterial(true)
	, DynamicMaterialInstance(nullptr)
{
}

void UPixelImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// Synchronize properties from widget to Slate
	if (bAutoInitializeMaterial && PixelAsset)
	{
		RefreshMaterialParameters();
	}
}

void UPixelImage::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();

	// Handle property changes for real-time preview in UMG Designer
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, PixelAsset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, TargetSlice) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, bAutoInitializeMaterial))
	{
		if (bAutoInitializeMaterial && PixelAsset)
		{
			// Refresh material parameters immediately for Designer preview
			RefreshMaterialParameters();
		}
	}
}

void UPixelImage::SetPixelAsset(UPixelComponentAsset* NewAsset)
{
	if (PixelAsset != NewAsset)
	{
		PixelAsset = NewAsset;

		if (bAutoInitializeMaterial && NewAsset)
		{
			RefreshMaterialParameters();
		}
	}
}

void UPixelImage::SetTargetSlice(const FString& NewSlice)
{
	if (TargetSlice != NewSlice)
	{
		TargetSlice = NewSlice;

		if (bAutoInitializeMaterial && PixelAsset)
		{
			// Only need to refresh material parameters, not reinitialize
			SendMaterialParameters();
		}
	}
}

void UPixelImage::SetAutoInitializeMaterialEnabled(bool bEnabled)
{
	if (bAutoInitializeMaterial != bEnabled)
	{
		bAutoInitializeMaterial = bEnabled;

		if (bEnabled && PixelAsset)
		{
			RefreshMaterialParameters();
		}
		else if (!bEnabled)
		{
			// Clear the dynamic material if disabling auto-init
			ClearMaterial();
		}
	}
}

void UPixelImage::RefreshMaterialParameters()
{
	if (!ValidateConfiguration())
	{
		UE_LOG(LogPixelImage, Warning, TEXT("Cannot refresh material: invalid configuration"));
		return;
	}

	// Initialize material instance if needed
	if (!DynamicMaterialInstance && bAutoInitializeMaterial)
	{
		InitializeMaterialInstance();
	}

	// Send all material parameters
	if (DynamicMaterialInstance)
	{
		SendMaterialParameters();
	}
}

void UPixelImage::ClearMaterial()
{
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->ConditionalBeginDestroy();
		DynamicMaterialInstance = nullptr;
	}

	// Reset brush to default
	SetBrush(FSlateBrush());
}

void UPixelImage::InitializeMaterialInstance()
{
	if (!PixelAsset)
	{
		UE_LOG(LogPixelImage, Warning, TEXT("Cannot initialize material: no PixelAsset assigned"));
		return;
	}

	// Get the source texture from the asset
	UTexture2D* SourceTexture = PixelAsset->GetSourceTexture();
	if (!SourceTexture)
	{
		UE_LOG(LogPixelImage, Warning, TEXT("Cannot initialize material: PixelAsset has no source texture"));
		return;
	}

	// Try to get base material from current brush
	UMaterialInterface* BaseMaterial = GetBaseMaterial();

	if (!BaseMaterial)
	{
		UE_LOG(LogPixelImage, Log, TEXT("No base material in brush, using default material"));

		// Use a default material if no brush material exists
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));

		if (!BaseMaterial)
		{
			UE_LOG(LogPixelImage, Error, TEXT("Failed to load default material. Please assign a material to the widget's brush."));
			return;
		}
	}

	// Create dynamic material instance
	DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);

	if (!DynamicMaterialInstance)
	{
		UE_LOG(LogPixelImage, Error, TEXT("Failed to create dynamic material instance"));
		return;
	}

	// Set the texture parameter
	DynamicMaterialInstance->SetTextureParameterValue(FName(TEXT("PixelTexture")), SourceTexture);

	// Apply the material to the brush
	SetBrushFromMaterial(DynamicMaterialInstance);

	UE_LOG(LogPixelImage, Log, TEXT("Initialized material instance for PixelImage"));
}

void UPixelImage::SendMaterialParameters()
{
	if (!PixelAsset || !DynamicMaterialInstance)
	{
		return;
	}

	// Send texture dimensions (automatically retrieves from SourceTexture)
	FPixelComponentMaterialLibrary::SendTextureDimensionsToMaterial(PixelAsset, DynamicMaterialInstance);

	// Send slice UV coordinates if a target slice is specified
	if (!TargetSlice.IsEmpty())
	{
		FPixelComponentMaterialLibrary::SendSliceUVToMaterial(PixelAsset, TargetSlice, DynamicMaterialInstance);
		
		// Also send 9-slice data if the slice has 9-slice margins
		const FSliceData* Slice = PixelAsset->FindSliceByName(TargetSlice);
		if (Slice && Slice->bIsNineSlice)
		{
			FPixelComponentMaterialLibrary::SendSliceNineSliceToMaterial(
				PixelAsset, TargetSlice, DynamicMaterialInstance, true);
		}
	}
	else
	{
		// Send default 9-slice data if available
		FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(PixelAsset, DynamicMaterialInstance, true);
	}

	// Send pivot point if needed
	FPixelComponentMaterialLibrary::SendPivotToMaterial(PixelAsset, TargetSlice, DynamicMaterialInstance);

	UE_LOG(LogPixelImage, Verbose, TEXT("Sent material parameters for PixelImage"));
}

bool UPixelImage::ValidateConfiguration() const
{
	if (!PixelAsset)
	{
		return false;
	}

	if (!PixelAsset->HasValidTexture())
	{
		UE_LOG(LogPixelImage, Warning, TEXT("PixelAsset has no valid source texture"));
		return false;
	}

	// Validate target slice if specified
	if (!TargetSlice.IsEmpty())
	{
		bool bFound = false;
		PixelAsset->GetSliceByName(TargetSlice, bFound);
		
		if (!bFound)
		{
			UE_LOG(LogPixelImage, Warning, TEXT("Target slice '%s' not found in PixelAsset"), *TargetSlice);
			return false;
		}
	}

	return true;
}

UMaterialInterface* UPixelImage::GetBaseMaterial() const
{
	// Get the material from the current brush
	const FSlateBrush CurrentBrush = GetBrush();
	if (CurrentBrush.GetResourceObject())
	{
		return Cast<UMaterialInterface>(CurrentBrush.GetResourceObject());
	}

	return nullptr;
}
