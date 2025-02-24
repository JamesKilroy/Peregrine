// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "IT_ItemAbility.generated.h"

/**Object to handle the ItemAbility Component
 */
UCLASS(DisplayName = "Item Ability")
class INVENTORYFRAMEWORKPLUGIN_API UIT_ItemAbility : public UIT_ItemComponentTrait
{
	GENERATED_BODY()

public:

	/**The tag query that the owner must pass (if any)
	 * The owners tags are acquired through the GetOwnersTags function
	 * inside the item ability component once it is created. */
	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadOnly)
	FGameplayTagQuery ActivationQuery;

	/**Allows you to loosely check if an ability can be activated.
	 * This should ALWAYS assume that the ability doesn't exist yet,
	 * so do not check for the abilities cooldown in here.
	 *
	 * This is primarily meant for things like "Is the player dead?"
	 * type conditions to help reduce the amount of Component being
	 * constructed, then possibly being instantly destroyed if the
	 * ability failed to activate.
	 * This can also help reduce RPC's to minimize network traffic.*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable, BlueprintPure, BlueprintNativeEvent)
	bool LooselyCheckCanActivateAbility(FS_InventoryItem Item);

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;

	virtual bool AllowMultipleCopiesInDataAsset_Implementation() override { return true; }
};
