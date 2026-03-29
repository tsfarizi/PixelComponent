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
 * 
 * HD Pixel Art Features:
 * - Pre-calculated normalized UVs (FPixelUVRect) for all slices
 * - Animation sequences from Aseprite frame tags
 * - Layer to material parameter mapping
 * - Pivot points for consistent transformation
 */
UCLASS(BlueprintType, Category = "PixelComponent")
class PIXELCOMPONENT_API UPixelComponentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPixelComponentAsset();

	// ========================================================================
	// Asset Properties
	// ========================================================================

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
	 * Get the asset name from Aseprite metadata.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	const FString& GetAssetName() const { return AssetName; }

	// ========================================================================
	// Slice Data Access
	// ========================================================================

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
	 * @deprecated Use GetSliceByName instead for Blueprint compatibility
	 */
	const FSliceData* FindSliceByName(const FString& SliceName) const;

	/**
	 * Get a specific slice by name (Blueprint-safe version).
	 * @param SliceName Name of the slice to find
	 * @param bFound Output: true if slice was found
	 * @return Slice data (invalid struct if not found)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	FSliceData GetSliceByName(const FString& SliceName, bool& bFound) const;

	/**
	 * Get slice count.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	int32 GetSliceCount() const { return Slices.Num(); }

	/**
	 * Get all slice names.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	TArray<FString> GetAllSliceNames() const;

	// ========================================================================
	// Layer Data Access
	// ========================================================================

	/**
	 * Get all layer metadata.
	 * @return Array of layer metadata
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	const TArray<FLayerMetadata>& GetLayers() const { return Layers; }

	/**
	 * Get layer to material parameter mapping.
	 * Maps Aseprite layer names to material parameter names.
	 * @note For Blueprint, use GetLayerParameterNames() to get all mappings
	 */
	const TMap<FString, FName>& GetLayerToMaterialMap() const { return LayerToMaterialParamMap; }

	/**
	 * Get all layer parameter mappings as a struct for Blueprint.
	 * @return Array of layer name and parameter name pairs
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Material")
	TArray<FString> GetAllLayerParameterNames() const;

	/**
	 * Get material parameter name for a specific layer.
	 * @param LayerName Aseprite layer name
	 * @return Material parameter name (or NAME_None if not mapped)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Material")
	FName GetMaterialParameterForLayer(const FString& LayerName) const;

	/**
	 * Set layer to material parameter mapping.
	 * @param LayerName Aseprite layer name
	 * @param ParamName Material parameter name
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Material")
	void SetLayerMaterialMapping(const FString& LayerName, const FName& ParamName);

	// ========================================================================
	// 9-Slice Data
	// ========================================================================

	/**
	 * Get the 9-slice margins for the default slice (in pixels).
	 * @return FMargin containing left, top, right, bottom margins in pixels
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|9-Slice")
	FMargin GetNineSliceMargins() const { return NineSliceMargins; }

	/**
	 * Get 9-slice margins in UV space for a specific slice.
	 * @param SliceName Name of the slice
	 * @return UV margins (Left, Top, Right, Bottom)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|9-Slice")
	FPixelNineSliceMargins GetSliceNineSliceMarginsUV(const FString& SliceName) const;

	// ========================================================================
	// Texture Dimensions
	// ========================================================================

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
	 * Get texture dimensions as vector.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Asset")
	FVector2f GetTextureSize() const;

	// ========================================================================
	// Global Settings Access
	// ========================================================================

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
	float GetScaleFactor() const { return AppliedScaleFactor; }

	/**
	 * Check if this asset has been scaled from its original size.
	 * @return true if scaling was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	bool IsScaled() const { return bIsScaled; }

	// ========================================================================
	// UV Calculations
	// ========================================================================

	/**
	 * Compute normalized UV coordinates for a given pixel rectangle.
	 * This converts from pixel-space to 0-1 UV space.
	 * 
	 * @param PixelRect The pixel-space rectangle
	 * @return Normalized UV rectangle
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FPixelUVRect ComputeNormalizedUVs(const FPixelRect& PixelRect) const;

	/**
	 * Get the normalized UV rectangle for a specific slice.
	 * @param SliceName Name of the slice
	 * @return Normalized UV rectangle, or invalid if slice not found
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FPixelUVRect GetSliceNormalizedUVRect(const FString& SliceName) const;

	/**
	 * Get the center UV region for a 9-slice.
	 * @param SliceName Name of the 9-slice
	 * @return UV rectangle for the center (tileable) region
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	FPixelUVRect GetNineSliceCenterUVRect(const FString& SliceName) const;

	/**
	 * Get all UV coordinates as array for batch material updates.
	 * @return Array of UV rectangles (MinX, MinY, MaxX, MaxY as Vector4f)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|UV")
	TArray<FVector4f> GetAllSliceUVsAsVector4() const;

	// ========================================================================
	// Animation Data
	// ========================================================================

	/**
	 * Get animation sequences parsed from Aseprite frame tags.
	 * @return Map of sequence name to animation data
	 * @note For Blueprint, use GetAllAnimationSequenceNames() and GetAnimationSequenceByName() instead
	 */
	const TMap<FString, FPixelAnimSequence>& GetAnimationSequences() const { return AnimationSequences; }

	/**
	 * Get a specific animation sequence by name.
	 * @param SequenceName Name of the animation sequence
	 * @param bFound Output: true if sequence was found
	 * @return Animation sequence data (invalid struct if not found)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	FPixelAnimSequence GetAnimationSequenceByName(const FString& SequenceName, bool& bFound) const;

	/**
	 * Get all animation sequence names.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	TArray<FString> GetAllAnimationSequenceNames() const;

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
	 * Get frame count.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Animation")
	int32 GetFrameCount() const { return Frames.Num(); }

	// ========================================================================
	// Pivot Points
	// ========================================================================

	/**
	 * Get pivot point for a specific slice.
	 * @param SliceName Name of the slice
	 * @return Pivot point data
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Pivot")
	FPixelPivot GetSlicePivot(const FString& SliceName) const;

	/**
	 * Get default pivot point (used when slice has no specific pivot).
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Pivot")
	FPixelPivot GetDefaultPivot() const { return DefaultPivot; }

	// ========================================================================
	// Parsing (Internal)
	// ========================================================================

	/**
	 * Parse slice data from Aseprite JSON.
	 * @param JsonObject The root Aseprite JSON object
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseSlicesFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Parse layer data from Aseprite JSON.
	 * @param JsonObject The root Aseprite JSON object
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseLayersFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Parse animation data from Aseprite JSON.
	 * @param JsonObject The root Aseprite JSON object
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseAnimationFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Parse asset metadata from Aseprite JSON.
	 * @param JsonObject The root Aseprite JSON object
	 * @return Parse result indicating success/failure
	 */
	FAsepriteParseResult ParseMetadataFromJson(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Refresh all normalized UV calculations.
	 * Call this after changing texture or slice data.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Internal")
	void RefreshNormalizedUVs();

	/**
	 * Validate all asset data.
	 * @return true if all data is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Internal")
	bool ValidateAsset() const;

protected:
	// ========================================================================
	// Properties
	// ========================================================================

	/** Source texture referenced by this asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	UTexture2D* SourceTexture;

	/** Asset name from Aseprite metadata */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	FString AssetName;

	/** All slices defined in the Aseprite file */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	TArray<FSliceData> Slices;

	/** Layer metadata from Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
	TArray<FLayerMetadata> Layers;

	/** Layer name to material parameter mapping */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Material")
	TMap<FString, FName> LayerToMaterialParamMap;

	/** Default 9-slice margins (for the primary slice) in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|9-Slice")
	FMargin NineSliceMargins;

	/** Animation sequences from Aseprite frame tags */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Animation")
	TMap<FString, FPixelAnimSequence> AnimationSequences;

	/** Legacy animation tags (for backwards compatibility) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Animation|Deprecated")
	TArray<FAnimationTag> AnimationTags;

	/** Frame data for animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Animation")
	TArray<FFrameData> Frames;

	/** Default pivot point for slices without specific pivot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Pivot")
	FPixelPivot DefaultPivot;

	/** Cached texture width */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	int32 CachedTextureWidth;

	/** Cached texture height */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	int32 CachedTextureHeight;

	/** Original texture width (before scaling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	int32 OriginalTextureWidth;

	/** Original texture height (before scaling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	int32 OriginalTextureHeight;

	/** Whether UVs have been computed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	bool bUVsComputed;

	/** Whether this asset has been scaled from original size */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	bool bIsScaled;

	/** Scale factor applied to this asset */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelComponent|Cache")
	float AppliedScaleFactor;

private:
	/**
	 * Internal helper to compute texture dimensions from source texture.
	 */
	void UpdateTextureDimensionsCache();

	/**
	 * Validate UV coordinates are within bounds.
	 */
	bool ValidateUVs() const;

public:
	/**
	 * Set texture dimensions directly (used by Factory during import).
	 * @param Width Texture width
	 * @param Height Texture height
	 * @param bApplyScaling Whether to apply global pixel size scaling
	 */
	void SetTextureDimensionsFromImport(int32 Width, int32 Height, bool bApplyScaling = true);

	/**
	 * Set scaling parameters from import (used by Factory).
	 */
	void SetScalingFromImport(float InScaleFactor, bool InbIsScaled);
};
