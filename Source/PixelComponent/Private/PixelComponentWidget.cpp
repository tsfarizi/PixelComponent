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
{
	bIsVariable = true;
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

	UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
	if (!Settings || !Settings->DefaultPixelMaterial)
	{
		UE_LOG(LogPixelComponent, Error, TEXT("No default pixel material specified in settings"));
		return;
	}

	DynamicMaterialInstance = UMaterialInstanceDynamic::Create(Settings->DefaultPixelMaterial, this);

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
	CurrentBrush.ImageSize = FVector2f(static_cast<float>(SourceTexture->GetSurfaceWidth()), 
		static_cast<float>(SourceTexture->GetSurfaceHeight()));
	SetBrush(CurrentBrush);

	UE_LOG(LogPixelComponent, Verbose, TEXT("Applied texture from asset: %s"), *SourceTexture->GetName());
}
