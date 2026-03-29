// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelAssetPicker.h"
#include "PixelComponentMaterialLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelAssetPicker, Log, All);

UPixelAssetPicker::UPixelAssetPicker()
	: bCacheDirty(true)
{
	bIsVariable = true;
}

void UPixelAssetPicker::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (bCacheDirty)
	{
		RefreshAssets();
	}
}

void UPixelAssetPicker::RefreshAssets()
{
	CachedAssets = FPixelComponentMaterialLibrary::GetAllAvailablePixelAssets();
	bCacheDirty = false;

	UE_LOG(LogPixelAssetPicker, Log, TEXT("Refreshed asset cache: %d assets found"), CachedAssets.Num());
}

TArray<UPixelComponentAsset*> UPixelAssetPicker::FilterAssets(const FString& SearchTerm) const
{
	if (SearchTerm.IsEmpty())
	{
		return CachedAssets;
	}

	TArray<UPixelComponentAsset*> FilteredAssets;
	const FString SearchTermLower = SearchTerm.ToLower();

	for (UPixelComponentAsset* Asset : CachedAssets)
	{
		if (Asset)
		{
			const FString AssetName = Asset->GetAssetName().ToLower();
			if (AssetName.Contains(SearchTermLower))
			{
				FilteredAssets.Add(Asset);
			}
		}
	}

	UE_LOG(LogPixelAssetPicker, Verbose, TEXT("Filtered assets: %d matches for '%s'"), FilteredAssets.Num(), *SearchTerm);
	return FilteredAssets;
}

void UPixelAssetPicker::SelectAsset(UPixelComponentAsset* Asset)
{
	if (!Asset)
	{
		UE_LOG(LogPixelAssetPicker, Warning, TEXT("Attempted to select null asset"));
		return;
	}

	if (OnAssetSelected.IsBound())
	{
		OnAssetSelected.Broadcast(Asset);
		UE_LOG(LogPixelAssetPicker, Log, TEXT("Asset selected: %s"), *Asset->GetAssetName());
	}
	else
	{
		UE_LOG(LogPixelAssetPicker, Warning, TEXT("OnAssetSelected event is not bound"));
	}
}

TArray<UPixelComponentAsset*> UPixelAssetPicker::DiscoverAssets(const FString& SearchTerm) const
{
	if (SearchTerm.IsEmpty())
	{
		return FPixelComponentMaterialLibrary::GetAllAvailablePixelAssets();
	}
	else
	{
		return FPixelComponentMaterialLibrary::GetFilteredPixelAssets(SearchTerm);
	}
}
