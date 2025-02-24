// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "O_ItemQueryBase.h"
#include "Components/ActorComponent.h"
#include "Core/Data/IFP_CoreData.h"
#include "AC_ItemQueryManager.generated.h"

class UO_ItemQueryBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQueriesRegistered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQueryRegistered, UO_ItemQueryBase*, ItemQuery);

/**Manager component for handling and storing item queries
 *
 * Docs:
 * https://inventoryframework.github.io/systems/itemquery/ */
UCLASS(ClassGroup=(IFP), meta=(BlueprintSpawnableComponent),
	HideCategories = (ComponentTick, ComponentReplication, Activation, Cooking, Replication, Navigation))
class IFP_ITEMQUERY_API UAC_ItemQueryManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAC_ItemQueryManager();

	/**Our item queries that will filter out a set of items from
	 * the inventory component through their custom filtering function.*/
	UPROPERTY(Category = "Item Query Manager", BlueprintReadOnly, Instanced, EditAnywhere)
	TArray<UO_ItemQueryBase*> ItemQueries;

	UPROPERTY(Category = "Item Query Manager", BlueprintReadOnly)
	TObjectPtr<UAC_Inventory> Inventory = nullptr;

	int32 QueuedQueries = 0;
	int32 QueuedQueriesFinished = 0;
	void IncrementQueriesFinished();
	//All queued queries have been registered. 
	UPROPERTY(Category = "Item Query Manager", BlueprintAssignable)
	FQueriesRegistered QueriesRegistered;

	UPROPERTY(Category = "Item Query Manager", BlueprintAssignable)
	FQueriesRegistered QueryRegistered;

	/**Register a query with this manager.
	 * @DoNotRefresh Do not refresh the query. This means it will not
	 * try to populate its registered items array instantly.
	 * @MultiThreadRefresh Whether we send the refresh task to
	 * a different thread.*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	void RegisterItemQuery(UO_ItemQueryBase* ItemQuery, bool DoNotRefresh = false, bool MultiThreadRefresh = true);

	/**Same as RegisterItemQuery, but is always multithreaded and
	 * provides a callback for when the task is finished.*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	void RegisterItemQueryWithCallback(UO_ItemQueryBase* ItemQuery, FQueryRefreshCallback Callback);

	/**Attempt to register @Item with all registered queries.
	 * Returns a list of all queries that accepted the item*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	TArray<UO_ItemQueryBase*> RegisterItemWithQueries(FS_InventoryItem Item);

	/**Attempt to unregister @Item with all registered queries.*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	void UnregisterItemWithQueries(FS_InventoryItem Item);

	/**Go through all registered queries until one of the appropriate
	 * class is found, then fetch the items from it.
	 * @UpdateCachedItems If true, we will automatically update all
	 * registered items to ensure you are getting the most up-to-date
	 * copy. You will only want to turn this to false if this query
	 * has been ran and no items have been modified since you last
	 * ran it, which should be very rare.
	 * @SortByContainerAndItemIndex Should the array be sorted by
	 * the items container and item index?*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	TArray<FS_InventoryItem> GetItemsFromQueryByClass(TSubclassOf<UO_ItemQueryBase> QueryClass, bool UpdateCachedItems = true,
		bool SortByContainerAndItemIndex = false);

	/**Go through all registered queries until one of the appropriate
	 * identifier is found, then fetch the items from it.
	 * @UpdateCachedItems If true, we will automatically update all
	 * registered items to ensure you are getting the most up-to-date
	 * copy. You will only want to turn this to false if this query
	 * has been ran and no items have been modified since you last
	 * ran it, which should be very rare.
	 * @SortByContainerAndItemIndex Should the array be sorted by
	 * the items container and item index?*/
	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	TArray<FS_InventoryItem> GetItemsFromQueryByIdentifier(FGameplayTag Identifier, bool UpdateCachedItems = true,
		bool SortByContainerAndItemIndex = false);

	UFUNCTION(Category = "Item Query manager", BlueprintCallable, meta=(DeterminesOutputType="QueryClass", DynamicOutputParam="Queries"))
	void GetQueriesByClass(TSubclassOf<UO_ItemQueryBase> QueryClass, TArray<UO_ItemQueryBase*>& Queries);

	UFUNCTION(Category = "Item Query manager", BlueprintCallable)
	TArray<UO_ItemQueryBase*> GetQueriesByIdentifier(FGameplayTag Identifier);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
};
