// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Core/Components/ItemComponent.h"
#include "IC_ItemAbility.generated.h"

class UIT_ItemAbility;
UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_EventActivateItemAbility)

/**Item Component that behaves similar to GAS abilities and common
 * ability system architecture.
 * While GAS abilities should be used over this, there are some limits that
 * GAS abilities have that can't be overcome without HEAVILY modifying
 * GAS's architecture. This includes having abilities follow and treat
 * the item as the owner, where are GAS abilities do not seem to have support
 * for changing owners and are very limited in options to have multiple
 * instances of the same ability.
 *
 * This has a similar interface, as in:
 *	- Checking and applying a cooldown
 *	- Activating, ending and cancelling an ability
 *	- Checking tags for activation
 *
 * Docs:
 * http://inventoryframework.github.io/systems/abilitysystemintegration/#item-ability-driver */
UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYFRAMEWORKPLUGIN_API UIC_ItemAbility : public UItemComponent
{
	GENERATED_BODY()

	bool AbilityIsActive = false;

public:
	// Sets default values for this component's properties
	UIC_ItemAbility();

	virtual void ActivateEvent_Implementation(FGameplayTag Event, FItemComponentPayload Payload) override;

	/**Returns true if the cooldown is currently active*/
	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	bool CheckCooldown();

	/**Try to apply this abilities' cooldown.*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable)
	virtual bool TryApplyCooldown();

	/**The cooldown was successfully applied*/
	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	void CooldownApplied();
	
	UFUNCTION(Category = "Item Ability", BlueprintPure, BlueprintCallable, BlueprintImplementableEvent)
	float GetCurrentCooldownTime();

	UFUNCTION(Category = "Item Ability", BlueprintNativeEvent, BlueprintCallable)
	bool CanActivateAbility(FItemComponentPayload Payload = FItemComponentPayload());

	/**By default, this does not return anything. This is meant to hook into your
	 * tag system, for example GAS has a tag system in their ability system component.
	 * Some projects have a global tag system and some have their tags living on different
	 * actors, like the player controller or player state.
	 * This is meant to be overriden and return a tag container from your tag system.*/
	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	FGameplayTagContainer GetOwnersTags();

	/**Attempt to activate the ability. This can fail for a few reasons:
	 * 1. The ability is already active
	 * 2. The activation query from the AbilityObject failed
	 * 3. @CanActivateAbility returned false
	 *
	 * If any of these happen, then @ActivationFailed will be called */
	UFUNCTION(Category = "Item Ability", BlueprintCallable)
	virtual void TryActivateAbility(FItemComponentPayload Payload = FItemComponentPayload());

	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	void AbilityActivated(FItemComponentPayload Payload = FItemComponentPayload());

	/**An attempt was made to activate the ability, but failed*/
	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	void ActivationFailed();

	/**Request to end the ability*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable)
	virtual void EndAbility(bool WasCancelled = false);

	/**The ability has been requested to end. This is where
	 * you'd end any timers and remove any references.*/
	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	void AbilityEnded(bool WasCancelled = false);

	/**Cancel the ability. Will call EndAbility*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable)
	virtual void CancelAbility();

	UFUNCTION(Category = "Item Ability", BlueprintImplementableEvent, BlueprintCallable)
	void AbilityCancelled();

	UFUNCTION(Category = "Item Ability", BlueprintPure, BlueprintCallable)
	virtual bool IsAbilityActive() { return AbilityIsActive; }

	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	
	/**Attempt to activate an item ability.
	 * If you are calling this as a client, keep in mind that the return
	 * value will in most cases return null.
	 * It is advised to bind to the inventory components ItemAbilityActivated
	 * delegate if you need to execute code once it has been activated
	 * outside the ability.*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable, meta = (DefaultToSelf = "Instigator", ReturnDisplayName = "Ability"))
	static UIC_ItemAbility* TryActivateItemAbility(FS_InventoryItem Item, UIT_ItemComponentTrait* AbilityObject, AActor* InInstigator, FItemComponentPayload Payload = FItemComponentPayload());

	/**Attempt to activate an equipped items' ability.
	 * If you are calling this as a client, keep in mind that the return
	 * value will in most cases return null.
	 * It is advised to bind to the inventory components ItemAbilityActivated
	 * delegate if you need to execute code once it has been activated
	 * outside the ability.*/
	UFUNCTION(Category = "Item Ability", BlueprintCallable, meta = (DefaultToSelf = "Instigator", ReturnDisplayName = "Ability"))
	static UIC_ItemAbility* TryActivateEquippedItemAbility(UAC_Inventory* Inventory, FGameplayTag EquipmentContainerIdentifier, TSubclassOf<UIT_ItemAbility> AbilityObjectClass, AActor* InInstigator, FItemComponentPayload Payload = FItemComponentPayload());

	/**Get all instantiated abilities currently tied to the @Item*/
	UFUNCTION(Category = "Item Ability", BlueprintPure, BlueprintCallable)
	static TArray<UIC_ItemAbility*> GetItemsAbilities(FS_InventoryItem Item);

	UFUNCTION(Category = "Item Ability", BlueprintPure, BlueprintCallable)
	static TArray<UIC_ItemAbility*> GetAllItemAbilities(UAC_Inventory* Inventory);
};
