// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentTypes.h"

// =============================================================================
// FPixelUVRect Implementation
// =============================================================================

FPixelUVRect::FPixelUVRect()
	: MinX(0.0f)
	, MinY(0.0f)
	, MaxX(0.0f)
	, MaxY(0.0f)
{
}

FPixelUVRect::FPixelUVRect(float InMinX, float InMinY, float InMaxX, float InMaxY)
	: MinX(InMinX)
	, MinY(InMinY)
	, MaxX(InMaxX)
	, MaxY(InMaxY)
{
	Validate();
}

FPixelUVRect FPixelUVRect::FromPixelRect(int32 PixelX, int32 PixelY, int32 PixelW, int32 PixelH,
	int32 TextureWidth, int32 TextureHeight)
{
	if (TextureWidth <= 0 || TextureHeight <= 0)
	{
		return FPixelUVRect();
	}

	const float InvTexW = 1.0f / static_cast<float>(TextureWidth);
	const float InvTexH = 1.0f / static_cast<float>(TextureHeight);

	const float MinXVal = static_cast<float>(PixelX) * InvTexW;
	const float MinYVal = 1.0f - static_cast<float>(PixelY + PixelH) * InvTexH;
	const float MaxXVal = static_cast<float>(PixelX + PixelW) * InvTexW;
	const float MaxYVal = 1.0f - static_cast<float>(PixelY) * InvTexH;

	return FPixelUVRect(MinXVal, MinYVal, MaxXVal, MaxYVal);
}

bool FPixelUVRect::Validate() const
{
	bool bWasValid = true;
	float TempMinX = MinX, TempMinY = MinY, TempMaxX = MaxX, TempMaxY = MaxY;

	if (TempMinX < 0.0f || TempMinX > 1.0f) { TempMinX = FMath::Clamp(TempMinX, 0.0f, 1.0f); bWasValid = false; }
	if (TempMinY < 0.0f || TempMinY > 1.0f) { TempMinY = FMath::Clamp(TempMinY, 0.0f, 1.0f); bWasValid = false; }
	if (TempMaxX < 0.0f || TempMaxX > 1.0f) { TempMaxX = FMath::Clamp(TempMaxX, 0.0f, 1.0f); bWasValid = false; }
	if (TempMaxY < 0.0f || TempMaxY > 1.0f) { TempMaxY = FMath::Clamp(TempMaxY, 0.0f, 1.0f); bWasValid = false; }

	if (TempMinX > TempMaxX) { float Temp = TempMinX; TempMinX = TempMaxX; TempMaxX = Temp; bWasValid = false; }
	if (TempMinY > TempMaxY) { float Temp = TempMinY; TempMinY = TempMaxY; TempMaxY = Temp; bWasValid = false; }

	return bWasValid;
}

float FPixelUVRect::GetWidth() const
{
	return MaxX - MinX;
}

float FPixelUVRect::GetHeight() const
{
	return MaxY - MinY;
}

FVector2f FPixelUVRect::GetCenter() const
{
	return FVector2f((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
}

bool FPixelUVRect::IsValid() const
{
	return (GetWidth() > 0.0f) && (GetHeight() > 0.0f) &&
		   (MinX >= 0.0f && MinX <= 1.0f) &&
		   (MinY >= 0.0f && MinY <= 1.0f) &&
		   (MaxX >= 0.0f && MaxX <= 1.0f) &&
		   (MaxY >= 0.0f && MaxY <= 1.0f);
}

FBox2f FPixelUVRect::ToBox2f() const
{
	return FBox2f(FVector2f(MinX, MinY), FVector2f(MaxX, MaxY));
}

bool FPixelUVRect::operator==(const FPixelUVRect& Other) const
{
	const float Tolerance = 0.0001f;
	return FMath::Abs(MinX - Other.MinX) < Tolerance &&
		   FMath::Abs(MinY - Other.MinY) < Tolerance &&
		   FMath::Abs(MaxX - Other.MaxX) < Tolerance &&
		   FMath::Abs(MaxY - Other.MaxY) < Tolerance;
}

// =============================================================================
// FPixelNineSliceMargins Implementation
// =============================================================================

FPixelNineSliceMargins::FPixelNineSliceMargins()
	: Left(0.0f), Top(0.0f), Right(0.0f), Bottom(0.0f)
{
}

FPixelNineSliceMargins::FPixelNineSliceMargins(float InLeft, float InTop, float InRight, float InBottom)
	: Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom)
{
}

FPixelNineSliceMargins FPixelNineSliceMargins::FromPixelMargins(float PixelLeft, float PixelTop, float PixelRight, float PixelBottom,
	int32 TextureWidth, int32 TextureHeight)
{
	if (TextureWidth <= 0 || TextureHeight <= 0)
	{
		return FPixelNineSliceMargins();
	}

	return FPixelNineSliceMargins(
		PixelLeft / static_cast<float>(TextureWidth),
		PixelTop / static_cast<float>(TextureHeight),
		PixelRight / static_cast<float>(TextureWidth),
		PixelBottom / static_cast<float>(TextureHeight)
	);
}

FPixelUVRect FPixelNineSliceMargins::GetCenterUV(const FPixelUVRect& OuterUV) const
{
	return FPixelUVRect(
		OuterUV.MinX + Left,
		OuterUV.MinY + Bottom,
		OuterUV.MaxX - Right,
		OuterUV.MaxY - Top
	);
}

bool FPixelNineSliceMargins::IsValid() const
{
	return Left >= 0.0f && Top >= 0.0f && Right >= 0.0f && Bottom >= 0.0f &&
		   (Left + Right) <= 1.0f && (Top + Bottom) <= 1.0f;
}

// =============================================================================
// FPixelRect Implementation
// =============================================================================

FPixelRect::FPixelRect()
	: X(0), Y(0), Width(0), Height(0)
{
}

FPixelRect::FPixelRect(int32 InX, int32 InY, int32 InWidth, int32 InHeight)
	: X(InX), Y(InY), Width(InWidth), Height(InHeight)
{
}

FPixelUVRect FPixelRect::ToNormalizedUV(int32 TextureWidth, int32 TextureHeight) const
{
	return FPixelUVRect::FromPixelRect(X, Y, Width, Height, TextureWidth, TextureHeight);
}

bool FPixelRect::IsValid() const
{
	return Width > 0 && Height > 0;
}

// =============================================================================
// FPixelPivot Implementation
// =============================================================================

FPixelPivot::FPixelPivot()
	: X(0.0f), Y(0.0f), bIsCenter(true)
{
}

FPixelPivot::FPixelPivot(float InX, float InY, bool bInIsCenter)
	: X(InX), Y(InY), bIsCenter(bInIsCenter)
{
}

FPixelPivot FPixelPivot::FromPercentage(float PercentX, float PercentY, int32 SliceWidth, int32 SliceHeight)
{
	return FPixelPivot(
		PercentX * static_cast<float>(SliceWidth),
		PercentY * static_cast<float>(SliceHeight),
		false
	);
}

FVector2f FPixelPivot::GetNormalized(int32 SliceWidth, int32 SliceHeight) const
{
	if (SliceWidth <= 0 || SliceHeight <= 0)
	{
		return FVector2f(0.5f, 0.5f);
	}

	return FVector2f(
		X / static_cast<float>(SliceWidth),
		Y / static_cast<float>(SliceHeight)
	);
}

// =============================================================================
// FSliceData Implementation
// =============================================================================

FSliceData::FSliceData()
	: bIsNineSlice(false)
	, SliceColor(FColor::White)
{
}

void FSliceData::ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight)
{
	NormalizedUVRect = FPixelUVRect::FromPixelRect(
		PixelRect.X, PixelRect.Y, PixelRect.Width, PixelRect.Height,
		TextureWidth, TextureHeight
	);

	NormalizedUVs = NormalizedUVRect.ToBox2f();
}

FPixelUVRect FSliceData::GetCenterUV() const
{
	if (!bIsNineSlice || !NormalizedUVRect.IsValid())
	{
		return NormalizedUVRect;
	}

	return NineSliceMarginsUV.GetCenterUV(NormalizedUVRect);
}

bool FSliceData::Validate(int32 MaxTextureWidth, int32 MaxTextureHeight) const
{
	bool bValid = true;

	if (!PixelRect.IsValid())
	{
		bValid = false;
	}

	if (!NormalizedUVRect.Validate())
	{
		bValid = false;
	}

	if (bIsNineSlice && !NineSliceMarginsUV.IsValid())
	{
		bValid = false;
	}

	return bValid;
}

// =============================================================================
// FLayerMetadata Implementation
// =============================================================================

FLayerMetadata::FLayerMetadata()
	: bVisible(true)
	, Opacity(1.0f)
	, UserDataColor(FColor::White)
	, LayerIndex(0)
{
}

// =============================================================================
// FPixelAnimSequence Implementation
// =============================================================================

FPixelAnimSequence::FPixelAnimSequence()
	: SequenceName(NAME_None)
	, StartFrame(0)
	, EndFrame(0)
	, FrameDurationMs(100)
	, Direction(EPixelAnimDirection::Forward)
	, LoopCount(0)
	, TotalDurationMs(0)
{
}

void FPixelAnimSequence::CalculateDuration()
{
	const int32 FrameCount = FMath::Max(1, EndFrame - StartFrame + 1);
	TotalDurationMs = FrameCount * FrameDurationMs;
}

int32 FPixelAnimSequence::GetFrameCount() const
{
	return FMath::Max(0, EndFrame - StartFrame + 1);
}

bool FPixelAnimSequence::Validate(int32 MaxFrameIndex) const
{
	if (StartFrame < 0) return false;
	if (EndFrame > MaxFrameIndex) return false;
	if (StartFrame > EndFrame) return false;
	return true;
}

// =============================================================================
// FFrameData Implementation
// =============================================================================

FFrameData::FFrameData()
	: FrameIndex(0)
	, DurationMs(100)
{
}

void FFrameData::ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight)
{
	NormalizedUVRect = FPixelUVRect::FromPixelRect(
		Position.X, Position.Y, Position.Width, Position.Height,
		TextureWidth, TextureHeight
	);
}

// =============================================================================
// FAnimationTag Implementation
// =============================================================================

FAnimationTag::FAnimationTag()
	: FromFrame(0)
	, ToFrame(0)
	, Repeat(0)
{
}

FPixelAnimSequence FAnimationTag::ToAnimSequence() const
{
	FPixelAnimSequence Seq;
	Seq.SequenceName = *Name;
	Seq.StartFrame = FromFrame;
	Seq.EndFrame = ToFrame;
	Seq.LoopCount = Repeat;

	if (Direction == TEXT("reverse"))
	{
		Seq.Direction = EPixelAnimDirection::Reverse;
	}
	else if (Direction == TEXT("pingpong"))
	{
		Seq.Direction = EPixelAnimDirection::PingPong;
	}
	else if (Direction == TEXT("pingpong_reverse"))
	{
		Seq.Direction = EPixelAnimDirection::PingPongReverse;
	}
	else
	{
		Seq.Direction = EPixelAnimDirection::Forward;
	}

	Seq.CalculateDuration();
	return Seq;
}

// =============================================================================
// FPixelPaletteProfile Implementation
// =============================================================================

FPixelPaletteProfile::FPixelPaletteProfile()
{
	GrayscaleMap.SetNum(256);
	for (int32 i = 0; i < 256; i++)
	{
		const float Gray = static_cast<float>(i) / 255.0f;
		GrayscaleMap[i] = FLinearColor(Gray, Gray, Gray, 1.0f);
	}
}

void FPixelPaletteProfile::AddColorOverride(const FName& LayerName, const FLinearColor& Color)
{
	ColorOverrides.Add(LayerName, Color);
}

FLinearColor FPixelPaletteProfile::GetColorOverride(const FName& LayerName, bool& bFound) const
{
	const FLinearColor* FoundColor = ColorOverrides.Find(LayerName);
	if (FoundColor)
	{
		bFound = true;
		return *FoundColor;
	}
	bFound = false;
	return FLinearColor::White;
}

void FPixelPaletteProfile::SetGrayscaleColor(int32 Index, const FLinearColor& Color)
{
	if (Index >= 0 && Index < 256)
	{
		GrayscaleMap[Index] = Color;
	}
}

FLinearColor FPixelPaletteProfile::GetGrayscaleColor(int32 Index) const
{
	if (Index >= 0 && Index < 256)
	{
		return GrayscaleMap[Index];
	}
	return FLinearColor::White;
}

bool FPixelPaletteProfile::IsValid() const
{
	return ColorOverrides.Num() > 0 || GrayscaleMap.Num() > 0;
}

// =============================================================================
// FAsepriteParseResult Implementation
// =============================================================================

FAsepriteParseResult::FAsepriteParseResult()
	: bSuccess(true)
{
}

FAsepriteParseResult FAsepriteParseResult::Success()
{
	return FAsepriteParseResult();
}

FAsepriteParseResult FAsepriteParseResult::Error(const FString& InErrorMessage)
{
	FAsepriteParseResult Result;
	Result.bSuccess = false;
	Result.ErrorMessage = InErrorMessage;
	return Result;
}

void FAsepriteParseResult::AddWarning(const FString& Warning)
{
	Warnings.Add(Warning);
}

void FAsepriteParseResult::AddValidationError(const FString& Error)
{
	ValidationErrors.Add(Error);
	bSuccess = false;
}

void FAsepriteParseResult::LogResults(const FString& Context) const
{
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("PixelComponent Parse Error [%s]: %s"), *Context, *ErrorMessage);
		for (const FString& Err : ValidationErrors)
		{
			UE_LOG(LogTemp, Error, TEXT("  Validation: %s"), *Err);
		}
	}

	for (const FString& Warn : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("PixelComponent Parse Warning [%s]: %s"), *Context, *Warn);
	}
}
