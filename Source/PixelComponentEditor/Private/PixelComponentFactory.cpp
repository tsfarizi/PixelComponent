// Copyright Epic Games, Inc. All Rights Reserved.

#include "PixelComponentFactory.h"
#include "PixelComponentAsset.h"
#include "PixelComponentSettings.h"
#include "PixelFormat.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Logging/MessageLog.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentFactory, Log, All);

UPixelComponentFactory::UPixelComponentFactory()
{
	// Configure factory for .json files
	SupportedExtensions.Add(TEXT("json"));
	
	bEditorImport = true;
	bText = true;
	
	// Set supported class to PixelComponentAsset
	SupportedClass = UPixelComponentAsset::StaticClass();
	
	// Import priority
	Priority = 50;
}

bool UPixelComponentFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename).ToLower();
	return SupportedExtensions.Contains(Extension);
}

UObject* UPixelComponentFactory::FactoryCreateText(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const TCHAR*& Buffer,
	const TCHAR* BufferEnd,
	FFeedbackContext* Warn)
{
	// Get the file path from the import context
	const FString CurrentFilename = FLinkerLoadGetCurrentFilename();
	const FString FileContent = FString(BufferEnd - Buffer, Buffer);

	UE_LOG(LogPixelComponentFactory, Log, TEXT("Importing PixelComponentAsset: %s"), *InName.ToString());

	UPixelComponentAsset* Asset = ParseAsepriteJson(FileContent, InParent, InName, Warn);

	if (Asset)
	{
		// Try to link the source texture automatically
		FindAndLinkSourceTexture(CurrentFilename, Asset, Warn);
		
		// Refresh UV calculations after texture is linked
		Asset->RefreshNormalizedUVs();

		Asset->AddToRoot();
		return Asset;
	}

	return nullptr;
}

UPixelComponentAsset* UPixelComponentFactory::ParseAsepriteJson(
	const FString& JsonContent,
	UObject* InParent,
	FName InName,
	FFeedbackContext* Warn)
{
	// Parse JSON using TJsonReader
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse JSON for %s"), *InName.ToString());
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: Invalid JSON format"));
		}
		return nullptr;
	}

	// Validate Aseprite schema
	if (!ValidateAsepriteSchema(JsonObject))
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("JSON does not match Aseprite schema for %s"), *InName.ToString());
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: JSON does not match Aseprite export format"));
		}
		return nullptr;
	}

	// Create the asset
	UPixelComponentAsset* Asset = NewObject<UPixelComponentAsset>(InParent, InName, RF_Public | RF_Standalone);
	Asset->AddToRoot();

	// Parse slices
	FAsepriteParseResult SlicesResult = Asset->ParseSlicesFromJson(JsonObject);
	if (!SlicesResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse slices: %s"), *SlicesResult.ErrorMessage);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: %s"), *SlicesResult.ErrorMessage);
		}
		// Continue anyway - slices are optional
	}

	for (const FString& Warning : SlicesResult.Warnings)
	{
		UE_LOG(LogPixelComponentFactory, Warning, TEXT("Slice parse warning: %s"), *Warning);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Warning, TEXT("PixelComponent: %s"), *Warning);
		}
	}

	// Parse layers
	FAsepriteParseResult LayersResult = Asset->ParseLayersFromJson(JsonObject);
	if (!LayersResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse layers: %s"), *LayersResult.ErrorMessage);
	}

	for (const FString& Warning : LayersResult.Warnings)
	{
		UE_LOG(LogPixelComponentFactory, Warning, TEXT("Layer parse warning: %s"), *Warning);
	}

	// Parse animation data
	FAsepriteParseResult AnimResult = Asset->ParseAnimationFromJson(JsonObject);
	if (!AnimResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse animation: %s"), *AnimResult.ErrorMessage);
	}

	// Extract texture dimensions from JSON metadata
	int32 TexWidth = 0, TexHeight = 0;
	if (ExtractTextureDimensions(JsonObject, TexWidth, TexHeight))
	{
		UE_LOG(LogPixelComponentFactory, Verbose, 
			TEXT("Extracted texture dimensions from JSON: %dx%d"), TexWidth, TexHeight);
		
		// Store original dimensions in asset (before scaling)
		Asset->OriginalTextureWidth = TexWidth;
		Asset->OriginalTextureHeight = TexHeight;

		// Apply global pixel size scaling if enabled
		UPixelComponentSettings* Settings = UPixelComponentSettings::Get();
		if (Settings && Settings->bEnableAutoScaleOnImport)
		{
			const float ScaleFactor = UPixelComponentSettings::CalculateScaleFactor(FMath::Max(TexWidth, TexHeight));
			Asset->AppliedScaleFactor = ScaleFactor;
			Asset->bIsScaled = !FMath::IsNearlyEqual(ScaleFactor, 1.0f);

			if (Asset->bIsScaled)
			{
				UE_LOG(LogPixelComponentFactory, Log, 
					TEXT("Applying pixel size scaling: %dx%d -> %dx%d (scale: %.2f, global: %dpx)"),
					TexWidth, TexHeight,
					FMath::RoundToInt(TexWidth * ScaleFactor),
					FMath::RoundToInt(TexHeight * ScaleFactor),
					ScaleFactor,
					Settings->GlobalPixelSize);
			}
		}
	}

	UE_LOG(LogPixelComponentFactory, Log, 
		TEXT("Successfully parsed Aseprite JSON: %s (Slices: %d, Layers: %d, Frames: %d)"),
		*InName.ToString(), 
		Asset->GetSlices().Num(), 
		Asset->GetLayers().Num(),
		Asset->GetFrames().Num());

	return Asset;
}

bool UPixelComponentFactory::FindAndLinkSourceTexture(
	const FString& JsonFilePath,
	UPixelComponentAsset* Asset,
	FFeedbackContext* Warn)
{
	if (!Asset)
	{
		return false;
	}

	// Get the directory and base name
	const FString Directory = FPaths::GetPath(JsonFilePath);
	const FString BaseName = FPaths::GetBaseFilename(JsonFilePath);

	// Look for PNG file with same base name
	const FString PngPath = FPaths::Combine(Directory, BaseName + TEXT(".png"));
	
	// Also check for common image formats
	TArray<FString> PossibleExtensions = { TEXT("png"), TEXT("PNG"), TEXT("jpg"), TEXT("jpeg"), TEXT("bmp") };
	
	FString FoundTexturePath;
	for (const FString& Ext : PossibleExtensions)
	{
		const FString TestPath = FPaths::Combine(Directory, BaseName + TEXT(".") + Ext);
		if (FPaths::FileExists(TestPath))
		{
			FoundTexturePath = TestPath;
			break;
		}
	}

	if (FoundTexturePath.IsEmpty())
	{
		UE_LOG(LogPixelComponentFactory, Warning, 
			TEXT("No source texture found for %s (expected %s.png or similar)"), 
			*BaseName, *BaseName);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Warning, 
				TEXT("PixelComponent: Source texture not found. Please import the PNG manually."));
		}
		return false;
	}

	// Convert to Unreal package path
	const FString PackagePath = FPaths::ConvertRelativePathToFull(FoundTexturePath);
	const FString RelativePath = FPaths::MakePathRelativeTo(PackagePath, *FPaths::ProjectContentDir());
	const FString AssetPath = TEXT("/Game/") + FPaths::GetBaseFilename(RelativePath);

	// Try to load the texture
	UObject* ExistingObject = FindObject<UObject>(nullptr, *AssetPath);
	if (!ExistingObject)
	{
		// Try to load from disk
		ExistingObject = LoadObject<UObject>(nullptr, *AssetPath);
	}

	if (ExistingObject && ExistingObject->IsA<UTexture2D>())
	{
		UTexture2D* Texture = Cast<UTexture2D>(ExistingObject);
		Asset->SetSourceTexture(Texture);
		UE_LOG(LogPixelComponentFactory, Log, 
			TEXT("Linked source texture: %s"), *Texture->GetName());
		return true;
	}
	else
	{
		UE_LOG(LogPixelComponentFactory, Warning, 
			TEXT("Found texture file but could not load: %s"), *FoundTexturePath);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Warning, 
				TEXT("PixelComponent: Texture file exists but could not be loaded. Import the PNG first."));
		}
		return false;
	}
}

bool UPixelComponentFactory::ValidateAsepriteSchema(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	// Aseprite JSON must have "frames" or "meta" field at minimum
	const bool bHasFrames = JsonObject->HasField(TEXT("frames"));
	const bool bHasMeta = JsonObject->HasField(TEXT("meta"));
	const bool bHasSprite = JsonObject->HasField(TEXT("meta")) && 
		JsonObject->GetObjectField(TEXT("meta"))->HasField(TEXT("image"));

	// Check for "meta" object with required fields
	if (bHasMeta)
	{
		const TSharedPtr<FJsonObject>* MetaObject = JsonObject->TryGetField(TEXT("meta")).Get();
		if (MetaObject && MetaObject->IsValid())
		{
			// Aseprite meta should have "app" or "version" field
			const bool bHasApp = (*MetaObject)->HasField(TEXT("app"));
			const bool bHasVersion = (*MetaObject)->HasField(TEXT("version"));
			
			if (bHasApp || bHasVersion)
			{
				return true;
			}
		}
	}

	// Alternative: check for sprite sheet format
	if (bHasFrames)
	{
		return true;
	}

	return false;
}

bool UPixelComponentFactory::ExtractTextureDimensions(
	const TSharedPtr<FJsonObject>& JsonObject,
	int32& OutWidth,
	int32& OutHeight)
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	// Try to get dimensions from "meta" -> "size"
	const TSharedPtr<FJsonObject>* MetaObject;
	if (JsonObject->TryGetObjectField(TEXT("meta"), MetaObject) && MetaObject->IsValid())
	{
		const TSharedPtr<FJsonObject>* SizeObject;
		if ((*MetaObject)->TryGetObjectField(TEXT("size"), SizeObject) && SizeObject->IsValid())
		{
			int32 W, H;
			if ((*SizeObject)->TryGetNumberField(TEXT("w"), W) &&
				(*SizeObject)->TryGetNumberField(TEXT("h"), H))
			{
				OutWidth = W;
				OutHeight = H;
				return true;
			}
		}
	}

	// Alternative: get from first frame's dimensions
	const TArray<TSharedPtr<FJsonValue>>* FramesArray;
	if (JsonObject->TryGetArrayField(TEXT("frames"), FramesArray) && FramesArray->Num() > 0)
	{
		const TSharedPtr<FJsonObject>* FirstFrame = (*FramesArray)[0]->AsObject();
		if (FirstFrame && FirstFrame->IsValid())
		{
			const TSharedPtr<FJsonObject>* FrameRectObject;
			if ((*FirstFrame)->TryGetObjectField(TEXT("frame"), FrameRectObject) && FrameRectObject->IsValid())
			{
				int32 W, H;
				if ((*FrameRectObject)->TryGetNumberField(TEXT("w"), W) &&
					(*FrameRectObject)->TryGetNumberField(TEXT("h"), H))
				{
					OutWidth = W;
					OutHeight = H;
					return true;
				}
			}
		}
	}

	return false;
}
