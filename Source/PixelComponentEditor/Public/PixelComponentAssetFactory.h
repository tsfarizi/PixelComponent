// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PixelComponentAssetFactory.generated.h"

/**
 * UPixelComponentAssetFactory
 *
 * Factory for creating PixelComponentAsset instances via the Content Browser.
 * Appears in right-click menu under "Pixel Component" category.
 */
UCLASS(ClassGroup = (PixelComponent), meta = (DisplayName = "Pixel Component Asset", DisplayCategory = "Pixel Component"))
class PIXELCOMPONENTEDITOR_API UPixelComponentAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPixelComponentAssetFactory();

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override;
	//~ End UFactory Interface

protected:
	/** The texture to assign to the new asset */
	UPROPERTY()
	UTexture2D* SourceTexture;
};
