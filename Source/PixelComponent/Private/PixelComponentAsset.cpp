// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentAsset.h"
#include "Engine/Texture2D.h"
#include "PixelComponentSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentAsset, Log, All);

UPixelComponentAsset::UPixelComponentAsset()
	: SourceMode(EPixelSourceMode::Texture)
	, SourceTexture(nullptr)
	, SourceMaterial(nullptr)
	, NineSliceMargins(0.0f)
	, DefaultPivot(0.0f, 0.0f, true)
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

UMaterialInterface* UPixelComponentAsset::GetActiveMaterial() const
{
	if (SourceMode == EPixelSourceMode::Material)
	{
		return SourceMaterial;
	}

	return nullptr;
}

void UPixelComponentAsset::SetSourceMaterial(UMaterialInterface* NewMaterial)
{
	SourceMaterial = NewMaterial;
	SourceMode = EPixelSourceMode::Material;
	RefreshNormalizedUVs();
}

void UPixelComponentAsset::SetSourceMode(EPixelSourceMode NewMode)
{
	if (SourceMode != NewMode)
	{
		SourceMode = NewMode;

		if (NewMode == EPixelSourceMode::Texture)
		{
			SourceMaterial = nullptr;
		}
		else
		{
			SourceTexture = nullptr;
		}

		RefreshNormalizedUVs();
	}
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

FSliceData UPixelComponentAsset::GetSliceByName(const FString& SliceName, bool& bFound) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice)
	{
		bFound = true;
		return *Slice;
	}
	bFound = false;
	return FSliceData();
}

TArray<FString> UPixelComponentAsset::GetAllSliceNames() const
{
	TArray<FString> Names;
	Names.Reserve(Slices.Num());

	for (const FSliceData& Slice : Slices)
	{
		Names.Add(Slice.Name);
	}

	return Names;
}

TArray<FString> UPixelComponentAsset::GetAllLayerParameterNames() const
{
	TArray<FString> Result;
	for (const auto& Pair : LayerToMaterialParamMap)
	{
		Result.Add(Pair.Key + TEXT(":") + Pair.Value.ToString());
	}
	return Result;
}

FName UPixelComponentAsset::GetMaterialParameterForLayer(const FString& LayerName) const
{
	const FName* FoundParam = LayerToMaterialParamMap.Find(LayerName);
	return FoundParam ? *FoundParam : NAME_None;
}

void UPixelComponentAsset::SetLayerMaterialMapping(const FString& LayerName, const FName& ParamName)
{
	if (!LayerName.IsEmpty())
	{
		LayerToMaterialParamMap.Add(LayerName, ParamName);
	}
}

TArray<FString> UPixelComponentAsset::GetAllPaletteProfileNames() const
{
	TArray<FString> Names;
	PaletteProfiles.GetKeys(Names);
	return Names;
}

FPixelPaletteProfile UPixelComponentAsset::GetPaletteProfile(const FString& ProfileName, bool& bFound) const
{
	const FPixelPaletteProfile* FoundProfile = PaletteProfiles.Find(ProfileName);
	if (FoundProfile)
	{
		bFound = true;
		return *FoundProfile;
	}
	bFound = false;
	return FPixelPaletteProfile();
}

void UPixelComponentAsset::SetPaletteProfile(const FString& ProfileName, const FPixelPaletteProfile& Profile)
{
	if (!ProfileName.IsEmpty())
	{
		FPixelPaletteProfile NewProfile = Profile;
		NewProfile.ProfileName = ProfileName;
		PaletteProfiles.Add(ProfileName, NewProfile);
	}
}

bool UPixelComponentAsset::RemovePaletteProfile(const FString& ProfileName)
{
	if (PaletteProfiles.Contains(ProfileName))
	{
		PaletteProfiles.Remove(ProfileName);
		return true;
	}
	return false;
}

FLinearColor UPixelComponentAsset::GetColorOverrideFromProfile(const FString& ProfileName, const FName& LayerName, bool& bFound) const
{
	const FPixelPaletteProfile* FoundProfile = PaletteProfiles.Find(ProfileName);
	if (FoundProfile)
	{
		return FoundProfile->GetColorOverride(LayerName, bFound);
	}
	bFound = false;
	return FLinearColor::White;
}

FPixelNineSliceMargins UPixelComponentAsset::GetSliceNineSliceMarginsUV(const FString& SliceName) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice && Slice->bIsNineSlice)
	{
		return Slice->NineSliceMarginsUV;
	}
	return FPixelNineSliceMargins();
}

void UPixelComponentAsset::GetTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
	OutWidth = CachedTextureWidth;
	OutHeight = CachedTextureHeight;
}

void UPixelComponentAsset::GetEffectiveTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
	OutWidth = CachedTextureWidth;
	OutHeight = CachedTextureHeight;
}

void UPixelComponentAsset::GetOriginalTextureDimensions(int32& OutWidth, int32& OutHeight) const
{
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

FVector2f UPixelComponentAsset::GetTextureSize() const
{
	return FVector2f(static_cast<float>(CachedTextureWidth), static_cast<float>(CachedTextureHeight));
}

int32 UPixelComponentAsset::GetGlobalPixelSize()
{
	return UPixelComponentSettings::GetGlobalPixelSize();
}

FPixelUVRect UPixelComponentAsset::ComputeNormalizedUVs(const FPixelRect& PixelRect) const
{
	int32 TexWidth = CachedTextureWidth;
	int32 TexHeight = CachedTextureHeight;

	if (SourceTexture)
	{
		TexWidth = SourceTexture->GetSurfaceWidth();
		TexHeight = SourceTexture->GetSurfaceHeight();
	}

	return FPixelUVRect::FromPixelRect(PixelRect.X, PixelRect.Y, PixelRect.Width, PixelRect.Height,
		TexWidth, TexHeight);
}

FPixelUVRect UPixelComponentAsset::GetSliceNormalizedUVRect(const FString& SliceName) const
{
	if (SliceName.IsEmpty() || Slices.Num() == 0)
	{
		return FPixelUVRect(0.0f, 0.0f, 1.0f, 1.0f);
	}

	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice)
	{
		return Slice->NormalizedUVRect;
	}

	UE_LOG(LogPixelComponentAsset, Warning, TEXT("Slice '%s' not found, returning full UVs"), *SliceName);
	return FPixelUVRect(0.0f, 0.0f, 1.0f, 1.0f);
}

FPixelUVRect UPixelComponentAsset::GetNineSliceCenterUVRect(const FString& SliceName) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice && Slice->bIsNineSlice)
	{
		return Slice->GetCenterUV();
	}
	return FPixelUVRect();
}

TArray<FVector4f> UPixelComponentAsset::GetAllSliceUVsAsVector4() const
{
	TArray<FVector4f> UVs;
	UVs.Reserve(Slices.Num());

	for (const FSliceData& Slice : Slices)
	{
		if (Slice.NormalizedUVRect.IsValid())
		{
			UVs.Add(FVector4f(
				Slice.NormalizedUVRect.MinX,
				Slice.NormalizedUVRect.MinY,
				Slice.NormalizedUVRect.MaxX,
				Slice.NormalizedUVRect.MaxY
			));
		}
	}

	return UVs;
}

FPixelAnimSequence UPixelComponentAsset::GetAnimationSequenceByName(const FString& SequenceName, bool& bFound) const
{
	const FPixelAnimSequence* Seq = AnimationSequences.Find(SequenceName);
	if (Seq)
	{
		bFound = true;
		return *Seq;
	}
	bFound = false;
	return FPixelAnimSequence();
}

TArray<FString> UPixelComponentAsset::GetAllAnimationSequenceNames() const
{
	TArray<FString> Names;
	AnimationSequences.GetKeys(Names);
	return Names;
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

FPixelPivot UPixelComponentAsset::GetSlicePivot(const FString& SliceName) const
{
	const FSliceData* Slice = FindSliceByName(SliceName);
	if (Slice)
	{
		return Slice->Pivot;
	}
	return DefaultPivot;
}

void UPixelComponentAsset::RefreshNormalizedUVs()
{
	UpdateTextureDimensionsCache();

	int32 TexWidth = CachedTextureWidth;
	int32 TexHeight = CachedTextureHeight;

	if (SourceTexture)
	{
		TexWidth = SourceTexture->GetSurfaceWidth();
		TexHeight = SourceTexture->GetSurfaceHeight();
	}

	if (TexWidth <= 0 || TexHeight <= 0)
	{
		UE_LOG(LogPixelComponentAsset, Warning, TEXT("Cannot compute UVs: invalid texture dimensions %dx%d"),
			TexWidth, TexHeight);
		bUVsComputed = false;
		return;
	}

	for (FSliceData& Slice : Slices)
	{
		Slice.ComputeNormalizedUVs(TexWidth, TexHeight);
	}

	for (FFrameData& Frame : Frames)
	{
		Frame.ComputeNormalizedUVs(TexWidth, TexHeight);
	}

	bUVsComputed = true;

	if (!ValidateUVs())
	{
		UE_LOG(LogPixelComponentAsset, Error, TEXT("UV validation failed for asset %s"), *GetPathName());
	}

	UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Refreshed UVs for %d slices, %d frames (effective size: %dx%d)"),
		Slices.Num(), Frames.Num(), TexWidth, TexHeight);
}

bool UPixelComponentAsset::ValidateAsset() const
{
	bool bValid = true;

	if (!SourceTexture)
	{
		UE_LOG(LogPixelComponentAsset, Warning, TEXT("Asset %s has no source texture"), *GetPathName());
	}

	if (!ValidateUVs())
	{
		bValid = false;
	}

	for (const FSliceData& Slice : Slices)
	{
		if (!Slice.Validate(CachedTextureWidth, CachedTextureHeight))
		{
			UE_LOG(LogPixelComponentAsset, Error, TEXT("Invalid slice: %s"), *Slice.Name);
			bValid = false;
		}
	}

	for (const auto& SeqPair : AnimationSequences)
	{
		if (!SeqPair.Value.Validate(Frames.Num() - 1))
		{
			UE_LOG(LogPixelComponentAsset, Error, TEXT("Invalid animation sequence: %s"), *SeqPair.Key);
			bValid = false;
		}
	}

	return bValid;
}

bool UPixelComponentAsset::ValidateUVs() const
{
	bool bAllValid = true;

	for (const FSliceData& Slice : Slices)
	{
		if (!Slice.NormalizedUVRect.IsValid())
		{
			UE_LOG(LogPixelComponentAsset, Error,
				TEXT("Slice '%s' has invalid UVs: (%.4f, %.4f, %.4f, %.4f)"),
				*Slice.Name,
				Slice.NormalizedUVRect.MinX, Slice.NormalizedUVRect.MinY,
				Slice.NormalizedUVRect.MaxX, Slice.NormalizedUVRect.MaxY);
			bAllValid = false;
		}
	}

	return bAllValid;
}

FAsepriteParseResult UPixelComponentAsset::ParseMetadataFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FAsepriteParseResult Result;

	if (!JsonObject.IsValid())
	{
		return FAsepriteParseResult::Error(TEXT("Invalid JSON object for metadata parsing"));
	}

	const TSharedPtr<FJsonObject>* MetaObject;
	if (JsonObject->TryGetObjectField(TEXT("meta"), MetaObject) && MetaObject)
	{
		FString ImagePath;
		if ((*MetaObject)->TryGetStringField(TEXT("image"), ImagePath))
		{
			AssetName = FPaths::GetBaseFilename(ImagePath);
		}
		else
		{
			AssetName = TEXT("UnnamedPixelArt");
		}

		const TSharedPtr<FJsonObject>* UserDataObject;
		if ((*MetaObject)->TryGetObjectField(TEXT("userData"), UserDataObject) && UserDataObject)
		{
			float PivotX = 0.5f, PivotY = 0.5f;
			(*UserDataObject)->TryGetNumberField(TEXT("pivotX"), PivotX);
			(*UserDataObject)->TryGetNumberField(TEXT("pivotY"), PivotY);

			DefaultPivot = FPixelPivot(PivotX, PivotY, false);
		}
	}

	return Result;
}

FAsepriteParseResult UPixelComponentAsset::ParseSlicesFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FAsepriteParseResult Result;

	if (!JsonObject.IsValid())
	{
		return FAsepriteParseResult::Error(TEXT("Invalid JSON object for slice parsing"));
	}

	const TArray<TSharedPtr<FJsonValue>>* SlicesArray;
	if (!JsonObject->TryGetArrayField(TEXT("slices"), SlicesArray))
	{
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("No slices found in JSON"));
		return Result;
	}

	Slices.Empty(SlicesArray->Num());

	for (const TSharedPtr<FJsonValue>& SliceValue : *SlicesArray)
	{
		const TSharedPtr<FJsonObject> SliceObject = SliceValue->AsObject();
		if (!SliceObject)
		{
			Result.AddWarning(TEXT("Invalid slice object in JSON"));
			continue;
		}

		FSliceData Slice;

		FString SliceName;
		if (SliceObject->TryGetStringField(TEXT("name"), SliceName))
		{
			Slice.Name = SliceName;
		}
		else
		{
			Result.AddWarning(FString::Printf(TEXT("Slice missing name field")));
			continue;
		}

		const TSharedPtr<FJsonObject>* BoundsObject;
		if (SliceObject->TryGetObjectField(TEXT("bounds"), BoundsObject) && BoundsObject)
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

		FString ColorStr;
		if (SliceObject->TryGetStringField(TEXT("color"), ColorStr))
		{
			if (ColorStr == TEXT("#ff0000") || ColorStr == TEXT("red"))
			{
				Slice.SliceColor = FColor::Red;
			}
			else if (ColorStr == TEXT("#00ff00") || ColorStr == TEXT("green"))
			{
				Slice.SliceColor = FColor::Green;
			}
			else if (ColorStr == TEXT("#0000ff") || ColorStr == TEXT("blue"))
			{
				Slice.SliceColor = FColor::Blue;
			}
			else
			{
				Slice.SliceColor = FColor::White;
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* KeysArray;
		if (SliceObject->TryGetArrayField(TEXT("keys"), KeysArray) && KeysArray->Num() > 0)
		{
			const TSharedPtr<FJsonObject> FirstKey = (*KeysArray)[0]->AsObject();
			if (FirstKey && FirstKey)
			{
				const TSharedPtr<FJsonObject>* KeyBoundsObject;
				if (FirstKey->TryGetObjectField(TEXT("bounds"), KeyBoundsObject) && KeyBoundsObject)
				{
					Slice.bIsNineSlice = true;

					int32 KeyX, KeyY, KeyW, KeyH;
					if ((*KeyBoundsObject)->TryGetNumberField(TEXT("x"), KeyX) &&
						(*KeyBoundsObject)->TryGetNumberField(TEXT("y"), KeyY) &&
						(*KeyBoundsObject)->TryGetNumberField(TEXT("w"), KeyW) &&
						(*KeyBoundsObject)->TryGetNumberField(TEXT("h"), KeyH))
					{
						const int32 LeftMargin = KeyX - Slice.PixelRect.X;
						const int32 TopMargin = KeyY - Slice.PixelRect.Y;
						const int32 RightMargin = (Slice.PixelRect.X + Slice.PixelRect.Width) - (KeyX + KeyW);
						const int32 BottomMargin = (Slice.PixelRect.Y + Slice.PixelRect.Height) - (KeyY + KeyH);

						Slice.NineSliceMarginsUV = FPixelNineSliceMargins::FromPixelMargins(
							static_cast<float>(LeftMargin),
							static_cast<float>(TopMargin),
							static_cast<float>(RightMargin),
							static_cast<float>(BottomMargin),
							Slice.PixelRect.Width,
							Slice.PixelRect.Height
						);

						NineSliceMargins = FMargin(
							static_cast<float>(LeftMargin),
							static_cast<float>(TopMargin),
							static_cast<float>(RightMargin),
							static_cast<float>(BottomMargin)
						);

						UE_LOG(LogPixelComponentAsset, Verbose,
							TEXT("Parsed 9-slice for '%s': L=%d, T=%d, R=%d, B=%d"),
							*Slice.Name, LeftMargin, TopMargin, RightMargin, BottomMargin);
					}
				}
			}
		}

		Slices.Add(Slice);
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed slice: %s (%dx%d @ %d,%d)"),
			*Slice.Name, Slice.PixelRect.Width, Slice.PixelRect.Height, Slice.PixelRect.X, Slice.PixelRect.Y);
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

	const TArray<TSharedPtr<FJsonValue>>* LayersArray;
	if (!JsonObject->TryGetArrayField(TEXT("layers"), LayersArray))
	{
		UE_LOG(LogPixelComponentAsset, Verbose, TEXT("No layers found in JSON"));
		return Result;
	}

	Layers.Empty(LayersArray->Num());

	for (int32 LayerIdx = 0; LayerIdx < LayersArray->Num(); ++LayerIdx)
	{
		const TSharedPtr<FJsonValue>& LayerValue = (*LayersArray)[LayerIdx];
		const TSharedPtr<FJsonObject> LayerObject = LayerValue->AsObject();
		if (!LayerObject)
		{
			Result.AddWarning(TEXT("Invalid layer object in JSON"));
			continue;
		}

		FLayerMetadata Layer;
		Layer.LayerIndex = LayerIdx;

		FString LayerName;
		if (LayerObject->TryGetStringField(TEXT("name"), LayerName))
		{
			Layer.Name = LayerName;
		}

		bool bVisible = true;
		if (LayerObject->TryGetBoolField(TEXT("show"), bVisible))
		{
			Layer.bVisible = bVisible;
		}

		int32 OpacityInt = 255;
		if (LayerObject->TryGetNumberField(TEXT("opacity"), OpacityInt))
		{
			Layer.Opacity = FMath::Clamp(OpacityInt / 255.0f, 0.0f, 1.0f);
		}

		FString BlendMode;
		if (LayerObject->TryGetStringField(TEXT("blendMode"), BlendMode))
		{
			Layer.BlendMode = BlendMode;
		}

		FString ParentName;
		if (LayerObject->TryGetStringField(TEXT("parent"), ParentName))
		{
			Layer.ParentLayerName = ParentName;
		}

		const TSharedPtr<FJsonObject>* UserDataObject;
		if (LayerObject->TryGetObjectField(TEXT("userData"), UserDataObject) && UserDataObject)
		{
			FString UserDataColorStr;
			if ((*UserDataObject)->TryGetStringField(TEXT("color"), UserDataColorStr))
			{
				if (UserDataColorStr.StartsWith(TEXT("#")))
				{
					UserDataColorStr = UserDataColorStr.RightChop(1);
					if (UserDataColorStr.Len() >= 6)
					{
						uint32 ColorValue = 0;
						for (int32 i = 0; i < 6 && i < UserDataColorStr.Len(); i++)
						{
							TCHAR c = UserDataColorStr[i];
							ColorValue <<= 4;
							if (c >= TEXT('0') && c <= TEXT('9'))
								ColorValue |= (c - TEXT('0'));
							else if (c >= TEXT('a') && c <= TEXT('f'))
								ColorValue |= (c - TEXT('a') + 10);
							else if (c >= TEXT('A') && c <= TEXT('F'))
								ColorValue |= (c - TEXT('A') + 10);
						}
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

	const TArray<TSharedPtr<FJsonValue>>* TagsArray;
	if (JsonObject->TryGetArrayField(TEXT("frameTags"), TagsArray))
	{
		AnimationSequences.Empty(TagsArray->Num());

		for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
		{
			const TSharedPtr<FJsonObject> TagObject = TagValue->AsObject();
			if (!TagObject)
			{
				continue;
			}

			FPixelAnimSequence Sequence;

			FString TagName;
			if (TagObject->TryGetStringField(TEXT("name"), TagName))
			{
				Sequence.SequenceName = *TagName;
			}
			else
			{
				Result.AddWarning(TEXT("Animation tag missing name"));
				continue;
			}

			int32 FromFrame = 0;
			int32 ToFrame = 0;
			if (TagObject->TryGetNumberField(TEXT("from"), FromFrame))
			{
				Sequence.StartFrame = FromFrame;
			}
			if (TagObject->TryGetNumberField(TEXT("to"), ToFrame))
			{
				Sequence.EndFrame = ToFrame;
			}

			FString DirectionStr;
			if (TagObject->TryGetStringField(TEXT("direction"), DirectionStr))
			{
				if (DirectionStr == TEXT("reverse"))
				{
					Sequence.Direction = EPixelAnimDirection::Reverse;
				}
				else if (DirectionStr == TEXT("pingpong"))
				{
					Sequence.Direction = EPixelAnimDirection::PingPong;
				}
				else if (DirectionStr == TEXT("pingpong_reverse"))
				{
					Sequence.Direction = EPixelAnimDirection::PingPongReverse;
				}
				else
				{
					Sequence.Direction = EPixelAnimDirection::Forward;
				}
			}

			int32 Repeat = 0;
			if (TagObject->TryGetNumberField(TEXT("repeat"), Repeat))
			{
				Sequence.LoopCount = Repeat;
			}

			Sequence.CalculateDuration();

			if (!Sequence.Validate(Frames.Num() - 1))
			{
				Result.AddWarning(FString::Printf(TEXT("Animation sequence '%s' has invalid frame range"), *TagName));
			}

			AnimationSequences.Add(TagName, Sequence);
			UE_LOG(LogPixelComponentAsset, Verbose, TEXT("Parsed animation: %s (%d-%d, %d loops)"),
				*TagName, Sequence.StartFrame, Sequence.EndFrame, Sequence.LoopCount);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* FramesArray;
	if (JsonObject->TryGetArrayField(TEXT("frames"), FramesArray))
	{
		Frames.Empty(FramesArray->Num());

		for (int32 FrameIndex = 0; FrameIndex < FramesArray->Num(); ++FrameIndex)
		{
			const TSharedPtr<FJsonValue>& FrameValue = (*FramesArray)[FrameIndex];

			const TSharedPtr<FJsonObject> FrameObject = FrameValue->AsObject();
			if (!FrameObject)
			{
				continue;
			}

			FFrameData Frame;
			Frame.FrameIndex = FrameIndex;

			int32 DurationMs = 100;
			if (FrameObject->TryGetNumberField(TEXT("duration"), DurationMs))
			{
				Frame.DurationMs = DurationMs;
			}
			else
			{
				const TSharedPtr<FJsonObject>* DurationObject;
				if (FrameObject->TryGetObjectField(TEXT("duration"), DurationObject))
				{
					(*DurationObject)->TryGetNumberField(TEXT("value"), DurationMs);
					Frame.DurationMs = DurationMs;
				}
			}

			const TSharedPtr<FJsonObject>* FrameRectObject;
			if (FrameObject->TryGetObjectField(TEXT("frame"), FrameRectObject) && FrameRectObject)
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

	if (JsonObject->TryGetArrayField(TEXT("tags"), TagsArray))
	{
		AnimationTags.Empty(TagsArray->Num());

		for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
		{
			const TSharedPtr<FJsonObject> TagObject = TagValue->AsObject();
			if (!TagObject)
			{
				continue;
			}

			FAnimationTag Tag;
			TagObject->TryGetStringField(TEXT("name"), Tag.Name);

			int32 FromFrame = 0;
			int32 ToFrame = 0;
			TagObject->TryGetNumberField(TEXT("from"), FromFrame);
			TagObject->TryGetNumberField(TEXT("to"), ToFrame);
			Tag.FromFrame = FromFrame;
			Tag.ToFrame = ToFrame;

			TagObject->TryGetStringField(TEXT("direction"), Tag.Direction);
			TagObject->TryGetNumberField(TEXT("repeat"), Tag.Repeat);

			AnimationTags.Add(Tag);
		}
	}

	return Result;
}

void UPixelComponentAsset::UpdateTextureDimensionsCache()
{
	if (SourceTexture && SourceTexture->GetResource())
	{
		const int32 SurfaceWidth = SourceTexture->GetSurfaceWidth();
		const int32 SurfaceHeight = SourceTexture->GetSurfaceHeight();

		OriginalTextureWidth = SurfaceWidth;
		OriginalTextureHeight = SurfaceHeight;

		UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
		if (Settings && Settings->bEnableAutoScaleOnImport)
		{
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
		CachedTextureWidth = 0;
		CachedTextureHeight = 0;
		OriginalTextureWidth = 0;
		OriginalTextureHeight = 0;
		AppliedScaleFactor = 1.0f;
		bIsScaled = false;
	}
}

void UPixelComponentAsset::SetTextureDimensionsFromImport(int32 Width, int32 Height, bool bApplyScaling)
{
	OriginalTextureWidth = Width;
	OriginalTextureHeight = Height;

	if (bApplyScaling)
	{
		UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
		if (Settings && Settings->bEnableAutoScaleOnImport)
		{
			AppliedScaleFactor = UPixelComponentSettings::CalculateScaleFactor(FMath::Max(Width, Height));
			bIsScaled = !FMath::IsNearlyEqual(AppliedScaleFactor, 1.0f);

			CachedTextureWidth = FMath::RoundToInt(static_cast<float>(Width) * AppliedScaleFactor);
			CachedTextureHeight = FMath::RoundToInt(static_cast<float>(Height) * AppliedScaleFactor);
		}
		else
		{
			CachedTextureWidth = Width;
			CachedTextureHeight = Height;
			AppliedScaleFactor = 1.0f;
			bIsScaled = false;
		}
	}
	else
	{
		CachedTextureWidth = Width;
		CachedTextureHeight = Height;
		AppliedScaleFactor = 1.0f;
		bIsScaled = false;
	}
}

void UPixelComponentAsset::SetScalingFromImport(float InScaleFactor, bool InbIsScaled)
{
	AppliedScaleFactor = InScaleFactor;
	bIsScaled = InbIsScaled;
}

void UPixelComponentAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponentAsset, SourceMode))
	{
		if (SourceMode == EPixelSourceMode::Texture)
		{
			SourceMaterial = nullptr;
		}
		else
		{
			SourceTexture = nullptr;
		}

		RefreshNormalizedUVs();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponentAsset, SourceTexture))
	{
		UpdateTextureDimensionsCache();
		RefreshNormalizedUVs();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UPixelComponentAsset, SourceMaterial))
	{
		RefreshNormalizedUVs();
	}
}
