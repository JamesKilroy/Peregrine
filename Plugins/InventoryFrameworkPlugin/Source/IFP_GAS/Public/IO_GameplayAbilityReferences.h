// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AC_GASHelper.h"
#include "Abilities/GameplayAbility.h"
#include "Core/Traits/ItemTrait.h"

#include "IO_GameplayAbilityReferences.generated.h"



/**Object for storing a set of GameplayEffects and events for
 * granting and removing them. These events are handled through
 * the AC_GASHelper component.
 *
 * Docs:
 * https://inventoryframework.github.io/systems/AbilitySystem/#native-gas-support */
UCLASS(DisplayName = "GAS Abilities")
class IFP_GAS_API UIO_GameplayAbilityReferences : public UItemTrait
{
	GENERATED_BODY()

public:
	
	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere)
	TMap<TSoftClassPtr<UGameplayAbility>, FGASTagEvents> Abilities;

	/**Get all abilities that are listening for the @Event*/
	UFUNCTION(Category = "Attribute Set References", BlueprintCallable, BlueprintPure)
	TArray<TSoftClassPtr<UGameplayAbility>> GetAbilitiesForEvent(FGameplayTag Event);

	/**Go through all abilities and check if any contain
	 * either Grant or Remove tags with any of the @Events
	 * tags inside of them.*/
	UFUNCTION(Category = "Attribute Set References", BlueprintCallable, BlueprintPure)
	bool HasAnyTagEvents(FGameplayTagContainer Events);

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;
};
