// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include <Core/Actors/Parents/A_ItemActor.h>

#include "Core/Data/FL_InventoryFramework.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AA_ItemActor::AA_ItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Construct();
}

AA_ItemActor::AA_ItemActor(const FObjectInitializer& ObjectInitializer)
{
	Construct();
}

void AA_ItemActor::Construct()
{
	Inventory = CreateDefaultSubobject<UAC_Inventory>("Inventory Component");
}

void AA_ItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AA_ItemActor, RootItemID);
}

void AA_ItemActor::PassUniqueID_Implementation(FS_UniqueID UniqueID)
{
	II_Inventory::PassUniqueID_Implementation(UniqueID);

	RootItemID = UniqueID;
}

UAC_Inventory* AA_ItemActor::GetInventory()
{
	if(UFL_InventoryFramework::IsUniqueIDValid(RootItemID))
	{
		return RootItemID.ParentComponent;
	}
	else
	{
		//Item is not a proxy, ask for the inventory component.
		UAC_Inventory* FoundComponent;
		Execute_GetInventoryComponent(this, FoundComponent);

		//Just in case the GetInventoryComponent interface was overridden,
		//but no component was passed in. This should never happen.
		if(!IsValid(FoundComponent))
		{
			UFL_InventoryFramework::LogIFPMessage(this, "GetInventoryComponent was overridden, but no inventory component was passed in.");
			return Inventory;
		}
		return FoundComponent;
	}
}

FS_InventoryItem AA_ItemActor::GetRootItemData()
{
	//Check if this item is a proxy for the real item.
	if(RootItemID.IsValid())
	{
		FS_InventoryItem ItemData = RootItemID.ParentComponent->GetItemByUniqueID(RootItemID);
		if(ItemData.IsValid())
		{
			return ItemData;
		}
	}

	//If this item is NOT a proxy, check if container 0 is set to ThisActor and return that item.
	//Should always return this if it's an actor in the level that was placed by a developer
	//or is a dropped item.
	if(Inventory->ContainerSettings.IsValidIndex(0))
	{
		if(Inventory->ContainerSettings[0].Items.IsValidIndex(0) && Inventory->ContainerSettings[0].ContainerType == ThisActor)
		{
			return Inventory->ContainerSettings[0].Items[0];
		}
	}

	return FS_InventoryItem();
}

bool AA_ItemActor::IsProxyActor()
{
	return UFL_InventoryFramework::IsUniqueIDValid(RootItemID);
}

void AA_ItemActor::GetInventoryComponent_Implementation(UAC_Inventory*& Component)
{
	Component = Inventory;
}

void AA_ItemActor::BeginPlay()
{
	Super::BeginPlay();

	if(AllowBeginPlay)
	{
		if(ItemInstanceTemplate)
		{
			Inventory->ContainerSettings[0].Items[0].ItemInstance = ItemInstanceTemplate;
		}
		
		//Might want to overwrite the container settings,
		//this is mostly used when dropping an item and
		//spawning this actor.
		if(ContainerSettingsOverwrite.IsValidIndex(0))
		{
			Inventory->ContainerSettings = ContainerSettingsOverwrite;
			//UniqueID's might be valid, reset them so
			//StartComponent can function correctly.
			Inventory->ResetAllUniqueIDs();

			/**This array might be referencing objects that will
			 * want to be GC'd later on. Empty it so we don't
			 * force them to stay in memory.*/
			ContainerSettingsOverwrite.Empty();
		}

		//Designer might be manually activating the component,
		//most likely when the player is in range and can see
		//this actor.
		if(StartInventoryOnBeginPlay)
		{
			Inventory->StartComponent();
		}
	}
}

void AA_ItemActor::SetIsPreviewActor_Implementation(bool NewStatus, AActor* OriginalActor)
{
	OriginalActorIfPreview = OriginalActor;
	bIsPreviewActor = true;
}

bool AA_ItemActor::IsPreviewActor_Implementation()
{
	if(IsValid(GetOwner()))
	{
		if(GetOwner()->GetClass()->ImplementsInterface(UI_Inventory::StaticClass()))
		{
			/**In the case of recursive items, this actor might not
			 * be labelled as a preview actor, because it's attached
			 * to one, so we ask the owner recursively until we find
			 * out if the root actor is a preview actor.*/
			if(II_Inventory::Execute_IsPreviewActor(GetOwner()))
			{
				return true;
			}
		}
	}
	
	return IsValid(OriginalActorIfPreview) || bIsPreviewActor;
}

