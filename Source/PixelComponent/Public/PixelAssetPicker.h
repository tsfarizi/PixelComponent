// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "PixelComponentAsset.h"
#include "PixelAssetPicker.generated.h"

/**
 * Delegate fired when a PixelComponentAsset is selected from the picker.
 * @param SelectedAsset The asset that was selected
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPixelAssetSelected, UPixelComponentAsset*, SelectedAsset);

/**
 * UPixelAssetPicker
 *
 * UMG Widget for discovering and selecting PixelComponentAsset instances from the project.
 * Provides a searchable list of all available PixelComponent assets with caching support.
 *
 * Features:
 * - Automatic asset discovery via Asset Registry
 * - Case-insensitive name filtering
 * - Cached results to avoid repeated registry scans
 * - Manual refresh capability
 *
 * Usage:
 * 1. Add "Pixel Asset Picker" to UMG layout
 * 2. Bind OnAssetSelected event to handle selection
 * 3. Call RefreshAssets() to populate the list
 * 4. Use FilterAssets() to search by name
 */
UCLASS(ClassGroup = (Custom), meta = (DisplayName = "Pixel Asset Picker", Category = "Pixel Component"))
class PIXELCOMPONENT_API UPixelAssetPicker : public UWidget
{
	GENERATED_BODY()

public:
	UPixelAssetPicker();

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	/**
	 * Event fired when an asset is selected from the picker.
	 * Bind this in Blueprint to handle asset selection.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pixel Component|Events")
	FOnPixelAssetSelected OnAssetSelected;

	/**
	 * Refreshes the internal list of available PixelComponent assets.
	 * Scans the Asset Registry and updates the cached list.
	 * Call this when assets may have changed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	void RefreshAssets();

	/**
	 * Filters the cached asset list by name.
	 * @param SearchTerm Substring to search for (case-insensitive)
	 * @return Array of assets matching the search term
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	TArray<UPixelComponentAsset*> FilterAssets(const FString& SearchTerm) const;

	/**
	 * Gets all cached assets without filtering.
	 * @return Array of all cached PixelComponentAsset objects
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	TArray<UPixelComponentAsset*> GetAllCachedAssets() const { return CachedAssets; }

	/**
	 * Gets the number of assets currently cached.
	 * @return Number of cached assets
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	int32 GetAssetCount() const { return CachedAssets.Num(); }

	/**
	 * Checks if the asset cache is empty (no assets discovered).
	 * @return true if no assets are cached
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	bool IsCacheEmpty() const { return CachedAssets.Num() == 0; }

	/**
	 * Selects an asset from the list and fires the OnAssetSelected event.
	 * @param Asset The asset to select
	 */
	UFUNCTION(BlueprintCallable, Category = "Pixel Component|Asset Discovery")
	void SelectAsset(UPixelComponentAsset* Asset);

protected:
	/** Cached list of discovered PixelComponent assets */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Pixel Component|Cache")
	TArray<UPixelComponentAsset*> CachedAssets;

	/** Indicates whether the cache needs to be refreshed */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Pixel Component|Cache")
	bool bCacheDirty;

	/**
	 * Internal function to discover assets via Asset Registry.
	 * @param SearchTerm Optional filter term
	 * @return Array of discovered assets
	 */
	TArray<UPixelComponentAsset*> DiscoverAssets(const FString& SearchTerm) const;
};
