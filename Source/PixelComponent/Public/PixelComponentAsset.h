// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PixelComponentTypes.h"
#include "PixelComponentAsset.generated.h"

/**
 * UPixelComponentAsset
 * 
 * Data asset representing an imported Aseprite pixel art file.
 * Contains texture reference, slice data (including 9-slice margins),
 * layer metadata, and animation information.
 * 
 * This asset is created automatically by UPixelComponentFactory when
 * importing .json files exported from Aseprite.
 * 
 * Global Settings:
 * - Asset dimensions and UV calculations are affected by UPixelComponentSettings::GlobalPixelSize
 * - Use GetEffectiveTextureDimensions() to get dimensions after global pixel size scaling
 */
UCLASS(BlueprintType, Category = "PixelComponent")
class PIXELCOMPONENT_API UPixelComponentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPixelComponentAsset();

	/**
	 * Get the source texture for this pixel art asset.
	 * @return Pointer to the source texture (may be null if not linked)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	UTexture2D* GetSourceTexture() const { return SourceTexture; }

	/**
	 * Set the source texture for this asset.
	 * @param NewTexture The texture to associate with this asset
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	void SetSourceTexture(UTexture2D* NewTexture);

	/**
	 * Get all slices defined in this asset.
	 * @return Array of slice data
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	const TArray<FSliceData>& GetSlices() const { return Slices; }

	/**
	 * Get a specific slice by name.
	 * @param SliceName Name of the slice to find
	 * @return Pointer to slice data, or nullptr if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	const FSliceData* FindSliceByName(const FString& SliceName) const;

	/**
	 * Get all layer metadata.
	 * @return Array of layer metadata
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	const TArray<FLayerMetadata>& GetLayers() const { return Layers; }

	/**
	 * Get the 9-slice margins for the default slice.
	 * @return FMargin containing left, top, right, bottom margins in pixels
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	FMargin GetNineSliceMargins() const { return NineSliceMargins; }

	/**
	 * Get the texture dimensions.
	 * @param OutWidth Output for texture width
	 * @param OutHeight Output for texture height
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	void GetTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get the effective texture dimensions after applying global pixel size scaling.
	 * 
	 * @param OutWidth Output for effective width
	 * @param OutHeight Output for effective height
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	void GetEffectiveTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get the original texture dimensions (before any scaling).
	 * @param OutWidth Output for original width
	 * @param OutHeight Output for original height
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	void GetOriginalTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get the global pixel size setting.
	 * @return Global pixel size from settings
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static int32 GetGlobalPixelSize();

	/**
	 * Get the scale factor applied to this asset based on global pixel size.
	 * @return Scale factor (1.0 = no scaling)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	float GetScaleFactor() const;

	/**
	 * Check if this asset has been scaled from its original size.
	 * @return true if scaling was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	bool IsScaled() const { return bIsScaled; }

	/**
	 * Compute normalized UV coordinates for a given pixel rectangle.
	 * This converts from pixel-space to 0-1 UV space.
	 * 
	 * @param PixelRect The pixel-space rectangle
	 * @return Normalized UV box (Min = top-left, Max = bottom-right in UV space)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FBox2f ComputeNormalizedUVs(const FPixelRect& PixelRect) const;

	/**
	 * Get the normalized UVs for a specific slice.
	 * @param SliceName Name of the slice
	 * @return Normalized UV box, or invalid box if slice not found
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FBox2f GetSliceNormalizedUVs(const FString& SliceName) const;

	/**
	 * Get the center UV region for a 9-slice.
	 * @param SliceName Name of the 9-slice
	 * @return UV box for the center (tileable) region
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FBox2f GetNineSliceCenterUVs(const FString& SliceName) const;

	/**
	 * Get animation tags defined in the Aseprite file.
	 * @return Array of animation tags
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	const TArray<FAnimationTag>& GetAnimationTags() const { return AnimationTags; }

	/**
	 * Get frame data for animation.
	 * @return Array of frame data
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	const TArray<FFrameData>& GetFrames() const { return Frames; }

	/**
	 * Get total animation duration in milliseconds.
	 * @return Total duration
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	int32 GetTotalAnimationDurationMs() const;

	/**
	 * Refresh all normalized UV calculations.
	 * Call this after changing texture or slice data.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Internal")
	void RefreshNormalizedUVs();

	/**
	 * Serialize slice data from Aseprite JSON.
	 * @param JsonArray The "slices" array from Aseprite JSON
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseSlicesFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Serialize layer data from Aseprite JSON.
	 * @param JsonArray The "layers" array from Aseprite JSON
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseLayersFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Serialize animation data from Aseprite JSON.
	 * @param JsonObject The root Aseprite JSON object
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseAnimationFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Extract 9-slice margins from a slice's key array.
	 * @param KeyArray The "key" array containing 9-slice data
	 * @param OutMargins Output for the extracted margins
	 * @return true if 9-slice data was found and parsed
	 */
	static bool ExtractNineSliceMargins(
		const TArray<TSharedPtr<FJsonValue>>& KeyArray,
		FMargin& OutMargins
	);

protected:
	/** Source texture referenced by this asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	UTexture2D* SourceTexture;

	/** All slices defined in the Aseprite file */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	TArray<FSliceData> Slices;

	/** Layer metadata from Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	TArray<FLayerMetadata> Layers;

	/** Default 9-slice margins (for the primary slice) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|9-Slice")
	FMargin NineSliceMargins;

	/** Animation tags from Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Animation")
	TArray<FAnimationTag> AnimationTags;

	/** Frame data for animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Animation")
	TArray<FFrameData> Frames;

	/** Cached texture width */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	int32 CachedTextureWidth;

	/** Cached texture height */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	int32 CachedTextureHeight;

	/** Original texture width (before scaling) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	int32 OriginalTextureWidth;

	/** Original texture height (before scaling) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	int32 OriginalTextureHeight;

	/** Whether UVs have been computed */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	bool bUVsComputed;

	/** Whether this asset has been scaled from original size */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	bool bIsScaled;

	/** Scale factor applied to this asset */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Cache")
	float AppliedScaleFactor;

private:
	/**
	 * Internal helper to compute texture dimensions from source texture.
	 */
	void UpdateTextureDimensionsCache();
};
