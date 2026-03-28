// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentAsset.h"
#include "Engine/Texture2D.h"
#include "PixelComponentSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentAsset, Log, All);

UPixelComponentAsset::UPixelComponentAsset()
	: SourceTexture(nullptr)
	, NineSliceMargins(0.0f)
	, CachedTextureWidth(0)
	, CachedTextureHeight(0)
	, OriginalTextureWidth(0)
	, OriginalTextureHeight(0)
	, bUVsComputed(false)
	, bIsScaled(false)
	, AppliedScaleFactor(1.0f)
{
}

void UPixelComponentAsset::SetSourceTexture(UTexture2D* NewTexture)
{
	SourceTexture = NewTexture;
	UpdateTextureDimensionsCache();
	RefreshNormalizedUVs();
}

const FSliceData* UPixelComponentAsset::FindSliceByName(const FString& SliceName) const
{
	for (const FSliceData& Slice : Slices)
	{
		if (Slice.Name == SliceName)
		{
			return &Slice;
		}
	}
	return nullptr;
}

void UPixelComponentAsset::GetTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
	OutWidth = CachedTextureWidth;
	OutHeight = CachedTextureHeight;
}

void UPixelComponentAsset::GetEffectiveTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
	// Return dimensions after applying global pixel size scaling
	OutWidth = CachedTextureWidth;
	OutHeight = CachedTextureHeight;
}

void UPixelComponentAsset::GetOriginalTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
	// Return original dimensions before any scaling
	if (OriginalTextureWidth > 0 && OriginalTextureHeight > 0)
	{
		OutWidth = OriginalTextureWidth;
		OutHeight = OriginalTextureHeight;
	}
	else
	{
		OutWidth = CachedTextureWidth;
		OutHeight = CachedTextureHeight;
	}
}

int32 UPixelComponentAsset::GetGlobalPixelSize()
{
	return UPixelComponentSettings::GetGlobalPixelSize();
}

float UPixelComponentAsset::GetScaleFactor() const
{
	return AppliedScaleFactor;
}

FBox2f UPixelComponentAsset::ComputeNormalizedUVs(const FPixelRect& PixelRect) const
{
	return PixelRect.ToNormalizedUV(CachedTextureWidth, CachedTextureHeight);
}

FBox2f UPixelComponentAsset::GetSliceNormalizedUVs(const FString& SliceName) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice)
	{
		return Slice->NormalizedUVs;
	}
	return FBox2f(FVector2f::ZeroVector, FVector2f::ZeroVector);
}

FBox2f UPixelComponentAsset::GetNineSliceCenterUVs(const FString& SliceName) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice && Slice->bIsNineSlice)
	{
		return Slice->GetCenterUV();
	}
	return FBox2f(FVector2f::ZeroVector, FVector2f::ZeroVector);
}

int32 UPixelComponentAsset::GetTotalAnimationDurationMs() const
{
	int32 TotalDuration = 0;
	for (const FFrameData& Frame : Frames)
	{
		TotalDuration += Frame.DurationMs;
	}
	return TotalDuration;
}

void UPixelComponentAsset::RefreshNormalizedUVs()
{
	UpdateTextureDimensionsCache();

	if (CachedTextureWidth <= 0 || CachedTextureHeight <= 0)
	{
		UE_LOG(LogPixelComponentAsset, Warning, TEXT("Cannot compute UVs: invalid texture dimensions %dx%d"), 
			CachedTextureWidth, CachedTextureHeight);
		bUVsComputed = false;
		return;
	}

	// Compute UVs for all slices using effective (scaled) dimensions
	for (FSliceData& Slice : Slices)
	{
		Slice.ComputeNormalizedUVs(CachedTextureWidth, CachedTextureHeight);
	}

	bUVsComputed = true;
	UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Refreshed UVs for %d slices (effective size: %dx%d)"), 
		Slices.Num(), CachedTextureWidth, CachedTextureHeight);
}

FAsepriteParseResult UPixelComponentAsset::ParseSlicesFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FAsepriteParseResult Result;

	if (!JsonObject.IsValid())
	{
		return FAsepriteParseResult::Error(TEXT("Invalid JSON object for slice parsing"));
	}

	// Get the "slices" array from Aseprite JSON
	const TArray<TSharedPtr<FJsonValue>>* SlicesArray;
	if (!JsonObject->TryGetArrayField(TEXT("slices"), SlicesArray))
	{
		// No slices is not necessarily an error - some sprites don't have slices
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("No slices found in JSON"));
		return Result;
	}

	Slices.Empty(SlicesArray->Num());

	for (const TSharedPtr<FJsonValue>& SliceValue : *SlicesArray)
	{
		const TSharedPtr<FJsonObject>* SliceObject = SliceValue->AsObject();
		if (!SliceObject || !SliceObject->IsValid())
		{
			Result.AddWarning(TEXT("Invalid slice object in JSON"));
			continue;
		}

		FSliceData Slice;

		// Parse slice name
		FString SliceName;
		if ((*SliceObject)->TryGetStringField(TEXT("name"), SliceName))
		{
			Slice.Name = SliceName;
		}
		else
		{
			Result.AddWarning(FString::Printf(TEXT("Slice missing name field")));
			continue;
		}

		// Parse the "bounds" object for pixel rectangle
		const TSharedPtr<FJsonObject>* BoundsObject;
		if ((*SliceObject)->TryGetObjectField(TEXT("bounds"), BoundsObject) && BoundsObject->IsValid())
		{
			int32 X, Y, W, H;
			if ((*BoundsObject)->TryGetNumberField(TEXT("x"), X) &&
				(*BoundsObject)->TryGetNumberField(TEXT("y"), Y) &&
				(*BoundsObject)->TryGetNumberField(TEXT("w"), W) &&
				(*BoundsObject)->TryGetNumberField(TEXT("h"), H))
			{
				Slice.PixelRect = FPixelRect(X, Y, W, H);
			}
			else
			{
				Result.AddWarning(FString::Printf(TEXT("Slice '%s' has invalid bounds"), *SliceName));
				continue;
			}
		}
		else
		{
			Result.AddWarning(FString::Printf(TEXT("Slice '%s' missing bounds"), *SliceName));
			continue;
		}

		// Check for 9-slice data in the "keys" array
		const TArray<TSharedPtr<FJsonValue>>* KeysArray;
		if ((*SliceObject)->TryGetArrayField(TEXT("keys"), KeysArray) && KeysArray->Num() > 0)
		{
			// Check the first key for 9-slice information
			const TSharedPtr<FJsonObject>* FirstKey = (*KeysArray)[0]->AsObject();
			if (FirstKey && FirstKey->IsValid())
			{
				// Look for "bounds" in the key that indicates 9-slice
				const TSharedPtr<FJsonObject>* KeyBoundsObject;
				if ((*FirstKey)->TryGetObjectField(TEXT("bounds"), KeyBoundsObject) && KeyBoundsObject->IsValid())
				{
					// This slice has 9-slice data
					Slice.bIsNineSlice = true;
					
					if (ExtractNineSliceMargins(*KeysArray, Slice.NineSliceMargins))
					{
						UE_LOG(LogPixelComponentAsset, Verbose, 
							TEXT("Parsed 9-slice margins for '%s': L=%f, T=%f, R=%f, B=%f"),
							*Slice.Name, 
							Slice.NineSliceMargins.Left,
							Slice.NineSliceMargins.Top,
							Slice.NineSliceMargins.Right,
							Slice.NineSliceMargins.Bottom);
					}
					else
					{
						Result.AddWarning(FString::Printf(TEXT("Slice '%s' marked as 9-slice but margins could not be extracted"), *SliceName));
					}
				}
			}
		}

		Slices.Add(Slice);
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed slice: %s (%dx%d @ %d,%d)"),
			*Slice.Name, Slice.PixelRect.Width, Slice.PixelRect.Height, Slice.PixelRect.X, Slice.PixelRect.Y);
	}

	// Set default 9-slice margins from the first 9-slice found
	for (const FSliceData& Slice : Slices)
	{
		if (Slice.bIsNineSlice)
		{
			NineSliceMargins = Slice.NineSliceMargins;
			break;
		}
	}

	return Result;
}

FAsepriteParseResult UPixelComponentAsset::ParseLayersFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FAsepriteParseResult Result;

	if (!JsonObject.IsValid())
	{
		return FAsepriteParseResult::Error(TEXT("Invalid JSON object for layer parsing"));
	}

	// Get the "layers" array from Aseprite JSON
	const TArray<TSharedPtr<FJsonValue>>* LayersArray;
	if (!JsonObject->TryGetArrayField(TEXT("layers"), LayersArray))
	{
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("No layers found in JSON"));
		return Result;
	}

	Layers.Empty(LayersArray->Num());

	for (const TSharedPtr<FJsonValue>& LayerValue : *LayersArray)
	{
		const TSharedPtr<FJsonObject>* LayerObject = LayerValue->AsObject();
		if (!LayerObject || !LayerObject->IsValid())
		{
			Result.AddWarning(TEXT("Invalid layer object in JSON"));
			continue;
		}

		FLayerMetadata Layer;

		// Parse layer name
		FString LayerName;
		if ((*LayerObject)->TryGetStringField(TEXT("name"), LayerName))
		{
			Layer.Name = LayerName;
		}

		// Parse visibility
		bool bVisible = true;
		if ((*LayerObject)->TryGetBoolField(TEXT("show"), bVisible))
		{
			Layer.bVisible = bVisible;
		}

		// Parse opacity (0-255 in Aseprite, convert to 0-1)
		int32 OpacityInt = 255;
		if ((*LayerObject)->TryGetNumberField(TEXT("opacity"), OpacityInt))
		{
			Layer.Opacity = FMath::Clamp(OpacityInt / 255.0f, 0.0f, 1.0f);
		}

		// Parse blend mode
		FString BlendMode;
		if ((*LayerObject)->TryGetStringField(TEXT("blendMode"), BlendMode))
		{
			Layer.BlendMode = BlendMode;
		}

		// Parse user data color (optional)
		const TSharedPtr<FJsonObject>* UserDataObject;
		if ((*LayerObject)->TryGetObjectField(TEXT("userData"), UserDataObject) && UserDataObject->IsValid())
		{
			FString UserDataColorStr;
			if ((*UserDataObject)->TryGetStringField(TEXT("color"), UserDataColorStr))
			{
				// Parse hex color if present
				if (UserDataColorStr.StartsWith(TEXT("#")))
				{
					UserDataColorStr = UserDataColorStr.RightChop(1);
					if (UserDataColorStr.Len() >= 6)
					{
						uint32 ColorValue = FCString::Strtoui(*UserDataColorStr.Left(6), nullptr, 16);
						Layer.UserDataColor.R = (ColorValue >> 16) & 0xFF;
						Layer.UserDataColor.G = (ColorValue >> 8) & 0xFF;
						Layer.UserDataColor.B = ColorValue & 0xFF;
						Layer.UserDataColor.A = 255;
					}
				}
			}
		}

		Layers.Add(Layer);
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed layer: %s (visible=%d, opacity=%.2f)"),
			*Layer.Name, Layer.bVisible, Layer.Opacity);
	}

	return Result;
}

FAsepriteParseResult UPixelComponentAsset::ParseAnimationFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FAsepriteParseResult Result;

	if (!JsonObject.IsValid())
	{
		return FAsepriteParseResult::Error(TEXT("Invalid JSON object for animation parsing"));
	}

	// Parse animation tags
	const TArray<TSharedPtr<FJsonValue>>* TagsArray;
	if (JsonObject->TryGetArrayField(TEXT("tags"), TagsArray))
	{
		AnimationTags.Empty(TagsArray->Num());

		for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
		{
			const TSharedPtr<FJsonObject>* TagObject = TagValue->AsObject();
			if (!TagObject || !TagObject->IsValid())
			{
				continue;
			}

			FAnimationTag Tag;

			// Parse tag name
			(*TagObject)->TryGetStringField(TEXT("name"), Tag.Name);

			// Parse from/to frames
			int32 FromFrame = 0;
			int32 ToFrame = 0;
			if ((*TagObject)->TryGetNumberField(TEXT("from"), FromFrame))
			{
				Tag.FromFrame = FromFrame;
			}
			if ((*TagObject)->TryGetNumberField(TEXT("to"), ToFrame))
			{
				Tag.ToFrame = ToFrame;
			}

			// Parse direction
			(*TagObject)->TryGetStringField(TEXT("direction"), Tag.Direction);

			// Parse repeat count
			int32 Repeat = 0;
			if ((*TagObject)->TryGetNumberField(TEXT("repeat"), Repeat))
			{
				Tag.Repeat = Repeat;
			}

			AnimationTags.Add(Tag);
			UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed animation tag: %s (%d-%d)"),
				*Tag.Name, Tag.FromFrame, Tag.ToFrame);
		}
	}

	// Parse frame data
	const TArray<TSharedPtr<FJsonValue>>* FramesArray;
	if (JsonObject->TryGetArrayField(TEXT("frames"), FramesArray))
	{
		Frames.Empty(FramesArray->Num());

		for (int32 FrameIndex = 0; FrameIndex < FramesArray->Num(); ++FrameIndex)
		{
			const TSharedPtr<FJsonValue>& FrameValue = (*FramesArray)[FrameIndex];
			const TSharedPtr<FJsonObject>* FrameObject = FrameValue->AsObject();
			if (!FrameObject || !FrameObject->IsValid())
			{
				continue;
			}

			FFrameData Frame;
			Frame.FrameIndex = FrameIndex;

			// Parse duration
			const TSharedPtr<FJsonObject>* DurationObject;
			if ((*FrameObject)->TryGetObjectField(TEXT("duration"), DurationObject))
			{
				int32 DurationMs = 100;
				if ((*DurationObject)->TryGetNumberField(TEXT("value"), DurationMs))
				{
					Frame.DurationMs = DurationMs;
				}
			}
			else
			{
				// Duration might be a direct number
				int32 DurationMs = 100;
				if ((*FrameObject)->TryGetNumberField(TEXT("duration"), DurationMs))
				{
					Frame.DurationMs = DurationMs;
				}
			}

			// Parse frame rectangle (for sprite sheets)
			const TSharedPtr<FJsonObject>* FrameRectObject;
			if ((*FrameObject)->TryGetObjectField(TEXT("frame"), FrameRectObject) && FrameRectObject->IsValid())
			{
				int32 X, Y, W, H;
				if ((*FrameRectObject)->TryGetNumberField(TEXT("x"), X) &&
					(*FrameRectObject)->TryGetNumberField(TEXT("y"), Y) &&
					(*FrameRectObject)->TryGetNumberField(TEXT("w"), W) &&
					(*FrameRectObject)->TryGetNumberField(TEXT("h"), H))
				{
					Frame.Position = FPixelRect(X, Y, W, H);
				}
			}

			Frames.Add(Frame);
		}

		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed %d frames"), Frames.Num());
	}

	return Result;
}

bool UPixelComponentAsset::ExtractNineSliceMargins(
	const TArray<TSharedPtr<FJsonValue>>& KeyArray,
	FMargin& OutMargins)
{
	if (KeyArray.Num() == 0)
	{
		return false;
	}

	// Aseprite stores 9-slice data in the "bounds" of each key
	// The margins are computed from the difference between slice bounds and key bounds
	// We need to find the key that has the 9-slice information

	for (const TSharedPtr<FJsonValue>& KeyValue : KeyArray)
	{
		const TSharedPtr<FJsonObject>* KeyObject = KeyValue->AsObject();
		if (!KeyObject || !KeyObject->IsValid())
		{
			continue;
		}

		// Look for 9-slice specific bounds
		const TSharedPtr<FJsonObject>* KeyBoundsObject;
		if ((*KeyObject)->TryGetObjectField(TEXT("bounds"), KeyBoundsObject) && KeyBoundsObject->IsValid())
		{
			int32 KeyX, KeyY, KeyW, KeyH;
			if ((*KeyBoundsObject)->TryGetNumberField(TEXT("x"), KeyX) &&
				(*KeyBoundsObject)->TryGetNumberField(TEXT("y"), KeyY) &&
				(*KeyBoundsObject)->TryGetNumberField(TEXT("w"), KeyW) &&
				(*KeyBoundsObject)->TryGetNumberField(TEXT("h"), KeyH))
			{
				// The key bounds represent the center region of the 9-slice
				// We need additional data to compute margins properly
				// Aseprite also provides "color" field for 9-slice keys
				
				FString ColorStr;
				if ((*KeyObject)->TryGetStringField(TEXT("color"), ColorStr))
				{
					// This is a 9-slice key - the color indicates the region type
					// For now, we'll use a simplified approach and extract from userData if available
					
					const TSharedPtr<FJsonObject>* UserDataObject;
					if ((*KeyObject)->TryGetObjectField(TEXT("userData"), UserDataObject) && UserDataObject->IsValid())
					{
						// Check for explicit margin data
						int32 Left = 0, Top = 0, Right = 0, Bottom = 0;
						bool bHasMargins = 
							(*UserDataObject)->TryGetNumberField(TEXT("left"), Left) &&
							(*UserDataObject)->TryGetNumberField(TEXT("top"), Top) &&
							(*UserDataObject)->TryGetNumberField(TEXT("right"), Right) &&
							(*UserDataObject)->TryGetNumberField(TEXT("bottom"), Bottom);

						if (bHasMargins)
						{
							OutMargins.Left = static_cast<float>(Left);
							OutMargins.Top = static_cast<float>(Top);
							OutMargins.Right = static_cast<float>(Right);
							OutMargins.Bottom = static_cast<float>(Bottom);
							return true;
						}
					}
				}
			}
		}
	}

	// Alternative: Aseprite may store 9-slice in a different format
	// Check for "nineSlice" or similar custom fields
	for (const TSharedPtr<FJsonValue>& KeyValue : KeyArray)
	{
		const TSharedPtr<FJsonObject>* KeyObject = KeyValue->AsObject();
		if (!KeyObject || !KeyObject->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* NineSliceObject;
		if ((*KeyObject)->TryGetObjectField(TEXT("nineSlice"), NineSliceObject) && NineSliceObject->IsValid())
		{
			double Left, Top, Right, Bottom;
			if ((*NineSliceObject)->TryGetNumberField(TEXT("left"), Left) &&
				(*NineSliceObject)->TryGetNumberField(TEXT("top"), Top) &&
				(*NineSliceObject)->TryGetNumberField(TEXT("right"), Right) &&
				(*NineSliceObject)->TryGetNumberField(TEXT("bottom"), Bottom))
			{
				OutMargins.Left = static_cast<float>(Left);
				OutMargins.Top = static_cast<float>(Top);
				OutMargins.Right = static_cast<float>(Right);
				OutMargins.Bottom = static_cast<float>(Bottom);
				return true;
			}
		}
	}

	return false;
}

void UPixelComponentAsset::UpdateTextureDimensionsCache()
{
	if (SourceTexture && SourceTexture->GetResource())
	{
		const int32 SurfaceWidth = SourceTexture->GetSurfaceWidth();
		const int32 SurfaceHeight = SourceTexture->GetSurfaceHeight();

		// Store original dimensions
		OriginalTextureWidth = SurfaceWidth;
		OriginalTextureHeight = SurfaceHeight;

		// Apply global pixel size scaling
		UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
		if (Settings && Settings->bEnableAutoScaleOnImport)
		{
			// Calculate scale factor based on original dimensions
			AppliedScaleFactor = UPixelComponentSettings::CalculateScaleFactor(FMath::Max(SurfaceWidth, SurfaceHeight));
			bIsScaled = !FMath::IsNearlyEqual(AppliedScaleFactor, 1.0f);

			CachedTextureWidth = FMath::RoundToInt(static_cast<float>(SurfaceWidth) * AppliedScaleFactor);
			CachedTextureHeight = FMath::RoundToInt(static_cast<float>(SurfaceHeight) * AppliedScaleFactor);

			if (Settings->bSnapToPixelGrid)
			{
				const int32 PixelSize = Settings->GlobalPixelSize;
				CachedTextureWidth = FMath::RoundToInt(static_cast<float>(CachedTextureWidth) / PixelSize) * PixelSize;
				CachedTextureHeight = FMath::RoundToInt(static_cast<float>(CachedTextureHeight) / PixelSize) * PixelSize;
			}
		}
		else
		{
			CachedTextureWidth = SurfaceWidth;
			CachedTextureHeight = SurfaceHeight;
			AppliedScaleFactor = 1.0f;
			bIsScaled = false;
		}
	}
	else
	{
		// Try to get dimensions from import settings or default to 0
		CachedTextureWidth = 0;
		CachedTextureHeight = 0;
		OriginalTextureWidth = 0;
		OriginalTextureHeight = 0;
		AppliedScaleFactor = 1.0f;
		bIsScaled = false;
	}
}
