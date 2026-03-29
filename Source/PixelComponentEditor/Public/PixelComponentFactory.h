// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PixelComponentFactory.generated.h"

/**
 * UPixelComponentFactory
 *
 * Factory class for importing Aseprite JSON files (.json) and PNG images
 * into UPixelComponentAsset.
 *
 * Usage:
 * 1. Import PNG directly for simple pixel art assets (no slices/animations)
 * 2. Import Aseprite JSON for full-featured assets with slices and animations
 * 3. The factory will automatically link textures and parse metadata
 *
 * The factory supports:
 * - Direct PNG import for standard pixel art assets
 * - Aseprite JSON import with automatic texture linking
 * - Parsing of slice data including 9-slice margins
 * - Layer metadata extraction
 * - Animation tag and frame data parsing
 */
UCLASS(ClassGroup = (PixelComponent), meta = (DisplayName = "PixelComponent Asset Factory"))
class PIXELCOMPONENTEDITOR_API UPixelComponentFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPixelComponentFactory();

	//~ Begin UFactory Interface
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* FactoryCreateText(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const TCHAR*& Buffer,
		const TCHAR* BufferEnd,
		FFeedbackContext* Warn
	) override;
	virtual UObject* FactoryCreateBinary(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const uint8*& Buffer,
		const uint8* BufferEnd,
		FFeedbackContext* Warn
	) override;
	//~ End UFactory Interface

	/**
	 * Parse Aseprite JSON content and populate a PixelComponentAsset.
	 * 
	 * @param JsonContent The raw JSON string from Aseprite
	 * @param InParent Parent object for the new asset
	 * @param InName Name for the new asset
	 * @param Warn Feedback context for logging
	 * @return The created asset, or nullptr on failure
	 */
	static class UPixelComponentAsset* ParseAsepriteJson(
		const FString& JsonContent,
		UObject* InParent,
		FName InName,
		FFeedbackContext* Warn
	);

	/**
	 * Find and link the source texture for a pixel art asset.
	 * Looks for a PNG file with the same base name in the same directory.
	 * 
	 * @param JsonFilePath Full path to the JSON file
	 * @param Asset The asset to link the texture to
	 * @param Warn Feedback context for logging
	 * @return true if texture was found and linked
	 */
	static bool FindAndLinkSourceTexture(
		const FString& JsonFilePath,
		class UPixelComponentAsset* Asset,
		FFeedbackContext* Warn
	);

	/**
	 * Create PixelComponentAsset from PNG file.
	 * Creates a basic asset with texture reference but no slices or animations.
	 *
	 * @param InParent Parent object for the new asset
	 * @param InName Name for the new asset
	 * @param TexturePath Path to the texture file
	 * @param Warn Feedback context for logging
	 * @return The created asset
	 */
	static class UPixelComponentAsset* CreateAssetFromPNG(
		UObject* InParent,
		FName InName,
		const FString& TexturePath,
		FFeedbackContext* Warn
	);

protected:
	/** Supported file extensions */
	TArray<FString> SupportedExtensions;

private:
	/**
	 * Validate that the JSON content matches Aseprite schema.
	 * @param JsonObject The parsed JSON object
	 * @return true if valid Aseprite JSON
	 */
	static bool ValidateAsepriteSchema(const TSharedPtr<FJsonObject>& JsonObject);

	/**
	 * Extract texture dimensions from Aseprite JSON metadata.
	 * @param JsonObject The parsed JSON object
	 * @param OutWidth Output for width
	 * @param OutHeight Output for height
	 * @return true if dimensions were extracted
	 */
	static bool ExtractTextureDimensions(
		const TSharedPtr<FJsonObject>& JsonObject,
		int32& OutWidth,
		int32& OutHeight
	);

	/**
	 * Validate texture dimensions against slice data.
	 * Logs warnings if slice bounds exceed texture dimensions or UVs are out of range.
	 * 
	 * @param JsonObject The parsed JSON object
	 * @param ExpectedWidth Expected texture width
	 * @param ExpectedHeight Expected texture height
	 * @param Asset The asset being imported
	 * @param Warn Feedback context for logging
	 */
	static void ValidateTextureDimensions(
		const TSharedPtr<FJsonObject>& JsonObject,
		int32 ExpectedWidth,
		int32 ExpectedHeight,
		class UPixelComponentAsset* Asset,
		FFeedbackContext* Warn
	);
};
