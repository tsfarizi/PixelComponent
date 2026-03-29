// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentMaterialLibrary.h"
#include "PixelComponentAsset.h"
#include "PixelComponentSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentMaterial, Log, All);

const FName FPixelComponentMaterialLibrary::Param_Prefix(TEXT("PixelComponent_"));
const FName FPixelComponentMaterialLibrary::Param_PixelTextureWidth(TEXT("PixelTextureWidth"));
const FName FPixelComponentMaterialLibrary::Param_PixelTextureHeight(TEXT("PixelTextureHeight"));
const FName FPixelComponentMaterialLibrary::Param_PixelTextureSize(TEXT("PixelTextureSize"));
const FName FPixelComponentMaterialLibrary::Param_GlobalPixelScale(TEXT("GlobalPixelScale"));
const FName FPixelComponentMaterialLibrary::Param_VirtualResolution(TEXT("VirtualResolution"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceLeft(TEXT("NineSliceLeft"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceTop(TEXT("NineSliceTop"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceRight(TEXT("NineSliceRight"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceBottom(TEXT("NineSliceBottom"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceMarginsUV(TEXT("NineSliceMarginsUV"));
const FName FPixelComponentMaterialLibrary::Param_SliceUVRect(TEXT("SliceUVRect"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceCenterUV(TEXT("NineSliceCenterUV"));
const FName FPixelComponentMaterialLibrary::Param_SliceUVArray(TEXT("SliceUVArray"));
const FName FPixelComponentMaterialLibrary::Param_Pivot(TEXT("Pivot"));

bool FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(
	const UPixelComponentAsset* Asset,
	UMaterialInstanceDynamic* MaterialInstance,
	bool bUseNormalizedUVs)
{
	if (!Asset || !MaterialInstance)
	{
		UE_LOG(LogPixelComponentMaterial, Warning, TEXT("Invalid asset or material instance"));
		return false;
	}

	bool bSuccess = true;

	bSuccess &= InjectGlobalSettings(MaterialInstance);
	bSuccess &= SendTextureDimensionsToMaterial(Asset, MaterialInstance);

	const FMargin Margins = Asset->GetNineSliceMargins();

	if (Margins.Left > 0 || Margins.Top > 0 || Margins.Right > 0 || Margins.Bottom > 0)
	{
		if (bUseNormalizedUVs)
		{
			int32 TexWidth, TexHeight;
			Asset->GetTextureDimensions(TexWidth, TexHeight);

			if (TexWidth > 0 && TexHeight > 0)
			{
				const FMargin UVMargins = ConvertMarginsToNormalizedUV(Margins, TexWidth, TexHeight);

				MaterialInstance->SetVectorParameterValue(
					Param_NineSliceMarginsUV,
					FLinearColor(UVMargins.Left, UVMargins.Top, UVMargins.Right, UVMargins.Bottom)
				);
			}
		}

		MaterialInstance->SetScalarParameterValue(Param_NineSliceLeft, Margins.Left);
		MaterialInstance->SetScalarParameterValue(Param_NineSliceTop, Margins.Top);
		MaterialInstance->SetScalarParameterValue(Param_NineSliceRight, Margins.Right);
		MaterialInstance->SetScalarParameterValue(Param_NineSliceBottom, Margins.Bottom);

		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Sent 9-slice margins to material: L=%f, T=%f, R=%f, B=%f"),
			Margins.Left, Margins.Top, Margins.Right, Margins.Bottom);
	}

	return bSuccess;
}

bool FPixelComponentMaterialLibrary::SendSliceNineSliceToMaterial(
	const UPixelComponentAsset* Asset,
	const FString& SliceName,
	UMaterialInstanceDynamic* MaterialInstance,
	bool bUseNormalizedUVs)
{
	if (!Asset || !MaterialInstance)
	{
		return false;
	}

	const FSliceData* Slice = Asset->FindSliceByName(SliceName);
	if (!Slice || !Slice->bIsNineSlice)
	{
		UE_LOG(LogPixelComponentMaterial, Warning,
			TEXT("Slice '%s' not found or is not a 9-slice"), *SliceName);
		return false;
	}

	int32 TexWidth, TexHeight;
	Asset->GetTextureDimensions(TexWidth, TexHeight);

	FPixelNineSliceMargins MarginsToSend = Slice->NineSliceMarginsUV;

	if (!bUseNormalizedUVs && TexWidth > 0 && TexHeight > 0)
	{
		MarginsToSend.Left *= TexWidth;
		MarginsToSend.Top *= TexHeight;
		MarginsToSend.Right *= TexWidth;
		MarginsToSend.Bottom *= TexHeight;
	}

	MaterialInstance->SetVectorParameterValue(
		Param_NineSliceMarginsUV,
		FLinearColor(MarginsToSend.Left, MarginsToSend.Top, MarginsToSend.Right, MarginsToSend.Bottom)
	);

	SendSliceUVToMaterial(Asset, SliceName, MaterialInstance);

	return true;
}

bool FPixelComponentMaterialLibrary::SendTextureDimensionsToMaterial(
	const UPixelComponentAsset* Asset,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance)
	{
		return false;
	}

	UTexture2D* SourceTexture = Asset->GetSourceTexture();

	int32 TexWidth = 0;
	int32 TexHeight = 0;

	if (SourceTexture)
	{
		TexWidth = SourceTexture->GetSurfaceWidth();
		TexHeight = SourceTexture->GetSurfaceHeight();
	}
	else
	{
		Asset->GetTextureDimensions(TexWidth, TexHeight);
	}

	MaterialInstance->SetScalarParameterValue(Param_PixelTextureWidth, static_cast<float>(TexWidth));
	MaterialInstance->SetScalarParameterValue(Param_PixelTextureHeight, static_cast<float>(TexHeight));

	MaterialInstance->SetVectorParameterValue(
		Param_PixelTextureSize,
		FLinearColor(static_cast<float>(TexWidth), static_cast<float>(TexHeight), 0.0f, 0.0f)
	);

	UE_LOG(LogPixelComponentMaterial, Verbose,
		TEXT("Sent texture dimensions to material: %dx%d (from SourceTexture: %s)"),
		TexWidth, TexHeight, SourceTexture ? *SourceTexture->GetName() : TEXT("nullptr, using cached"));

	return true;
}

bool FPixelComponentMaterialLibrary::SendSliceUVToMaterial(
	const UPixelComponentAsset* Asset,
	const FString& SliceName,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance)
	{
		return false;
	}

	const FPixelUVRect UVRect = Asset->GetSliceNormalizedUVRect(SliceName);

	MaterialInstance->SetVectorParameterValue(
		Param_SliceUVRect,
		FLinearColor(UVRect.MinX, UVRect.MinY, UVRect.MaxX, UVRect.MaxY)
	);

	if (SliceName.IsEmpty() || Asset->GetSliceCount() == 0)
	{
		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Sent full texture UVs (no slice specified): (%.4f, %.4f) - (%.4f, %.4f)"),
			UVRect.MinX, UVRect.MinY, UVRect.MaxX, UVRect.MaxY);
	}
	else
	{
		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Sent slice UVs for '%s': (%.4f, %.4f) - (%.4f, %.4f)"),
			*SliceName, UVRect.MinX, UVRect.MinY, UVRect.MaxX, UVRect.MaxY);
	}

	return true;
}

bool FPixelComponentMaterialLibrary::SendNineSliceCenterUVToMaterial(
	const UPixelComponentAsset* Asset,
	const FString& SliceName,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance)
	{
		return false;
	}

	const FPixelUVRect CenterUVs = Asset->GetNineSliceCenterUVRect(SliceName);

	if (CenterUVs.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(
			Param_NineSliceCenterUV,
			FLinearColor(CenterUVs.MinX, CenterUVs.MinY, CenterUVs.MaxX, CenterUVs.MaxY)
		);

		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Sent 9-slice center UVs for '%s': (%.4f, %.4f) - (%.4f, %.4f)"),
			*SliceName, CenterUVs.MinX, CenterUVs.MinY, CenterUVs.MaxX, CenterUVs.MaxY);

		return true;
	}

	return false;
}

int32 FPixelComponentMaterialLibrary::SendAllSliceUVsAsBatch(
	const UPixelComponentAsset* Asset,
	UMaterialInstanceDynamic* MaterialInstance,
	const FName& ParameterName)
{
	if (!Asset || !MaterialInstance)
	{
		return 0;
	}

	const TArray<FVector4f> UVs = Asset->GetAllSliceUVsAsVector4();

	if (UVs.Num() == 0)
	{
		UE_LOG(LogPixelComponentMaterial, Warning, TEXT("No valid slice UVs to send"));
		return 0;
	}

	int32 Count = 0;
	for (const FVector4f& UV : UVs)
	{
		const FName ParamName = *FString::Printf(TEXT("%s_%d"), *ParameterName.ToString(), Count);
		MaterialInstance->SetVectorParameterValue(ParamName, FLinearColor(UV.X, UV.Y, UV.Z, UV.W));
		Count++;
	}

	UE_LOG(LogPixelComponentMaterial, Log,
		TEXT("Sent batch slice UVs: %d slices to parameter '%s'"), UVs.Num(), *ParameterName.ToString());

	return Count;
}

int32 FPixelComponentMaterialLibrary::SendLayerColorsToMaterial(
	const UPixelComponentAsset* Asset,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance)
	{
		return 0;
	}

	int32 LayersSet = 0;
	const TArray<FLayerMetadata>& Layers = Asset->GetLayers();
	const TMap<FString, FName>& LayerMap = Asset->GetLayerToMaterialMap();

	for (const FLayerMetadata& Layer : Layers)
	{
		const FName* ParamName = LayerMap.Find(Layer.Name);
		FName TargetParam = ParamName ? *ParamName : *Layer.Name;

		const FLinearColor LayerColor = Layer.UserDataColor.ReinterpretAsLinear();
		MaterialInstance->SetVectorParameterValue(TargetParam, LayerColor);
		LayersSet++;

		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Sent layer color for '%s' -> parameter '%s'"), *Layer.Name, *TargetParam.ToString());
	}

	return LayersSet;
}

bool FPixelComponentMaterialLibrary::SendPivotToMaterial(
	const UPixelComponentAsset* Asset,
	const FString& SliceName,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance)
	{
		return false;
	}

	FPixelPivot Pivot;

	if (!SliceName.IsEmpty())
	{
		Pivot = Asset->GetSlicePivot(SliceName);
	}
	else
	{
		Pivot = Asset->GetDefaultPivot();
	}

	int32 TexWidth, TexHeight;
	Asset->GetTextureDimensions(TexWidth, TexHeight);

	FVector2f NormalizedPivot = Pivot.GetNormalized(TexWidth, TexHeight);

	MaterialInstance->SetVectorParameterValue(
		Param_Pivot,
		FLinearColor(NormalizedPivot.X, NormalizedPivot.Y, 0.0f, 0.0f)
	);

	UE_LOG(LogPixelComponentMaterial, Verbose,
		TEXT("Sent pivot: (%.4f, %.4f)"), NormalizedPivot.X, NormalizedPivot.Y);

	return true;
}

int32 FPixelComponentMaterialLibrary::ApplyPaletteProfile(
	const UPixelComponentAsset* Asset,
	const FName& ProfileName,
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!Asset || !MaterialInstance || ProfileName == NAME_None)
	{
		return 0;
	}

	bool bFound = false;
	FPixelPaletteProfile Profile = Asset->GetPaletteProfile(ProfileName.ToString(), bFound);

	if (!bFound)
	{
		UE_LOG(LogPixelComponentMaterial, Warning, TEXT("Palette profile '%s' not found in asset"), *ProfileName.ToString());
		return 0;
	}

	int32 ColorsSet = 0;

	for (const auto& OverridePair : Profile.ColorOverrides)
	{
		const FName& LayerName = OverridePair.Key;
		const FLinearColor& Color = OverridePair.Value;

		const FName ParamName = *FString::Printf(TEXT("%sColor_%s"), *Param_Prefix.ToString(), *LayerName.ToString());
		MaterialInstance->SetVectorParameterValue(ParamName, Color);
		ColorsSet++;

		UE_LOG(LogPixelComponentMaterial, Verbose,
			TEXT("Applied palette override: %s -> (%.3f, %.3f, %.3f, %.3f)"),
			*LayerName.ToString(), Color.R, Color.G, Color.B, Color.A);
	}

	if (Profile.GrayscaleMap.Num() > 0)
	{
		const int32 SampleCount = FMath::Min(8, Profile.GrayscaleMap.Num());
		for (int32 i = 0; i < SampleCount; i++)
		{
			const FName ParamName = *FString::Printf(TEXT("%sGrayscale_%d"), *Param_Prefix.ToString(), i);
			MaterialInstance->SetVectorParameterValue(ParamName, Profile.GrayscaleMap[i]);
		}
	}

	UE_LOG(LogPixelComponentMaterial, Log,
		TEXT("Applied palette profile '%s': %d color overrides"), *ProfileName.ToString(), ColorsSet);

	return ColorsSet;
}

bool FPixelComponentMaterialLibrary::InjectGlobalSettings(
	UMaterialInstanceDynamic* MaterialInstance)
{
	if (!MaterialInstance)
	{
		return false;
	}

	UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
	if (!Settings)
	{
		return false;
	}

	const float GlobalPixelScale = static_cast<float>(Settings->GlobalPixelSize);
	MaterialInstance->SetScalarParameterValue(Param_GlobalPixelScale, GlobalPixelScale);

	const FVector2f VirtualRes(1920.0f, 1080.0f);
	MaterialInstance->SetVectorParameterValue(
		Param_VirtualResolution,
		FLinearColor(VirtualRes.X, VirtualRes.Y, 0.0f, 0.0f)
	);

	UE_LOG(LogPixelComponentMaterial, Verbose,
		TEXT("Injected global settings: PixelScale=%.2f"), GlobalPixelScale);

	return true;
}

FMargin FPixelComponentMaterialLibrary::ConvertMarginsToNormalizedUV(
	const FMargin& Margins,
	int32 TextureWidth,
	int32 TextureHeight)
{
	if (TextureWidth <= 0 || TextureHeight <= 0)
	{
		return Margins;
	}

	return FMargin(
		Margins.Left / static_cast<float>(TextureWidth),
		Margins.Top / static_cast<float>(TextureHeight),
		Margins.Right / static_cast<float>(TextureWidth),
		Margins.Bottom / static_cast<float>(TextureHeight)
	);
}

FMargin FPixelComponentMaterialLibrary::ConvertUVMarginsToPixels(
	const FMargin& UVMargins,
	int32 TextureWidth,
	int32 TextureHeight)
{
	if (TextureWidth <= 0 || TextureHeight <= 0)
	{
		return UVMargins;
	}

	return FMargin(
		UVMargins.Left * static_cast<float>(TextureWidth),
		UVMargins.Top * static_cast<float>(TextureHeight),
		UVMargins.Right * static_cast<float>(TextureWidth),
		UVMargins.Bottom * static_cast<float>(TextureHeight)
	);
}

TArray<UPixelComponentAsset*> FPixelComponentMaterialLibrary::GetAllAvailablePixelAssets()
{
	return GetFilteredPixelAssets(TEXT(""));
}

TArray<UPixelComponentAsset*> FPixelComponentMaterialLibrary::GetFilteredPixelAssets(const FString& SearchTerm)
{
	TArray<UPixelComponentAsset*> FoundAssets;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UPixelComponentAsset::StaticClass()->GetClassPathName(), AssetDataList, true);

	if (AssetDataList.Num() == 0)
	{
		UE_LOG(LogPixelComponentMaterial, Log, TEXT("No PixelComponentAsset instances found in project"));
		return FoundAssets;
	}

	const bool bHasSearchTerm = !SearchTerm.IsEmpty();
	const FString SearchTermLower = bHasSearchTerm ? SearchTerm.ToLower() : TEXT("");

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (bHasSearchTerm)
		{
			const FString AssetName = AssetData.AssetName.ToString();
			if (!AssetName.ToLower().Contains(SearchTermLower))
			{
				continue;
			}
		}

		UPixelComponentAsset* Asset = Cast<UPixelComponentAsset>(AssetData.GetAsset());
		if (Asset)
		{
			FoundAssets.Add(Asset);
		}
	}

	UE_LOG(LogPixelComponentMaterial, Log, TEXT("Found %d PixelComponentAsset instances (filtered: %d)"),
		AssetDataList.Num(), FoundAssets.Num());

	return FoundAssets;
}
