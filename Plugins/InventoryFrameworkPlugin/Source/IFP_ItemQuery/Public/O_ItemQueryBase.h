// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Data/IFP_CoreData.h"
#include "UObject/Object.h"
#include "O_ItemQueryBase.generated.h"

class UAC_ItemQueryManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQueryRefreshed);
DECLARE_DYNAMIC_DELEGATE(FQueryRefreshCallback);

/**Base class for handling item queries.
 * This is handled through the ItemQueryManager component.
 *
 * Docs:
 * https://inventoryframework.github.io/systems/itemquery/ */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew, AutoExpandCategories = "Item Query", HideCategories = "DoNotShow")
class IFP_ITEMQUERY_API UO_ItemQueryBase : public UObject
{
	GENERATED_BODY()

	virtual class UWorld* GetWorld() const override;

public:

	/** Cached items that have been registered in this query.
	 * NOTE: This is supposed to be private and only accessible through
	 * the GetItemsFromQuery function. The reason why this is readable
	 * is so that the blueprint debugger shows it in breakpoints and
	 * the blueprint debugger tool.
	 * Without BlueprintReadOnly, this property will be invisible
	 * to the debugging tools. You should NOT be using this variable
	 * for anything. Use the getter function.*/
	UPROPERTY(Category = "Item Query", BlueprintReadOnly)
	TArray<FS_InventoryItem> RegisteredItems;

	UPROPERTY(Category = "Item Query", BlueprintReadOnly)
	TObjectPtr<UAC_ItemQueryManager> ItemQueryManager = nullptr;
	
	UPROPERTY(Category = "Item Query", BlueprintReadOnly)
	TObjectPtr<UAC_Inventory> Inventory = nullptr;

	/**Optional identifier to uniquely identify this query.
	 * For example, this can be set as something like "AmmoItems",
	 * then you can call AC_ItemQueryManager -> GetQueryByIdentifier
	 *
	 * This can also be replaced with a gameplay tag, depending
	 * on how you plan on using this*/
	UPROPERTY(Category = "Item Query", BlueprintReadOnly, EditDefaultsOnly)
	FGameplayTag Identifier;

	UPROPERTY(Category = "Item Query", BlueprintAssignable)
	FQueryRefreshed QueryRefreshed;

	/**The query has been registered with a manager. This is your BeginPlay and you
	 * should bind any relevant delegates here.*/
	void QueryRegistered(UAC_ItemQueryManager* QueryManager, UAC_Inventory* InventoryComponent, bool DoNotRefresh = false, bool MultiThreadRefresh = true);

	/**The query has been registered with a manager. This is your BeginPlay and you
	 * should bind any relevant delegates here.*/
	UFUNCTION(Category = "Item Query", BlueprintImplementableEvent, DisplayName = "Query Registered")
	void K2_QueryRegistered(UAC_ItemQueryManager* QueryManager, UAC_Inventory* InventoryComponent);

	/**Attempt to register an item with this query.
	 * This can return false if the item did not pass the filtering function.*/
	UFUNCTION(Category = "Item Query", BlueprintCallable)
	bool RegisterItem(FS_InventoryItem Item);
	
	/**An item has been successfully registered*/
	UFUNCTION(Category = "Item Query", BlueprintImplementableEvent)
	void ItemRegistered(FS_InventoryItem Item);

	/**Attempt to unregister an item from this query.
	 * This can return false if the item was not in this query.*/
	UFUNCTION(Category = "Item Query", BlueprintCallable)
	bool UnregisterItem(FS_InventoryItem Item);

	/**An item has been unregistered*/
	UFUNCTION(Category = "Item Query", BlueprintImplementableEvent)
	void ItemUnregistered(FS_InventoryItem Item);

	/**Reset the @RegisteredItems array and refresh it.
	 *
	 * @OnlyRefreshRegisteredItems:
	 * - true: Re-evaluate all the currently registered items and
	 * unregister them if they do not pass the filter anymore
	 * - false: Go through all containers and all their items and
	 * attempt to register the item.
	 *
	 * If you ever need to run this and leave the option to true,
	 * then the query is most likely not setup correctly. This
	 * should realistically never have to be set to true.
	 *
	 * @MultiThreadRefresh If true, we attempt to do the refresh
	 * on any other available thread. You would now bind to the
	 * "QueryRefreshed" delegate or use the RefreshItemsWithCallback
	 * function if you need to run code for when the refresh is over.*/
	UFUNCTION(Category = "Item Query", BlueprintCallable)
	void RefreshItems(bool MultiThreadRefresh = false, bool OnlyRefreshRegisteredItems = false);
	
	/**Same as RefreshItems, but is always multithreaded and provides
	 * a callback for when the task is done.*/
	UFUNCTION(Category = "Item Query", BlueprintCallable)
	void RefreshItemsWithCallback(bool OnlyRefreshRegisteredItems, FQueryRefreshCallback Callback);
	UPROPERTY()
	FQueryRefreshCallback QueryRefreshCallback;

	/**Get the items that this query has registered.
	 * @UpdateCachedItems If true, we will automatically update all
	 * registered items to ensure you are getting the most up-to-date
	 * copy. You will only want to turn this to false if this query
	 * has been ran or refreshed recently and no items have been
	 * modified since you last ran it.
	 * @SortByContainerAndItemIndex Should the array be sorted by
	 * the items container and item index?*/
	UFUNCTION(Category = "Item Query", BlueprintCallable)
	TArray<FS_InventoryItem> GetItemsFromQuery(bool UpdateCachedItems = true, bool SortByContainerAndItemIndex = false);

	/**This is the query function that determines whether an item
	 * will be kept track of for future retrieval.*/
	UFUNCTION(Category = "Item Query", BlueprintCallable, BlueprintNativeEvent)
	bool DoesItemPassFilter(FS_InventoryItem Item);

private:

	UFUNCTION()
	void OnInventoryStarted();

	UFUNCTION()
	void OnInventoryStopped();
	
	UFUNCTION()
	void OnItemAdded(FS_InventoryItem Item, int32 ToIndex, FS_ContainerSettings ToContainer);
	
	UFUNCTION()
	void OnItemRemoved(FS_InventoryItem Item, FS_ContainerSettings FromContainer);
};
