// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelComponentTypes.generated.h"

/**
 * Forward declarations for efficient string handling
 * Note: Using FString instead of std::wstring_view for UE5 compatibility
 */

/**
 * Animation direction types from Aseprite.
 */
UENUM(BlueprintType)
enum class EPixelAnimDirection : uint8
{
	Forward		UMETA(DisplayName = "Forward"),
	Reverse		UMETA(DisplayName = "Reverse"),
	PingPong	UMETA(DisplayName = "Ping Pong"),
	PingPongReverse UMETA(DisplayName = "Ping Pong Reverse")
};

/**
 * Normalized UV Rectangle for Pixel Art HD pipeline.
 * Stores UV coordinates in 0-1 range with validation.
 * 
 * Technical Notes:
 * - MinX, MinY = Top-Left UV (Y is inverted for texture space)
 * - MaxX, MaxY = Bottom-Right UV
 * - All values must be in [0.0, 1.0] range
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelUVRect
{
	GENERATED_BODY()

	/** Minimum X (Left) UV coordinate [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinX;

	/** Minimum Y (Top) UV coordinate [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinY;

	/** Maximum X (Right) UV coordinate [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxX;

	/** Maximum Y (Bottom) UV coordinate [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxY;

	FPixelUVRect()
		: MinX(0.0f)
		, MinY(0.0f)
		, MaxX(0.0f)
		, MaxY(0.0f)
	{
	}

	FPixelUVRect(float InMinX, float InMinY, float InMaxX, float InMaxY)
		: MinX(InMinX)
		, MinY(InMinY)
		, MaxX(InMaxX)
		, MaxY(InMaxY)
	{
		Validate();
	}

	/**
	 * Create from pixel-space rectangle and texture dimensions.
	 * Performs pre-calculated UV normalization.
	 */
	static FPixelUVRect FromPixelRect(int32 PixelX, int32 PixelY, int32 PixelW, int32 PixelH, 
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

	/**
	 * Validate that all UV coordinates are within [0.0, 1.0] range.
	 * Clamps values if out of range.
	 * @return true if values were already valid
	 */
	bool Validate() const
	{
		bool bWasValid = true;
		float TempMinX = MinX, TempMinY = MinY, TempMaxX = MaxX, TempMaxY = MaxY;

		if (TempMinX < 0.0f || TempMinX > 1.0f) { TempMinX = FMath::Clamp(TempMinX, 0.0f, 1.0f); bWasValid = false; }
		if (TempMinY < 0.0f || TempMinY > 1.0f) { TempMinY = FMath::Clamp(TempMinY, 0.0f, 1.0f); bWasValid = false; }
		if (TempMaxX < 0.0f || TempMaxX > 1.0f) { TempMaxX = FMath::Clamp(TempMaxX, 0.0f, 1.0f); bWasValid = false; }
		if (TempMaxY < 0.0f || TempMaxY > 1.0f) { TempMaxY = FMath::Clamp(TempMaxY, 0.0f, 1.0f); bWasValid = false; }

		// Ensure Min < Max
		if (TempMinX > TempMaxX) { float Temp = TempMinX; TempMinX = TempMaxX; TempMaxX = Temp; bWasValid = false; }
		if (TempMinY > TempMaxY) { float Temp = TempMinY; TempMinY = TempMaxY; TempMaxY = Temp; bWasValid = false; }

		return bWasValid;
	}

	/**
	 * Get the width of the UV rectangle.
	 */
	float GetWidth() const { return MaxX - MinX; }

	/**
	 * Get the height of the UV rectangle.
	 */
	float GetHeight() const { return MaxY - MinY; }

	/**
	 * Get the center UV coordinate.
	 */
	FVector2f GetCenter() const
	{
		return FVector2f((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
	}

	/**
	 * Check if this UV rect is valid (non-zero area and within bounds).
	 */
	bool IsValid() const
	{
		return (GetWidth() > 0.0f) && (GetHeight() > 0.0f) &&
			   (MinX >= 0.0f && MinX <= 1.0f) &&
			   (MinY >= 0.0f && MinY <= 1.0f) &&
			   (MaxX >= 0.0f && MaxX <= 1.0f) &&
			   (MaxY >= 0.0f && MaxY <= 1.0f);
	}

	/**
	 * Convert to FBox2f for compatibility.
	 */
	FBox2f ToBox2f() const
	{
		return FBox2f(FVector2f(MinX, MinY), FVector2f(MaxX, MaxY));
	}

	/**
	 * Equality operator with tolerance for floating point comparison.
	 */
	bool operator==(const FPixelUVRect& Other) const
	{
		const float Tolerance = 0.0001f;
		return FMath::Abs(MinX - Other.MinX) < Tolerance &&
			   FMath::Abs(MinY - Other.MinY) < Tolerance &&
			   FMath::Abs(MaxX - Other.MaxX) < Tolerance &&
			   FMath::Abs(MaxY - Other.MaxY) < Tolerance;
	}
};

/**
 * 9-Slice margins in normalized UV space.
 * Used for HD pixel art tiling without distortion.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelNineSliceMargins
{
	GENERATED_BODY()

	/** Left margin UV [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Left;

	/** Top margin UV [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Top;

	/** Right margin UV [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Right;

	/** Bottom margin UV [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bottom;

	FPixelNineSliceMargins()
		: Left(0.0f), Top(0.0f), Right(0.0f), Bottom(0.0f)
	{
	}

	FPixelNineSliceMargins(float InLeft, float InTop, float InRight, float InBottom)
		: Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom)
	{
	}

	/**
	 * Create from pixel margins and texture dimensions.
	 */
	static FPixelNineSliceMargins FromPixelMargins(float PixelLeft, float PixelTop, float PixelRight, float PixelBottom,
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

	/**
	 * Get the center UV region for tiling.
	 */
	FPixelUVRect GetCenterUV(const FPixelUVRect& OuterUV) const
	{
		return FPixelUVRect(
			OuterUV.MinX + Left,
			OuterUV.MinY + Bottom,
			OuterUV.MaxX - Right,
			OuterUV.MaxY - Top
		);
	}

	bool IsValid() const
	{
		return Left >= 0.0f && Top >= 0.0f && Right >= 0.0f && Bottom >= 0.0f &&
			   (Left + Right) <= 1.0f && (Top + Bottom) <= 1.0f;
	}
};

/**
 * Animation sequence data extracted from Aseprite frame tags.
 * Contains all information needed for playback.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelAnimSequence
{
	GENERATED_BODY()

	/** Sequence name from Aseprite tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation")
	FName SequenceName;

	/** Starting frame index (inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 StartFrame;

	/** Ending frame index (inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 EndFrame;

	/** Frame duration in milliseconds (can be overridden per-frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "1"))
	int32 FrameDurationMs;

	/** Animation playback direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation")
	EPixelAnimDirection Direction;

	/** Number of loops (0 = infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 LoopCount;

	/** Total duration in milliseconds (computed) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Animation")
	int32 TotalDurationMs;

	FPixelAnimSequence()
		: SequenceName(NAME_None)
		, StartFrame(0)
		, EndFrame(0)
		, FrameDurationMs(100)
		, Direction(EPixelAnimDirection::Forward)
		, LoopCount(0)
		, TotalDurationMs(0)
	{
	}

	/**
	 * Calculate total duration based on frame count and duration.
	 */
	void CalculateDuration()
	{
		const int32 FrameCount = FMath::Max(1, EndFrame - StartFrame + 1);
		TotalDurationMs = FrameCount * FrameDurationMs;
	}

	/**
	 * Get the number of frames in this sequence.
	 */
	int32 GetFrameCount() const
	{
		return FMath::Max(0, EndFrame - StartFrame + 1);
	}

	/**
	 * Validate frame indices.
	 */
	bool Validate(int32 MaxFrameIndex) const
	{
		if (StartFrame < 0) return false;
		if (EndFrame > MaxFrameIndex) return false;
		if (StartFrame > EndFrame) return false;
		return true;
	}
};

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

	/** Parent layer name (for nested layers) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString ParentLayerName;

	/** Layer index in the stack (0 = bottom) */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	int32 LayerIndex;

	FLayerMetadata()
		: bVisible(true)
		, Opacity(1.0f)
		, UserDataColor(FColor::White)
		, LayerIndex(0)
	{
	}
};

/**
 * Rectangle defined in pixel space.
 * Represents a slice or region within the source texture.
 * Legacy structure - use FPixelUVRect for new code.
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
	FPixelUVRect ToNormalizedUV(int32 TextureWidth, int32 TextureHeight) const
	{
		return FPixelUVRect::FromPixelRect(X, Y, Width, Height, TextureWidth, TextureHeight);
	}

	bool IsValid() const
	{
		return Width > 0 && Height > 0;
	}
};

/**
 * Pivot point for slice transformation.
 * Used for consistent rotation and scaling anchor.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelPivot
{
	GENERATED_BODY()

	/** Pivot X in pixels (relative to slice origin) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	float X;

	/** Pivot Y in pixels (relative to slice origin) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	float Y;

	/** Pivot mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bIsCenter;

	FPixelPivot()
		: X(0.0f)
		, Y(0.0f)
		, bIsCenter(true)
	{
	}

	FPixelPivot(float InX, float InY, bool bInIsCenter = false)
		: X(InX)
		, Y(InY)
		, bIsCenter(bInIsCenter)
	{
	}

	/**
	 * Create pivot from percentage (0.0 = left/top, 1.0 = right/bottom).
	 */
	static FPixelPivot FromPercentage(float PercentX, float PercentY, int32 SliceWidth, int32 SliceHeight)
	{
		return FPixelPivot(
			PercentX * static_cast<float>(SliceWidth),
			PercentY * static_cast<float>(SliceHeight),
			false
		);
	}

	/**
	 * Get normalized pivot (0.0 - 1.0 range).
	 */
	FVector2f GetNormalized(int32 SliceWidth, int32 SliceHeight) const
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
};

/**
 * Slice data extracted from Aseprite JSON.
 * Enhanced with pre-calculated UVs and pivot points.
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

	/** Pre-calculated normalized UV rectangle [0.0 - 1.0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelUVRect NormalizedUVRect;

	/** Whether this slice has 9-slice metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bIsNineSlice;

	/** 9-slice margins in UV space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelNineSliceMargins NineSliceMarginsUV;

	/** Pivot point for rotation/scaling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelPivot Pivot;

	/** Slice color tag (from Aseprite) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FColor SliceColor;

	/**
	 * Legacy field for backwards compatibility.
	 * @deprecated Use NormalizedUVRect instead
	 */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Deprecated")
	FBox2f NormalizedUVs;

	FSliceData()
		: bIsNineSlice(false)
		, SliceColor(FColor::White)
	{
	}

	/**
	 * Compute normalized UVs based on texture dimensions.
	 * Updates both new (FPixelUVRect) and legacy (FBox2f) fields.
	 */
	void ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight)
	{
		NormalizedUVRect = FPixelUVRect::FromPixelRect(
			PixelRect.X, PixelRect.Y, PixelRect.Width, PixelRect.Height,
			TextureWidth, TextureHeight
		);

		// Legacy compatibility
		NormalizedUVs = NormalizedUVRect.ToBox2f();
	}

	/**
	 * Get the center UV region for 9-slice tiling.
	 */
	FPixelUVRect GetCenterUV() const
	{
		if (!bIsNineSlice || !NormalizedUVRect.IsValid())
		{
			return NormalizedUVRect;
		}

		return NineSliceMarginsUV.GetCenterUV(NormalizedUVRect);
	}

	/**
	 * Validate slice data.
	 */
	bool Validate(int32 MaxTextureWidth, int32 MaxTextureHeight) const
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "1"))
	int32 DurationMs;

	/** Frame position in the sprite sheet (if animated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelRect Position;

	/** Pre-calculated UV for this frame */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	FPixelUVRect NormalizedUVRect;

	FFrameData()
		: FrameIndex(0)
		, DurationMs(100)
	{
	}

	/**
	 * Compute UV for this frame.
	 */
	void ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight)
	{
		NormalizedUVRect = FPixelUVRect::FromPixelRect(
			Position.X, Position.Y, Position.Width, Position.Height,
			TextureWidth, TextureHeight
		);
	}
};

/**
 * Legacy animation tag structure.
 * @deprecated Use FPixelAnimSequence instead
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

	/**
	 * Convert to new FPixelAnimSequence format.
	 */
	FPixelAnimSequence ToAnimSequence() const
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

	/** Critical validation failures */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	TArray<FString> ValidationErrors;

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

	void AddValidationError(const FString& Error)
	{
		ValidationErrors.Add(Error);
		bSuccess = false;
	}

	/**
	 * Log all warnings and errors.
	 */
	void LogResults(const FString& Context) const
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
};

/**
 * Palette Profile for dynamic color overrides.
 * Maps layer names or color indices to new colors.
 * 
 * Technical Notes:
 * - Used for material instancing and dynamic recoloring
 * - Supports grayscale index mapping (0-255) to FLinearColor
 * - Layer-based overrides use FName keys (e.g., "Body", "Eyes")
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelPaletteProfile
{
	GENERATED_BODY()

	/** Profile display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString ProfileName;

	/** Color overrides: maps layer name or index to new color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (TitleProperty = "Key"))
	TMap<FName, FLinearColor> ColorOverrides;

	/** Grayscale index mapping (0-255) to color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	TArray<FLinearColor> GrayscaleMap;

	FPixelPaletteProfile()
	{
		// Pre-allocate grayscale map with 256 entries
		GrayscaleMap.SetNum(256);
		for (int32 i = 0; i < 256; i++)
		{
			const float Gray = static_cast<float>(i) / 255.0f;
			GrayscaleMap[i] = FLinearColor(Gray, Gray, Gray, 1.0f);
		}
	}

	/**
	 * Add a color override for a layer.
	 * @param LayerName Name of the layer to override
	 * @param Color New color to apply
	 */
	void AddColorOverride(const FName& LayerName, const FLinearColor& Color)
	{
		ColorOverrides.Add(LayerName, Color);
	}

	/**
	 * Get color override for a layer.
	 * @param LayerName Name of the layer
	 * @param bFound Output: true if override exists
	 * @return Override color or FLinearColor::White if not found
	 */
	FLinearColor GetColorOverride(const FName& LayerName, bool& bFound) const
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

	/**
	 * Set grayscale index to color mapping.
	 * @param Index Grayscale index (0-255)
	 * @param Color Color to map to
	 */
	void SetGrayscaleColor(int32 Index, const FLinearColor& Color)
	{
		if (Index >= 0 && Index < 256)
		{
			GrayscaleMap[Index] = Color;
		}
	}

	/**
	 * Get color from grayscale mapping.
	 * @param Index Grayscale index (0-255)
	 * @return Mapped color
	 */
	FLinearColor GetGrayscaleColor(int32 Index) const
	{
		if (Index >= 0 && Index < 256)
		{
			return GrayscaleMap[Index];
		}
		return FLinearColor::White;
	}

	/**
	 * Validate the profile.
	 * @return true if profile has valid data
	 */
	bool IsValid() const
	{
		return ColorOverrides.Num() > 0 || GrayscaleMap.Num() > 0;
	}
};
