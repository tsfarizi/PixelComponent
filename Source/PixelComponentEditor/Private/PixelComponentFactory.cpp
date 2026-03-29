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
#include "HAL/FileManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelComponentFactory, Log, All);

UPixelComponentFactory::UPixelComponentFactory()
{
	// Configure factory for .json and .png files
	SupportedExtensions.Add(TEXT("json"));
	SupportedExtensions.Add(TEXT("png"));

	bEditorImport = true;
	bText = true;  // JSON is text, but we'll handle PNG in FactoryCreateBinary

	// Set supported class to PixelComponentAsset
	SupportedClass = UPixelComponentAsset::StaticClass();
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
	const FString CurrentFilename = (Context && Context->GetOuter()) ? Context->GetPathName() : TEXT("");
	const FString FileContent = FString(BufferEnd - Buffer, Buffer);

	UE_LOG(LogPixelComponentFactory, Log, TEXT("Importing PixelComponentAsset: %s"), *InName.ToString());

	UPixelComponentAsset* Asset = ParseAsepriteJson(FileContent, InParent, InName, Warn);

	if (Asset)
	{
		// Try to link the source texture automatically
		FindAndLinkSourceTexture(CurrentFilename, Asset, Warn);

		// Refresh UV calculations after texture is linked
		Asset->RefreshNormalizedUVs();

		// Validate the asset
		if (!Asset->ValidateAsset())
		{
			UE_LOG(LogPixelComponentFactory, Warning, TEXT("Asset %s has validation issues"), *InName.ToString());
			if (Warn)
			{
				Warn->Logf(ELogVerbosity::Warning, TEXT("PixelComponent: Asset validation warnings - check output log"));
			}
		}

		Asset->AddToRoot();
		return Asset;
	}

	return nullptr;
}

UObject* UPixelComponentFactory::FactoryCreateBinary(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const uint8*& Buffer,
	const uint8* BufferEnd,
	FFeedbackContext* Warn)
{
	// Get the file path from the import context
	const FString CurrentFilename = (Context && Context->GetOuter()) ? Context->GetPathName() : TEXT("");
	const FString Extension = FPaths::GetExtension(CurrentFilename).ToLower();

	UE_LOG(LogPixelComponentFactory, Log, TEXT("Importing PixelComponentAsset (Binary): %s"), *InName.ToString());

	// Handle PNG import
	if (Extension == TEXT("png"))
	{
		UPixelComponentAsset* Asset = CreateAssetFromPNG(InParent, InName, CurrentFilename, Warn);

		if (Asset)
		{
			// Refresh UV calculations after texture is linked
			Asset->RefreshNormalizedUVs();

			// Validate the asset
			if (!Asset->ValidateAsset())
			{
				UE_LOG(LogPixelComponentFactory, Warning, TEXT("Asset %s has validation issues"), *InName.ToString());
			}

			Asset->AddToRoot();
			return Asset;
		}
	}

	return nullptr;
}

UPixelComponentAsset* UPixelComponentFactory::CreateAssetFromPNG(
	UObject* InParent,
	FName InName,
	const FString& TexturePath,
	FFeedbackContext* Warn)
{
	if (!FPaths::FileExists(TexturePath))
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("PNG file does not exist: %s"), *TexturePath);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: PNG file not found: %s"), *TexturePath);
		}
		return nullptr;
	}

	// Create the asset
	UPixelComponentAsset* Asset = NewObject<UPixelComponentAsset>(InParent, InName, RF_Public | RF_Standalone);
	Asset->AddToRoot();

	// Set asset name from filename
	Asset->SetAssetName(FPaths::GetBaseFilename(TexturePath));

	UE_LOG(LogPixelComponentFactory, Log, TEXT("Creating asset from PNG: %s"), *TexturePath);

	// Convert to Unreal package path
	FString FullPath = FPaths::ConvertRelativePathToFull(TexturePath);
	FString RelativePath = FullPath;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());
	const FString AssetPath = TEXT("/Game/") + FPaths::GetBaseFilename(RelativePath);

	UE_LOG(LogPixelComponentFactory, Log, TEXT("Loading texture from package path: %s"), *AssetPath);

	// Try to find existing loaded object first
	UObject* ExistingObject = FindObject<UObject>(nullptr, *AssetPath);

	if (!ExistingObject)
	{
		// Try to load from disk using StaticLoadObject
		ExistingObject = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath);
	}

	if (ExistingObject && ExistingObject->IsA<UTexture2D>())
	{
		UTexture2D* Texture = Cast<UTexture2D>(ExistingObject);
		Asset->SetSourceTexture(Texture);

		UE_LOG(LogPixelComponentFactory, Log, TEXT("Successfully loaded texture: %s"), *Texture->GetName());
		UE_LOG(LogPixelComponentFactory, Verbose, TEXT("Texture dimensions: %dx%d"),
			static_cast<int32>(Texture->GetSurfaceWidth()),
			static_cast<int32>(Texture->GetSurfaceHeight()));

		// Configure texture for pixel art (point filtering)
		Texture->Filter = TF_Nearest;
		Texture->UpdateResource();

		// Create default "FullTexture" slice for PNG-only imports
		FSliceData FullTextureSlice;
		FullTextureSlice.Name = TEXT("FullTexture");
		FullTextureSlice.PixelRect = FPixelRect(0, 0, Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight());
		FullTextureSlice.bIsNineSlice = false;
		
		// Compute normalized UVs (will be 0,0,1,1 for full texture)
		FullTextureSlice.ComputeNormalizedUVs(Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight());
		
		// Add the default slice
		TArray<FSliceData>& Slices = const_cast<TArray<FSliceData>&>(Asset->GetSlices());
		Slices.Add(FullTextureSlice);

		UE_LOG(LogPixelComponentFactory, Log, TEXT("Created default 'FullTexture' slice (UV: 0,0,1,1)"));
		UE_LOG(LogPixelComponentFactory, Log, TEXT("Created basic PixelComponentAsset (no slices/animations)"));
		return Asset;
	}
	else
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to load texture: %s"), *TexturePath);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: Could not load texture '%s'. Import the PNG first."), *TexturePath);
		}
		return nullptr;
	}
}

UPixelComponentAsset* UPixelComponentFactory::ParseAsepriteJson(
	const FString& JsonContent,
	UObject* InParent,
	FName InName,
	FFeedbackContext* Warn)
{
	// Parse JSON using TJsonReader - using efficient string handling
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

	// Parse metadata first (asset name, default pivot, etc.)
	FAsepriteParseResult MetaResult = Asset->ParseMetadataFromJson(JsonObject);
	if (!MetaResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Warning, TEXT("Metadata parse warning: %s"), *MetaResult.ErrorMessage);
	}
	MetaResult.LogResults(TEXT("Metadata"));

	// Parse slices with pre-calculated UVs
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
	SlicesResult.LogResults(TEXT("Slices"));

	// Parse layers
	FAsepriteParseResult LayersResult = Asset->ParseLayersFromJson(JsonObject);
	if (!LayersResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse layers: %s"), *LayersResult.ErrorMessage);
	}
	LayersResult.LogResults(TEXT("Layers"));

	// Parse animation data (including frame tags)
	FAsepriteParseResult AnimResult = Asset->ParseAnimationFromJson(JsonObject);
	if (!AnimResult.bSuccess)
	{
		UE_LOG(LogPixelComponentFactory, Error, TEXT("Failed to parse animation: %s"), *AnimResult.ErrorMessage);
	}
	AnimResult.LogResults(TEXT("Animation"));

	// Extract texture dimensions from JSON metadata for pre-calculated UVs
	int32 TexWidth = 0, TexHeight = 0;
	if (ExtractTextureDimensions(JsonObject, TexWidth, TexHeight))
	{
		UE_LOG(LogPixelComponentFactory, Verbose,
			TEXT("Extracted texture dimensions from JSON: %dx%d"), TexWidth, TexHeight);

		// Set texture dimensions from import (handles scaling internally)
		Asset->SetTextureDimensionsFromImport(TexWidth, TexHeight, true);

		// Validate texture dimensions match slice data
		ValidateTextureDimensions(JsonObject, TexWidth, TexHeight, Asset, Warn);
	}
	else
	{
		UE_LOG(LogPixelComponentFactory, Warning, TEXT("Could not extract texture dimensions from JSON"));
	}

	UE_LOG(LogPixelComponentFactory, Log, 
		TEXT("Successfully parsed Aseprite JSON: %s (Slices: %d, Layers: %d, Frames: %d, Animations: %d)"),
		*InName.ToString(), 
		Asset->GetSlices().Num(), 
		Asset->GetLayers().Num(),
		Asset->GetFrameCount(),
		Asset->GetAnimationSequences().Num());

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

	UE_LOG(LogPixelComponentFactory, Log,
		TEXT("Looking for source texture for JSON: %s"), *JsonFilePath);

	// Supported image extensions for texture lookup
	TArray<FString> PossibleExtensions = { TEXT("png"), TEXT("PNG"), TEXT("jpg"), TEXT("jpeg"), TEXT("bmp"), TEXT("tga") };

	FString FoundTexturePath;
	FString FoundExtension;

	// Use IFileManager for robust file detection
	IFileManager& FileManager = IFileManager::Get();

	for (const FString& Ext : PossibleExtensions)
	{
		const FString TestPath = FPaths::Combine(Directory, BaseName + TEXT(".") + Ext);
		
		// Check if file exists using FileManager
		if (FileManager.FileExists(*TestPath))
		{
			FoundTexturePath = TestPath;
			FoundExtension = Ext;
			UE_LOG(LogPixelComponentFactory, Log,
				TEXT("Found matching texture file: %s"), *FoundTexturePath);
			break;
		}
	}

	if (FoundTexturePath.IsEmpty())
	{
		UE_LOG(LogPixelComponentFactory, Warning,
			TEXT("No source texture found for %s (expected %s.png or similar in %s)"),
			*BaseName, *BaseName, *Directory);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Warning,
				TEXT("PixelComponent: Source texture not found for '%s'. Please import the PNG manually."),
				*BaseName);
		}
		return false;
	}

	// Convert to Unreal package path
	FString FullPath = FPaths::ConvertRelativePathToFull(FoundTexturePath);
	FString RelativePath = FullPath;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());
	const FString AssetPath = TEXT("/Game/") + FPaths::GetBaseFilename(RelativePath);

	UE_LOG(LogPixelComponentFactory, Log,
		TEXT("Attempting to load texture from package path: %s"), *AssetPath);

	// Try to find existing loaded object first
	UObject* ExistingObject = FindObject<UObject>(nullptr, *AssetPath);
	
	if (!ExistingObject)
	{
		// Try to load from disk using StaticLoadObject
		ExistingObject = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath);
		
		if (!ExistingObject)
		{
			// Try alternative path format
			const FString AlternativePath = FoundTexturePath;
			ExistingObject = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AlternativePath);
		}
	}

	if (ExistingObject && ExistingObject->IsA<UTexture2D>())
	{
		UTexture2D* Texture = Cast<UTexture2D>(ExistingObject);
		Asset->SetSourceTexture(Texture);
		
		UE_LOG(LogPixelComponentFactory, Log,
			TEXT("Successfully linked source texture: %s (loaded from %s)"),
			*Texture->GetName(), *FoundExtension);
		
		// Log texture dimensions for verification
		if (Texture)
		{
			UE_LOG(LogPixelComponentFactory, Verbose,
				TEXT("Texture dimensions: %dx%d"),
				static_cast<int32>(Texture->GetSurfaceWidth()),
				static_cast<int32>(Texture->GetSurfaceHeight()));
		}
		
		return true;
	}
	else
	{
		UE_LOG(LogPixelComponentFactory, Warning,
			TEXT("Found texture file but could not load: %s"), *FoundTexturePath);
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Warning,
				TEXT("PixelComponent: Texture file '%s' exists but could not be loaded. Import the PNG first."),
				*FoundTexturePath);
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
		const TSharedPtr<FJsonObject> MetaObject = JsonObject->GetObjectField(TEXT("meta"));
		if (MetaObject)
		{
			// Aseprite meta should have "app" or "version" field
			const bool bHasApp = MetaObject->HasField(TEXT("app"));
			const bool bHasVersion = MetaObject->HasField(TEXT("version"));
			
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
		const TSharedPtr<FJsonObject> FirstFrame = (*FramesArray)[0]->AsObject();
		if (FirstFrame)
		{
			const TSharedPtr<FJsonObject>* FrameRectObject;
			if (FirstFrame->TryGetObjectField(TEXT("frame"), FrameRectObject) && FrameRectObject)
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

void UPixelComponentFactory::ValidateTextureDimensions(
	const TSharedPtr<FJsonObject>& JsonObject,
	int32 ExpectedWidth,
	int32 ExpectedHeight,
	UPixelComponentAsset* Asset,
	FFeedbackContext* Warn)
{
	if (!Asset || !JsonObject.IsValid())
	{
		return;
	}

	// Validate slice bounds are within texture dimensions
	for (const FSliceData& Slice : Asset->GetSlices())
	{
		const int32 SliceRight = Slice.PixelRect.X + Slice.PixelRect.Width;
		const int32 SliceBottom = Slice.PixelRect.Y + Slice.PixelRect.Height;

		if (SliceRight > ExpectedWidth || SliceBottom > ExpectedHeight)
		{
			const FString WarningMsg = FString::Printf(
				TEXT("Slice '%s' bounds (%d,%d,%d,%d) exceed texture dimensions (%dx%d)"),
				*Slice.Name,
				Slice.PixelRect.X, Slice.PixelRect.Y, Slice.PixelRect.Width, Slice.PixelRect.Height,
				ExpectedWidth, ExpectedHeight);

			UE_LOG(LogPixelComponentFactory, Warning, TEXT("%s"), *WarningMsg);
			if (Warn)
			{
				Warn->Logf(ELogVerbosity::Warning, TEXT("PixelComponent: %s"), *WarningMsg);
			}
		}

		// Validate UV coordinates are in [0, 1] range
		if (Slice.NormalizedUVRect.IsValid())
		{
			if (Slice.NormalizedUVRect.MinX < 0.0f || Slice.NormalizedUVRect.MinX > 1.0f ||
				Slice.NormalizedUVRect.MinY < 0.0f || Slice.NormalizedUVRect.MinY > 1.0f ||
				Slice.NormalizedUVRect.MaxX < 0.0f || Slice.NormalizedUVRect.MaxX > 1.0f ||
				Slice.NormalizedUVRect.MaxY < 0.0f || Slice.NormalizedUVRect.MaxY > 1.0f)
			{
				const FString WarningMsg = FString::Printf(
					TEXT("Slice '%s' has UV coordinates outside [0,1] range: (%.4f, %.4f, %.4f, %.4f)"),
					*Slice.Name,
					Slice.NormalizedUVRect.MinX, Slice.NormalizedUVRect.MinY,
					Slice.NormalizedUVRect.MaxX, Slice.NormalizedUVRect.MaxY);

				UE_LOG(LogPixelComponentFactory, Error, TEXT("%s"), *WarningMsg);
				if (Warn)
				{
					Warn->Logf(ELogVerbosity::Error, TEXT("PixelComponent: %s"), *WarningMsg);
				}
			}
		}
	}
}
