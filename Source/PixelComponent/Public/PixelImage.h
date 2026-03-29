// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "PixelComponentAsset.h"
#include "PixelImage.generated.h"

/**
 * UPixelImage
 *
 * Custom UMG Widget for displaying PixelComponent assets with automatic
 * material parameter injection. This widget extends UImage to provide
 * seamless integration with the PixelComponent system.
 *
 * Features:
 * - Automatic material instance creation and management
 * - Slice UV coordinate injection for sprite sheet animation
 * - 9-slice margin support for scalable UI elements
 * - Real-time preview in UMG Designer via PostEditChangeProperty
 *
 * Usage:
 * 1. Add "Pixel Image" widget to your UMG layout from the Palette
 * 2. Assign a PixelComponentAsset to the PixelAsset property
 * 3. Optionally specify a TargetSlice for sprite sheet regions
 * 4. Enable bAutoInitializeMaterial for automatic setup
 *
 * The widget automatically creates a Dynamic Material Instance and configures
 * all necessary parameters (texture dimensions, UV coordinates, 9-slice margins)
 * when the PixelAsset or TargetSlice properties change.
 */
UCLASS(ClassGroup = (Custom), meta = (DisplayName = "Pixel Image", Category = "Pixel Component"))
class PIXELCOMPONENT_API UPixelImage : public UImage
{
	GENERATED_BODY()

public:
	UPixelImage();

	// ========================================================================
	// UWidget Interface
	// ========================================================================

	/**
	 * Synchronize properties from widget to Slate representation.
	 * Called when widget properties change in the Designer or at runtime.
	 */
	virtual void SynchronizeProperties() override;

	// ========================================================================
	// UObject Interface
	// ========================================================================

	/**
	 * Handle property changes in the Editor for real-time preview.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	// ========================================================================
	// Pixel Image API
	// ========================================================================

	/**
	 * Get the PixelComponent asset assigned to this widget.
	 * @return Pointer to the PixelComponent asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	UPixelComponentAsset* GetPixelAsset() const { return PixelAsset; }

	/**
	 * Set the PixelComponent asset for this widget.
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
	 * @param NewSlice Name of the slice to display
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Image")
	void SetTargetSlice(const FString& NewSlice);

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
	 * Call this after changing PixelAsset or TargetSlice at runtime.
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
	 * Assign your imported Aseprite asset here.
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
	 * Automatically create and configure material instance.
	 * When enabled, the widget will automatically create a Dynamic Material
	 * Instance and configure all parameters when PixelAsset changes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixel Component|Image", meta = (AllowPrivateAccess = "true"))
	bool bAutoInitializeMaterial;

	/**
	 * Internal dynamic material instance.
	 * Created automatically when bAutoInitializeMaterial is true.
	 */
	UPROPERTY(Transient, DuplicateTransient)
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	// ========================================================================
	// Internal Methods
	// ========================================================================

	/**
	 * Initialize the dynamic material instance.
	 * Creates a new MID from the Brush's material and configures parameters.
	 */
	void InitializeMaterialInstance();

	/**
	 * Send all material parameters based on current PixelAsset and TargetSlice.
	 * Includes texture dimensions, UV coordinates, and 9-slice margins.
	 */
	void SendMaterialParameters();

	/**
	 * Validate that the current configuration is valid.
	 * @return true if PixelAsset is valid and ready to use
	 */
	bool ValidateConfiguration() const;

	/**
	 * Get the base material from the current Brush.
	 * @return Pointer to the base material
	 */
	UMaterialInterface* GetBaseMaterial() const;
};
