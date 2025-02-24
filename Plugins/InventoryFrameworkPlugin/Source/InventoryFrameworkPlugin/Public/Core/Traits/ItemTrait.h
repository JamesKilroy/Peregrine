// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/Interfaces/I_Validation.h"
#include "ItemTrait.generated.h"

class UDA_CoreItem;

/**Base class for adding arbitrary/contextual data to any item,
 * regardless of their category.
 *
 * Docs:
 * https://inventoryframework.github.io/classes-and-settings/traitsandcomponents/ */
UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow", "Default"))
class INVENTORYFRAMEWORKPLUGIN_API UItemTrait : public UObject, public II_Validation
{
	GENERATED_BODY()

public:

	/**Arbitrary tag you can associate this object to, used by AC_Inventory -> GetObjectByTag*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Settings")
	FGameplayTag TraitTag;
	
	/**Called when this object is added to an assets TraitsAndComponents array.
	 * Allows you to create more complicated class defaults depending on the owning item,
	 * for example if this object is responsible for a spawn chance, you might want
	 * to set the spawn chance higher or lower than the default depending on the items rarity.
	 *
	 * As of 5.2, there's an issue that if you have a function in the event graph
	 * that is meant to be editor only, it'll spit out an error. Because of this,
	 * I'm making this into a bool function so it doesn't go into the event graph
	 * to reduce confusion. It does NOT matter if this returns true or false.*/
	UFUNCTION(BlueprintNativeEvent, Category = "Developer Settings", meta = (DevelopmentOnly))
	bool AddedToItemAsset(UDA_CoreItem* ItemAsset);
	
	/**Many objects are built to be unique inside the data asset and aren't meant to have
	 * multiple copies inside of its item asset. By default this return false but can be
	 * overriden to return true, but your code must be built to understand that multiple
	 * copies could exist inside a data asset at a time.
	 *
	 * Keep in mind that if you change this for an object that already has multiple
	 * copies of it inside a data asset, you will need to update that data asset yourself.*/
	UFUNCTION(BlueprintNativeEvent, Category = "Developer Settings", meta = (DevelopmentOnly))
	bool AllowMultipleCopiesInDataAsset();
	
};
