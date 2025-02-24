// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Layout/Margin.h"
#include "ItemTrait.h"
#include "IT_WidgetPaddingAndRadius.generated.h"

USTRUCT(Blueprintable)
struct FS_WidgetPaddingAndRadius
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FIntPoint Tile = FIntPoint(0, 0);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FMargin Padding;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FVector4 Rounding;
};

//Wrapper so we can have an array for each TMap entry
USTRUCT(Blueprintable)
struct FS_WidgetPaddingAndRadiusWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TEnumAsByte<ERotation> Rotation = Zero;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<FS_WidgetPaddingAndRadius> TileSettings;
};

/**Object for storing baked widget padding and radius calculations.
 * This should be calculated by an editor tool, then baked into the item.
 * This way we can save a ton of performance and a lot of headaches
 * revolving custom shapes and rotations.*/
UCLASS(DisplayName = "Widget Padding And Radius")
class INVENTORYFRAMEWORKPLUGIN_API UIT_WidgetPaddingAndRadius : public UItemTrait
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<FS_WidgetPaddingAndRadiusWrapper> PaddingAndRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FMargin Padding = FMargin(2, 2, 2, 2);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector4 Rounding;

	UFUNCTION(BlueprintCallable, Category = "Getters")
	bool GetSettingsForTile(TEnumAsByte<ERotation> Rotation, FIntPoint Tile, FS_WidgetPaddingAndRadius& Settings);

	UFUNCTION(BlueprintCallable, Category = "Baking")
	void BakeSettings(TEnumAsByte<ERotation> Rotation, UDA_CoreItem* ItemAsset);

	virtual bool AddedToItemAsset_Implementation(UDA_CoreItem* ItemAsset) override;

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;
};
