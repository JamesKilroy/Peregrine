// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/w_drag/


#pragma once

#include "CoreMinimal.h"
#include "W_Tile.h"
#include "Blueprint/UserWidget.h"
#include "W_Drag.generated.h"

UENUM(BlueprintType)
enum EDropOperation
{
	Cancel,
	MoveItem,
	Stack,
	Split,
	Combine,
	SwapItems,
	Delete,
	DropItem
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHoverTileUpdated, UW_Tile*, NewTile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDestroyed);

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UW_Drag : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category="Drag&Drop")
	TEnumAsByte<EDropOperation> DropOperation;

	UPROPERTY(BlueprintReadWrite, Category="Drag&Drop")
	UDragDropOperation* DragDropOperation = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	UW_Tile* CurrentHoverTile = nullptr;

	//Used to tell highlight where to interp it's location from.
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	UW_Tile* OldHoverTile = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	FS_InventoryItem ItemData;

	UPROPERTY(BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	TMap<TEnumAsByte<ERotation>, FIntPoint> AnchorPoint;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	int32 SplitAmount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool SkipCollisionCheck;

	UPROPERTY(BlueprintReadWrite, Category = "Stack")
	FS_InventoryItem ItemToStackWith;

	//X is ToContainer, Y is ToIndex.
	UPROPERTY(BlueprintReadWrite, Category = "Combine")
	FIntPoint CombineDirections = FIntPoint(-1, -1);

	UPROPERTY(BlueprintReadWrite, Category = "Combine")
	TEnumAsByte<ERotation> CombineRotation;

	/**What component will handle the drop event and where we retrieve
	 * keybindings from. This should always be the player controllers
	 * controlling pawn's actor component. If you are in the editor
	 * (for example an editor utility widget) you will have to set
	 * this manually as the player controller does not exist.*/
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	UAC_Inventory* TargetComponent = nullptr;

	/**Should we attempt to drop the item when this widget
	 * is destroyed?*/
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool PerformDropOnDestruction = true;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FHoverTileUpdated HoverTileUpdated;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FDestroyed Destroyed;

	UFUNCTION(Category = "Drag", BlueprintImplementableEvent, BlueprintCallable)
	void RotateItem();
};
