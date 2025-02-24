// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Components/AC_FragmentManager.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif


UAC_FragmentManager::UAC_FragmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UAC_Inventory* UAC_FragmentManager::GetInventory()
{
	if(Inventory.Get())
	{
		return Inventory;
	}

	Inventory = Cast<UAC_Inventory>(GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
	return Inventory;
}

void UAC_FragmentManager::InitializeFragments_Implementation()
{
	TArray<FS_ContainerSettings>& Containers = GetInventory()->ContainerSettings;
	FS_InventoryItem BlankItem;
	
	for(auto& CurrentContainer : Containers)
	{
		for(auto& ContainerFragment : CurrentContainer.ContainerFragments)
		{
			if(!ContainerFragment.IsValid())
			{
				continue;
			}
			
			ContainerFragment.GetMutable<>().InitializeFragment(GetInventory(), CurrentContainer, BlankItem);
		}
		
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			for(auto& ItemFragment : CurrentItem.ItemFragments)
			{
				if(!ItemFragment.IsValid())
				{
					continue;
				}
				
				ItemFragment.GetMutable<>().InitializeFragment(GetInventory(), CurrentContainer, CurrentItem);
			}
		}
	}
}

bool UAC_FragmentManager::ValidateInstancedStructAsFragment(TInstancedStruct<FCoreFragment> Fragment)
{
	/**Blueprint note: Blueprints don't have their own version of TInstancedStruct, it's automatically
	 * converted to FInstancedStruct on their end, allowing them to put whatever struct they wish
	 * into the pin. Hence why we do this check, even though to C++ people this may look redudant,
	 * it is necessary for Blueprint support.*/
	if(!Fragment.GetScriptStruct()->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return false;
	}

	if(!Fragment.IsValid())
	{
		return false;
	}
	
	return true;
}

bool UAC_FragmentManager::ValidateFragmentsReplicationConditions(EObjectNetworkingMethod NetworkingMethod,
	TInstancedStruct<FCoreFragment> Fragment)
{
	if(Fragment.GetMutablePtr<>()->GetNetworkingMethod() == Both)
	{
		//Both is always allowed
		return true;
	}

	if(Fragment.GetMutablePtr<>()->GetNetworkingMethod() == NetworkingMethod)
	{
		return true;
	}

	return false;
}

void UAC_FragmentManager::AddFragmentToItem(FS_InventoryItem Item, TInstancedStruct<FCoreFragment> Fragment)
{
	if(!Item.IsValid())
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}

	//Check if the item already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//Item already has fragment
			return;
		}
	}

	//Don't add a fragment that isn't compatible with the item
	if(!Fragment.GetMutablePtr<>()->IsCompatibleWithItem(Item.UniqueID.ParentComponent, Item))
	{
		return;
	}

	/**Standalone or client-only fragments should instantly go to the internal function*/
	if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, Fragment))
	{
		Internal_AddFragmentToItem(Item.UniqueID, Fragment);
		return;
	}

	GetInventory()->C_AddItemToNetworkQueue(Item.UniqueID);
	S_AddFragmentToItem(Item.UniqueID, Fragment, GetOwner()->GetLocalRole());
}

void UAC_FragmentManager::S_AddFragmentToItem_Implementation(FS_UniqueID ItemID,
                                                             TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}

	//Check if the item already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//Item already has fragment
			return;
		}
	}

	//Don't add a fragment that isn't compatible with the item
	if(!Fragment.GetMutablePtr<>()->IsCompatibleWithItem(Item.UniqueID.ParentComponent, Item))
	{
		return;
	}

	if(ValidateFragmentsReplicationConditions(Server, Fragment))
	{
		/**This fragment is server-only. Just continue to the internal function
		 * and don't send any client RPC's.*/
		Internal_AddFragmentToItem(ItemID, Fragment);
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_AddFragmentToItem(ItemID, Fragment);
			}
			else
			{
				Internal_AddFragmentToItem(ItemID, Fragment);
				C_AddFragmentToItem(ItemID, Fragment);
			}
		}
		else
		{
			Internal_AddFragmentToItem(ItemID, Fragment);
			C_AddFragmentToItem(ItemID, Fragment);
		}
	}
	else
	{
		Internal_AddFragmentToItem(ItemID, Fragment);
		C_AddFragmentToItem(ItemID, Fragment);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
		{
			CurrentListener->GetFragmentManager()->C_AddFragmentToItem(ItemID, Fragment);
		}
	}
}

bool UAC_FragmentManager::S_AddFragmentToItem_Validate(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment,
	ENetRole CallerLocalRole)
{
	return true;
}

void UAC_FragmentManager::C_AddFragmentToItem_Implementation(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	Internal_AddFragmentToItem(ItemID, Fragment);
	GetInventory()->C_RemoveItemFromNetworkQueue(ItemID);
}

void UAC_FragmentManager::Internal_AddFragmentToItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		//Item couldn't be found, might have been destroyed since RPC call
		return;
	}

	//Check if the item already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//Item already has fragment
			return;
		}
	}
	
	//New instances MUST have a set type to create.
	// TInstancedStruct<FCoreFragment> NewInstancedStruct;
	// NewInstancedStruct.InitializeAs(Fragment); 
	ItemID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments.Add(Fragment);
}

void UAC_FragmentManager::RemoveFragmentFromItem(FS_InventoryItem Item, UScriptStruct* FragmentType)
{
	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return;
	}

	if(!FragmentType)
	{
		return;
	}

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return;
	}

	//Check if the item has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			/**Standalone or client-only fragments should instantly go to the internal function*/
			if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, InstancedStruct))
			{
				Internal_RemoveFragmentFromItem(Item.UniqueID, FragmentType);
				return;
			}

			GetInventory()->C_AddItemToNetworkQueue(Item.UniqueID);
			S_RemoveFragmentFromItem(Item.UniqueID, FragmentType, GetOwner()->GetLocalRole());
			return;
		}
	}
}

void UAC_FragmentManager::S_RemoveFragmentFromItem_Implementation(FS_UniqueID ItemID,
                                                                  UScriptStruct* FragmentType, ENetRole CallerLocalRole)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}

	if(!FragmentType)
	{
		return;
	}

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}
	
	//Check if the item has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			if(ValidateFragmentsReplicationConditions(Server, InstancedStruct))
			{
				/**This fragment is server-only. Just continue to the internal function
				 * and don't send any client RPC's.*/
				Internal_RemoveFragmentFromItem(ItemID, FragmentType);
				return;
			}
			
			//Fragment with associated type was found. Start removing it
			if(CallerLocalRole == ROLE_Authority)
			{
				if(IsValid(GetOwner()->GetInstigatorController()))
				{
					if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
					{
						Internal_RemoveFragmentFromItem(ItemID, FragmentType);
					}
					else
					{
						Internal_RemoveFragmentFromItem(ItemID, FragmentType);
						C_RemoveFragmentFromItem(ItemID, FragmentType);
					}
				}
				else
				{
					Internal_RemoveFragmentFromItem(ItemID, FragmentType);
					C_RemoveFragmentFromItem(ItemID, FragmentType);
				}
			}
			else
			{
				Internal_RemoveFragmentFromItem(ItemID, FragmentType);
				C_RemoveFragmentFromItem(ItemID, FragmentType);
			}
	
			for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
			{
				//Update all clients that are currently listening to this component's replication calls.
				if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
				{
					CurrentListener->GetFragmentManager()->C_RemoveFragmentFromItem(ItemID, FragmentType);
				}
			}

			return;
		}
	}
}

bool UAC_FragmentManager::S_RemoveFragmentFromItem_Validate(FS_UniqueID ItemID,
	UScriptStruct* FragmentType, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_FragmentManager::C_RemoveFragmentFromItem_Implementation(FS_UniqueID ItemID, UScriptStruct* FragmentType)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	Internal_RemoveFragmentFromItem(ItemID, FragmentType);
	GetInventory()->C_RemoveItemFromNetworkQueue(ItemID);
}

void UAC_FragmentManager::Internal_RemoveFragmentFromItem(FS_UniqueID ItemID, UScriptStruct* FragmentType)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		//Item couldn't be found, might have been destroyed since RPC call
		return;
	}

	//Check if the item already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			ItemID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments.RemoveSingle(InstancedStruct);
			return;
		}
	}
}

void UAC_FragmentManager::OverrideFragmentOnItem(FS_InventoryItem Item, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}

	//Check if the item already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			/**Standalone or client-only fragments should instantly go to the internal function*/
			if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, Fragment))
			{
				Internal_OverrideFragmentOnItem(Item.UniqueID, Fragment, AddIfMissing);
				return;
			}

			GetInventory()->C_AddItemToNetworkQueue(Item.UniqueID);
			S_OverrideFragmentOnItem(Item.UniqueID, Fragment, GetOwner()->GetLocalRole(), AddIfMissing);
			return;
		}
	}

	if(AddIfMissing)
	{
		/**Standalone or client-only fragments should instantly go to the internal function*/
		if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, Fragment))
		{
			Internal_OverrideFragmentOnItem(Item.UniqueID, Fragment, AddIfMissing);
			return;
		}

		GetInventory()->C_AddItemToNetworkQueue(Item.UniqueID);
		S_OverrideFragmentOnItem(Item.UniqueID, Fragment, GetOwner()->GetLocalRole(), AddIfMissing);
		return;
	}
}

void UAC_FragmentManager::S_OverrideFragmentOnItem_Implementation(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment,
	ENetRole CallerLocalRole, bool AddIfMissing)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}
	
	//Check if the item has the fragment
	bool ContinueWithRPC = false;
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			ContinueWithRPC = true;
			break;
		}
	}

	if(ContinueWithRPC || AddIfMissing)
	{
		if(ValidateFragmentsReplicationConditions(Server, Fragment))
		{
			/**This fragment is server-only. Just continue to the internal function
			 * and don't send any client RPC's.*/
			Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
			return;
		}
		
		//Fragment with associated type was found. Start removing it
		if(CallerLocalRole == ROLE_Authority)
		{
			if(IsValid(GetOwner()->GetInstigatorController()))
			{
				if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
				{
					Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
				}
				else
				{
					Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
					C_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
				}
			}
			else
			{
				Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
				C_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
			}
		}
		else
		{
			Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
			C_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
		}
	
		for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
		{
			//Update all clients that are currently listening to this component's replication calls.
			if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
			{
				CurrentListener->GetFragmentManager()->C_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
			}
		}
	}
}

bool UAC_FragmentManager::S_OverrideFragmentOnItem_Validate(FS_UniqueID ItemID,
	TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole, bool AddIfMissing)
{
	return true;
}

void UAC_FragmentManager::C_OverrideFragmentOnItem_Implementation(FS_UniqueID ItemID,
	TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	Internal_OverrideFragmentOnItem(ItemID, Fragment, AddIfMissing);
	GetInventory()->C_RemoveItemFromNetworkQueue(ItemID);
}

void UAC_FragmentManager::Internal_OverrideFragmentOnItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(!ItemID.ParentComponent)
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		//Item couldn't be found, might have been destroyed since RPC call
		return;
	}
	
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : ItemID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			InstancedStruct = Fragment;
			return;
		}
	}

	if(AddIfMissing)
	{
		ItemID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments.Add(Fragment);
	}
}

void UAC_FragmentManager::AddFragmentToContainer(FS_ContainerSettings Container, TInstancedStruct<FCoreFragment> Fragment)
{
	if(!Container.IsValid())
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}

	//Check if the container already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//Container already has fragment
			return;
		}
	}

	//Don't add a fragment that isn't compatible with the container
	if(!Fragment.GetMutablePtr<>()->IsCompatibleWithContainer(Container.UniqueID.ParentComponent, Container))
	{
		return;
	}

	/**Standalone or client-only fragments should instantly go to the internal function*/
	if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, Fragment))
	{
		Internal_AddFragmentToContainer(Container.UniqueID, Fragment);
		return;
	}
	
	S_AddFragmentToContainer(Container.UniqueID, Fragment, GetOwner()->GetLocalRole());
}

void UAC_FragmentManager::S_AddFragmentToContainer_Implementation(FS_UniqueID ContainerID,
                                                             TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		return;
	}

	//Check if the container already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//Container already has fragment
			return;
		}
	}

	//Don't add a fragment that isn't compatible with the container
	if(!Fragment.GetMutablePtr<>()->IsCompatibleWithContainer(Container.UniqueID.ParentComponent, Container))
	{
		return;
	}

	if(ValidateFragmentsReplicationConditions(Server, Fragment))
	{
		/**This fragment is server-only. Just continue to the internal function
		 * and don't send any client RPC's.*/
		Internal_AddFragmentToContainer(ContainerID, Fragment);
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_AddFragmentToContainer(ContainerID, Fragment);
			}
			else
			{
				Internal_AddFragmentToContainer(ContainerID, Fragment);
				C_AddFragmentToContainer(ContainerID, Fragment);
			}
		}
		else
		{
			Internal_AddFragmentToContainer(ContainerID, Fragment);
			C_AddFragmentToContainer(ContainerID, Fragment);
		}
	}
	else
	{
		Internal_AddFragmentToContainer(ContainerID, Fragment);
		C_AddFragmentToContainer(ContainerID, Fragment);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
		{
			CurrentListener->GetFragmentManager()->C_AddFragmentToContainer(ContainerID, Fragment);
		}
	}
}

bool UAC_FragmentManager::S_AddFragmentToContainer_Validate(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment,
	ENetRole CallerLocalRole)
{
	return true;
}

void UAC_FragmentManager::C_AddFragmentToContainer_Implementation(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ContainerID.ParentComponent))
	{
		return;
	}
	
	Internal_AddFragmentToContainer(ContainerID, Fragment);
}

void UAC_FragmentManager::Internal_AddFragmentToContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		//Container couldn't be found, might have been destroyed since RPC call
		return;
	}

	//Check if the container already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			//container already has fragment
			return;
		}
	}
	
	ContainerID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments.Add(Fragment);
}

void UAC_FragmentManager::RemoveFragmentFromContainer(FS_ContainerSettings Container, UScriptStruct* FragmentType)
{
	if(!Container.IsValid())
	{
		return;
	}

	if(!FragmentType)
	{
		return;
	}

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return;
	}

	//Check if the container has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			/**Standalone or client-only fragments should instantly go to the internal function*/
			if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, InstancedStruct))
			{
				Internal_RemoveFragmentFromContainer(Container.UniqueID, FragmentType);
				return;
			}
			
			S_RemoveFragmentFromContainer(Container.UniqueID, FragmentType, GetOwner()->GetLocalRole());
			return;
		}
	}
}

void UAC_FragmentManager::S_RemoveFragmentFromContainer_Implementation(FS_UniqueID ContainerID,
                                                                  UScriptStruct* FragmentType, ENetRole CallerLocalRole)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}

	if(!FragmentType)
	{
		return;
	}

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		//Container couldn't be found, might have been destroyed since RPC call
		return;
	}
	
	//Check if the container has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			//Fragment with associated type was found. Start removing it

			if(ValidateFragmentsReplicationConditions(Server, InstancedStruct))
			{
				/**This fragment is server-only. Just continue to the internal function
				 * and don't send any client RPC's.*/
				Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
				return;
			}
			
			if(CallerLocalRole == ROLE_Authority)
			{
				if(IsValid(GetOwner()->GetInstigatorController()))
				{
					if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
					{
						Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
					}
					else
					{
						Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
						C_RemoveFragmentFromContainer(ContainerID, FragmentType);
					}
				}
				else
				{
					Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
					C_RemoveFragmentFromContainer(ContainerID, FragmentType);
				}
			}
			else
			{
				Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
				C_RemoveFragmentFromContainer(ContainerID, FragmentType);
			}
	
			for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
			{
				//Update all clients that are currently listening to this component's replication calls.
				if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
				{
					CurrentListener->GetFragmentManager()->C_RemoveFragmentFromContainer(ContainerID, FragmentType);
				}
			}
		}

		return;
	}
}

bool UAC_FragmentManager::S_RemoveFragmentFromContainer_Validate(FS_UniqueID ContainerID,
	UScriptStruct* FragmentType, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_FragmentManager::C_RemoveFragmentFromContainer_Implementation(FS_UniqueID ContainerID, UScriptStruct* FragmentType)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ContainerID.ParentComponent))
	{
		return;
	}
	
	Internal_RemoveFragmentFromContainer(ContainerID, FragmentType);
}

void UAC_FragmentManager::Internal_RemoveFragmentFromContainer(FS_UniqueID ContainerID, UScriptStruct* FragmentType)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		//Container couldn't be found, might have been destroyed since RPC call
		return;
	}

	//Check if the container already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == FragmentType)
		{
			ContainerID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments.RemoveSingle(InstancedStruct);
			return;
		}
	}
}

void UAC_FragmentManager::OverrideFragmentOnContainer(FS_ContainerSettings Container, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(!Container.IsValid())
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}

	//Check if the container already has the fragment
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			if(UKismetSystemLibrary::IsStandalone(this))
			{
				Internal_OverrideFragmentOnContainer(Container.UniqueID, Fragment, AddIfMissing);
				return;
			}
			
			S_OverrideFragmentOnContainer(Container.UniqueID, Fragment, GetOwner()->GetLocalRole(), AddIfMissing);
			return;
		}
	}

	if(AddIfMissing)
	{
		/**Standalone or client-only fragments should instantly go to the internal function*/
		if(UKismetSystemLibrary::IsStandalone(this) || ValidateFragmentsReplicationConditions(Client, Fragment))
		{
			Internal_OverrideFragmentOnContainer(Container.UniqueID, Fragment, AddIfMissing);
			return;
		}
		
		S_OverrideFragmentOnContainer(Container.UniqueID, Fragment, GetOwner()->GetLocalRole(), AddIfMissing);
	}
}

void UAC_FragmentManager::S_OverrideFragmentOnContainer_Implementation(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment,
	ENetRole CallerLocalRole, bool AddIfMissing)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}

	if(!ValidateInstancedStructAsFragment(Fragment))
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		//Container couldn't be found, might have been destroyed since RPC call
		return;
	}
	
	//Check if the container has the fragment
	bool ContinueWithRPC = false;
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Container.ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			ContinueWithRPC = true;
			break;
		}
	}

	if(ContinueWithRPC || AddIfMissing)
	{
		if(ValidateFragmentsReplicationConditions(Server, Fragment))
		{
			/**This fragment is server-only. Just continue to the internal function
			 * and don't send any client RPC's.*/
			Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
			return;
		}
		
		//Fragment with associated type was found. Start removing it
		if(CallerLocalRole == ROLE_Authority)
		{
			if(IsValid(GetOwner()->GetInstigatorController()))
			{
				if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
				{
					Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
				}
				else
				{
					Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
					C_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
				}
			}
			else
			{
				Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
				C_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
			}
		}
		else
		{
			Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
			C_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
		}
	
		for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
		{
			//Update all clients that are currently listening to this component's replication calls.
			if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != GetInventory())
			{
				CurrentListener->GetFragmentManager()->C_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
			}
		}
	}
}

bool UAC_FragmentManager::S_OverrideFragmentOnContainer_Validate(FS_UniqueID ContainerID,
	TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole, bool AddIfMissing)
{
	return true;
}

void UAC_FragmentManager::C_OverrideFragmentOnContainer_Implementation(FS_UniqueID ContainerID,
	TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}

	if(!IsValid(ContainerID.ParentComponent))
	{
		return;
	}
	
	Internal_OverrideFragmentOnContainer(ContainerID, Fragment, AddIfMissing);
}

void UAC_FragmentManager::Internal_OverrideFragmentOnContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing)
{
	if(!ContainerID.ParentComponent)
	{
		return;
	}
	
	FS_ContainerSettings Container = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		//Container couldn't be found, might have been destroyed since RPC call
		return;
	}
	
	for(TInstancedStruct<FCoreFragment>& InstancedStruct : ContainerID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments)
	{
		if(InstancedStruct.GetScriptStruct() == Fragment.GetScriptStruct())
		{
			InstancedStruct = Fragment;
			return;
		}
	}

	if(AddIfMissing)
	{
		ContainerID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments.Add(Fragment);
	}
}

TArray<FS_InventoryItem> UAC_FragmentManager::GetItemsWithFragment(UScriptStruct* FragmentType)
{
	TArray<FS_InventoryItem> FoundItems;

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return FoundItems;
	}

	if(!GetInventory())
	{
		return FoundItems;
	}

	for(auto& CurrentContainer : GetInventory()->ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			for(auto& CurrentFragment : CurrentItem.ItemFragments)
			{
				if(CurrentFragment.GetScriptStruct() == FragmentType)
				{
					FoundItems.Add(CurrentItem);
					break;
				}
			}
		}
	}

	return FoundItems;
}

TArray<FS_ContainerSettings> UAC_FragmentManager::GetContainersWithFragment(UScriptStruct* FragmentType)
{
	TArray<FS_ContainerSettings> FoundContainers;

	if(!FragmentType->IsChildOf(FCoreFragment::StaticStruct()))
	{
		return FoundContainers;
	}

	if(!GetInventory())
	{
		return FoundContainers;
	}

	for(auto& CurrentContainer : GetInventory()->ContainerSettings)
	{
		for(auto& CurrentFragment : CurrentContainer.ContainerFragments)
		{
			if(CurrentFragment.GetScriptStruct() == FragmentType)
			{
				FoundContainers.Add(CurrentContainer);
				break;
			}
		}
	}

	return FoundContainers;
}

#if WITH_EDITOR

void UAC_FragmentManager::InventoryPostPropertyChange(FPropertyChangedEvent& PropertyChangedEvent, UAC_Inventory* InventoryComponent)
{
	FString StructName = PropertyChangedEvent.Property->GetOwnerStruct()->GetName();
	const FString PropertyName = PropertyChangedEvent.GetPropertyName().ToString();

	/**BUG AS OF 5.4:
	 * The below code does work. It will reset any incompatible fragments back to null.
	 * BUT the editor UI does NOT update to indicate this, leading people to believe
	 * the fragment is compatible.
	 * I will investigate this in 5.5, but for now, this feature is not working.*/
	
	if(PropertyName == GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemFragments)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FS_ContainerSettings, ContainerFragments))
	{
		for(auto& CurrentContainer : InventoryComponent->ContainerSettings)
		{
			for(int32 CurrentIndex = 0; CurrentIndex < CurrentContainer.ContainerFragments.Num(); CurrentIndex++)
			{
				if(!CurrentContainer.ContainerFragments[CurrentIndex].IsValid())
				{
					continue;
				}
				
				if(!CurrentContainer.ContainerFragments[CurrentIndex].GetMutablePtr<>()->IsCompatibleWithContainer(InventoryComponent, CurrentContainer))
				{
					FString Error = FString::Printf(TEXT("Fragment %s is not compatible with container. Reset to null."), *CurrentContainer.ContainerFragments[CurrentIndex].GetScriptStruct()->GetName());
					UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
					
					FNotificationInfo Info(FText::FromString(Error));
					Info.ExpireDuration = 3.0f;
					Info.bUseLargeFont = false;
					Info.bFireAndForget = true;
					Info.bUseSuccessFailIcons = true;
					FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);

					CurrentContainer.ContainerFragments[CurrentIndex].Reset();
					continue;
				}
				
				CurrentContainer.ContainerFragments[CurrentIndex].GetMutablePtr<>()->PostEditorPropertyChange(PropertyChangedEvent);
				
				for(auto& CurrentError : CurrentContainer.ContainerFragments[CurrentIndex].GetMutablePtr<>()->GetEditorErrorMessage())
				{
					FString Error = CurrentError;
					UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
					
					FNotificationInfo Info(FText::FromString(Error));
					Info.ExpireDuration = 3.0f;
					Info.bUseLargeFont = false;
					Info.bFireAndForget = true;
					Info.bUseSuccessFailIcons = true;
					FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
				}
			}
			
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				for(auto& ItemFragment : CurrentItem.ItemFragments)
				{
					if(!ItemFragment.IsValid())
					{
						continue;
					}
					
					if(!ItemFragment.GetMutablePtr<>()->IsCompatibleWithItem(InventoryComponent, CurrentItem))
					{
						FString Error = FString::Printf(TEXT("Fragment %s is not compatible with item. Reset to null."), *ItemFragment.GetScriptStruct()->GetName());
						UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
					
						FNotificationInfo Info(FText::FromString(Error));
						Info.ExpireDuration = 3.0f;
						Info.bUseLargeFont = false;
						Info.bFireAndForget = true;
						Info.bUseSuccessFailIcons = true;
						FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
						
						ItemFragment.Reset();
						continue;
					}
					ItemFragment.GetMutablePtr<>()->PostEditorPropertyChange(PropertyChangedEvent);

					for(auto& CurrentError : ItemFragment.GetMutablePtr<>()->GetEditorErrorMessage())
					{
						FString Error = CurrentError;
						UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
					
						FNotificationInfo Info(FText::FromString(Error));
						Info.ExpireDuration = 3.0f;
						Info.bUseLargeFont = false;
						Info.bFireAndForget = true;
						Info.bUseSuccessFailIcons = true;
						FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
					}
				}
			}
		}
	}
}

#endif