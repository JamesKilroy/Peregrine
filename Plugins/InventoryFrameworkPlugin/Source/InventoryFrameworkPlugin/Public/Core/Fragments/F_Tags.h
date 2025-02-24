// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "F_Tags.generated.h"

/**Grants the ability to the container or item to have
 * tags and TagValues associated with them. This is one of
 * the most powerful systems of IFP and has covered hundreds
 * of gameplay cases.
 *
 * Docs:
 * https://inventoryframework.github.io/systems/tagsystem/ */
USTRUCT(BlueprintType, DisplayName = "🏷️ Tags and TagValues")
struct FTagFragment : public FCoreFragment
{
	GENERATED_BODY()

	/**Simple tags to associated with this item.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGameplayTagContainer Tags;

	/**Tags that can have values associated with them.
	 * For example, you might have a durability tag and want to give it a value.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "Tag"), Category = "Item")
	TArray<FS_TagValue> TagValues;
	
	virtual bool SupportsItemAsset() override { return true; }
};

UCLASS(Abstract, NotBlueprintable)
class UF_Tags : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(Category = "IFP|Fragments|Tags", BlueprintCallable)
	static FTagFragment GetTagFragmentFromItem(FS_InventoryItem Item);

	/**Get the fragment from the ItemAsset -> ItemAssetFragments array.
	 * If @GetFromAssetFragments is set to false, this will then get
	 * the fragment from the ItemAsset -> ItemStructFragments array,
	 * which is mostly just used for editor tools. */	
	UFUNCTION(Category = "IFP|Fragments|Tags", BlueprintCallable)
    static FTagFragment GetTagFragmentFromItemAsset(UDA_CoreItem* ItemAsset, bool GetFromAssetFragments = true);
	
	UFUNCTION(Category = "IFP|Fragments|Tags", BlueprintCallable)
	static FTagFragment GetTagFragmentFromContainer(FS_ContainerSettings Container);

	//Attempt to convert @Struct to TagFragment
	UFUNCTION(Category = "IFP|Fragments|Conversion", BlueprintCallable, BlueprintPure)
	static FTagFragment ToTagFragment(FInstancedStruct Struct);

	/**Adds tags to this items tag fragment.
	 * This is NOT how you should add tags to an item the majority of the time.
	 * Use the dedicated AddTagToItem function.
	 * This has no events, no replication.
	 * This is an example of how to add helper functions for custom fragments
	 * for either single player games OR for editor tools, which is how IFP
	 * uses this function inside the InventoryHelper.*/
	UFUNCTION(Category = "IFP|Fragments|Tags", BlueprintCallable)
	static void SimpleAddTagsToItemsTagFragment(FS_InventoryItem Item, FGameplayTagContainer Tags);

	/**Remove tags from this items tag fragment.
	 * This is NOT how you should remove tags from an item the majority of the time.
	 * Use the dedicated RemoveTagFromItem function.
	 * This has no events, no replication.
	 * This is an example of how to add helper functions for custom fragments
	 * for either single player games OR for editor tools, which is how IFP
	 * uses this function inside the InventoryHelper.*/
	UFUNCTION(Category = "IFP|Fragments|Tags", BlueprintCallable)
	static void SimpleRemoveTagsFromItemsTagFragment(FS_InventoryItem Item, FGameplayTagContainer Tags);
};
