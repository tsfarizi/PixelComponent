// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PixelComponentTypes.h"
#include "PixelComponentAsset.generated.h"

/**
 * UPixelComponentAsset
 *
 * Data asset representing pixel art imported from Aseprite or standard image files.
 * Stores texture references, slice definitions, 9-slice margins, layer metadata,
 * animation sequences, and palette profiles for dynamic recoloring.
 *
 * Import Methods:
 * - JSON Import: UPixelComponentFactory parses Aseprite JSON exports with full metadata
 * - PNG Import: Direct texture import creates asset with default "FullTexture" slice
 *
 * Features:
 * - Pre-calculated normalized UV coordinates (0-1 range) for all slices
 * - 9-slice margin support for scalable UI elements without distortion
 * - Animation sequence data from Aseprite frame tags
 * - Palette profiles for runtime color overrides and material instancing
 * - Pivot point definitions for consistent transformation anchors
 *
 * Global Settings Integration:
 * - Texture dimensions scale according to UPixelComponentSettings::GlobalPixelSize
 * - Use GetEffectiveTextureDimensions() for scaled dimensions
 * - Use GetOriginalTextureDimensions() for source dimensions
 */
UCLASS(BlueprintType, Category = "Pixel Component", meta = (DisplayName = "Pixel Component Asset", DisplayCategory = "Pixel Component"))
class PIXELCOMPONENT_API UPixelComponentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPixelComponentAsset();

	// ========================================================================
	// Asset Properties
	// ========================================================================

	/**
	 * Retrieves the source texture referenced by this asset.
	 * @return Pointer to UTexture2D, or nullptr if no texture is assigned or in Material mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	UTexture2D* GetSourceTexture() const { return SourceTexture; }

	/**
	 * Assigns a source texture to this asset and recalculates UV coordinates.
	 * @param NewTexture The texture to associate with this asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void SetSourceTexture(UTexture2D* NewTexture);

	/**
	 * Validates whether the asset has a valid source texture assigned.
	 * @return true if SourceTexture is non-null, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	bool HasValidTexture() const { return SourceTexture != nullptr; }

	/**
	 * Retrieves the active material based on SourceMode.
	 * In Texture mode: returns the default pixel material from settings.
	 * In Material mode: returns the custom SourceMaterial.
	 * @return Pointer to UMaterialInterface, or nullptr if no material is available
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	UMaterialInterface* GetActiveMaterial() const;

	/**
	 * Assigns a source material to this asset (sets SourceMode to Material).
	 * @param NewMaterial The material to use as primary source
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void SetSourceMaterial(UMaterialInterface* NewMaterial);

	/**
	 * Gets the current source mode.
	 * @return Current EPixelSourceMode value
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Source")
	EPixelSourceMode GetSourceMode() const { return SourceMode; }

	/**
	 * Sets the source mode and clears the inactive source.
	 * @param NewMode The new source mode to use
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Source")
	void SetSourceMode(EPixelSourceMode NewMode);

	/**
	 * Get source dimensions based on current SourceMode.
	 * Texture mode: Returns SourceTexture dimensions.
	 * Material mode: Returns ManualMaterialSize value.
	 * Applies global pixel size scaling if enabled in settings.
	 * @return Source dimensions as FVector2D (scaled)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Dimensions")
	FVector2D GetSourceDimensions() const;

	/**
	 * Get original source dimensions (before global pixel size scaling).
	 * @return Original dimensions as FIntPoint
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Dimensions")
	FIntPoint GetOriginalSourceDimensions() const;

	/**
	 * Retrieves the asset name from Aseprite metadata or import filename.
	 * @return Reference to the asset name string
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	const FString& GetAssetName() const { return AssetName; }

	/**
	 * Sets a custom name for this asset.
	 * @param NewName The new asset name to assign
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void SetAssetName(const FString& NewName) { AssetName = NewName; }

	// ========================================================================
	// Slice Data Access
	// ========================================================================

	/**
	 * Retrieves all slice definitions contained in this asset.
	 * @return Constant reference to array of FSliceData structures
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	const TArray<FSliceData>& GetSlices() const { return Slices; }

	/**
	 * Locates a slice by name. Internal use only.
	 * @param SliceName Name identifier of the slice to locate
	 * @return Pointer to FSliceData, or nullptr if not found
	 * @deprecated Use GetSliceByName() for Blueprint compatibility
	 */
	const FSliceData* FindSliceByName(const FString& SliceName) const;

	/**
	 * Retrieves a slice definition by name with Blueprint support.
	 * @param SliceName Name identifier of the slice to locate
	 * @param bFound Output parameter indicating whether slice was found
	 * @return FSliceData structure containing slice definition, or invalid struct if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	FSliceData GetSliceByName(const FString& SliceName, bool& bFound) const;

	/**
	 * Get slice count.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	int32 GetSliceCount() const { return Slices.Num(); }

	/**
	 * Get all slice names.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	TArray<FString> GetAllSliceNames() const;

	// ========================================================================
	// Layer Data Access
	// ========================================================================

	/**
	 * Get all layer metadata.
	 * @return Array of layer metadata
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
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
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Material")
	TArray<FString> GetAllLayerParameterNames() const;

	/**
	 * Get material parameter name for a specific layer.
	 * @param LayerName Aseprite layer name
	 * @return Material parameter name (or NAME_None if not mapped)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Material")
	FName GetMaterialParameterForLayer(const FString& LayerName) const;

	/**
	 * Set layer to material parameter mapping.
	 * @param LayerName Aseprite layer name
	 * @param ParamName Material parameter name
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Material")
	void SetLayerMaterialMapping(const FString& LayerName, const FName& ParamName);

	// ========================================================================
	// Palette Profile Access
	// ========================================================================

	/**
	 * Retrieves all palette profile names defined in this asset.
	 * @return Array of profile name strings
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	TArray<FString> GetAllPaletteProfileNames() const;

	/**
	 * Retrieves a palette profile definition by name.
	 * @param ProfileName Name identifier of the palette profile
	 * @param bFound Output parameter indicating whether profile exists
	 * @return FPixelPaletteProfile structure, or invalid struct if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	FPixelPaletteProfile GetPaletteProfile(const FString& ProfileName, bool& bFound) const;

	/**
	 * Creates or updates a palette profile with the specified definition.
	 * @param ProfileName Name identifier for the palette profile
	 * @param Profile FPixelPaletteProfile structure containing color mappings
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	void SetPaletteProfile(const FString& ProfileName, const FPixelPaletteProfile& Profile);

	/**
	 * Removes a palette profile from this asset.
	 * @param ProfileName Name identifier of the profile to remove
	 * @return true if profile was removed, false if profile did not exist
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	bool RemovePaletteProfile(const FString& ProfileName);

	/**
	 * Retrieves a color override value from a specific palette profile.
	 * @param ProfileName Name identifier of the palette profile
	 * @param LayerName Name identifier of the layer or color index
	 * @param bFound Output parameter indicating whether override exists
	 * @return FLinearColor override value, or White if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	FLinearColor GetColorOverrideFromProfile(const FString& ProfileName, const FName& LayerName, bool& bFound) const;

	// ========================================================================
	// 9-Slice Data
	// ========================================================================

	/**
	 * Get the 9-slice margins for the default slice (in pixels).
	 * @return FMargin containing left, top, right, bottom margins in pixels
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|9-Slice")
	FMargin GetNineSliceMargins() const { return NineSliceMargins; }

	/**
	 * Get 9-slice margins in UV space for a specific slice.
	 * @param SliceName Name of the slice
	 * @return UV margins (Left, Top, Right, Bottom)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|9-Slice")
	FPixelNineSliceMargins GetSliceNineSliceMarginsUV(const FString& SliceName) const;

	// ========================================================================
	// Texture Dimensions
	// ========================================================================

	/**
	 * Get the texture dimensions.
	 * @param OutWidth Output for texture width
	 * @param OutHeight Output for texture height
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void GetTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get the effective texture dimensions after applying global pixel size scaling.
	 * 
	 * @param OutWidth Output for effective width
	 * @param OutHeight Output for effective height
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void GetEffectiveTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get the original texture dimensions (before any scaling).
	 * @param OutWidth Output for original width
	 * @param OutHeight Output for original height
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	void GetOriginalTextureDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Get texture dimensions as vector.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset")
	FVector2f GetTextureSize() const;

	// ========================================================================
	// Global Settings Access
	// ========================================================================

	/**
	 * Get the global pixel size setting.
	 * @return Global pixel size from settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Settings")
	static int32 GetGlobalPixelSize();

	/**
	 * Get the scale factor applied to this asset based on global pixel size.
	 * @return Scale factor (1.0 = no scaling)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Settings")
	float GetScaleFactor() const { return AppliedScaleFactor; }

	/**
	 * Check if this asset has been scaled from its original size.
	 * @return true if scaling was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Settings")
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
	 * @note Uses SourceTexture dimensions if available, otherwise uses cached dimensions
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|UV")
	FPixelUVRect ComputeNormalizedUVs(const FPixelRect& PixelRect) const;

	/**
	 * Get the normalized UV rectangle for a specific slice.
	 * @param SliceName Name of the slice
	 * @return Normalized UV rectangle, or invalid if slice not found
	 * @note Uses SourceTexture dimensions if available for dynamic UV calculation
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|UV")
	FPixelUVRect GetSliceNormalizedUVRect(const FString& SliceName) const;

	/**
	 * Get the center UV region for a 9-slice.
	 * @param SliceName Name of the 9-slice
	 * @return UV rectangle for the center (tileable) region
	 * @note Uses SourceTexture dimensions if available for dynamic UV calculation
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|UV")
	FPixelUVRect GetNineSliceCenterUVRect(const FString& SliceName) const;

	/**
	 * Get all UV coordinates as array for batch material updates.
	 * @return Array of UV rectangles (MinX, MinY, MaxX, MaxY as Vector4f)
	 * @note Uses SourceTexture dimensions if available for dynamic UV calculation
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|UV")
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
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Animation")
	FPixelAnimSequence GetAnimationSequenceByName(const FString& SequenceName, bool& bFound) const;

	/**
	 * Get all animation sequence names.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Animation")
	TArray<FString> GetAllAnimationSequenceNames() const;

	/**
	 * Get frame data for animation.
	 * @return Array of frame data
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Animation")
	const TArray<FFrameData>& GetFrames() const { return Frames; }

	/**
	 * Get total animation duration in milliseconds.
	 * @return Total duration
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Animation")
	int32 GetTotalAnimationDurationMs() const;

	/**
	 * Get frame count.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Animation")
	int32 GetFrameCount() const { return Frames.Num(); }

	// ========================================================================
	// Pivot Points
	// ========================================================================

	/**
	 * Get pivot point for a specific slice.
	 * @param SliceName Name of the slice
	 * @return Pivot point data
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Pivot")
	FPixelPivot GetSlicePivot(const FString& SliceName) const;

	/**
	 * Get default pivot point (used when slice has no specific pivot).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Pivot")
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
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Internal")
	void RefreshNormalizedUVs();

	/**
	 * Validate all asset data.
	 * @return true if all data is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Internal")
	bool ValidateAsset() const;

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

protected:
	// ========================================================================
	// Properties
	// ========================================================================

	/**
	 * Source mode selection.
	 * Determines whether this asset uses a texture or material as its primary visual source.
	 * Only one source type is active at a time (mutual exclusion).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Source")
	EPixelSourceMode SourceMode;

	/**
	 * Source texture for Texture mode.
	 * This is the primary visual source when SourceMode is set to Texture.
	 * In Material mode, this is used as reference only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Source", meta = (EditCondition = "SourceMode == EPixelSourceMode::Texture", EditConditionHides))
	UTexture2D* SourceTexture;

	/**
	 * Source material for Material mode.
	 * This is the primary visual source when SourceMode is set to Material.
	 * Must have a Texture parameter named "PixelTexture" for compatibility.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Source", meta = (EditCondition = "SourceMode == EPixelSourceMode::Material", EditConditionHides))
	UMaterialInterface* SourceMaterial;

	/**
	 * Manual dimensions for Material mode (when source has no inherent size).
	 * Required when using Material source mode to prevent zero-size widgets.
	 * Default: 16x16 pixels.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Source", meta = (EditCondition = "SourceMode == EPixelSourceMode::Material", EditConditionHides))
	FIntPoint ManualMaterialSize;

	/** Asset name from Aseprite metadata */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component")
	FString AssetName;

	/** 
	 * Display name for this asset in UI and asset pickers.
	 * If empty, AssetName is used as fallback.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Metadata", meta = (MultiLine = "false"))
	FText DisplayName;

	/** 
	 * Search tags for asset discovery and filtering.
	 * Comma-separated keywords to improve searchability.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Metadata")
	FString SearchTags;

	/** All slices defined in the Aseprite file */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component")
	TArray<FSliceData> Slices;

	/** Layer metadata from Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component")
	TArray<FLayerMetadata> Layers;

	/** Layer name to material parameter mapping */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Material")
	TMap<FString, FName> LayerToMaterialParamMap;

	/** Palette profiles for dynamic color overrides */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Palette")
	TMap<FString, FPixelPaletteProfile> PaletteProfiles;

	/** Default 9-slice margins (for the primary slice) in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|9-Slice")
	FMargin NineSliceMargins;

	/** Animation sequences from Aseprite frame tags */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Animation")
	TMap<FString, FPixelAnimSequence> AnimationSequences;

	/** Legacy animation tags (for backwards compatibility) */
	UPROPERTY(VisibleAnywhere, Category = "Pixel Component|Animation|Deprecated")
	TArray<FAnimationTag> AnimationTags;

	/** Frame data for animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Animation")
	TArray<FFrameData> Frames;

	/** Default pivot point for slices without specific pivot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Pivot")
	FPixelPivot DefaultPivot;

	/** Cached texture width */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	int32 CachedTextureWidth;

	/** Cached texture height */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	int32 CachedTextureHeight;

	/** Original texture width (before scaling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	int32 OriginalTextureWidth;

	/** Original texture height (before scaling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	int32 OriginalTextureHeight;

	/** Whether UVs have been computed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	bool bUVsComputed;

	/** Whether this asset has been scaled from original size */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
	bool bIsScaled;

	/** Scale factor applied to this asset */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pixel Component|Cache")
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

	/**
	 * Validate ManualMaterialSize is non-zero for Material mode.
	 * Resets to 16x16 default if invalid.
	 */
	void ValidateManualMaterialSize();

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
