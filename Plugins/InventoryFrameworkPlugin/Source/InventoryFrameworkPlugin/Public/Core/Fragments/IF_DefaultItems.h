// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IF_DefaultItems.generated.h"

USTRUCT(BlueprintType)
struct FItemStructArrayWrapper
{
	GENERATED_BODY()

	UPROPERTY(Category = "Default Items", EditAnywhere, BlueprintReadWrite)
	TArray<FS_InventoryItem> Items;
};

/**When the item is created and has its default containers
 * initialized, this fragment will automatically fill in
 * the Items array in the designated containers.
 *
 * This fragment is much safer than the ItemsContainers
 * fragment, because that fragment allows you to
 * freely assign all the variables. Which in the long
 * term, is unsafe. For example, you want to remove a
 * container from an item, which you would do in an item
 * asset. But if you used that fragment to override
 * what the asset provides, that item won't reflect that
 * change. This fragment won't have that issue as it
 * waits until the default containers are imported, then
 * populates the items array, which is what most people
 * want to modify anyway when they are overriding
 * the default containers. */
USTRUCT(BlueprintType, DisplayName = "🏹 Default Items")
struct FDefaultItemsFragment : public FItemFragment
{
	GENERATED_BODY()

	/**The default items to add to what container.
	 * Tne tag is the container identifier and the Items
	 * are what items will be added to that container after
	 * it has been imported.*/
	UPROPERTY(Category = "Default Items", EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow))
	TMap<FGameplayTag, FItemStructArrayWrapper> DefaultItems;
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIF_DefaultItems : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

};
