# PixelComponent Plugin - HD Pixel Art Refactoring

## Overview

This document describes the comprehensive refactoring of the PixelComponent plugin to support **HD Pixel Art pipelines** with precise UV coordinates, animation sequences, and material integration.

---

## 1. Data Model Enhancement (`PixelComponentTypes.h`)

### New Structures

#### `FPixelUVRect`
Normalized UV rectangle for Pixel Art HD pipeline.

```cpp
USTRUCT(BlueprintType)
struct FPixelUVRect
{
    float MinX, MinY, MaxX, MaxY;  // All in [0.0 - 1.0] range
    
    static FPixelUVRect FromPixelRect(int32 PixelX, int32 PixelY, 
                                       int32 PixelW, int32 PixelH,
                                       int32 TextureWidth, int32 TextureHeight);
    bool Validate();  // Clamps values to [0,1]
    bool IsValid() const;
    FVector2f GetCenter() const;
};
```

**Key Features:**
- Pre-calculated UV normalization from pixel coordinates
- Automatic validation (clamps to [0,1] range)
- Y-axis inversion for texture space

#### `FPixelNineSliceMargins`
9-slice margins in **UV space** (not pixels).

```cpp
USTRUCT(BlueprintType)
struct FPixelNineSliceMargins
{
    float Left, Top, Right, Bottom;  // UV values [0.0 - 1.0]
    
    static FPixelNineSliceMargins FromPixelMargins(...);
    FPixelUVRect GetCenterUV(const FPixelUVRect& OuterUV) const;
};
```

#### `FPixelAnimSequence`
Animation sequence from Aseprite frame tags.

```cpp
USTRUCT(BlueprintType)
struct FPixelAnimSequence
{
    FName SequenceName;
    int32 StartFrame, EndFrame;
    int32 FrameDurationMs;
    EPixelAnimDirection Direction;
    int32 LoopCount;
    int32 TotalDurationMs;  // Auto-calculated
    
    void CalculateDuration();
    bool Validate(int32 MaxFrameIndex);
};
```

#### `FPixelPivot`
Pivot point for consistent rotation/scaling.

```cpp
USTRUCT(BlueprintType)
struct FPixelPivot
{
    float X, Y;  // Pixels relative to slice origin
    bool bIsCenter;
    
    static FPixelPivot FromPercentage(float PercentX, float PercentY, 
                                       int32 SliceWidth, int32 SliceHeight);
    FVector2f GetNormalized(int32 SliceWidth, int32 SliceHeight) const;
};
```

### Enhanced Structures

#### `FSliceData`
Now includes:
- `FPixelUVRect NormalizedUVRect` - Pre-calculated UVs
- `FPixelNineSliceMargins NineSliceMarginsUV` - UV-space margins
- `FPixelPivot Pivot` - Pivot point
- `FColor SliceColor` - Aseprite color tag

#### `FLayerMetadata`
Now includes:
- `FString ParentLayerName` - Nested layer support
- `int32 LayerIndex` - Stack order

#### `FFrameData`
Now includes:
- `FPixelUVRect NormalizedUVRect` - Per-frame UV

---

## 2. Asset Class Enhancement (`PixelComponentAsset.h/.cpp`)

### New Properties

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent")
FString AssetName;  // From Aseprite metadata

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Material")
TMap<FString, FName> LayerToMaterialParamMap;  // Layer → Material mapping

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Animation")
TMap<FString, FPixelAnimSequence> AnimationSequences;  // New animation system

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelComponent|Pivot")
FPixelPivot DefaultPivot;
```

### New Functions

#### UV Access
```cpp
FPixelUVRect GetSliceNormalizedUVRect(const FString& SliceName) const;
FPixelUVRect GetNineSliceCenterUVRect(const FString& SliceName) const;
TArray<FVector4f> GetAllSliceUVsAsVector4() const;  // For batch updates
```

#### Animation Access
```cpp
const TMap<FString, FPixelAnimSequence>& GetAnimationSequences() const;
const FPixelAnimSequence* GetAnimationSequence(const FString& SequenceName) const;
TArray<FString> GetAllAnimationSequenceNames() const;
```

#### Layer Mapping
```cpp
FName GetMaterialParameterForLayer(const FString& LayerName) const;
void SetLayerMaterialMapping(const FString& LayerName, const FName& ParamName);
```

#### Validation
```cpp
bool ValidateAsset() const;      // Full asset validation
bool ValidateUVs() const;         // UV bounds checking
```

---

## 3. Factory Parsing Enhancement (`PixelComponentFactory.h/.cpp`)

### Pre-Calculated UV Conversion

During import, all pixel coordinates are **immediately converted** to normalized UVs:

```cpp
FAsepriteParseResult UPixelComponentFactory::ParseAsepriteJson(...)
{
    // ... parse JSON ...
    
    // Extract texture dimensions FIRST
    int32 TexWidth, TexHeight;
    ExtractTextureDimensions(JsonObject, TexWidth, TexHeight);
    
    // Parse slices WITH pre-calculated UVs
    Asset->ParseSlicesFromJson(JsonObject);  // UVs computed inside
    
    // Parse animation tags
    Asset->ParseAnimationFromJson(JsonObject);  // FPixelAnimSequence
}
```

### Advanced 9-Slice Parsing

Recursive extraction from Aseprite JSON:

```cpp
// Calculate margins from slice bounds vs key bounds
const int32 LeftMargin = KeyX - Slice.PixelRect.X;
const int32 TopMargin = KeyY - Slice.PixelRect.Y;
const int32 RightMargin = (Slice.PixelRect.X + Slice.PixelRect.Width) - (KeyX + KeyW);
const int32 BottomMargin = (Slice.PixelRect.Y + Slice.PixelRect.Height) - (KeyY + KeyH);

// Store in UV space for HD pipeline
Slice.NineSliceMarginsUV = FPixelNineSliceMargins::FromPixelMargins(
    LeftMargin, TopMargin, RightMargin, BottomMargin,
    Slice.PixelRect.Width, Slice.PixelRect.Height
);
```

### Validation & Error Logging

```cpp
void UPixelComponentFactory::ValidateTextureDimensions(
    const TSharedPtr<FJsonObject>& JsonObject,
    int32 ExpectedWidth,
    int32 ExpectedHeight,
    UPixelComponentAsset* Asset,
    FFeedbackContext* Warn)
{
    for (const FSliceData& Slice : Asset->GetSlices())
    {
        // Check bounds exceed texture
        if (SliceRight > ExpectedWidth || SliceBottom > ExpectedHeight)
        {
            UE_LOG(LogPixelComponentFactory, Warning, ...);
        }
        
        // Check UVs in [0,1] range
        if (!Slice.NormalizedUVRect.IsValid())
        {
            UE_LOG(LogPixelComponentFactory, Error, ...);
        }
    }
}
```

---

## 4. Material Library Enhancement (`PixelComponentMaterialLibrary.h/.cpp`)

### Global State Injection

Automatically sends settings to materials:

```cpp
bool FPixelComponentMaterialLibrary::InjectGlobalSettings(
    UMaterialInstanceDynamic* MaterialInstance)
{
    UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
    
    // Send GlobalPixelScale
    MaterialInstance->SetScalarParameterValue(
        Param_GlobalPixelScale, 
        static_cast<float>(Settings->GlobalPixelSize)
    );
    
    // Send VirtualResolution
    MaterialInstance->SetVectorParameterValue(
        Param_VirtualResolution,
        FLinearColor(1920.0f, 1080.0f, 0.0f, 0.0f)
    );
}
```

### Batch Parameter Updates

Send all slice UVs in **one call**:

```cpp
int32 FPixelComponentMaterialLibrary::SendAllSliceUVsAsBatch(
    const UPixelComponentAsset* Asset,
    UMaterialInstanceDynamic* MaterialInstance,
    const FName& ParameterName = "SliceUVArray")
{
    const TArray<FVector4f> UVs = Asset->GetAllSliceUVsAsVector4();
    
    // Convert and set as array
    TArray<FVector4> UVsConverted;
    for (const FVector4f& UV : UVs)
    {
        UVsConverted.Add(FVector4(UV.X, UV.Y, UV.Z, UV.W));
    }
    
    MaterialInstance->SetVectorArrayParameterValue(ParameterName, UVsConverted);
    return UVs.Num();
}
```

### Layer Color Mapping

```cpp
int32 FPixelComponentMaterialLibrary::SendLayerColorsToMaterial(
    const UPixelComponentAsset* Asset,
    UMaterialInstanceDynamic* MaterialInstance)
{
    const TArray<FLayerMetadata>& Layers = Asset->GetLayers();
    const TMap<FString, FName>& LayerMap = Asset->GetLayerToMaterialMap();
    
    for (const FLayerMetadata& Layer : Layers)
    {
        FName TargetParam = LayerMap.FindRef(Layer.Name);
        FLinearColor LayerColor = Layer.UserDataColor.ReinterpretAsLinear();
        MaterialInstance->SetVectorParameterValue(TargetParam, LayerColor);
    }
}
```

### New Parameter Names

```cpp
static const FName Param_GlobalPixelScale;     // float
static const FName Param_VirtualResolution;    // Vector4
static const FName Param_SliceUVArray;         // Vector4 Array
static const FName Param_Pivot;                // Vector2
```

---

## 5. Technical Constraints Implementation

### Zero-Inference Rule

No assumptions about screen size - all data from `UPixelComponentSettings`:

```cpp
// ❌ WRONG: Don't assume
float Scale = 1.0f;

// ✅ CORRECT: Get from settings
UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
float GlobalPixelScale = static_cast<float>(Settings->GlobalPixelSize);
```

### Memory Efficiency

Using `FStringView` for intensive JSON parsing:

```cpp
using FStringView = std::wstring_view;

// Pass string views instead of copies
void ParseSliceName(FStringView NameView)
{
    // No allocation, just reference
}
```

### Validation Logging

Comprehensive error reporting:

```cpp
FAsepriteParseResult::LogResults(const FString& Context) const
{
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Parse Error [%s]: %s"), *Context, *ErrorMessage);
        for (const FString& Err : ValidationErrors)
        {
            UE_LOG(LogTemp, Error, TEXT("  Validation: %s"), *Err);
        }
    }
    
    for (const FString& Warn : Warnings)
    {
        UE_LOG(LogTemp, Warning, TEXT("Parse Warning [%s]: %s"), *Context, *Warn);
    }
}
```

---

## 6. Migration Guide

### Old Code → New Code

#### UV Access
```cpp
// OLD
FBox2f UVs = Asset->GetSliceNormalizedUVs("MySlice");

// NEW
FPixelUVRect UVs = Asset->GetSliceNormalizedUVRect("MySlice");
// Or for legacy compatibility:
FBox2f UVsLegacy = UVs.ToBox2f();
```

#### Animation
```cpp
// OLD
const FAnimationTag* Tag = &Asset->GetAnimationTags()[0];

// NEW
const FPixelAnimSequence* Seq = Asset->GetAnimationSequence("Idle");
if (Seq)
{
    int32 FrameCount = Seq->GetFrameCount();
    int32 Duration = Seq->TotalDurationMs;
}
```

#### Material Parameters
```cpp
// OLD - individual calls
Material->SetScalarParameterValue("NineSliceLeft", Margin.Left);
Material->SetScalarParameterValue("NineSliceRight", Margin.Right);
// ...

// NEW - batch
FPixelComponentMaterialLibrary::SendAllSliceUVsAsBatch(Asset, Material);
```

---

## 7. Usage Examples

### Basic Import & Display

```cpp
// Import (automatic via Factory)
UPixelComponentAsset* Asset = LoadObject<UPixelComponentAsset>(..., "MyPixelArt");

// Create dynamic material
UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);

// Send all data in one call
FPixelComponentMaterialLibrary::SendNineSliceDataToMaterial(Asset, MID);
FPixelComponentMaterialLibrary::SendAllSliceUVsAsBatch(Asset, MID);

// Apply to mesh
MeshComponent->SetMaterial(0, MID);
```

### Animation Playback

```cpp
// Get sequence
const FPixelAnimSequence* Seq = Asset->GetAnimationSequence("Run");
if (Seq)
{
    // Calculate current frame
    float Time = GetTimeSeconds();
    float NormalizedTime = FMath::Fmod(Time, Seq->TotalDurationMs) / Seq->TotalDurationMs;
    int32 CurrentFrame = Seq->StartFrame + FMath::RoundToInt(NormalizedTime * Seq->GetFrameCount());
    
    // Get frame UV
    if (CurrentFrame < Asset->GetFrames().Num())
    {
        FPixelUVRect FrameUV = Asset->GetFrames()[CurrentFrame].NormalizedUVRect;
        MID->SetVectorParameterValue("CurrentFrameUV", 
            FLinearColor(FrameUV.MinX, FrameUV.MinY, FrameUV.MaxX, FrameUV.MaxY));
    }
}
```

### Layer Color Tinting

```cpp
// Map layer to material parameter
Asset->SetLayerMaterialMapping("Body", "BodyColor");
Asset->SetLayerMaterialMapping("Eyes", "EyeColor");

// Send to material
FPixelComponentMaterialLibrary::SendLayerColorsToMaterial(Asset, MID);
```

---

## 8. File Summary

| File | Changes |
|------|---------|
| `PixelComponentTypes.h` | New structs: `FPixelUVRect`, `FPixelNineSliceMargins`, `FPixelAnimSequence`, `FPixelPivot` |
| `PixelComponentAsset.h` | New properties: `AnimationSequences`, `LayerToMaterialParamMap`, `DefaultPivot` |
| `PixelComponentAsset.cpp` | New parsing: `ParseMetadataFromJson`, validation, UV batch export |
| `PixelComponentFactory.h` | New method: `ValidateTextureDimensions` |
| `PixelComponentFactory.cpp` | Pre-calculated UVs, advanced 9-slice parsing, validation logging |
| `PixelComponentMaterialLibrary.h` | New methods: `InjectGlobalSettings`, `SendAllSliceUVsAsBatch`, `SendLayerColorsToMaterial` |
| `PixelComponentMaterialLibrary.cpp` | Global state injection, batch updates, layer mapping |

---

## 9. Benefits

1. **Precision**: All UVs pre-calculated and validated in [0,1] range
2. **Performance**: Batch parameter updates reduce material overhead
3. **Flexibility**: Layer-to-material mapping for dynamic tinting
4. **Animation**: Full support for Aseprite frame tags
5. **HD Ready**: UV-space margins scale to any resolution
6. **Validation**: Comprehensive error checking and logging

---

## 10. Next Steps

Recommended enhancements:

1. **Runtime Component** - `UPixelComponentRenderer` for animation playback
2. **Editor UI** - Custom detail panel for preview and configuration
3. **Sample Materials** - Pre-built shaders for 9-slice and tiling
4. **Animation Blueprint** - Integration with Unreal's animation system
5. **Sprite Sheet Support** - Automatic texture atlas generation
