// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Components/ItemComponent.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UItemComponent::UItemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UItemComponent::OnRep_UniqueID(FS_UniqueID OldUniqueID)
{
	//Remove this component from the old items Component array
	UAC_Inventory* OldComponent = OldUniqueID.ParentComponent;
	if(IsValid(OldComponent))
	{
		FS_InventoryItem OldItem = OldComponent->GetItemByUniqueID(OldUniqueID);
		if(OldItem.IsValid())
		{
			if(UFL_InventoryFramework::AreItemDirectionsValid(OldItem.UniqueID, OldItem.ContainerIndex, OldItem.ItemIndex))
			{
				OldComponent->ContainerSettings[OldItem.ContainerIndex].Items[OldItem.ItemIndex].ItemComponents.RemoveSingle(this);
			}
		}
	}

	//Add this component to the new items component array
	UAC_Inventory* ParentComponent = UniqueID.ParentComponent;
	if(IsValid(ParentComponent))
	{
		FS_InventoryItem ParentItem = ParentComponent->GetItemByUniqueID(UniqueID);
		if(ParentItem.IsValid())
		{
			if(UFL_InventoryFramework::AreItemDirectionsValid(ParentItem.UniqueID, ParentItem.ContainerIndex, ParentItem.ItemIndex))
			{
				ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items[ParentItem.ItemIndex].ItemComponents.AddUnique(this);
			}
		}
	}
}


void UItemComponent::OnRep_Owner(AActor* OldOwner)
{
	Rename(nullptr, Owner);
	C_NewOwnerAssigned(OldOwner, Owner);
}

void UItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UItemComponent, UniqueID);
	DOREPLIFETIME(UItemComponent, bIsBusy);
	DOREPLIFETIME(UItemComponent, Instigator)
	DOREPLIFETIME(UItemComponent, Owner)
	DOREPLIFETIME(UItemComponent, DataObject)
}

void UItemComponent::AssignNewOwner_Implementation(AActor* NewOwner)
{
	if(!IsValid(NewOwner))
	{
		return;
	}
	
	//Just return if the same owner is tried to be assigned again.
	if(NewOwner == GetOwner())
	{
		return;
	}
	AActor* OldOwner = GetOwner();
	
	PreNewOwnerAssigned(OldOwner, NewOwner);

	Owner = NewOwner;
	Rename(nullptr, NewOwner);
	S_NewOwnerAssigned(OldOwner, NewOwner);
}

void UItemComponent::C_NotifyClientOfNewOwner_Implementation(AActor* OldOwner, AActor* NewOwner)
{
	Rename(nullptr, NewOwner);
	C_NewOwnerAssigned(OldOwner, NewOwner);
}

FS_InventoryItem UItemComponent::GetItemData()
{
	return UniqueID.ParentComponent->GetItemByUniqueID(UniqueID);
}

UAC_Inventory* UItemComponent::GetInventoryComponent()
{
	return UniqueID.ParentComponent;
}

void UItemComponent::DestroyComponent(bool bPromoteChildren)
{
	UAC_Inventory* Inventory = GetInventoryComponent();
	if(!Inventory)
	{
		return;
	}

	FS_InventoryItem Item = GetItemData();
	if(UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		Inventory->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemComponents.RemoveSingle(this);
	}

	Super::DestroyComponent(bPromoteChildren);
}

void UItemComponent::S_ActivateEvent_Implementation(FGameplayTag Event, FItemComponentPayload Payload)
{
	ActivateEvent(Event, Payload);
}

void UItemComponent::Unequipped_Implementation(const TArray<FName>& TriggerFilters, FS_InventoryItem OldItemData)
{
}

void UItemComponent::ActivateEvent_Implementation(FGameplayTag Event, FItemComponentPayload Payload)
{
}

bool UItemComponent::IsBusy_Implementation()
{
	return bIsBusy;
}

void UItemComponent::Equipped_Implementation(const TArray<FName>& TriggerFilters)
{
}

void UItemComponent::ItemUsed_Implementation()
{
}

void UItemComponent::LoadObjectAssets_Implementation()
{
}

void UItemComponent::StopComponent_Implementation(FGameplayTag StopResponse)
{
	UAC_Inventory* ParentComponent = GetInventoryComponent();;

	if(!ParentComponent)
	{
		return;
	}

	ParentComponent->ItemComponentStopped.Broadcast(UniqueID, this, Instigator, StopResponse);

	//Start cleaning up assets and so forth.
	Cleanup();
}

void UItemComponent::BroadcastItemComponentFinished_Implementation(FGameplayTag FinishResponse)
{
	UAC_Inventory* ParentComponent = GetInventoryComponent();;
	
	if(!ParentComponent)
	{
		return;
	}

	ParentComponent->ItemComponentFinished.Broadcast(UniqueID, this, Instigator, FinishResponse);
}

void UItemComponent::Cleanup_Implementation()
{
}
