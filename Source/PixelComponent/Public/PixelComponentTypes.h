// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelComponentTypes.generated.h"

// =============================================================================
// PixelComponent Types
// =============================================================================
// Core data structures and enumerations for the PixelComponent system.
// Organized by domain: Animation, Source, UV/Geometry, Slices, Layers, Metadata
// =============================================================================

#pragma region Enums

/**
 * Animation playback direction from Aseprite frame tags.
 */
UENUM(BlueprintType)
enum class EPixelAnimDirection : uint8
{
	Forward			UMETA(DisplayName = "Forward"),
	Reverse			UMETA(DisplayName = "Reverse"),
	PingPong		UMETA(DisplayName = "Ping Pong"),
	PingPongReverse	UMETA(DisplayName = "Ping Pong Reverse")
};

/**
 * Source mode for PixelComponent assets.
 * Implements mutual exclusion - only one source type is active at a time.
 */
UENUM(BlueprintType)
enum class EPixelSourceMode : uint8
{
	/** Texture mode: Uses texture with default material for parameter injection */
	Texture		UMETA(DisplayName = "Texture"),

	/** Material mode: Uses custom material as primary visual source */
	Material	UMETA(DisplayName = "Material")
};

#pragma endregion

#pragma region UV_Geometry

/**
 * Normalized UV rectangle for HD pixel art pipeline.
 * Stores UV coordinates in 0-1 range with automatic validation.
 *
 * Coordinate System:
 * - MinX, MinY = Top-Left corner (Y inverted for texture space)
 * - MaxX, MaxY = Bottom-Right corner
 * - All values clamped to [0.0, 1.0] range
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelUVRect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxY;

	FPixelUVRect();
	FPixelUVRect(float InMinX, float InMinY, float InMaxX, float InMaxY);

	/** Create from pixel-space rectangle with automatic UV normalization */
	static FPixelUVRect FromPixelRect(int32 PixelX, int32 PixelY, int32 PixelW, int32 PixelH,
		int32 TextureWidth, int32 TextureHeight);

	/** Validate UV coordinates are within [0.0, 1.0] range */
	bool Validate() const;

	/** Get UV rectangle width */
	float GetWidth() const;

	/** Get UV rectangle height */
	float GetHeight() const;

	/** Get center UV coordinate */
	FVector2f GetCenter() const;

	/** Check if UV rect has valid non-zero area */
	bool IsValid() const;

	/** Convert to FBox2f for compatibility */
	FBox2f ToBox2f() const;

	/** Equality comparison with floating point tolerance */
	bool operator==(const FPixelUVRect& Other) const;
};

/**
 * 9-slice margins in normalized UV space.
 * Used for scalable UI elements without distortion.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelNineSliceMargins
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Top;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bottom;

	FPixelNineSliceMargins();
	FPixelNineSliceMargins(float InLeft, float InTop, float InRight, float InBottom);

	/** Create from pixel margins with automatic UV normalization */
	static FPixelNineSliceMargins FromPixelMargins(float PixelLeft, float PixelTop, float PixelRight, float PixelBottom,
		int32 TextureWidth, int32 TextureHeight);

	/** Get center UV region for tiling/stretching */
	FPixelUVRect GetCenterUV(const FPixelUVRect& OuterUV) const;

	/** Validate margins are within valid range */
	bool IsValid() const;
};

#pragma endregion

#pragma region Pixel_Geometry

/**
 * Pixel-space rectangle for slice definitions.
 * Legacy structure - prefer FPixelUVRect for new code.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelRect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Width;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Height;

	FPixelRect();
	FPixelRect(int32 InX, int32 InY, int32 InWidth, int32 InHeight);

	/** Convert to normalized UV coordinates */
	FPixelUVRect ToNormalizedUV(int32 TextureWidth, int32 TextureHeight) const;

	/** Validate rectangle has positive dimensions */
	bool IsValid() const;
};

/**
 * Pivot point for slice transformations.
 * Defines rotation/scaling anchor point.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelPivot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bIsCenter;

	FPixelPivot();
	FPixelPivot(float InX, float InY, bool bInIsCenter = false);

	/** Create pivot from percentage values (0.0 = left/top, 1.0 = right/bottom) */
	static FPixelPivot FromPercentage(float PercentX, float PercentY, int32 SliceWidth, int32 SliceHeight);

	/** Get normalized pivot coordinates (0.0 - 1.0 range) */
	FVector2f GetNormalized(int32 SliceWidth, int32 SliceHeight) const;
};

#pragma endregion

#pragma region Slices

/**
 * Slice definition extracted from Aseprite JSON.
 * Contains spatial data, 9-slice margins, and pivot information.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FSliceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelRect PixelRect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelUVRect NormalizedUVRect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bIsNineSlice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelNineSliceMargins NineSliceMarginsUV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelPivot Pivot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FColor SliceColor;

	/** Legacy field for backwards compatibility */
	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Deprecated")
	FBox2f NormalizedUVs;

	FSliceData();

	/** Compute normalized UVs from pixel rectangle */
	void ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight);

	/** Get center UV region for 9-slice tiling */
	FPixelUVRect GetCenterUV() const;

	/** Validate slice data integrity */
	bool Validate(int32 MaxTextureWidth, int32 MaxTextureHeight) const;
};

#pragma endregion

#pragma region Layers

/**
 * Layer metadata from Aseprite file.
 * Contains visibility, opacity, and blend mode information.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FLayerMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	bool bVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FColor UserDataColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString BlendMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString ParentLayerName;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	int32 LayerIndex;

	FLayerMetadata();
};

#pragma endregion

#pragma region Animation

/**
 * Animation sequence from Aseprite frame tags.
 * Contains all data needed for playback control.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelAnimSequence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation")
	FName SequenceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 StartFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 EndFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "1"))
	int32 FrameDurationMs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation")
	EPixelAnimDirection Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent|Animation", meta = (ClampMin = "0"))
	int32 LoopCount;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent|Animation")
	int32 TotalDurationMs;

	FPixelAnimSequence();

	/** Calculate total duration from frame count and duration */
	void CalculateDuration();

	/** Get number of frames in sequence */
	int32 GetFrameCount() const;

	/** Validate frame indices against asset frame count */
	bool Validate(int32 MaxFrameIndex) const;
};

/**
 * Animation frame data from Aseprite.
 * Contains timing and position information.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FFrameData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 FrameIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (ClampMin = "1"))
	int32 DurationMs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FPixelRect Position;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	FPixelUVRect NormalizedUVRect;

	FFrameData();

	/** Compute UV coordinates for this frame */
	void ComputeNormalizedUVs(int32 TextureWidth, int32 TextureHeight);
};

/**
 * Legacy animation tag structure.
 * @deprecated Use FPixelAnimSequence instead
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FAnimationTag
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 FromFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 ToFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	int32 Repeat;

	FAnimationTag();

	/** Convert to modern FPixelAnimSequence format */
	FPixelAnimSequence ToAnimSequence() const;
};

#pragma endregion

#pragma region Palette

/**
 * Palette profile for dynamic color overrides.
 * Maps layer names or grayscale indices to custom colors.
 *
 * Use Cases:
 * - Character variant colors (Gold, Silver, Bronze armor)
 * - Day/night palette swaps
 * - Team color customization
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FPixelPaletteProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	FString ProfileName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent", meta = (TitleProperty = "Key"))
	TMap<FName, FLinearColor> ColorOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelComponent")
	TArray<FLinearColor> GrayscaleMap;

	FPixelPaletteProfile();

	/** Add layer color override */
	void AddColorOverride(const FName& LayerName, const FLinearColor& Color);

	/** Get color override for layer */
	FLinearColor GetColorOverride(const FName& LayerName, bool& bFound) const;

	/** Set grayscale index to color mapping (0-255) */
	void SetGrayscaleColor(int32 Index, const FLinearColor& Color);

	/** Get color from grayscale mapping */
	FLinearColor GetGrayscaleColor(int32 Index) const;

	/** Validate profile has valid data */
	bool IsValid() const;
};

#pragma endregion

#pragma region Import

/**
 * Result of parsing Aseprite JSON data.
 * Used for error handling and validation feedback.
 */
USTRUCT(BlueprintType)
struct PIXELCOMPONENT_API FAsepriteParseResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	bool bSuccess;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	FString ErrorMessage;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	TArray<FString> Warnings;

	UPROPERTY(VisibleAnywhere, Category = "PixelComponent")
	TArray<FString> ValidationErrors;

	FAsepriteParseResult();

	static FAsepriteParseResult Success();
	static FAsepriteParseResult Error(const FString& InErrorMessage);

	void AddWarning(const FString& Warning);
	void AddValidationError(const FString& Error);

	/** Log all warnings and errors to output log */
	void LogResults(const FString& Context) const;
};

#pragma endregion
