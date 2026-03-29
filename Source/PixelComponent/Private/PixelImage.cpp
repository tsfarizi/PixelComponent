// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelImage.h"
#include "PixelComponentMaterialLibrary.h"
#include "Components/Widget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

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

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, PixelAsset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, TargetSlice) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, ActivePaletteProfile) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelImage, bAutoInitializeMaterial))
	{
		if (bAutoInitializeMaterial && PixelAsset)
		{
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
			SendMaterialParameters();
		}
	}
}

void UPixelImage::SetActivePaletteProfile(const FName& NewProfile)
{
	if (ActivePaletteProfile != NewProfile)
	{
		ActivePaletteProfile = NewProfile;

		if (bAutoInitializeMaterial && PixelAsset && DynamicMaterialInstance)
		{
			FPixelComponentMaterialLibrary::ApplyPaletteProfile(PixelAsset, ActivePaletteProfile, DynamicMaterialInstance);
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

	if (!DynamicMaterialInstance && bAutoInitializeMaterial)
	{
		InitializeMaterialInstance();
	}

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

	SetBrush(FSlateBrush());
}

void UPixelImage::InitializeMaterialInstance()
{
	if (!PixelAsset)
	{
		UE_LOG(LogPixelImage, Warning, TEXT("Cannot initialize material: no PixelAsset assigned"));
		return;
	}

	UTexture2D* SourceTexture = PixelAsset->GetSourceTexture();
	if (!SourceTexture)
	{
		UE_LOG(LogPixelImage, Warning, TEXT("Cannot initialize material: PixelAsset has no source texture"));
		return;
	}

	UMaterialInterface* BaseMaterial = GetBaseMaterial();

	if (!BaseMaterial)
	{
		UE_LOG(LogPixelImage, Log, TEXT("No base material in brush, using default material"));

		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));

		if (!BaseMaterial)
		{
			UE_LOG(LogPixelImage, Error, TEXT("Failed to load default material. Please assign a material to the widget's brush."));
			return;
		}
	}

	DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);

	if (!DynamicMaterialInstance)
	{
		UE_LOG(LogPixelImage, Error, TEXT("Failed to create dynamic material instance"));
		return;
	}

	DynamicMaterialInstance->SetTextureParameterValue(FName(TEXT("PixelTexture")), SourceTexture);

	SetBrushFromMaterial(DynamicMaterialInstance);

	UE_LOG(LogPixelImage, Log, TEXT("Initialized material instance for PixelImage"));
}

void UPixelImage::SendMaterialParameters()
{
	if (!PixelAsset || !DynamicMaterialInstance)
	{
		return;
	}

	FPixelComponentMaterialLibrary::SendTextureDimensionsToMaterial(PixelAsset, DynamicMaterialInstance);

	FPixelComponentMaterialLibrary::SendSliceUVToMaterial(PixelAsset, TargetSlice, DynamicMaterialInstance);

	if (!TargetSlice.IsEmpty())
	{
		const FSliceData* Slice = PixelAsset->FindSliceByName(TargetSlice);
		if (Slice && Slice->bIsNineSlice)
		{
			FPixelComponentMaterialLibrary::SendSliceNineSliceToMaterial(
				PixelAsset, TargetSlice, DynamicMaterialInstance, true);
		}
	}
	else
	{
		FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(PixelAsset, DynamicMaterialInstance, true);
	}

	FPixelComponentMaterialLibrary::SendPivotToMaterial(PixelAsset, TargetSlice, DynamicMaterialInstance);

	if (ActivePaletteProfile != NAME_None)
	{
		FPixelComponentMaterialLibrary::ApplyPaletteProfile(PixelAsset, ActivePaletteProfile, DynamicMaterialInstance);
	}

	UE_LOG(LogPixelImage, Verbose, TEXT("Sent material parameters for PixelImage (Slice: %s, Palette: %s)"),
		TargetSlice.IsEmpty() ? TEXT("<full texture>") : *TargetSlice,
		ActivePaletteProfile == NAME_None ? TEXT("<none>") : *ActivePaletteProfile.ToString());
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
	const FSlateBrush CurrentBrush = GetBrush();
	if (CurrentBrush.GetResourceObject())
	{
		return Cast<UMaterialInterface>(CurrentBrush.GetResourceObject());
	}

	return nullptr;
}
