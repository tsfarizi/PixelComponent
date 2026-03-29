// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PixelComponentAsset.h"
#include "PixelComponentAssetFactory.generated.h"

/**
 * UPixelComponentAssetFactory
 *
 * Factory for creating PixelComponentAsset instances via the Content Browser.
 * Allows users to right-click in Content Browser and select
 * "Pixel Component > Pixel Component Asset" to create a new asset.
 */
UCLASS(ClassGroup = (PixelComponent), meta = (DisplayName = "Pixel Component Asset", DisplayCategory = "Pixel Component"))
class PIXELCOMPONENT_API UPixelComponentAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPixelComponentAssetFactory();

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory Interface

protected:
	/** The texture to assign to the new asset */
	UPROPERTY()
	UTexture2D* SourceTexture;
};
