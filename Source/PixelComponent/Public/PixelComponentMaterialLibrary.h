// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelComponentTypes.h"

class UPixelComponentAsset;
class UMaterialInstanceDynamic;

/**
 * FPixelComponentMaterialLibrary
 *
 * Static utility library for integrating PixelComponent assets with material instances.
 * Provides functions for transmitting 9-slice margins, UV coordinates, texture dimensions,
 * pivot points, and palette profiles to material parameters for shader-based rendering.
 *
 * Required Material Parameters:
 * - Scalar: PixelTextureWidth, PixelTextureHeight - Texture dimensions in pixels
 * - Scalar: NineSliceLeft, NineSliceTop, NineSliceRight, NineSliceBottom - 9-slice margins
 * - Vector4: NineSliceMarginsUV - Normalized UV margins (Left, Top, Right, Bottom)
 * - Vector4: SliceUVRect - UV rectangle for slice (MinX, MinY, MaxX, MaxY)
 * - Vector4: PixelTextureSize - Texture dimensions as vector (Width, Height, 0, 0)
 * - Vector4: NineSliceCenterUV - Center UV region for 9-slice tiling
 * - Vector4: Pivot - Normalized pivot point (X, Y, 0, 0)
 * - Vector4 Array: SliceUVArray - Batch UV coordinates for multiple slices
 * - Vector4: PixelComponent_Color_{LayerName} - Palette profile color overrides
 * - Vector4: PixelComponent_Grayscale_{Index} - Grayscale mapping samples
 *
 * Usage Pattern:
 *   UMaterialInstanceDynamic* MID = CreateDynamicMaterialInstance();
 *   FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(Asset, MID);
 *   FPixelComponentMaterialLibrary::ApplyPaletteProfile(Asset, ProfileName, MID);
 */
class PIXELCOMPONENT_API FPixelComponentMaterialLibrary
{
public:
	/**
	 * Transmits 9-slice margin data and texture parameters to a material instance.
	 * Automatically injects GlobalPixelScale and VirtualResolution from settings.
	 *
	 * @param Asset Source PixelComponent asset containing slice and margin data
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @param bUseNormalizedUVs If true, sends UV-space margins; if false, sends pixel-space margins
	 * @return true if all parameters were successfully transmitted
	 */
	static bool SendNineSliceDataToMaterial(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance,
		bool bUseNormalizedUVs = true
	);

	/**
	 * Transmits 9-slice margins for a specific slice to a material instance.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param SliceName Name identifier of the slice to retrieve margins from
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @param bUseNormalizedUVs If true, converts margins to normalized UV space
	 * @return true if parameters were successfully transmitted
	 */
	static bool SendSliceNineSliceToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance,
		bool bUseNormalizedUVs = true
	);

	/**
	 * Transmits texture dimensions to material parameters.
	 * Retrieves dimensions from Asset->GetSourceTexture() if available, otherwise uses cached values.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return true if parameters were successfully transmitted
	 */
	static bool SendTextureDimensionsToMaterial(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Transmits UV rectangle for a specified slice to a material instance.
	 * Falls back to full texture UVs (0,0,1,1) if slice is not found or empty.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param SliceName Name identifier of the slice
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return true if parameters were successfully transmitted
	 */
	static bool SendSliceUVToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Transmits the center UV region for a 9-slice to a material instance.
	 * The center region defines the tileable area that should be stretched during rendering.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param SliceName Name identifier of the 9-slice
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return true if parameters were successfully transmitted
	 */
	static bool SendNineSliceCenterUVToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Transmits all slice UV coordinates as a batch for efficient material updates.
	 * Creates indexed parameters (SliceUVArray_0, SliceUVArray_1, etc.) for each slice.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @param ParameterName Base name for the array parameter (default: "SliceUVArray")
	 * @return Number of UV coordinates transmitted, or 0 on failure
	 */
	static int32 SendAllSliceUVsAsBatch(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance,
		const FName& ParameterName = "SliceUVArray"
	);

	/**
	 * Transmits layer color data to material parameters using layer-to-parameter mappings.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return Number of layer color parameters transmitted
	 */
	static int32 SendLayerColorsToMaterial(
		const UPixelComponentAsset* Asset,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Transmits pivot point data to material parameters.
	 * Uses slice-specific pivot if SliceName is provided, otherwise uses default pivot.
	 *
	 * @param Asset Source PixelComponent asset
	 * @param SliceName Name identifier of the slice (empty for default pivot)
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return true if parameters were successfully transmitted
	 */
	static bool SendPivotToMaterial(
		const UPixelComponentAsset* Asset,
		const FString& SliceName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Applies a palette profile to a material instance by injecting color override parameters.
	 * Creates parameters with prefix "PixelComponent_Color_{LayerName}" for each override.
	 *
	 * @param Asset Source PixelComponent asset containing palette profiles
	 * @param ProfileName Name identifier of the palette profile to apply
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return Number of color parameters applied, or 0 if profile not found
	 */
	static int32 ApplyPaletteProfile(
		const UPixelComponentAsset* Asset,
		const FName& ProfileName,
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Injects global configuration parameters from UPixelComponentSettings.
	 * Transmits GlobalPixelScale and VirtualResolution to the material instance.
	 *
	 * @param MaterialInstance Target dynamic material instance for parameter injection
	 * @return true if parameters were successfully transmitted
	 */
	static bool InjectGlobalSettings(
		UMaterialInstanceDynamic* MaterialInstance
	);

	/**
	 * Converts pixel-space margins to normalized UV-space margins.
	 *
	 * @param Margins Pixel-space margin values
	 * @param TextureWidth Source texture width in pixels
	 * @param TextureHeight Source texture height in pixels
	 * @return Normalized UV-space margins (0-1 range)
	 */
	static FMargin ConvertMarginsToNormalizedUV(
		const FMargin& Margins,
		int32 TextureWidth,
		int32 TextureHeight
	);

	/**
	 * Converts normalized UV-space margins to pixel-space margins.
	 *
	 * @param UVMargins Normalized UV-space margins (0-1 range)
	 * @param TextureWidth Source texture width in pixels
	 * @param TextureHeight Source texture height in pixels
	 * @return Pixel-space margin values
	 */
	static FMargin ConvertUVMarginsToPixels(
		const FMargin& UVMargins,
		int32 TextureWidth,
		int32 TextureHeight
	);

	/**
	 * Parameter name constants used by the material library.
	 * These constants define the standard parameter naming convention.
	 */
	static const FName Param_Prefix;
	static const FName Param_PixelTextureWidth;
	static const FName Param_PixelTextureHeight;
	static const FName Param_PixelTextureSize;
	static const FName Param_GlobalPixelScale;
	static const FName Param_VirtualResolution;
	static const FName Param_NineSliceLeft;
	static const FName Param_NineSliceTop;
	static const FName Param_NineSliceRight;
	static const FName Param_NineSliceBottom;
	static const FName Param_NineSliceMarginsUV;
	static const FName Param_SliceUVRect;
	static const FName Param_NineSliceCenterUV;
	static const FName Param_SliceUVArray;
	static const FName Param_Pivot;
};
