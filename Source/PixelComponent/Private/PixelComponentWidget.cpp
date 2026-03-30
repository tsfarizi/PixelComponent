// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentWidget.h"
#include "PixelComponentMaterialLibrary.h"
#include "PixelComponentSettings.h"
#include "Components/Widget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponent, Log, All);

UPixelComponent::UPixelComponent()
	: PixelAsset(nullptr)
	, bAutoInitializeMaterial(true)
	, DynamicMaterialInstance(nullptr)
	, bFixedSize(true)
{
	bIsVariable = true;
	
	DesiredSize = FVector2f(128.0f, 128.0f);
}

void UPixelComponent::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (bAutoInitializeMaterial && PixelAsset)
	{
		RefreshMaterialParameters();
	}
}

void UPixelComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponent, PixelAsset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponent, TargetSlice) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponent, ActivePaletteProfile) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponent, bAutoInitializeMaterial))
	{
		if (bAutoInitializeMaterial && PixelAsset)
		{
			RefreshMaterialParameters();
		}
	}
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponent, PixelAsset))
	{
		CalculateAndApplyDimensions();
	}
}

void UPixelComponent::SetPixelAsset(UPixelComponentAsset* NewAsset)
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

void UPixelComponent::SetTargetSlice(const FString& NewSlice)
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

void UPixelComponent::SetActivePaletteProfile(const FName& NewProfile)
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

void UPixelComponent::SetAutoInitializeMaterialEnabled(bool bEnabled)
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

void UPixelComponent::RefreshMaterialParameters()
{
	if (!ValidateConfiguration())
	{
		UE_LOG(LogPixelComponent, Warning, TEXT("Cannot refresh material: invalid configuration"));
		return;
	}

	ApplyTextureFromAsset();

	if (!DynamicMaterialInstance && bAutoInitializeMaterial)
	{
		InitializeMaterialInstance();
	}

	if (DynamicMaterialInstance)
	{
		SendMaterialParameters();
	}
	
	CalculateAndApplyDimensions();
}

void UPixelComponent::ClearMaterial()
{
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->ConditionalBeginDestroy();
		DynamicMaterialInstance = nullptr;
	}

	FSlateBrush EmptyBrush;
	EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	SetBrush(EmptyBrush);
}

void UPixelComponent::InitializeMaterialInstance()
{
	if (!PixelAsset)
	{
		UE_LOG(LogPixelComponent, Warning, TEXT("Cannot initialize material: no PixelAsset assigned"));
		return;
	}

	UTexture2D* SourceTexture = PixelAsset->GetSourceTexture();
	if (!SourceTexture)
	{
		UE_LOG(LogPixelComponent, Warning, TEXT("Cannot initialize material: PixelAsset has no source texture"));
		return;
	}

	UMaterialInterface* BaseMaterial = PixelAsset->GetActiveMaterial();
	
	if (!BaseMaterial)
	{
		UE_LOG(LogPixelComponent, Log, TEXT("No active material from asset, using default"));
		
		static UMaterialInterface* DefaultMaterial = nullptr;
		if (!DefaultMaterial)
		{
			DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
		}
		
		BaseMaterial = DefaultMaterial;
		
		if (!BaseMaterial)
		{
			UE_LOG(LogPixelComponent, Error, TEXT("Failed to load default material"));
			return;
		}
	}

	DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);

	if (!DynamicMaterialInstance)
	{
		UE_LOG(LogPixelComponent, Error, TEXT("Failed to create dynamic material instance"));
		return;
	}

	DynamicMaterialInstance->SetTextureParameterValue(FName(TEXT("PixelTexture")), SourceTexture);

	SetBrushFromMaterial(DynamicMaterialInstance);

	UE_LOG(LogPixelComponent, Log, TEXT("Initialized material instance for PixelComponent"));
}

void UPixelComponent::SendMaterialParameters()
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

	UE_LOG(LogPixelComponent, Verbose, TEXT("Sent material parameters (Slice: %s, Palette: %s)"),
		TargetSlice.IsEmpty() ? TEXT("<full texture>") : *TargetSlice,
		ActivePaletteProfile == NAME_None ? TEXT("<none>") : *ActivePaletteProfile.ToString());
}

bool UPixelComponent::ValidateConfiguration() const
{
	if (!PixelAsset)
	{
		return false;
	}

	if (!PixelAsset->HasValidTexture())
	{
		UE_LOG(LogPixelComponent, Warning, TEXT("PixelAsset has no valid source texture"));
		return false;
	}

	if (!TargetSlice.IsEmpty())
	{
		bool bFound = false;
		PixelAsset->GetSliceByName(TargetSlice, bFound);

		if (!bFound)
		{
			UE_LOG(LogPixelComponent, Warning, TEXT("Target slice '%s' not found in PixelAsset"), *TargetSlice);
			return false;
		}
	}

	return true;
}

void UPixelComponent::ApplyTextureFromAsset()
{
	if (!PixelAsset)
	{
		return;
	}

	UTexture2D* SourceTexture = PixelAsset->GetSourceTexture();
	if (!SourceTexture)
	{
		UE_LOG(LogPixelComponent, Verbose, TEXT("No source texture to apply"));
		return;
	}

	FSlateBrush CurrentBrush = GetBrush();
	CurrentBrush.SetResourceObject(SourceTexture);

	FVector2D SourceDimensions = PixelAsset->GetSourceDimensions();
	CurrentBrush.ImageSize = SourceDimensions;
	SetBrush(CurrentBrush);

	UE_LOG(LogPixelComponent, Verbose, TEXT("Applied texture from asset: %s"), *SourceTexture->GetName());
}

void UPixelComponent::CalculateAndApplyDimensions()
{
	if (!PixelAsset)
	{
		DesiredSize = FVector2f(128.0f, 128.0f);
		OriginalDimensions = FVector2f(128.0f, 128.0f);
		return;
	}

	FVector2D SourceDimensions = PixelAsset->GetSourceDimensions();
	FIntPoint OriginalSize = PixelAsset->GetOriginalSourceDimensions();

	OriginalDimensions = FVector2f(static_cast<float>(OriginalSize.X), static_cast<float>(OriginalSize.Y));

	if (SourceDimensions.IsNearlyZero())
	{
		UE_LOG(LogPixelComponent, Warning, TEXT("Source dimensions are zero. Using default 128x128."));
		DesiredSize = FVector2f(128.0f, 128.0f);
	}
	else
	{
		DesiredSize = FVector2f(static_cast<float>(SourceDimensions.X), static_cast<float>(SourceDimensions.Y));
	}

	UE_LOG(LogPixelComponent, Log, TEXT("Calculated fixed dimensions: %.0fx%.0f (original: %.0fx%.0f)"),
		DesiredSize.X, DesiredSize.Y, OriginalDimensions.X, OriginalDimensions.Y);
}
