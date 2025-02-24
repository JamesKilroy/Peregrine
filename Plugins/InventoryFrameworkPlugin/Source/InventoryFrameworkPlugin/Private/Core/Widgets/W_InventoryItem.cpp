// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Widgets/W_InventoryItem.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Items/DA_CoreItem.h"
#include "Core/Widgets/W_Container.h"
#include "Core/Widgets/W_Drag.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


FS_InventoryItem UW_InventoryItem::GetItemData()
{
	if(IsValid(ItemID.ParentComponent))
	{
		FS_InventoryItem ItemData = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
		if(ItemData.IsValid())
		{
			return ItemData;
		}
	}
	
	UW_Container* ContainerWidget = GetParentContainer();;
	if(IsValid(ContainerWidget))
	{
		FS_ContainerSettings ContainerSettings = ContainerWidget->GetContainerSettings();

		const int32 ContainerIndex = ContainerSettings.ContainerIndex;
		UAC_Inventory* ParentComponent = nullptr;
		GetInventory(ParentComponent);
		if(IsValid(ParentComponent))
		{
			if(ParentComponent->ContainerSettings.IsValidIndex(ContainerIndex))
			{
				if(ParentComponent->ContainerSettings[ContainerIndex].Items.IsValidIndex(ItemsArrayIndex))
				{
					return ParentComponent->ContainerSettings[ContainerIndex].Items[ItemsArrayIndex];
				}
			}
		}
	}

	return RawItemData;
}

void UW_InventoryItem::GetParentTile(UW_Tile*& ParentTile)
{
	ParentTile = nullptr;
	UW_Container* ContainerWidget = GetParentContainer();
	if(!IsValid(ContainerWidget))
	{
		return;
	}

	FS_InventoryItem ItemData = GetItemData();
	if(ParentContainer->Tiles.IsValidIndex(ItemData.TileIndex))
	{
		ParentTile = ParentContainer->Tiles[ItemData.TileIndex];
	}
}

TArray<UW_Tile*> UW_InventoryItem::GetOwningTiles()
{
	TArray<UW_Tile*> OwningTiles;
	UAC_Inventory* ParentComponent = nullptr;
	GetInventory(ParentComponent);

	if(!IsValid(ParentComponent))
	{
		return OwningTiles;
	}

	UW_Container* ContainerWidget = GetParentContainer();
	if(!IsValid(ContainerWidget))
	{
		return OwningTiles;
	}
	
	FS_InventoryItem ItemData = GetItemData();
	TArray<int32> Tiles;
	bool InvalidTileFound;
	ParentComponent->GetItemsTileIndexes(ItemData, Tiles, InvalidTileFound);

	for(auto& CurrentTile : Tiles)
	{
		if(ContainerWidget->Tiles.IsValidIndex(CurrentTile))
		{
			OwningTiles.Add(ContainerWidget->Tiles[CurrentTile]);
		}
	}

	return OwningTiles;
}

void UW_InventoryItem::GetInventory(UAC_Inventory*& Component)
{
	Component = nullptr;
	if(IsValid(ItemID.ParentComponent))
	{
		Component = ItemID.ParentComponent;
		return;
	}

	if(IsValid(RawItemData.UniqueID.ParentComponent))
	{
		Component = RawItemData.UniqueID.ParentComponent;
		return;
	}
}


UW_Container* UW_InventoryItem::GetParentContainer()
{
	if(ParentContainer)
	{
		return ParentContainer;
	}

	if(!IsValid(ItemID.ParentComponent))
	{
		return nullptr;
	}

	FS_ContainerSettings ContainerStruct = ItemID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(ContainerStruct.IsValid())
	{
		if(UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(ContainerStruct))
		{
			return ContainerWidget;
		}
	}
	
	return nullptr;
}

const TMap<TEnumAsByte<ERotation> ,FIntPoint> UW_InventoryItem::GetAnchorPoint_Implementation()
{
	FS_InventoryItem Item = GetItemData();
	FIntPoint ItemAnchor = Item.ItemAsset->GetAnchorPoint();
	TArray<FIntPoint> ItemsShape = Item.ItemAsset->GetItemsPureShape(Zero);
	int32 AnchorIndex = ItemsShape.Find(ItemAnchor);

	if(AnchorIndex == -1)
	{
		TMap<TEnumAsByte<ERotation>, FIntPoint> DefaultAnchor;
		DefaultAnchor.Add(Zero, FIntPoint(0,0));
		return DefaultAnchor;
	}

	TMap<TEnumAsByte<ERotation>, FIntPoint> Anchor;
	for(ERotation CurrentRotation : TEnumRange<ERotation>())
	{
		if(CurrentRotation == Zero)
		{
			Anchor.Add(Zero, ItemAnchor);
			continue;
		}
		
		TMap<TEnumAsByte<ERotation>, FIntPoint> RotatedAnchor;
		ItemsShape = Item.ItemAsset->GetItemsPureShape(CurrentRotation);
		Anchor.Add(CurrentRotation, ItemsShape[AnchorIndex]);
	}
	
	return Anchor;
}

void UW_InventoryItem::ConstructItemWidget_Implementation()
{
	FS_InventoryItem ItemData = GetItemData();
	Execute_SizeUpdated(this, ItemData);
	Execute_RotationUpdated(this, ItemData);
	Execute_NameUpdated(this, ItemData);
	Execute_ItemCountUpdated(this, ItemData, ItemData.Count, ItemData.Count);
	Execute_LocationUpdated(this, ItemData);
	Execute_RarityUpdated(this, ItemData);
}

void UW_InventoryItem::DestroyWidget_Implementation()
{
}

FVector2D UW_InventoryItem::GetItemWidgetSize(FVector2D TileSizeOverride, bool IgnoreRotation)
{
	FVector2D FinalTileSize;
	FVector2D FinalSize = FVector2D();
	UW_Container* ContainerWidget = GetParentContainer();
	
	if(!IsValid(ContainerWidget))
	{
		if(GetImage())
		{
			return GetImage()->GetBrush().ImageSize;
		}
		else
		{
			return FinalSize;
		}
	}

	//Resolve if caller wants to use custom tile size
	if(!TileSizeOverride.IsZero())
	{
		FinalTileSize = TileSizeOverride;
	}
	else
	{
		FinalTileSize = ContainerWidget->TileSize;
	}

	switch (ContainerWidget->TemporaryContainerSettings.Style)
	{
	case Grid:
		{
			FS_InventoryItem ItemData = GetItemData();
			FIntPoint Dimensions;

			Dimensions = UFL_InventoryFramework::GetItemDimensions(ItemData, false, IgnoreRotation);
			
			FinalSize.X = UKismetMathLibrary::SelectFloat(FinalTileSize.X * Dimensions.X, FinalTileSize.X,
				ContainerWidget->TemporaryContainerSettings.Style == Grid);
			FinalSize.Y = UKismetMathLibrary::SelectFloat(FinalTileSize.Y * Dimensions.Y, FinalTileSize.Y,
				ContainerWidget->TemporaryContainerSettings.Style == Grid);
			return FinalSize;
		}
	case Traditional:
		{
			return ContainerWidget->TileSize;
		}
	case DataOnly:
		{
			return ContainerWidget->TileSize;
		}
	default: break;
	}

	return FinalSize;
}


void UW_InventoryItem::ItemHighlightUpdated_Implementation(bool NewIsHighlighted)
{
	IsHighlighted = NewIsHighlighted;
	ScaleTarget = IsHighlighted ? 1.1 : 1;
}

UDragDropOperation* UW_InventoryItem::StartDragItem_Implementation()
{
	IsDragging = true;
	UDragDropOperation* DropOperation = nullptr;
	OnDragDetected(FGeometry(), FPointerEvent(), DropOperation);
	return DropOperation;
}

void UW_InventoryItem::StopDragItem_Implementation()
{
	IsDragging = false;
	ItemDragEnded.Broadcast(this);
	if(UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this))
	{
		LocalInventory->DragWidget->RemoveFromParent();
		LocalInventory->DragWidget = nullptr;
	}
}

void UW_InventoryItem::CancelDragItem_Implementation()
{
	IsDragging = false;
	ItemDragEnded.Broadcast(this);
	if(UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this))
	{
		LocalInventory->DragWidget->PerformDropOnDestruction = false;
		LocalInventory->DragWidget->RemoveFromParent();
		LocalInventory->DragWidget = nullptr;
	}
}

void UW_InventoryItem::BackgroundColorUpdated_Implementation(FS_InventoryItem Item, FLinearColor NewColor,
                                                             bool IsTemporary)
{
	II_ExternalObjects::BackgroundColorUpdated_Implementation(Item, NewColor, IsTemporary);

	if(!IsTemporary)
	{
		CurrentBackgroundColor = NewColor;
	}
}

FLinearColor UW_InventoryItem::GetBackgroundColor_Implementation()
{
	return CurrentBackgroundColor;
}

void UW_InventoryItem::NativeTick(const FGeometry& MovieSceneBlends, float InDeltaTime)
{
	Super::NativeTick(MovieSceneBlends, InDeltaTime);

	SetRenderOpacity(FMath::FInterpTo(GetRenderOpacity(), OpacityTarget, InDeltaTime, 15.0));

	//Technically scaling the widget like this is bad,
	//because large items will scale a lot more than small items
	SetRenderScale(FVector2D(FMath::Vector2DInterpTo(GetRenderTransform().Scale, FVector2D(ScaleTarget), InDeltaTime, 15.0)));
}
