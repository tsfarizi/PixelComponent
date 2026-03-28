// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelComponentTypes.h"

class UPixelComponentAsset;
class UMaterialInstanceDynamic;

/**
 * FPixelComponentMaterialLibrary
 * 
 * Static helper library for integrating PixelComponent assets with materials.
 * Provides functions to send 9-slice margins, UV coordinates, and texture
 * dimensions to material parameters for shader-based tiling logic.
 * 
 * Expected Material Parameters:
 * - Scalar: "PixelTextureWidth" - Texture width in pixels
 * - Scalar: "PixelTextureHeight" - Texture height in pixels
 * - Scalar: "NineSliceLeft" - Left margin in pixels (or normalized UV)
 * - Scalar: "NineSliceTop" - Top margin in pixels (or normalized UV)
 * - Scalar: "NineSliceRight" - Right margin in pixels (or normalized UV)
 * - Scalar: "NineSliceBottom" - Bottom margin in pixels (or normalized UV)
 * - Vector4: "NineSliceMarginsUV" - Normalized UV margins (Left, Top, Right, Bottom)
 * - Vector4: "SliceUVRect" - UV rectangle for a slice (MinX, MinY, MaxX, MaxY)
 * - Vector2: "PixelTextureSize" - Texture dimensions as vector
 * 
 * Usage:
 *   UMaterialInstanceDynamic* MID = ...;
 *   FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(Asset, MID);
 */
class PIXELCOMPONENT_API FPixelComponentMaterialLibrary
{
public:
	/**
	 * Send all 9-slice and texture data to a material instance.
	 * 
	 * @param Asset The PixelComponent asset containing slice data
	 * @param MaterialInstance The dynamic material instance to update
	 * @param bUseNormalizedUVs If true, send normalized UV values; if false, send pixel values
	 * @return true if parameters were successfully set
	 */
	static bool SendNineSliceDataToMaterial(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance,
		bool bUseNormalizedUVs = true
	);

	/**
	 * Send 9-slice margins for a specific slice to a material.
	 * 
	 * @param Asset The PixelComponent asset
	 * @param SliceName Name of the slice to get margins from
	 * @param MaterialInstance The dynamic material instance to update
	 * @param bUseNormalizedUVs If true, convert margins to normalized UV space
	 * @return true if parameters were successfully set
	 */
	static bool SendSliceNineSliceToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance,
		bool bUseNormalizedUVs = true
	);

	/**
	 * Send texture dimensions to material parameters.
	 * 
	 * @param Asset The PixelComponent asset
	 * @param MaterialInstance The dynamic material instance to update
	 * @return true if parameters were successfully set
	 */
	static bool SendTextureDimensionsToMaterial(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Send UV rectangle for a slice to material.
	 * 
	 * @param Asset The PixelComponent asset
	 * @param SliceName Name of the slice
	 * @param MaterialInstance The dynamic material instance to update
	 * @return true if parameters were successfully set
	 */
	static bool SendSliceUVToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Send the center UV region for a 9-slice to material.
	 * This is the tileable region that should be stretched.
	 * 
	 * @param Asset The PixelComponent asset
	 * @param SliceName Name of the 9-slice
	 * @param MaterialInstance The dynamic material instance to update
	 * @return true if parameters were successfully set
	 */
	static bool SendNineSliceCenterUVToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Convert pixel margins to normalized UV margins.
	 * 
	 * @param Margins Pixel-space margins
	 * @param TextureWidth Texture width in pixels
	 * @param TextureHeight Texture height in pixels
	 * @return Normalized UV margins
	 */
	static FMargin ConvertMarginsToNormalizedUV(
		const FMargin& Margins,
		int32 TextureWidth,
		int32 TextureHeight
	);

	/**
	 * Convert normalized UV margins to pixel margins.
	 * 
	 * @param UVMargins Normalized UV margins (0-1)
	 * @param TextureWidth Texture width in pixels
	 * @param TextureHeight Texture height in pixels
	 * @return Pixel-space margins
	 */
	static FMargin ConvertUVMarginsToPixels(
		const FMargin& UVMargins,
		int32 TextureWidth,
		int32 TextureHeight
	);

	/**
	 * Parameter names used by the material library.
	 * Override these if your material uses different parameter names.
	 */
	static const FName Param_PixelTextureWidth;
	static const FName Param_PixelTextureHeight;
	static const FName Param_PixelTextureSize;
	static const FName Param_NineSliceLeft;
	static const FName Param_NineSliceTop;
	static const FName Param_NineSliceRight;
	static const FName Param_NineSliceBottom;
	static const FName Param_NineSliceMarginsUV;
	static const FName Param_SliceUVRect;
	static const FName Param_NineSliceCenterUV;
};
