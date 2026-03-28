// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentMaterialLibrary.h"
#include "PixelComponentAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentMaterial, Log, All);

// Parameter name definitions
const FName FPixelComponentMaterialLibrary::Param_PixelTextureWidth(TEXT("PixelTextureWidth"));
const FName FPixelComponentMaterialLibrary::Param_PixelTextureHeight(TEXT("PixelTextureHeight"));
const FName FPixelComponentMaterialLibrary::Param_PixelTextureSize(TEXT("PixelTextureSize"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceLeft(TEXT("NineSliceLeft"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceTop(TEXT("NineSliceTop"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceRight(TEXT("NineSliceRight"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceBottom(TEXT("NineSliceBottom"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceMarginsUV(TEXT("NineSliceMarginsUV"));
const FName FPixelComponentMaterialLibrary::Param_SliceUVRect(TEXT("SliceUVRect"));
const FName FPixelComponentMaterialLibrary::Param_NineSliceCenterUV(TEXT("NineSliceCenterUV"));

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

	// Send texture dimensions
	bSuccess &= SendTextureDimensionsToMaterial(Asset, MaterialInstance);

	// Get 9-slice margins
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

				// Send as Vector4 for efficient shader access
				MaterialInstance->SetVectorParameterValue(
					Param_NineSliceMarginsUV,
					FLinearColor(UVMargins.Left, UVMargins.Top, UVMargins.Right, UVMargins.Bottom)
				);
			}
		}

		// Also send individual scalar parameters for flexibility
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

	FMargin MarginsToSend = Slice->NineSliceMargins;

	if (bUseNormalizedUVs && TexWidth > 0 && TexHeight > 0)
	{
		MarginsToSend = ConvertMarginsToNormalizedUV(Slice->NineSliceMargins, TexWidth, TexHeight);
	}

	MaterialInstance->SetVectorParameterValue(
		Param_NineSliceMarginsUV,
		FLinearColor(MarginsToSend.Left, MarginsToSend.Top, MarginsToSend.Right, MarginsToSend.Bottom)
	);

	// Also send the slice UV rectangle
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

	int32 TexWidth, TexHeight;
	Asset->GetTextureDimensions(TexWidth, TexHeight);

	// Send as individual scalars
	MaterialInstance->SetScalarParameterValue(Param_PixelTextureWidth, static_cast<float>(TexWidth));
	MaterialInstance->SetScalarParameterValue(Param_PixelTextureHeight, static_cast<float>(TexHeight));

	// Also send as vector for convenience
	MaterialInstance->SetVectorParameterValue(
		Param_PixelTextureSize,
		FLinearColor(static_cast<float>(TexWidth), static_cast<float>(TexHeight), 0.0f, 0.0f)
	);

	UE_LOG(LogPixelComponentMaterial, Verbose, 
		TEXT("Sent texture dimensions to material: %dx%d"), TexWidth, TexHeight);

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

	const FBox2f UVs = Asset->GetSliceNormalizedUVs(SliceName);

	if (UVs.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(
			Param_SliceUVRect,
			FLinearColor(UVs.Min.X, UVs.Min.Y, UVs.Max.X, UVs.Max.Y)
		);

		UE_LOG(LogPixelComponentMaterial, Verbose, 
			TEXT("Sent slice UVs for '%s': (%.4f, %.4f) - (%.4f, %.4f)"),
			*SliceName, UVs.Min.X, UVs.Min.Y, UVs.Max.X, UVs.Max.Y);

		return true;
	}

	return false;
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

	const FBox2f CenterUVs = Asset->GetNineSliceCenterUVs(SliceName);

	if (CenterUVs.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(
			Param_NineSliceCenterUV,
			FLinearColor(CenterUVs.Min.X, CenterUVs.Min.Y, CenterUVs.Max.X, CenterUVs.Max.Y)
		);

		UE_LOG(LogPixelComponentMaterial, Verbose, 
			TEXT("Sent 9-slice center UVs for '%s': (%.4f, %.4f) - (%.4f, %.4f)"),
			*SliceName, CenterUVs.Min.X, CenterUVs.Min.Y, CenterUVs.Max.X, CenterUVs.Max.Y);

		return true;
	}

	return false;
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
