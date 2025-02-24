// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemTrait.h"
#include "IT_CustomShapeData.generated.h"



UCLASS(DisplayName = "Custom Shape")
class INVENTORYFRAMEWORKPLUGIN_API UIT_CustomShapeData : public UItemTrait
{
	GENERATED_BODY()

public:

	/**If the item is in a grid container, which tiles should be excluded from being added
	 * to the tile map? This allows you to make custom shaped items.
 	 * For example; If your item is 2x2, and you add 0/1, the top right tile is disabled
 	 * and won't be used for collision testing.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape Settings")
	TArray<FIntPoint> DisabledTiles;

	/**Typically the system treats the top left index of an item as the anchor point.
	 * For custom shaped items, you might hide the top left index of an item.
	 * This allows you to dictate the anchor point, so if the item has to be rotated
	 * during StartComponent, it can be rotated correctly.
	 *
	 * This will NOT change how the TileIndex is assigned to the item.
	 * The items TileIndex will always be dictated by the first tile the system
	 * can be found for its current rotation going from top left, scanning to the right
	 * and then going down one row, repeating until it reaches the bottom right.
	 * So if only 0,0 is hidden, it'll treat the 1,0 as the TileIndex.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape Settings")
	FIntPoint AnchorPoint = FIntPoint(0, 0);

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;
};