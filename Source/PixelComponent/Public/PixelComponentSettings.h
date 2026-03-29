// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PixelComponentSettings.generated.h"

/**
 * Pixel Size Scaling Mode
 * Determines how the global pixel size affects imported assets.
 */
UENUM(BlueprintType)
enum class EPixelSizeScalingMode : uint8
{
	/** No scaling - use original pixel dimensions from Aseprite */
	None					UMETA(DisplayName = "None (Original Size)"),
	
	/** Scale all assets to match the global pixel size */
	ScaleToGlobal			UMETA(DisplayName = "Scale to Global Pixel Size"),
	
	/** Scale down only - assets larger than global size will be scaled down */
	ScaleDownOnly			UMETA(DisplayName = "Scale Down Only"),
	
	/** Scale up only - assets smaller than global size will be scaled up */
	ScaleUpOnly				UMETA(DisplayName = "Scale Up Only")
};

/**
 * UPixelComponentSettings
 * 
 * Global configuration for PixelComponent plugin.
 * Access via: Project Settings > Plugins > PixelComponent
 * 
 * These settings affect all PixelComponent assets globally and can be
 * overridden per-asset if needed.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "PixelComponent"))
class PIXELCOMPONENT_API UPixelComponentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPixelComponentSettings();

	/**
	 * Get the global settings singleton.
	 * @return Pointer to the settings object (may be null in some contexts)
	 */
	static UPixelComponentSettings* Get();

	/**
	 * Get the current global pixel size in pixels.
	 * This is the reference size used for scaling calculations.
	 * 
	 * @return Global pixel size (e.g., 16, 32, 64)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static int32 GetGlobalPixelSize();

	/**
	 * Set the global pixel size.
	 * Changes will affect newly imported assets.
	 * 
	 * @param NewSize The new global pixel size
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static void SetGlobalPixelSize(int32 NewSize);

	/**
	 * Get the scaling mode for pixel size.
	 * @return Current scaling mode
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static EPixelSizeScalingMode GetScalingMode();

	/**
	 * Calculate the scale factor for a given asset pixel size.
	 * 
	 * @param AssetPixelSize The original pixel size of the asset
	 * @return Scale factor to apply (1.0 = no scaling)
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static float CalculateScaleFactor(int32 AssetPixelSize);

	/**
	 * Apply global pixel size scaling to a dimension.
	 * 
	 * @param OriginalSize Original size in pixels
	 * @return Scaled size in pixels
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static int32 ApplyPixelSizeScaling(int32 OriginalSize);

	/**
	 * Get the effective texture dimensions after applying pixel size scaling.
	 * 
	 * @param OriginalWidth Original width in pixels
	 * @param OriginalHeight Original height in pixels
	 * @param OutWidth Output for scaled width
	 * @param OutHeight Output for scaled height
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	static void GetScaledDimensions(
		int32 OriginalWidth,
		int32 OriginalHeight,
		int32& OutWidth,
		int32& OutHeight
	);

	/**
	 * Check if the settings are valid.
	 * @return true if settings are valid
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelComponent|Settings")
	bool AreSettingsValid() const;

protected:
	//~ Begin UDeveloperSettings Interface
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	//~ End UDeveloperSettings Interface

public:
	/**
	 * Global Pixel Size
	 * 
	 * The reference pixel size used for all PixelComponent assets.
	 * Common values: 8, 16, 32, 64, 128
	 * 
	 * This setting affects:
	 * - UV coordinate calculations
	 * - 9-slice margin scaling
	 * - Material parameter values
	 * - Runtime rendering scale
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Global Settings", meta = (
		DisplayName = "Global Pixel Size",
		ToolTip = "The reference pixel size for all PixelComponent assets (e.g., 16, 32, 64)",
		ClampMin = "1",
		ClampMax = "1024",
		UIMin = "8",
		UIMax = "256"
	))
	int32 GlobalPixelSize;

	/**
	 * Pixel Size Scaling Mode
	 * 
	 * Controls how assets are scaled to match the global pixel size.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Global Settings", meta = (
		DisplayName = "Scaling Mode",
		ToolTip = "How assets should be scaled to match the global pixel size"
	))
	EPixelSizeScalingMode ScalingMode;

	/**
	 * Enable automatic scaling on import
	 * 
	 * If true, assets will be automatically scaled during import.
	 * If false, assets keep their original size.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Global Settings", meta = (
		DisplayName = "Enable Auto-Scale on Import",
		ToolTip = "Automatically scale assets during import to match global pixel size"
	))
	bool bEnableAutoScaleOnImport;

	/**
	 * Snap imported sprites to global pixel size
	 * 
	 * If true, sprite dimensions will be rounded to nearest multiple of global pixel size.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Global Settings", meta = (
		DisplayName = "Snap to Pixel Grid",
		ToolTip = "Snap sprite dimensions to multiples of global pixel size"
	))
	bool bSnapToPixelGrid;

	/**
	 * Default texture interpolation mode for pixel art.
	 *
	 * For pixel art, typically you want Nearest/Point filtering to preserve crisp pixels.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Texture Settings", meta = (
		DisplayName = "Default Texture Filter",
		ToolTip = "Default texture filter mode for imported pixel art textures"
	))
	TEnumAsByte<TextureFilter> DefaultTextureFilter;

	/**
	 * Whether to enable mipmaps for pixel art textures.
	 *
	 * Typically disabled for pixel art to preserve crisp edges at all distances.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Texture Settings", meta = (
		DisplayName = "Generate Mipmaps",
		ToolTip = "Generate mipmaps for imported pixel art textures (typically disabled for pixel art)"
	))
	bool bGenerateMipmaps;

	/**
	 * Default material for PixelComponent widgets.
	 *
	 * This material is used by UPixelComponent widgets to render pixel art.
	 * Should have a Texture parameter named "PixelTexture".
	 */
	UPROPERTY(EditAnywhere, Category = "Material Settings", meta = (
		DisplayName = "Default Pixel Material",
		ToolTip = "Default material used by PixelComponent widgets (should have PixelTexture parameter)"
	))
	UMaterialInterface* DefaultPixelMaterial;

	/**
	 * Group name for organizing imported assets.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Import Settings", meta = (
		DisplayName = "Asset Group",
		ToolTip = "Group name for organizing imported PixelComponent assets"
	))
	FString AssetGroupName;

private:
	/** Cached settings pointer */
	static UPixelComponentSettings* CachedSettings;
};
