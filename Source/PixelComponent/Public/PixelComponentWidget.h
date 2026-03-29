// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "PixelComponentAsset.h"
#include "PixelComponentWidget.generated.h"

/**
 * UPixelComponent
 *
 * UMG Widget for displaying PixelComponent assets with automatic material
 * and metadata injection. This widget extends UImage to provide seamless
 * integration with the PixelComponent system.
 *
 * Visual Encapsulation:
 * - Texture source is exclusively from UPixelComponentAsset
 * - No manual texture input allowed
 * - Brush is automatically configured when PixelAsset is assigned
 *
 * Features:
 * - Automatic material instance creation from settings
 * - Slice UV coordinate injection for sprite sheet regions
 * - 9-slice margin support for scalable UI elements
 * - Palette profile support for dynamic recoloring
 * - Real-time preview in UMG Designer via PostEditChangeProperty
 *
 * Usage:
 * 1. Add "Pixel Component" widget to UMG layout from Palette
 * 2. Assign PixelComponentAsset to PixelAsset property
 * 3. Optionally specify TargetSlice for sprite sheet regions
 * 4. Optionally assign ActivePaletteProfile for color overrides
 *
 * The widget automatically manages Dynamic Material Instance and configures
 * all parameters (texture, UVs, margins, colors) when properties change.
 */
UCLASS(ClassGroup = (Custom), meta = (DisplayName = "Pixel Component", Category = "Pixel Component"))
class PIXELCOMPONENT_API UPixelComponent : public UImage
{
	GENERATED_BODY()

public:
	UPixelComponent();

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	// ========================================================================
	// Pixel Component API
	// ========================================================================

	/**
	 * Get the PixelComponent asset assigned to this widget.
	 * @return Pointer to the PixelComponent asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	UPixelComponentAsset* GetPixelAsset() const { return PixelAsset; }

	/**
	 * Set the PixelComponent asset for this widget.
	 * Texture and material are automatically configured from the asset.
	 * @param NewAsset The PixelComponent asset to display
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void SetPixelAsset(UPixelComponentAsset* NewAsset);

	/**
	 * Get the target slice name for sprite sheet regions.
	 * @return Name of the target slice
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	FString GetTargetSlice() const { return TargetSlice; }

	/**
	 * Set the target slice name for sprite sheet regions.
	 * Leave empty to display the full texture.
	 * @param NewSlice Name of the slice to display
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void SetTargetSlice(const FString& NewSlice);

	/**
	 * Get the active palette profile name.
	 * @return Name of the active palette profile
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	FName GetActivePaletteProfile() const { return ActivePaletteProfile; }

	/**
	 * Set the active palette profile for dynamic color overrides.
	 * @param NewProfile Name of the palette profile
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Palette")
	void SetActivePaletteProfile(const FName& NewProfile);

	/**
	 * Check if auto-initialize material is enabled.
	 * @return true if material is auto-initialized
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	bool IsAutoInitializeMaterialEnabled() const { return bAutoInitializeMaterial; }

	/**
	 * Enable or disable auto-initialize material.
	 * @param bEnabled Whether to auto-initialize material
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void SetAutoInitializeMaterialEnabled(bool bEnabled);

	/**
	 * Get the dynamic material instance created by this widget.
	 * @return Pointer to the dynamic material instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	UMaterialInstanceDynamic* GetPixelDynamicMaterial() const { return DynamicMaterialInstance; }

	/**
	 * Manually refresh material parameters.
	 * Call this after changing properties at runtime.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void RefreshMaterialParameters();

	/**
	 * Clear the current material and reset the widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void ClearMaterial();

protected:
	// ========================================================================
	// Properties
	// ========================================================================

	/**
	 * The PixelComponent asset to display.
	 * Texture and material are automatically derived from this asset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Image", meta = (AllowPrivateAccess = "true"))
	UPixelComponentAsset* PixelAsset;

	/**
	 * The specific slice to display from the sprite sheet.
	 * Leave empty to display the full texture.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Image", meta = (AllowPrivateAccess = "true"))
	FString TargetSlice;

	/**
	 * Active palette profile for dynamic color overrides.
	 * Leave empty to use default colors.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Palette", meta = (AllowPrivateAccess = "true"))
	FName ActivePaletteProfile;

	/**
	 * Automatically create and configure material instance.
	 * When enabled, the widget automatically creates a Dynamic Material
	 * Instance and configures all parameters when PixelAsset changes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Image", meta = (AllowPrivateAccess = "true"))
	bool bAutoInitializeMaterial;

	/**
	 * Internal dynamic material instance.
	 * Created automatically when bAutoInitializeMaterial is true.
	 */
	UPROPERTY(Transient, DuplicateTransient, BlueprintReadOnly, Category = "Pixel Component|Internal")
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	// ========================================================================
	// Internal Methods
	// ========================================================================

	/**
	 * Initialize the dynamic material instance.
	 * Creates a new MID from settings base material and configures parameters.
	 */
	void InitializeMaterialInstance();

	/**
	 * Send all material parameters based on current configuration.
	 * Includes texture, UV coordinates, 9-slice margins, and palette colors.
	 */
	void SendMaterialParameters();

	/**
	 * Validate that the current configuration is valid.
	 * @return true if PixelAsset is valid and ready to use
	 */
	bool ValidateConfiguration() const;

	/**
	 * Apply texture from PixelAsset to the widget brush.
	 * Automatically retrieves SourceTexture from the asset.
	 */
	void ApplyTextureFromAsset();
};
