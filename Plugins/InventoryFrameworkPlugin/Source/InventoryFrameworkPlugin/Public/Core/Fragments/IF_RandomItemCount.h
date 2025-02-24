// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IF_RandomItemCount.generated.h"

/**When the component is activated, use this fragment to
 * evaluate the random min/max count rather than what
 * is declared in the item asset.
 * Once the item has been created, this fragment is removed.*/
USTRUCT(BlueprintType, DisplayName = "🎲 Random Count")
struct FRandomItemCountFragment : public FItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FIntPoint MinMaxCount;

	virtual bool IsCompatibleWithItem(UObject* ItemOwner, const FS_InventoryItem& Item) override;

	/**Only the server cares about this fragment since clients
	 * are never responsible for assigning an item count.*/
	virtual EObjectNetworkingMethod GetNetworkingMethod() override
	{
		return EObjectNetworkingMethod::Server;
	};
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIF_RandomItemCount : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FRandomItemCountFragment GetRandomItemCountFragmentFromItem(FS_InventoryItem Item);

	/**Get the fragment from the ItemAsset -> ItemAssetFragments array.
	 * If @GetFromAssetFragments is set to false, this will then get
	 * the fragment from the ItemAsset -> ItemStructFragments array,
	 * which is mostly just used for editor tools. */	
	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FRandomItemCountFragment GetRandomItemCountFragmentFromItemAsset(UDA_CoreItem* ItemAsset, bool GetFromAssetFragments = true);
	
	//Attempt to convert @Struct to this fragment
	UFUNCTION(Category = "IFP|Fragments|Conversion", BlueprintCallable, BlueprintPure)
	static FRandomItemCountFragment ToRandomItemCountFragment(FInstancedStruct Struct);
};
