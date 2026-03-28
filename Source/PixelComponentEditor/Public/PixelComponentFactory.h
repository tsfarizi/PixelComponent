// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PixelComponentFactory.generated.h"

/**
 * UPixelComponentFactory
 * 
 * Factory class for importing Aseprite JSON files (.json) into UPixelComponentAsset.
 * 
 * Usage:
 * 1. Place your Aseprite-exported JSON file in the Content folder
 * 2. Ensure the corresponding PNG file is in the same directory
 * 3. Import the JSON file through the Content Browser
 * 4. The factory will automatically parse the JSON and link the texture
 * 
 * The factory supports:
 * - Automatic detection of .json extension for Aseprite files
 * - Parsing of slice data including 9-slice margins
 * - Automatic linking of source texture (PNG) if present
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
};
