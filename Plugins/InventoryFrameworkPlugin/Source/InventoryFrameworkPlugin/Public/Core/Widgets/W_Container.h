// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "W_InventoryItem.h"
#include "Blueprint/UserWidget.h"
#include "W_Tile.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Core/Components/AC_Inventory.h"
#include "W_Container.generated.h"

UENUM(BlueprintType)
enum EContainerNavigationDirection
{
	NavigateUp,
	NavigateRight,
	NavigateDown,
	NavigateLeft
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNavigatedOutOfBounds, EContainerNavigationDirection, LastDirection, UW_Container*, FromContainer, UW_Container*, ToContainer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNavigatedTo, EContainerNavigationDirection, InDirection, UW_Container*, FromContainer, UW_Container*, ToContainer);

/**Widget representing a container, which hosts tiles and items.*/
UCLASS(Abstract)
class INVENTORYFRAMEWORKPLUGIN_API UW_Container : public UUserWidget, public II_ExternalObjects
{
	GENERATED_BODY()

public:

	//--------------------
	// Variables
	
	//Arbitrary tags that can be used for gameplay and code logic.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Tags")
	FGameplayTagContainer TagsContainer;

	/**This is only set when this widget is constructed. You should not trust most data from this.
	 * This is so programmers and designers don't have to constantly keep this widget up to date
	 * with the struct version. The only data you should trust from this is data that never changes.
	 * For example UniqueID, ContainerType, ContainerIdentifier, it all depends on your project and what
	 * information you don't change.
	 * This is only here to allow you to get a visual representation inside the UMG editor and bind
	 * this widget to a specific container struct inside of a inventory component.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn=true))
	FS_ContainerSettings TemporaryContainerSettings;

	//Tile widget we use to create the tiles.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TSubclassOf<UW_Tile> TileClass = nullptr;

	//Widget used to create items. This can be overriden by individual items.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TSubclassOf<UW_InventoryItem> ItemClass = nullptr;
	
	//The tiles themselves.
	UPROPERTY(BlueprintReadWrite, Category = "Tiles")
	TArray<UW_Tile*> Tiles;

	//How big are the tiles.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Settings")
	FVector2D TileSize = FVector2D(60, 60);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Settings")
	FMargin TilePadding;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Settings")
	FMargin ItemPadding;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Settings", meta = (InlineEditConditionToggle))
	bool UseCustomTileBrush = false;

	/**Optional brush for the tile to receive and use instead
	 * of its default.
	 * This is used when you want to use a specific tile class,
	 * but only want to override the background texture.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "UseCustomTileBrush"))
	FSlateBrush CustomTileBrush;

	UPROPERTY(BlueprintReadWrite, Category = "Items", meta = (DeprecatedProperty, DeprecationMessage = "Replaced with GetAllItemWidgets"))
	TArray<TObjectPtr<UW_InventoryItem>> Items;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Initialization")
	bool Initialized;
	
	UPROPERTY(Category = "Navigation", BlueprintReadOnly)
	FS_InventoryItem LastNavigatedItem;

protected:
	/**The currently navigated tile, mostly used for either controller
	 * or keyboard navigation.*/
	UPROPERTY()
	int32 CurrentNavigatedTile = 0;

	UPROPERTY()
	FS_InventoryItem CurrentNavigatedItem;
	
public:

	UPROPERTY(Category="Navigation", BlueprintReadWrite)
	TObjectPtr<UW_Container> NavigateUpContainerOverride = nullptr;

	UPROPERTY(Category="Navigation", BlueprintReadWrite)
	TObjectPtr<UW_Container> NavigateRightContainerOverride = nullptr;

	UPROPERTY(Category="Navigation", BlueprintReadWrite)
	TObjectPtr<UW_Container> NavigateDownContainerOverride = nullptr;

	UPROPERTY(Category="Navigation", BlueprintReadWrite)
	TObjectPtr<UW_Container> NavigateLeftContainerOverride = nullptr;

	/**The user has pressed a navigation key, but have hit the
	* boundaries of the container. Usually, you want to navigate to
	* an adjacent container in this case or un-focus the container.*/
	UPROPERTY(Category = "Navigation", BlueprintAssignable, BlueprintCallable)
	FNavigatedOutOfBounds NavigatedOutOfBounds;

	/**The user has navigated to this container through a navigation key*/
	UPROPERTY(Category = "Navigation", BlueprintAssignable, BlueprintCallable)
	FNavigatedTo NavigatedTo;
	
	//--------------------
	// Functions

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Border"))
	UBorder* GetBorder();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Overlay"))
	UOverlay* GetOverlay();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Grid Panel"))
	UUniformGridPanel* GetGridPanel();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Scroll Box"))
	UScrollBox* GetScrollBox();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CPP Functions||Initialization")
	void ConstructContainers(FS_ContainerSettings ContainerSetting, UAC_Inventory* InventoryComponent, bool Reinitialize);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Inventory"))
	UAC_Inventory* GetInventory();

	/**Even though container widgets have a ContainerSettings, you should only use that variable
	 * for information that would never change (for example it's UniqueID, container type...)
	 * since this function might get heavy if a component has A LOT of containers.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Settings"))
	FS_ContainerSettings GetContainerSettings();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Item Widgets"))
	TArray<UW_InventoryItem*> GetAllItemWidgets();

	/**This is mainly handled on the Blueprint level because designers might want to create children
	 * of W_Container and have this function behave completely differently.
	 * This should allow this system to be converted into a list style inventory.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items")
	void CreateWidgetForItem(UPARAM(ref) FS_InventoryItem& Item, UW_InventoryItem*& Widget);

	UFUNCTION(BlueprintCallable, Category = "Items", meta = (DeprecatedFunction = "Moved to helper library, use FL_InventoryFramework -> GetWidgetForItem"))
	void GetWidgetForItem(FS_InventoryItem Item, UW_InventoryItem*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Items")
	void EnableItemCollision();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Items")
	void DisableItemCollision();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Cosmetic")
	void ScrollUp();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Cosmetic")
	void ScrollDown();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void NavigateContainer(EContainerNavigationDirection Direction);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	FS_InventoryItem GetNextItemToNavigateTo(EContainerNavigationDirection RequestedDirection);

	/**Get the next container to navigate towards.
	 * This will always prioritize the overrides first,
	 * then ask the navigation data which widget to go to.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Navigation")
	UW_Container* GetNextContainerToNavigateTo(EContainerNavigationDirection RequestedDirection);

	/**This container has either been navigated into or out of
	 *
	 * @NewContainer The new container that has received focus.
	 * This might not be valid in some scenarios.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void NavigationFocusUpdated(bool HasFocus, EContainerNavigationDirection InOrOutDirection, UW_Container* NewContainer);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void ItemNavigationFocusUpdated(FS_InventoryItem Item, bool HasFocus);

	UFUNCTION(BlueprintCallable, Category = "Navigation", BlueprintPure)
	int32 GetCurrentNavigatedTile();

	/**Assign a new navigated tile.
	 * Can return false either if the tile is the same
	 * as the current or if the tile is invalid.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	bool SetCurrentNavigatedTile(int32 NewTile);
	
	UFUNCTION(BlueprintCallable, Category = "Navigation", BlueprintPure)
	FS_InventoryItem GetCurrentNavigatedItem();
	
	/**Assign a new navigated item.
	 * Can return false either if the item is the same
	 * as the current or if the item is invalid.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation", BlueprintNativeEvent)
	bool SetCurrentNavigatedItem(FS_InventoryItem NewItem);

	/**End all gamepad navigation, returning control to the
	 * cursor for regular navigation.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void StopNavigation();

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
};