// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Widgets/W_Container.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Widgets/W_Drag.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"


void UW_Container::ConstructContainers_Implementation(FS_ContainerSettings ContainerSetting, UAC_Inventory* InventoryComponent, bool Reinitialize)
{
	//Handled in blueprint.
}

UAC_Inventory* UW_Container::GetInventory()
{
	if(IsValid(TemporaryContainerSettings.UniqueID.ParentComponent))
	{
		return TemporaryContainerSettings.UniqueID.ParentComponent;
	}
	return nullptr;
}

FS_ContainerSettings UW_Container::GetContainerSettings()
{
	if(UAC_Inventory* ParentComponent = GetInventory(); IsValid(ParentComponent))
	{
		FS_ContainerSettings ContainerSettings = ParentComponent->GetContainerByUniqueID(TemporaryContainerSettings.UniqueID);
		if(ContainerSettings.IsValid())
		{
			return ContainerSettings;
		}
	}

	return TemporaryContainerSettings;
}

TArray<UW_InventoryItem*> UW_Container::GetAllItemWidgets()
{
	TArray<UW_InventoryItem*> FoundItems;

	FS_ContainerSettings ContainerSettings = GetContainerSettings();
	for(auto& CurrentItem : ContainerSettings.Items)
	{
		if(IsValid(CurrentItem.Widget))
		{
			FoundItems.Add(CurrentItem.Widget);
		}
	}
	
	return FoundItems;
}

void UW_Container::GetWidgetForItem(FS_InventoryItem Item, UW_InventoryItem*& Widget)
{
	Widget = nullptr;

	if(IsValid(Item.Widget))
	{
		Widget = Item.Widget;
		return;
	}

	FS_ContainerSettings ContainerSettings = GetContainerSettings();
	for(auto& CurrentItem : ContainerSettings.Items)
	{
		if(CurrentItem.UniqueID == Item.UniqueID)
		{
			Widget = CurrentItem.Widget;
			return;
		}
	}
}

int32 UW_Container::GetCurrentNavigatedTile()
{
	return CurrentNavigatedTile;
}

bool UW_Container::SetCurrentNavigatedTile(int32 NewTile)
{
	if(Tiles.IsValidIndex(CurrentNavigatedTile))
	{
		Tiles[CurrentNavigatedTile]->IsTileHighlighted = false;
	}
	
	if(Tiles.IsValidIndex(NewTile))
	{
		CurrentNavigatedTile = NewTile;
		Tiles[CurrentNavigatedTile]->IsTileHighlighted = true;
		if(UScrollBox* ScrollBox = GetScrollBox())
		{
			ScrollBox->ScrollWidgetIntoView(Tiles[NewTile]);
		}
		return true;
	}

	//Navigated to invalid tile
	return false;
}

FS_InventoryItem UW_Container::GetCurrentNavigatedItem()
{
	return CurrentNavigatedItem;
}

bool UW_Container::SetCurrentNavigatedItem_Implementation(FS_InventoryItem NewItem)
{
	if(CurrentNavigatedItem == NewItem)
	{
		return false;
	}
	
	if(CurrentNavigatedItem.IsValid())
	{
		if(UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(CurrentNavigatedItem))
		{
			ItemWidget->ItemHighlightUpdated(false);
		}
		ItemNavigationFocusUpdated(CurrentNavigatedItem, false);
	}

	LastNavigatedItem = CurrentNavigatedItem;
	CurrentNavigatedItem = NewItem;
	if(CurrentNavigatedItem.IsValid())
	{
		if(UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(NewItem))
		{
			ItemWidget->ItemHighlightUpdated(true);
		}
		ItemNavigationFocusUpdated(CurrentNavigatedItem, true);

		return true;
	}

	return false;
}

void UW_Container::StopNavigation()
{
}

void UW_Container::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if(!Tiles.IsValidIndex(CurrentNavigatedTile))
	{
		return;
	}

	//V: Opacity is handled in here rather than NativePaint because it's CONST :))))
	//V: We handle the tiles Tick logic in here so we can continue having tick disabled
	//on the tile widget to improve performance.
	// Tiles[CurrentNavigatedTile]->HighlightOpacity = FMath::FInterpTo(Tiles[CurrentNavigatedTile]->HighlightOpacity,
	// 	Tiles[CurrentNavigatedTile]->IsTileHighlighted ? 1 : 0, FApp::GetDeltaTime(), 15);
	if(Tiles[CurrentNavigatedTile]->IsTileHighlighted && Tiles[CurrentNavigatedTile]->IsVisible())
	{
		//The player is using gamepad navigation, try to set the mouse location to the tiles screen position
		//to simulate as if the player is still using the mouse
		if(APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
		{
			FVector2D PixelPosition;
			FVector2D ViewportPosition;
			USlateBlueprintLibrary::LocalToViewport(this, Tiles[CurrentNavigatedTile]->GetCachedGeometry(), FVector2D(0), PixelPosition, ViewportPosition);
			FVector2D TileGeometrySize = Tiles[CurrentNavigatedTile]->GetCachedGeometry().GetAbsoluteSize();
				
			//Remember to ceil the float, otherwise there might be cases where the cursor slightly
			//touches an item widget in an adjacent tile and the tooltip will appear
			Controller->SetMouseLocation(FMath::FloorToInt(PixelPosition.X) + TileGeometrySize.X - 2, FMath::CeilToInt(PixelPosition.Y) + (TileGeometrySize.Y / 2));
		}
	}
}

UW_Container* UW_Container::GetNextContainerToNavigateTo_Implementation(
	EContainerNavigationDirection RequestedDirection)
{
	switch (RequestedDirection)
	{
	case NavigateUp:
		{
			if(NavigateUpContainerOverride)
			{
				if(NavigateUpContainerOverride->IsVisible())
				{
					return NavigateUpContainerOverride;
				}
			}
			if(Navigation)
			{
				if(Navigation->Up.Widget.Get())
				{
					return Cast<UW_Container>(Navigation->Up.Widget.Get());
				}
			}
			break;
		}
	case NavigateRight:
		{
			if(NavigateRightContainerOverride)
			{
				if(NavigateRightContainerOverride->IsVisible())
				{
					return NavigateRightContainerOverride;
				}
			}
			if(Navigation)
			{
				if(Navigation->Right.Widget.Get())
				{
					return Cast<UW_Container>(Navigation->Right.Widget.Get());
				}
			}
			break;
		}
	case NavigateDown:
		{
			if(NavigateDownContainerOverride)
			{
				if(NavigateDownContainerOverride->IsVisible())
				{
					return NavigateDownContainerOverride;
				}
			}
			if(Navigation)
			{
				if(Navigation->Down.Widget.Get())
				{
					return Cast<UW_Container>(Navigation->Down.Widget.Get());
				}
			}
			break;
		}
	case NavigateLeft:
		{
			if(NavigateLeftContainerOverride)
			{
				if(NavigateLeftContainerOverride->IsVisible())
				{
					return NavigateLeftContainerOverride;
				}
			}
			if(Navigation)
			{
				if(Navigation->Left.Widget.Get())
				{
					return Cast<UW_Container>(Navigation->Left.Widget.Get());
				}
			}
			break;
		}
		default:
		{
			return nullptr;
		}
	}

	return nullptr;
}


void UW_Container::CreateWidgetForItem_Implementation(FS_InventoryItem& Item, UW_InventoryItem*& Widget)
{
	//Handled in blueprint.
}


void UW_Container::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this);
	if(!LocalInventory)
	{
		return;
	}

	/**If we are using IFP's custom drag drop system, then call regular OnDragEnter
	 * if we are dragging an item.*/
	if(LocalInventory->DragWidget && !LocalInventory->UseDefaultDragDropBehavior)
	{
		TSharedPtr<FDragDropOperation> SlateDragOp = FSlateApplication::Get().GetDragDroppingContent();
		FDragDropEvent DragDropEvent(InMouseEvent, SlateDragOp);
		NativeOnDragEnter(InGeometry, DragDropEvent, LocalInventory->DragWidget->DragDropOperation);
	}
}

void UW_Container::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	/**If we are using IFP's custom drag drop system, then call regular OnDragLeave
	* if we are dragging an item.*/
	UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this);
	if(!LocalInventory)
	{
		return;
	}

	if(LocalInventory->DragWidget && !LocalInventory->UseDefaultDragDropBehavior)
	{
		TSharedPtr<FDragDropOperation> SlateDragOp = FSlateApplication::Get().GetDragDroppingContent();
		FDragDropEvent DragDropEvent(InMouseEvent, SlateDragOp);
		NativeOnDragLeave(DragDropEvent, LocalInventory->DragWidget->DragDropOperation);
	}
}
