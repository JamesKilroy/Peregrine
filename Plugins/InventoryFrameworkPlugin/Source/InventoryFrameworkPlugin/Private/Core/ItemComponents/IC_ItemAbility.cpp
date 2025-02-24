// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/ItemComponents/IC_ItemAbility.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/ItemComponents/IT_ItemAbility.h"

UE_DEFINE_GAMEPLAY_TAG(IFP_EventActivateItemAbility, "IFP.Event.ActivateItemAbility");


// Sets default values for this component's properties
UIC_ItemAbility::UIC_ItemAbility()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIC_ItemAbility::ActivateEvent_Implementation(FGameplayTag Event, FItemComponentPayload Payload)
{
	Super::ActivateEvent_Implementation(Event, Payload);

	if(Event == IFP_EventActivateItemAbility)
	{
		TryActivateAbility(Payload);
	}
}

bool UIC_ItemAbility::TryApplyCooldown()
{
	TRACE_CPUPROFILER_EVENT_SCOPE("TryApplyCooldown")
	if(!CheckCooldown())
	{
		CooldownApplied();
		return true;
	}

	return false;
}

bool UIC_ItemAbility::CanActivateAbility_Implementation(FItemComponentPayload Payload)
{
	return !CheckCooldown();
}

void UIC_ItemAbility::TryActivateAbility(FItemComponentPayload Payload)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("TryActivateAbility")
	if(IsAbilityActive())
	{
		return;
	}
	
	bool PassesTagQuery = true;
	if(!Cast<UIT_ItemAbility>(DataObject)->ActivationQuery.IsEmpty() && !GetOwnersTags().IsEmpty())
	{
		PassesTagQuery = Cast<UIT_ItemAbility>(DataObject)->ActivationQuery.Matches(GetOwnersTags());
		if(!PassesTagQuery)
		{
			return;
		}
	}
	
	if(CanActivateAbility(Payload) && PassesTagQuery)
	{
		AbilityIsActive = true;
		AbilityActivated(Payload);
		if(UAC_Inventory* Inventory = GetInventoryComponent())
		{
			Inventory->ItemAbilityActivated.Broadcast(GetItemData(), this);
		}
	}
	else
	{
		ActivationFailed();
	}
}

void UIC_ItemAbility::EndAbility(bool WasCancelled)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("EndAbility")
	if(!IsAbilityActive())
	{
		return;
	}

	AbilityIsActive = false;
	AbilityEnded(WasCancelled);
	if(UAC_Inventory* Inventory = GetInventoryComponent())
	{
		Inventory->ItemAbilityEnded.Broadcast(GetItemData(), this);
	}
}

void UIC_ItemAbility::CancelAbility()
{
	EndAbility(true);
	AbilityCancelled();
}

void UIC_ItemAbility::DestroyComponent(bool bPromoteChildren)
{
	CancelAbility();
	
	Super::DestroyComponent(bPromoteChildren);
}

UIC_ItemAbility* UIC_ItemAbility::TryActivateItemAbility(FS_InventoryItem Item, UIT_ItemComponentTrait* AbilityObject,
                                                         AActor* InInstigator, FItemComponentPayload Payload)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("TryActivateItemAbility")
	if(!Item.IsValid() || !AbilityObject)
	{
		return nullptr;
	}

	if(UIT_ItemAbility* ItemAbilityObject = Cast<UIT_ItemAbility>(AbilityObject))
	{
		//Cheapest method of preventing unnecessary ability component being created
		//when the player should not be able to activate the ability, for example
		//if the player is dead, you would check that here
		if(!ItemAbilityObject->LooselyCheckCanActivateAbility(Item))
		{
			return nullptr;
		}
	}

	UAC_Inventory* Inventory = Item.UniqueID.ParentComponent;
	UIC_ItemAbility* ItemAbility = Cast<UIC_ItemAbility>(Inventory->GetItemComponent(Item, AbilityObject, true, InInstigator, IFP_EventActivateItemAbility, Payload));
	if(ItemAbility)
	{
		/**Ability might already exist, so the event won't get sent.
		 * In this case, manually try to activate the ability by
		 * sending a server RPC*/
		if(ItemAbility->CreationTime != ItemAbility->GetOwner()->GetGameTimeSinceCreation())
		{
			ItemAbility->S_ActivateEvent(IFP_EventActivateItemAbility, Payload);
		}
	}
	
	return ItemAbility;
}

UIC_ItemAbility* UIC_ItemAbility::TryActivateEquippedItemAbility(UAC_Inventory* Inventory, FGameplayTag EquipmentContainerIdentifier,
	TSubclassOf<UIT_ItemAbility> AbilityObjectClass, AActor* InInstigator, FItemComponentPayload Payload)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("TryActivateEquippedItemAbility")
	if(!EquipmentContainerIdentifier.IsValid() || !Inventory)
	{
		return nullptr;
	}

	bool FoundContainer = false;
	FS_ContainerSettings Container;
	int32 ContainerIndex;
	Inventory->GetContainerByIdentifier(EquipmentContainerIdentifier, Inventory->ContainerSettings, false, FoundContainer, Container, ContainerIndex);
	if(!FoundContainer)
	{
		return nullptr;
	}

	if(Container.Items.IsEmpty())
	{
		return nullptr;
	}

	UIT_ItemAbility* AbilityObjectInstance = Cast<UIT_ItemAbility>(UFL_InventoryFramework::GetTraitByClassForItem(Container.Items[0].ItemAsset, AbilityObjectClass, true));

	return TryActivateItemAbility(Container.Items[0], AbilityObjectInstance, InInstigator, Payload);
}

TArray<UIC_ItemAbility*> UIC_ItemAbility::GetItemsAbilities(FS_InventoryItem Item)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("GetItemsAbilities")
	
	TArray<UIC_ItemAbility*> Abilities;
	UFL_InventoryFramework::UpdateItemStruct(Item);
	
	for(auto& CurrentComponent : Item.ItemComponents)
	{
		if(!CurrentComponent)
		{
			continue;
		}
		
		if(UIC_ItemAbility* ItemAbility = Cast<UIC_ItemAbility>(CurrentComponent))
		{
			Abilities.Add(ItemAbility);
		}
	}
	
	return Abilities;
}

TArray<UIC_ItemAbility*> UIC_ItemAbility::GetAllItemAbilities(UAC_Inventory* Inventory)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("GetAllItemAbilities")
	TArray<UIC_ItemAbility*> Abilities;
	Inventory->GetOwner()->GetComponents(UIC_ItemAbility::StaticClass(), Abilities);
	return Abilities;
}


