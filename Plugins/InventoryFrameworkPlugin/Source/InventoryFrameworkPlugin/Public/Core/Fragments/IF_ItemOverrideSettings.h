// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "UObject/Object.h"
#include "IF_ItemOverrideSettings.generated.h"

/**Allows you to override certain aspects of the item,
 * such as its name or description.*/
USTRUCT(BlueprintType, DisplayName = "🔧 Override Settings")
struct FItemOverrideSettings : public FItemFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	//Soft object reference is important as texture files can easily be the largest file for an item.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> InventoryImage = nullptr;
	
	/**Data assets can have select default currencies. If any entry is added to this,
	 * that list will be ignored and this one will be used instead.
	 * For optimization, don't have the same currency multiple times in here.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(NoElementDuplicate), Category = "Item")
	TArray<UIDA_Currency*> AcceptedCurrenciesOverwrite;
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIF_ItemOverrideSettings : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FItemOverrideSettings GetOverrideSettingsFragmentFromItem(FS_InventoryItem Item);

	/**Get the fragment from the ItemAsset -> ItemAssetFragments array.
	 * If @GetFromAssetFragments is set to false, this will then get
	 * the fragment from the ItemAsset -> ItemStructFragments array,
	 * which is mostly just used for editor tools. */	
	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FItemOverrideSettings GetOverrideSettingsFragmentFromItemAsset(UDA_CoreItem* ItemAsset, bool GetFromAssetFragments = true);

	//Attempt to convert @Struct to TagFragment
	UFUNCTION(Category = "IFP|Fragments|Conversion", BlueprintCallable, BlueprintPure)
	static FItemOverrideSettings ToItemOverrideSettings(FInstancedStruct Struct);
};
