// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentSettings.h"
#include "PixelComponent.h"

UPixelComponentSettings* UPixelComponentSettings::CachedSettings = nullptr;

UPixelComponentSettings::UPixelComponentSettings()
	: GlobalPixelSize(16)
	, ScalingMode(EPixelSizeScalingMode::ScaleToGlobal)
	, bEnableAutoScaleOnImport(true)
	, bSnapToPixelGrid(false)
	, DefaultTextureFilter(TextureFilter::TF_Nearest)
	, bGenerateMipmaps(false)
	, AssetGroupName(TEXT("PixelArt"))
{
	CachedSettings = this;
}

UPixelComponentSettings* UPixelComponentSettings::Get()
{
	if (CachedSettings)
	{
		return CachedSettings;
	}

	// Try to get from config
	UPixelComponentSettings* Settings = GetMutableDefault<UPixelComponentSettings>();
	if (Settings)
	{
		CachedSettings = Settings;
		return Settings;
	}

	return nullptr;
}

int32 UPixelComponentSettings::GetGlobalPixelSize()
{
	UPixelComponentSettings* Settings = Get();
	if (Settings)
	{
		return Settings->GlobalPixelSize;
	}
	return 16; // Default fallback
}

void UPixelComponentSettings::SetGlobalPixelSize(int32 NewSize)
{
	UPixelComponentSettings* Settings = Get();
	if (Settings && NewSize > 0)
	{
		Settings->GlobalPixelSize = FMath::Clamp(NewSize, 1, 1024);
		Settings->SaveConfig();
		CachedSettings = Settings;
	}
}

EPixelSizeScalingMode UPixelComponentSettings::GetScalingMode()
{
	UPixelComponentSettings* Settings = Get();
	if (Settings)
	{
		return Settings->ScalingMode;
	}
	return EPixelSizeScalingMode::ScaleToGlobal;
}

float UPixelComponentSettings::CalculateScaleFactor(int32 AssetPixelSize)
{
	if (AssetPixelSize <= 0)
	{
		return 1.0f;
	}

	UPixelComponentSettings* Settings = Get();
	if (!Settings || !Settings->bEnableAutoScaleOnImport)
	{
		return 1.0f;
	}

	const int32 GlobalSize = Settings->GlobalPixelSize;

	switch (Settings->ScalingMode)
	{
	case EPixelSizeScalingMode::None:
		return 1.0f;

	case EPixelSizeScalingMode::ScaleToGlobal:
		return static_cast<float>(GlobalSize) / static_cast<float>(AssetPixelSize);

	case EPixelSizeScalingMode::ScaleDownOnly:
		if (AssetPixelSize > GlobalSize)
		{
			return static_cast<float>(GlobalSize) / static_cast<float>(AssetPixelSize);
		}
		return 1.0f;

	case EPixelSizeScalingMode::ScaleUpOnly:
		if (AssetPixelSize < GlobalSize)
		{
			return static_cast<float>(GlobalSize) / static_cast<float>(AssetPixelSize);
		}
		return 1.0f;

	default:
		return 1.0f;
	}
}

int32 UPixelComponentSettings::ApplyPixelSizeScaling(int32 OriginalSize)
{
	UPixelComponentSettings* Settings = Get();
	if (!Settings || !Settings->bEnableAutoScaleOnImport)
	{
		return OriginalSize;
	}

	const float ScaleFactor = CalculateScaleFactor(OriginalSize);
	int32 ScaledSize = FMath::RoundToInt(static_cast<float>(OriginalSize) * ScaleFactor);

	if (Settings->bSnapToPixelGrid && ScaledSize > 0)
	{
		// Round to nearest multiple of GlobalPixelSize
		const int32 PixelSize = Settings->GlobalPixelSize;
		ScaledSize = FMath::RoundToInt(static_cast<float>(ScaledSize) / PixelSize) * PixelSize;
	}

	return FMath::Max(1, ScaledSize);
}

void UPixelComponentSettings::GetScaledDimensions(
	int32 OriginalWidth,
	int32 OriginalHeight,
	int32& OutWidth,
	int32& OutHeight)
{
	OutWidth = ApplyPixelSizeScaling(OriginalWidth);
	OutHeight = ApplyPixelSizeScaling(OriginalHeight);
}

bool UPixelComponentSettings::AreSettingsValid() const
{
	return GlobalPixelSize > 0 && GlobalPixelSize <= 1024;
}

FName UPixelComponentSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName UPixelComponentSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UPixelComponentSettings::GetSectionName() const
{
	return TEXT("PixelComponent");
}

FText UPixelComponentSettings::GetSectionText() const
{
	return INVTEXT("PixelComponent");
}

FText UPixelComponentSettings::GetSectionDescription() const
{
	return INVTEXT("Configure global settings for PixelComponent plugin (Aseprite importer)");
}
