// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IF_ItemContainers.generated.h"

/**Allows the item to have containers and children items.
 *
 * This fragment is only compatible with items that have
 * the GetDefaultContainers function overriden.
 *
 * In general, it is not advised to manually add this fragment
 * and start overriding the items default containers.
 * Because if you ever want to change the items default
 * containers inside the asset, those changes won't
 * reflect in here.
 *
 * Most of the time, all you want is to add default items
 * into the items array. For that, I recommend using
 * the DefaultItems fragment, which is much safer. */
USTRUCT(BlueprintType, DisplayName = "👜 Item Containers")
struct FItemContainersFragment : public FItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FS_ContainerSettings> Containers;

	virtual bool IsCompatibleWithItem(UObject* ItemOwner, const FS_InventoryItem& Item) override;
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIF_ItemContainers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FItemContainersFragment GetItemContainersFragmentFromItem(FS_InventoryItem Item);

	/**Get the fragment from the ItemAsset -> ItemAssetFragments array.
	 * If @GetFromAssetFragments is set to false, this will then get
	 * the fragment from the ItemAsset -> ItemStructFragments array,
	 * which is mostly just used for editor tools. */	
	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FItemContainersFragment GetItemContainersFragmentFromItemAsset(UDA_CoreItem* ItemAsset, bool GetFromAssetFragments = true);

	//Attempt to convert @Struct to TagFragment
	UFUNCTION(Category = "IFP|Fragments|Conversion", BlueprintCallable, BlueprintPure)
	static FItemContainersFragment ToItemContainersFragment(FInstancedStruct Struct);
};
