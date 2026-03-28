// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelComponentTypes.generated.h"

/**
 * Metadata for a single layer in the Aseprite file.
 * Contains information about layer visibility, opacity, and blend mode.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FLayerMetadata
{
	GENERATED_BODY()

	/** Name of the layer as defined in Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	/** Whether the layer is visible in Aseprite */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bVisible;

	/** Layer opacity (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity;

	/** User data/color associated with the layer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FColor UserDataColor;

	/** Blend mode from Aseprite (normal, multiply, screen, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString BlendMode;

	FLayerMetadata()
		: bVisible(true)
		, Opacity(1.0f)
		, UserDataColor(FColor::White)
	{
	}
};

/**
 * Rectangle defined in pixel space.
 * Represents a slice or region within the source texture.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelRect
{
	GENERATED_BODY()

	/** X position in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 X;

	/** Y position in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Y;

	/** Width in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Width;

	/** Height in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Height;

	FPixelRect()
		: X(0)
		, Y(0)
		, Width(0)
		, Height(0)
	{
	}

	FPixelRect(int32 InX, int32 InY, int32 InWidth, int32 InHeight)
		: X(InX)
		, Y(InY)
		, Width(InWidth)
		, Height(InHeight)
	{
	}

	/** Convert to normalized UV coordinates given texture dimensions */
	FBox2f ToNormalizedUV(int32 TextureWidth, int32 TextureHeight) const
	{
		const FVector2f MinUV(
			static_cast<float>(X) / static_cast<float>(TextureWidth),
			1.0f - static_cast<float>(Y + Height) / static_cast<float>(TextureHeight)
		);
		const FVector2f MaxUV(
			static_cast<float>(X + Width) / static_cast<float>(TextureWidth),
			1.0f - static_cast<float>(Y) / static_cast<float>(TextureHeight)
		);
		return FBox2f(MinUV, MaxUV);
	}

	bool IsValid() const
	{
		return Width > 0 && Height > 0;
	}
};

/**
 * Slice data extracted from Aseprite JSON.
 * Can represent a regular slice or a 9-slice region.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FSliceData
{
	GENERATED_BODY()

	/** Name of the slice */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	/** Base pixel-space rectangle for the slice */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelRect PixelRect;

	/** Whether this slice has 9-slice metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bIsNineSlice;

	/** 9-slice margins (Left, Top, Right, Bottom) in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FMargin NineSliceMargins;

	/** Normalized UV coordinates (computed at runtime) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	FBox2f NormalizedUVs;

	FSliceData()
		: bIsNineSlice(false)
		, NineSliceMargins(0.0f)
	{
	}

	/** Compute normalized UVs based on texture dimensions */
	void ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight)
	{
		NormalizedUVs = PixelRect.ToNormalizedUV(TextureWidth, TextureHeight);
	}

	/** Get the UV rectangle for the center region of a 9-slice */
	FBox2f GetCenterUV() const
	{
		if (!bIsNineSlice || !PixelRect.IsValid())
		{
			return NormalizedUVs;
		}

		const int32 TexW = PixelRect.Width;
		const int32 TexH = PixelRect.Height;

		const float LeftUV = NineSliceMargins.Left / static_cast<float>(TexW);
		const float RightUV = NineSliceMargins.Right / static_cast<float>(TexW);
		const float TopUV = NineSliceMargins.Top / static_cast<float>(TexH);
		const float BottomUV = NineSliceMargins.Bottom / static_cast<float>(TexH);

		const FVector2f MinUV(NormalizedUVs.Min.X + LeftUV, NormalizedUVs.Min.Y + BottomUV);
		const FVector2f MaxUV(NormalizedUVs.Max.X - RightUV, NormalizedUVs.Max.Y - TopUV);

		return FBox2f(MinUV, MaxUV);
	}
};

/**
 * Animation frame data from Aseprite.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FFrameData
{
	GENERATED_BODY()

	/** Frame index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 FrameIndex;

	/** Frame duration in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 DurationMs;

	/** Frame position in the sprite sheet (if animated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelRect Position;

	FFrameData()
		: FrameIndex(0)
		, DurationMs(100)
	{
	}
};

/**
 * Tags for organizing animations in Aseprite.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FAnimationTag
{
	GENERATED_BODY()

	/** Tag name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	/** Starting frame index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 FromFrame;

	/** Ending frame index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 ToFrame;

	/** Animation direction (forward, reverse, pingpong) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Direction;

	/** Number of times to repeat (0 = infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Repeat;

	FAnimationTag()
		: FromFrame(0)
		, ToFrame(0)
		, Repeat(0)
	{
	}
};

/**
 * Result of parsing Aseprite JSON data.
 * Used for error handling and validation.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FAsepriteParseResult
{
	GENERATED_BODY()

	/** Whether parsing was successful */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	bool bSuccess;

	/** Error message if parsing failed */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	FString ErrorMessage;

	/** Warnings encountered during parsing */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	TArray<FString> Warnings;

	FAsepriteParseResult()
		: bSuccess(true)
	{
	}

	static FAsepriteParseResult Success()
	{
		return FAsepriteParseResult();
	}

	static FAsepriteParseResult Error(const FString& InErrorMessage)
	{
		FAsepriteParseResult Result;
		Result.bSuccess = false;
		Result.ErrorMessage = InErrorMessage;
		return Result;
	}

	void AddWarning(const FString& Warning)
	{
		Warnings.Add(Warning);
	}
};
