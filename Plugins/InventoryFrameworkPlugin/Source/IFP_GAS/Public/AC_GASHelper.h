// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "NativeGameplayTags.h"
#include "Components/ActorComponent.h"
#include "Core/Data/IFP_CoreData.h"
#include "AC_GASHelper.generated.h"


class UAttributeSet;
struct FActiveGameplayEffectHandle;
class UIO_GameplayEffectReferences;
class UIO_GameplayAbilityReferences;
class UIO_AttributeSetReferences;
class UGameplayAbility;
class UAbilitySystemComponent;
class UIO_GrantAbilities;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTagEventBroadcasted, FGameplayTag, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityGranted, TSubclassOf<UGameplayAbility>, Ability, FGameplayAbilitySpecHandle, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityRemoved, TSubclassOf<UGameplayAbility>, Ability);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeSetGranted, UAttributeSet*, AttributeSet);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeSetRemoved, TSubclassOf<UAttributeSet>, AttributeSet);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEffectApplied, TSubclassOf<UGameplayAbility>, Ability, FGameplayAbilitySpecHandle, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEffectRemoved, TSubclassOf<UGameplayAbility>, Ability);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(GASHELPER_Event_StartComponent)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GASHELPER_Event_ItemEquipped)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GASHELPER_Event_ItemUnequipped)

USTRUCT(BlueprintType)
struct FGASTagEvents
{
	GENERATED_BODY()

	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere)
	FGameplayTagContainer GrantEvents;

	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere)
	FGameplayTagContainer RemoveEvents;

	bool HasTagEvent(const FGameplayTagContainer& Tags) const
	{
		if(GrantEvents.HasAnyExact(Tags))
		{
			return true;
		}
	
		if(RemoveEvents.HasAnyExact(Tags))
		{
			return true;
		}
	
		return false;
	}
};

/**Wrapper struct to help with tracking an effect
 * handle to help with removing the effect.*/
USTRUCT()
struct FEffectTracker
{
	GENERATED_BODY()

	UPROPERTY()
	UDA_CoreItem* ItemAsset = nullptr;

	UPROPERTY()
	FGameplayTagContainer RemoveEvents;

	UPROPERTY()
	FActiveGameplayEffectHandle EffectHandle;

	FEffectTracker() {}

	FEffectTracker(UDA_CoreItem* InItemAsset, FGameplayTagContainer InRemoveEvents, FActiveGameplayEffectHandle InEffectHandle)
	{
		ItemAsset = InItemAsset;
		RemoveEvents = InRemoveEvents;
		EffectHandle = InEffectHandle;
	}
};

UCLASS(Blueprintable, DisplayName = "IFP GAS Helper", ClassGroup=(IFP), meta=(BlueprintSpawnableComponent))
class IFP_GAS_API UAC_GASHelper : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UAC_GASHelper();

	
	/**Attempt to both grant all abilities and effects from relevant items
	 * and bind any relevant delegates from the inventory component.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void StartHelperComponent();

	UPROPERTY(Category = "GAS Helper", BlueprintReadOnly)
	TObjectPtr<UAC_Inventory> Inventory = nullptr;

#pragma region Delegates
	UPROPERTY(Category = "GAS Helper", BlueprintAssignable, BlueprintCallable)
	FTagEventBroadcasted TagEventBroadcasted;
	
	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FAbilityGranted AbilityGranted;
	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FAbilityRemoved AbilityRemoved;

	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FAttributeSetGranted AttributeSetGranted;
	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FAttributeSetRemoved AttributeSetRemoved;

	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FEffectApplied EffectApplied;
	UPROPERTY(Category = "GAS Helper", BlueprintAssignable)
	FEffectRemoved EffectRemoved;
#pragma endregion

	/**Currently active effects applied by items.
	 * Used to properly remove them on remove events.*/
	UPROPERTY(transient)
	TArray<FEffectTracker> EffectHandles;

	/**Get all items with the GAS traits we care about in their item asset.
	 * By default, this simply goes through all containers and items.
	 * For some, you might want to override this and fetch the items through an item query.
	 * Keep in mind, since we are just dealing with item assets in this case, we do not
	 * need the full, fledged item struct, so this should not return duplicate item assets*/
	UFUNCTION(Category = "GAS Helper", BlueprintNativeEvent)
	TArray<UDA_CoreItem*> GetItemsForEvent(FGameplayTag Event);

	/**Get the ability system component. By default, this tries to get the
	 * component from the owner of this component. This can be overriden
	 * to redirect from where the ability component is retrieved from,
	 * for example the player pawn or controller.*/
	UFUNCTION(Category = "GAS Helper", BlueprintNativeEvent)
	UAbilitySystemComponent* GetAbilityComponent();

	/**Bind any relevant delegates for this component to listen to.
	 * By default, this will just listen to ItemEquipped and ItemUnequipped
	 * since that covers vast majority of cases. You might want to override
	 * this and customize what events this component will listen to.*/
	UFUNCTION(Category = "GAS Helper", BlueprintNativeEvent)
	void BindDefaultDelegates();

	/**Attempt to grant the items abilities, attributes and effects.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void ProcessItem(UDA_CoreItem* Item, FGameplayTagContainer Events);

	/**Get the items abilities and grant them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void GrantItemAbilities(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter);
	void GrantItemAbilitiesInternal(UIO_GameplayAbilityReferences* Abilities, FGameplayTagContainer GrantEventsFilter);

	/**Get the items abilities and remove them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void RemoveItemAbilities(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter);
	void RemoveItemAbilitiesInternal(UIO_GameplayAbilityReferences* Abilities, FGameplayTagContainer RemoveEventsFilter);

	/**Get the items attributes and grant them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void GrantItemAttributes(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter);
	void GrantItemAttributesInternal(UIO_AttributeSetReferences* Attributes, FGameplayTagContainer GrantEventsFilter);

	/**Get the items attributes and remove them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void RemoveItemAttributes(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter);
	void RemoveItemAttributesInternal(UIO_AttributeSetReferences* Attributes, FGameplayTagContainer RemoveEventsFilter);

	/**Get the items effects and grant them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void GrantItemEffects(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter);
	void ApplyItemEffectsInternal(UDA_CoreItem* Item, UIO_GameplayEffectReferences* Effects, FGameplayTagContainer GrantEventsFilter);

	/**Get the items effects and remove them.*/
	UFUNCTION(Category = "GAS Helper", BlueprintCallable)
	void RemoveItemEffects(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter);
	void RemoveItemEffectsInternal(UDA_CoreItem* Item, UIO_GameplayEffectReferences* Effects, FGameplayTagContainer RemoveEventsFilter);

#pragma region Delegates

	UFUNCTION()
	void OnTagEventBroadcasted(FGameplayTag Event);

	UFUNCTION()
	void OnInventoryStarted();

	//Delegate event for handling StartComponent broadcast
	UFUNCTION()
	void OnStartComponentEvent(FGameplayTag Event);

	UFUNCTION()
	void OnItemEquipped(FS_InventoryItem Item, const TArray<FName>& CustomTriggerFilters);

	UFUNCTION()
	void OnItemUnequipped(FS_InventoryItem ItemData, FS_InventoryItem OldItemData, const TArray<FName>& CustomTriggerFilters);

#pragma endregion

protected:
	
	virtual void BeginPlay() override;

};
