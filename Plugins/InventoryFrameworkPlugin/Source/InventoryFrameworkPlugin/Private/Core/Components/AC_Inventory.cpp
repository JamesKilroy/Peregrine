// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Components/AC_Inventory.h"

#include "InventoryFrameworkPlugin.h"
#include "Core/Components/AC_FragmentManager.h"
#include "Core/Components/ItemComponent.h"
#include "Engine/GameInstance.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/CF_CompatibilitySettings.h"
#include "Core/Fragments/CF_TileTags.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Fragments/IF_ItemContainers.h"
#include "Core/Fragments/F_Tags.h"
#include "Core/Fragments/IF_DefaultItems.h"
#include "Core/Fragments/IF_ItemOverrideSettings.h"
#include "Core/Fragments/IF_RandomItemCount.h"
#include "Core/Interfaces/I_Inventory.h"
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "Core/Widgets/W_Container.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Core/Items/DA_CoreItem.h"
#include "Core/Items/IDA_Currency.h"
#include "Core/Objects/Parents/ItemInstance.h"
#include "Core/Widgets/W_AttachmentParent.h"
#include "Engine/ActorChannel.h"
#include "Engine/AssetManager.h"
#include "LootTableSystem/Components/AC_LootTable.h"
#include "LootTableSystem/Data/FL_LootTableHelpers.h"
#include "Net/UnrealNetwork.h"

struct FTagFragment;
UE_DEFINE_GAMEPLAY_TAG(IFP_SkipValidation, "IFP.Initialization.SkipValidation");
UE_DEFINE_GAMEPLAY_TAG(IFP_IncludeLootTables, "IFP.Initialization.IncludeLootTables");
UE_DEFINE_GAMEPLAY_TAG(IFP_SpawnChanceValue, "IFP.Initialization.SpawnChance");
UE_DEFINE_GAMEPLAY_TAG(IFP_PriceOverrideValue, "IFP.Initialization.PriceOverrideValue");
UE_DEFINE_GAMEPLAY_TAG(IFP_MaxStackOverride, "IFP.Items.Attributes.MaxStackOverride");

// Sets default values for this component's properties
UAC_Inventory::UAC_Inventory()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...

	SetIsReplicatedByDefault(true);

	//Allows us to replicate the item item instances
	//through the inventory item struct
	bReplicateUsingRegisteredSubObjectList = true;
}

void UAC_Inventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAC_Inventory, TagsContainer)
	DOREPLIFETIME(UAC_Inventory, TagValuesContainer);
}

UAC_FragmentManager* UAC_Inventory::GetFragmentManager()
{
	if(FragmentManager.Get())
	{
		return FragmentManager;
	}

	if(GetOwner())
	{
		FragmentManager = Cast<UAC_FragmentManager>(GetOwner()->GetComponentByClass(FragmentManagerClass));
		if(FragmentManager.Get())
		{
			return FragmentManager;
		}
	}
	
	//Prevent component being constructed if no game instance is valid
	if(UGameplayStatics::GetGameInstance(this))
	{
		FragmentManager = NewObject<UAC_FragmentManager>(GetOwner(), FragmentManagerClass);
		if(FragmentManager.Get())
		{
			FragmentManager->RegisterComponent();
		}
	}
	
	return FragmentManager;
}

void UAC_Inventory::StartComponent(bool RemoveSkipValidationTags)
{
	if(!IsValid(GetOwner()))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Inventory component has no owner. Something has gone horribly wrong");
		return;
	}

	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddUObject(this, &UAC_Inventory::CleanupNullReferences);
	
	if(!GetOwner()->HasAuthority())
	{
		if(!Initialized)
		{
			C_RequestServerContainerData(true);
			return;
		}
	}
	if(Initialized)
	{
		return;
	}

	TArray<FS_ContainerSettings> ContainersToRemove;
	//We need to call ItemEquipStatusUpdated AFTER everything has been processed,
	//so keep a record of all items that were equipped.
	TArray<FS_InventoryItem> EquippedItems;

	if(RemoveSkipValidationTags)
	{
		for(auto& CurrentContainer : ContainerSettings)
		{
			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentContainer.ContainerFragments, true))
			{
				TagFragment->Tags.RemoveTag(IFP_SkipValidation);
			}
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, true))
				{
					TagFragment->Tags.RemoveTag(IFP_SkipValidation);
				}
				/* If we're loading from a save, this property will reset
				 * to null, but for us to convert to raw state correctly
				 * while BelongsToItem is filled, then this needs to
				 * be filled out for GetItemsContainers to work correctly.*/
				CurrentItem.UniqueID.ParentComponent = this;
			}
		}

		ConvertToRawState();
	}
	
	//We need containers and items to have their indexes set first.
	RefreshIndexes();

	//Initialize our fragments
	if(GetFragmentManager())
	{
		GetFragmentManager()->InitializeFragments();
	}
	
	//Populate the TileMap
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentContainer.ContainerFragments, true))
		{
			if(TagFragment->Tags.HasTag(IFP_SkipValidation))
			{
				continue;
			}
		}
		
		InitializeTileMap(CurrentContainer);
	}

	TArray<UAC_LootTable*> LootTables = UFL_LootTableHelpers::GetLootTableComponents(GetOwner());

	/**Before we start processing items, we generate ID's
	 * and update any BelongsToItem directions so when
	 * we start processing items, loot tables and so forth,
	 * everything is prepped for them.*/
	for(int32 ContainerIndex = 0; ContainerIndex < ContainerSettings.Num(); ContainerIndex++)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE("Initialize ID's")
		
		if(ContainerSettings[ContainerIndex].UniqueID.IdentityNumber > 0)
		{
			ContainerSettings[ContainerIndex].UniqueID.ParentComponent = this;
		}
		else
		{
			ContainerSettings[ContainerIndex].UniqueID = GenerateUniqueID();
		}

		for(int32 ItemIndex = 0; ItemIndex < ContainerSettings[ContainerIndex].Items.Num(); ItemIndex++)
		{
			FS_InventoryItem& CurrentItem = ContainerSettings[ContainerIndex].Items[ItemIndex];
			//Item is invalid. Just continue and allow the next Items loop
			//to handle the removal.
			if(CurrentItem.ItemAssetSoftReference.IsNull())
			{
				continue;
			}

			CurrentItem.ItemAsset = CurrentItem.ItemAssetSoftReference.LoadSynchronous();
			
			if(CurrentItem.UniqueID.IdentityNumber > 0)
			{
				CurrentItem.UniqueID.ParentComponent = this;
			}
			else
			{
				CurrentItem.UniqueID = GenerateUniqueID();
			}

			/**Add default tags and the tag values to the item, but don't override any tag values.
			 * We do this before the SkipValidation check, because if the game is updated
			 * and the item asset has a new set of default tags, we still want to apply those.*/
			UFL_InventoryFramework::AddDefaultTagsToItem(CurrentItem, false);
			UFL_InventoryFramework::AddDefaultTagValuesToItem(CurrentItem, false, false);
			
			/**Find out if the item can have containers. If so, process them*/
			TArray<FS_ContainerSettings> ItemDefaultContainers = CurrentItem.ItemAsset->GetDefaultContainers();
			if(UIF_ItemContainers::GetItemContainersFragmentFromItem(CurrentItem).Containers.IsValidIndex(0))
			{
				ItemDefaultContainers = UIF_ItemContainers::GetItemContainersFragmentFromItem(CurrentItem).Containers;
			}
			if(ItemDefaultContainers.IsValidIndex(0))
			{
				TArray<FS_ContainerSettings> ItemsContainers;
				//Find the items containers.
				for(auto& CurrentContainer2 : ContainerSettings)
				{
					if((CurrentContainer2.BelongsToItem.X == ContainerSettings[ContainerIndex].ContainerIndex && CurrentContainer2.BelongsToItem.Y == CurrentItem.ItemIndex) ||
						(CurrentContainer2.BelongsToItem.X == ContainerSettings[ContainerIndex].UniqueID.IdentityNumber && CurrentContainer2.BelongsToItem.Y == CurrentItem.UniqueID.IdentityNumber))
					{
						ItemsContainers.Add(CurrentContainer2);
						CurrentContainer2.BelongsToItem.X = ContainerSettings[ContainerIndex].UniqueID.IdentityNumber;
						CurrentContainer2.BelongsToItem.Y = CurrentItem.UniqueID.IdentityNumber;
					}
				}

				/**In some cases, a designer might have added only some containers
				 * to the component, or a designer has added more containers to an
				 * item in an update. Find out if we missed any and add those.*/
				FDefaultItemsFragment* DefaultItemsFragment = FindFragment<FDefaultItemsFragment>(CurrentItem.ItemFragments);
				for(auto& CurrentDefaultContainer : ItemDefaultContainers)
				{
					bool AlreadyAdded = false;
					for(auto& CurrentContainer3 : ItemsContainers)
					{
						if(CurrentContainer3.ContainerIdentifier == CurrentDefaultContainer.ContainerIdentifier)
						{
							AlreadyAdded = true;
							break;
						}
					}
					if(AlreadyAdded)
					{
						continue;
					}
						
					InitializeTileMap(CurrentDefaultContainer);
					CurrentDefaultContainer.BelongsToItem.X = ContainerSettings[ContainerIndex].UniqueID.IdentityNumber;
					CurrentDefaultContainer.BelongsToItem.Y = CurrentItem.UniqueID.IdentityNumber;
					CurrentDefaultContainer.ContainerIndex = ContainerSettings.Num();
					if(DefaultItemsFragment)
					{
						if(FItemStructArrayWrapper* ItemsArray = DefaultItemsFragment->DefaultItems.Find(CurrentDefaultContainer.ContainerIdentifier))
						{
							CurrentDefaultContainer.Items.Append(ItemsArray->Items);
						}
					}
					
					ContainerSettings.Add(CurrentDefaultContainer);
				}
			}

			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, false))
			{
				if(TagFragment->Tags.HasTag(IFP_IncludeLootTables))
				{
					for(auto& CurrentLootTable : CurrentItem.ItemAsset->GetLootTables())
					{
						TRACE_CPUPROFILER_EVENT_SCOPE("Spawn Loot table component")
						/**Since the item assets loot tables are instanced objects, we can't use that
						 * raw object, we have to create a new component using it as a template.*/
						LootTables.Add(NewObject<UAC_LootTable>(GetOwner(), CurrentLootTable->GetClass(),
							NAME_None, RF_NoFlags, CurrentLootTable));
						LootTables.Last()->ItemID = CurrentItem.UniqueID;
					}

					TagFragment->Tags.RemoveTag(IFP_IncludeLootTables);
				}
			}
		}
	}
	
	for(int32 CurrentTable = 0; CurrentTable < LootTables.Num(); CurrentTable++)
	{
		LootTables[CurrentTable]->PreInventoryInitialized(this);

		for(int32 CurrentIndex = 0; CurrentIndex < QueuedLootTableItems.Num(); CurrentIndex++)
		{
			FS_InventoryItem& Item = ContainerSettings[QueuedLootTableItems[CurrentIndex].ContainerIndex].Items[QueuedLootTableItems[CurrentIndex].ItemIndex];
			
			Item.UniqueID = GenerateUniqueID();
			
			/**Find out if the item can have containers. If so, process them*/
			TArray<FS_ContainerSettings> ItemDefaultContainers = Item.ItemAsset->GetDefaultContainers();
			if(ItemDefaultContainers.IsValidIndex(0))
			{
				/**In some cases, a designer might have added only some containers
				 * to the component, or a designer has added more containers to an
				 * item in an update. Find out if we missed any and add those.*/
				for(auto& CurrentDefaultContainer : ItemDefaultContainers)
				{
					InitializeTileMap(CurrentDefaultContainer);
					CurrentDefaultContainer.BelongsToItem.X = ContainerSettings[Item.ContainerIndex].UniqueID.IdentityNumber;
					CurrentDefaultContainer.BelongsToItem.Y = Item.UniqueID.IdentityNumber;
					CurrentDefaultContainer.ContainerIndex = ContainerSettings.Num();
					CurrentDefaultContainer.UniqueID = GenerateUniqueID();
					ContainerSettings.Add(CurrentDefaultContainer);
				}
			}

			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.ItemFragments, false))
			{
				if(TagFragment->Tags.HasTag(IFP_IncludeLootTables))
				{
					for(auto& CurrentLootTable : Item.ItemAsset->GetLootTables())
					{
						TRACE_CPUPROFILER_EVENT_SCOPE("Spawn Loot table component")
						/**Since the item assets loot tables are instanced objects, we can't use that
						 * raw object, we have to create a new component using it as a template.*/
						LootTables.Add(NewObject<UAC_LootTable>(GetOwner(), CurrentLootTable->GetClass(),
							NAME_None, RF_NoFlags, CurrentLootTable));
						LootTables.Last()->ItemID = Item.UniqueID;

						TagFragment->Tags.RemoveTag(IFP_IncludeLootTables);
					}
				}
			}
		}
		
		QueuedLootTableItems.Empty();
	}

	//Start initializing all items inside every container.
	for(int32 ContainerIndex = 0; ContainerIndex < ContainerSettings.Num(); ContainerIndex++)
	{
		/**V: I don't know why, I couldn't figure out why, but if I create a ref
		 * to the container we are working with like this:
		 * FS_ContainerSettings& Container = ContainerSettings[ContainerIndex];
		 * The container index would very randomly be a random integer
		 * and you could not remove items from the items array and when this happend,
		 * it would ALWAYS fail on the fourth item. Not the third, not the fifth,
		 * ALWAYS the fourth item in the array.
		 * It was eating up enough time to resolve that I decided to just ignore it
		 * and make this code a little bit uglier than it needs to be.*/
		
		/**In case a loot table added a container*/
		if(!ContainerSettings[ContainerIndex].UniqueID.IsValid())
		{
			ContainerSettings[ContainerIndex].UniqueID = GenerateUniqueID();
		}

		TArray<FS_InventoryItem> ItemsToRemove;

		for(int32 ItemIndex = 0; ItemIndex < ContainerSettings[ContainerIndex].Items.Num(); ItemIndex++)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Initialize item)
			FS_InventoryItem& CurrentItem = ContainerSettings[ContainerIndex].Items[ItemIndex];
			
			if(!IsValid(CurrentItem.ItemAsset))
			{
				//Invalid data asset
				ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but found no item asset"),
					ContainerIndex, ItemIndex));
				ItemsToRemove.Add(CurrentItem);
				continue;
			}

			/**In case a loot table added a item*/
			if(!CurrentItem.UniqueID.IsValid())
			{
				CurrentItem.UniqueID = GenerateUniqueID();
			}

			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, false))
			{
				if(TagFragment->Tags.HasTagExact(IFP_SkipValidation))
				{
					/**Item wishes to skip validation. If anything goes wrong
					 * after this, a designer or a bug has incorrectly applied
					 * this tag.*/
					TagFragment->Tags.RemoveTag(IFP_SkipValidation);
					if(ContainerSettings[ContainerIndex].ContainerType == Equipment)
					{
						EquippedItems.Add(CurrentItem);
					}

					/* Container might not have the skip validation, so this item needs
					 * to register its collision.*/
					if(!UFL_InventoryFramework::GetContainersTags(ContainerSettings[ContainerIndex]).HasTagExact(IFP_SkipValidation) && ContainerSettings[ContainerIndex].SupportsTileMap())
					{
						AddItemToTileMap(CurrentItem);
					}
				
					continue;
				}
			}

			//Generate data beforehand, so if the item failed to spawn, the delegate broadcast will get the info it needs.
			CurrentItem.ContainerIndex = ContainerSettings[ContainerIndex].ContainerIndex;
			CurrentItem.ItemIndex = ItemIndex;

			//Determine spawn chance
			if(!UKismetMathLibrary::RandomBoolWithWeight(UFL_InventoryFramework::GetItemsSpawnChance(CurrentItem)))
			{
				ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but it failed its spawn change"),
					ContainerIndex, ItemIndex));
				//Find the items containers. Since the items containers don't have the Unique ID's assigned yet, we have to manually find them.
				for(auto& CurrentContainer : ContainerSettings)
				{
					if(CurrentContainer.BelongsToItem.X == ContainerIndex && CurrentContainer.BelongsToItem.Y == CurrentItem.ItemIndex)
					{
						ContainersToRemove.Add(CurrentContainer);
					}
				}
				ItemsToRemove.Add(CurrentItem);
				continue;
			}
			else
			{
				//Item succeeded spawn chance. Check if a game session is valid
				//and remove the SpawnChance value. This is so we don't re-roll the chance
				//when we pick up the item or restart the component.
				if(IsValid(UGameplayStatics::GetGameInstance(this)))
				{
					Internal_RemoveTagValueFromItem(CurrentItem, IFP_SpawnChanceValue);
				}
			}

			//If we are a vendor, ensure the item has acceptable currencies. Also ensure that the item is not a currency 
			if(InventoryType == Vendor && !CurrentItem.ItemAsset->GetClass()->IsChildOf(UIDA_Currency::StaticClass()))
			{
				TArray<UIDA_Currency*> AcceptedCurrencies = UFL_InventoryFramework::GetAcceptedCurrencies(CurrentItem);
				if(!AcceptedCurrencies.IsValidIndex(0))
				{
					//Find the items containers. Since the items containers don't have the Unique ID's assigned yet, we have to manually find them.
					for(auto& CurrentContainer : ContainerSettings)
					{
						if(CurrentContainer.BelongsToItem.X == ContainerIndex && CurrentContainer.BelongsToItem.Y == CurrentItem.ItemIndex)
						{
							ContainersToRemove.Add(CurrentContainer);
						}
						
					}
					
					ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but item had no acceptable currencies to exchange for"),
						ContainerIndex, ItemIndex));
					ItemsToRemove.Add(CurrentItem);
					continue;
				}
			}

			/**Handle item count. If the count is 0 or less, that means this item has never
			 * been initialized before. So it's safe to say we want to evaluate the stack count.
			 * But if it has been initialized before, we don't want to run this, because it would
			 * mean that the count would be randomized every single time a save is loaded or
			 * the item is dropped.*/
			if(CurrentItem.ItemAsset->CanItemStack() && CurrentItem.Count <= 0)
			{
				bool FragmentsFromStruct = false;
				if(FRandomItemCountFragment* RandomCountFragment = FindFragmentFromItem<FRandomItemCountFragment>(CurrentItem, FragmentsFromStruct))
				{
					CurrentItem.Count = UKismetMathLibrary::RandomIntegerInRange(RandomCountFragment->MinMaxCount.X, RandomCountFragment->MinMaxCount.Y);
					//Only remove the fragment if it's part of the item struct.
					if(FragmentsFromStruct)
					{
						RemoveFragmentFromArray(CurrentItem.ItemFragments, RandomCountFragment);
					}
				}
				else
				{
					CurrentItem.Count = CurrentItem.ItemAsset->DefaultStack;
				}
			}
			else if (!CurrentItem.ItemAsset->CanItemStack())
			{
				//Item can't stack, set its count to 1
				CurrentItem.Count = 1;
			}

			//Random count went to 0 or less, remove the item.
			if(CurrentItem.Count <= 0)
			{
				ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but item count was less than 1"),
					ContainerIndex, ItemIndex));
				ItemsToRemove.Add(CurrentItem);
				continue;
			}

			if(!CheckCompatibility(CurrentItem, ContainerSettings[ContainerIndex]))
			{
				ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but item was incompatible with its container"),
					ContainerIndex, ItemIndex));
				ItemsToRemove.Add(CurrentItem);
				continue;
			}

			//Equipment and ThisActor types are much simpler, lots of logic can be skipped
			if(ContainerSettings[ContainerIndex].ContainerType == Equipment || ContainerSettings[ContainerIndex].ContainerType == ThisActor)
			{
				//Container is an equipment container, so we skip all tile checks and forcefully add this item.
				if(ContainerSettings[ContainerIndex].TileMap[0] == -1)
				{
					ContainerSettings[ContainerIndex].TileMap[0] = CurrentItem.UniqueID.IdentityNumber;
					CurrentItem.ItemIndex = 0;
					CurrentItem.TileIndex = 0;
										
					if(ContainerSettings[ContainerIndex].ContainerType == Equipment)
					{
						EquippedItems.Add(CurrentItem);
					}
				}
				
				//Since only one item can be in this equipment or CurrentItem container, immediately break out of the loop.
				break;
			}
			
			if(!ContainerSettings[ContainerIndex].SupportsTileMap())
			{
				//Container does not support positioning, skip all positioning logic.
				continue;
			}
			
			/**Item is in a container that can have multiple items, start finding a spot*/
			//Start finding a spot
			int32 StartAtIndex = CurrentItem.TileIndex == -1 ? 0 : CurrentItem.TileIndex;
			bool SpotFound = false;
			TArray<int32> TilesToIgnore = GetGenericIndexesToIgnore(ContainerSettings[ContainerIndex]);
			
			//find out if we check a specific tile.
			if(UKismetMathLibrary::SelectInt(-1, CurrentItem.TileIndex, CurrentItem.TileIndex == -1) > 0)
			{
				TArray<FS_InventoryItem> ItemsInTheWay;
				TArray<FS_InventoryItem> ItemsToIgnore;
				CheckAllRotationsForSpace(CurrentItem, ContainerSettings[ContainerIndex], StartAtIndex, ItemsToIgnore, TilesToIgnore, SpotFound, CurrentItem.Rotation, CurrentItem.TileIndex, ItemsInTheWay);
			}
			else
			{
				//Determine if we find a random tile or first available tile. -2 is random, -1 is first available.
				if(CurrentItem.TileIndex == -2)
				{
					TArray<FS_InventoryItem> ItemsInTheWay;
					//Attempt to find a free random spot, but limit it to 10 attempts.
					FIntPoint ContainerSize;
					UFL_InventoryFramework::GetContainerDimensions(ContainerSettings[ContainerIndex], ContainerSize.X, ContainerSize.Y);
					int32 ContainerLength = (ContainerSize.X * ContainerSize.Y) - 1;
					for(int32 LoopAttempt = 0; LoopAttempt < 10; LoopAttempt++)
					{
						UKismetMathLibrary::RandomIntegerInRange(0, ContainerLength);
						TArray<FS_InventoryItem> ItemsToIgnore;
						CheckAllRotationsForSpace(CurrentItem, ContainerSettings[ContainerIndex], UKismetMathLibrary::RandomIntegerInRange(0, ContainerLength),
							ItemsToIgnore, TilesToIgnore, SpotFound, CurrentItem.Rotation, CurrentItem.TileIndex, ItemsInTheWay);
						if(SpotFound)
						{
							break;
						}
					}
				}
				else
				{
					GetFirstAvailableTile(CurrentItem, ContainerSettings[ContainerIndex], TilesToIgnore, SpotFound, CurrentItem.TileIndex, CurrentItem.Rotation);
				}
			}

			if(SpotFound)
			{
				//A free spot was found. Add it to the tile map.
				AddItemToTileMap(CurrentItem);
			}
			//No free spot was found.
			else
			{
				//Find the items containers. Since the items containers don't have the Unique ID's assigned yet, we have to manually find them.
				for(auto& CurrentContainer : ContainerSettings)
				{
					if((CurrentContainer.BelongsToItem.X == ContainerIndex && CurrentContainer.BelongsToItem.Y == CurrentItem.ItemIndex))
					{
						ContainersToRemove.Add(CurrentContainer);
					}
				}
				
				ItemFailedSpawn.Broadcast(CurrentItem, FString::Printf(TEXT("Tried to initialize item (Container index: %d - Item Index: %d) but no free spot was found"),
					ContainerIndex, ItemIndex));
				ItemsToRemove.Add(CurrentItem);
			}
		}

		FTagFragment* TagFragment = FindFragment<FTagFragment>(ContainerSettings[ContainerIndex].ContainerFragments);
		if(TagFragment)
		{
			TagFragment->Tags.RemoveTag(IFP_SkipValidation);
		}

		for(auto& CurrentRemovingItem : ItemsToRemove)
		{
			TArray<FS_ContainerSettings> ItemsContainers;
			GetAllContainersAssociatedWithItem(CurrentRemovingItem, ItemsContainers);
			for(auto& CurrentContainer : ItemsContainers)
			{
				ContainerSettings.RemoveSingle(CurrentContainer);
			}
			
			ContainerSettings[ContainerIndex].Items.RemoveSingle(CurrentRemovingItem);
		}
		ItemsToRemove.Empty();

		UFL_InventoryFramework::SortItemsByIndex(ContainerSettings[ContainerIndex].Items, ContainerSettings[ContainerIndex].Items);
	}

	if(ContainersToRemove.IsValidIndex(0))
	{
		int32 ContainersRemoved = 0;
		for(auto& RemovingContainer : ContainersToRemove)
		{
			ContainerSettings.RemoveAt(RemovingContainer.ContainerIndex - ContainersRemoved);
			ContainersRemoved++;
		}
	}

	RefreshIndexes();
	Initialized = true;

	if(GetOwner()->Implements<UI_Inventory>())
	{
		if(!II_Inventory::Execute_IsPreviewActor(GetOwner()))
		{
			for(auto& CurrentContainer : ContainerSettings)
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(CreateItemItemInstance - Loop)
				for(auto& CurrentItem : CurrentContainer.Items)
				{
					
					UItemInstance* Template = CurrentItem.ItemInstance;
					if(!Template)
					{
						//No template can be found, check the asset
						if(CurrentItem.ItemAsset->ItemInstance.IsNull())
						{
							continue;
						}

						//Try to load and get the default object.
						Template = Cast<UItemInstance>(CurrentItem.ItemAsset->ItemInstance.LoadSynchronous()->GetDefaultObject());
						if(!Template)
						{
							//Neither item struct nor item asset has a template
							continue;
						}
					}
			
					if(Template->ConstructOnRequest)
					{
						//Object wants to be constructed during GetItemsInstance,
						//not during StartComponent
						continue;
					}
			
					CreateItemInstanceForItem(CurrentItem);
				}
			}
		}
	}

	/**Alert any systems that want to work with the initialized inventory
	 * data that everything is ready. Since we are about to initialize
	 * equipment, which can be heavy, this is the ideal place to run
	 * parallel work. Main example is the ItemQuery system*/
	StartMultithreadWork.Broadcast();

	for(auto& CurrentEquippedItem : EquippedItems)
	{
		UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(CurrentEquippedItem, true, TArray<FName>());
	}
	
	for(auto& CurrentTable : LootTables)
	{
		CurrentTable->PostInventoryInitialized(this);
	}

	ComponentStarted.Broadcast();

	if(!UKismetSystemLibrary::IsStandalone(this) && UKismetSystemLibrary::IsServer(this) && InventoryType == Player)
	{
		S_SendContainerDataToClient(true);
	}
}

void UAC_Inventory::RefreshIndexes()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RefreshIndexes)
	for(int32 ContainerIndex = 0; ContainerIndex < ContainerSettings.Num(); ContainerIndex++)
	{
		ContainerSettings[ContainerIndex].ContainerIndex = ContainerIndex;
		
		//Loop through all items in the container and set their ContainerIndex
		for (int32 ItemIndex = 0; ItemIndex < ContainerSettings[ContainerIndex].Items.Num(); ItemIndex++)
		{
			ContainerSettings[ContainerIndex].Items[ItemIndex].ContainerIndex = ContainerIndex;
			ContainerSettings[ContainerIndex].Items[ItemIndex].ItemIndex = ItemIndex;
		}
		
	}

	//If you are calling RefreshIndexes, you will always
	//want to refresh the ID map as RefreshIndexes falls
	//under all the same scenarios that you'd call
	//RefreshIDMap, but not the other way around.
	RefreshIDMap();
	
	UFL_InventoryFramework::SortContainers(ContainerSettings, ContainerSettings);
}

void UAC_Inventory::RefreshItemsIndexes(const FS_ContainerSettings Container)
{
	if(!ContainerSettings.IsValidIndex(Container.ContainerIndex))
	{
		return;
	}
	
	FS_ContainerSettings& ContainerRef = ContainerSettings[Container.ContainerIndex];
	if(!IsValid(ContainerRef.UniqueID.ParentComponent))
	{
		return;
	}

	TArray<FS_ContainerSettings> ProcessedContainers;
	UFL_InventoryFramework::SortItemsByIndex(ContainerRef.Items, ContainerRef.Items);
	for(int32 CurrentItem = 0; CurrentItem < ContainerRef.Items.Num(); CurrentItem++)
	{
		UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(ContainerRef.Items[CurrentItem]);
		
		ContainerRef.Items[CurrentItem].ItemIndex = CurrentItem;
		if(IsValid(ItemWidget))
		{
			ItemWidget->ItemsArrayIndex = CurrentItem;
		}

		AddUniqueIDToIDMap(ContainerRef.Items[CurrentItem].UniqueID, FIntPoint(Container.ContainerIndex, CurrentItem));
	}
}

void UAC_Inventory::InitializeTileMap(FS_ContainerSettings& Container)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(InitializeTileMap)
	//In case we've stopped the component and generating everything again, the tile map might already be populated.
	if(!UFL_InventoryFramework::GetContainersTags(Container).HasTagExact(IFP_SkipValidation))
	{
		Container.TileMap.Empty();
		Container.IndexCoordinates.Empty();

		if(!Container.SupportsTileMap())
		{
			return;
		}

		int32 CurrentIndex = 0;
		for(int32 ColumnY = 0; ColumnY < Container.Dimensions.Y; ColumnY++)
		{
			if(ColumnY < Container.Dimensions.Y)
			{
				for(int32 RowX = 0; RowX < Container.Dimensions.X; RowX++)
				{
					if(RowX < Container.Dimensions.X)
					{
						Container.TileMap.Add(-1);
						Container.IndexCoordinates.Add(FIntPoint(RowX, ColumnY), CurrentIndex);
						CurrentIndex++;
					}
				}
			}
		}
	}
	else
	{
		Container.IndexCoordinates.Empty();

		if(!Container.SupportsTileMap())
		{
			return;
		}

		int32 CurrentIndex = 0;
		for(int32 ColumnY = 0; ColumnY < Container.Dimensions.Y; ColumnY++)
		{
			if(ColumnY < Container.Dimensions.Y)
			{
				for(int32 RowX = 0; RowX < Container.Dimensions.X; RowX++)
				{
					if(RowX < Container.Dimensions.X)
					{
						Container.IndexCoordinates.Add(FIntPoint(RowX, ColumnY), CurrentIndex);
						CurrentIndex++;
					}
				}
			}
		}
	}
}

void UAC_Inventory::RefreshTileMap(FS_ContainerSettings& Container)
{
	if(!Initialized)
	{
		return;
	}

	if(!Container.SupportsTileMap())
	{
		return;
	}
	
	for(auto& CurrentItem : Container.Items)
	{
		if(CurrentItem.TileIndex != -1 && Container.TileMap.IsValidIndex(CurrentItem.TileIndex))
		{
			//Check if the item already is valid on the tile map. If it isn't, update it.
			if(CurrentItem.UniqueID.IdentityNumber != Container.TileMap[CurrentItem.TileIndex])
			{
				AddItemToTileMap(CurrentItem);
			}
		}
	}
}

void UAC_Inventory::RebuildTileMap(FS_ContainerSettings& Container)
{
	InitializeTileMap(Container);

	if(!Container.SupportsTileMap())
	{
		return;
	}
	
	for(auto& CurrentItem : Container.Items)
	{
		if(CurrentItem.TileIndex != -1 && Container.TileMap.IsValidIndex(CurrentItem.TileIndex) && CheckCompatibility(CurrentItem, Container))
		{
			//Check if the item already is valid on the tile map. If it isn't, update it.
			if(CurrentItem.UniqueID.IdentityNumber != Container.TileMap[CurrentItem.TileIndex])
			{
				AddItemToTileMap(CurrentItem);
			}
		}
	}
}

void UAC_Inventory::C_RequestServerContainerData_Implementation(bool CallServerDataReceived)
{
	S_SendContainerDataToClient(CallServerDataReceived);
}

void UAC_Inventory::S_SendContainerDataToClient_Implementation(bool CallServerDataReceived)
{
	if(!Initialized)
	{
		StartComponent();
	}

	/**Send a sanitized ContainerSettings to the client.
	 * This removes any data we don't want clients to have or is
	 * cheap to generate for clients but expensive to replicate,
	 * like the TileMap.*/
	C_ReceiveServerContainerData(UFL_InventoryFramework::SanitizeContainersForClientRPC(ContainerSettings), CallServerDataReceived);
}

void UAC_Inventory::C_ReceiveServerContainerData_Implementation(const TArray<FS_ContainerSettings> &ServerContainerSettings, bool CallServerDataReceived)
{
	ContainerSettings = ServerContainerSettings;

	//Clients receive container settings with no tile map, rebuild them.
	for(auto& CurrentContainer : ContainerSettings)
	{
		RebuildTileMap(CurrentContainer);
	}

	RefreshIDMap();
	
	Initialized = true;

	TArray<UItemComponent*> ItemComponents;
	GetItemComponentOwner()->GetComponents(ItemComponents);

	for(auto& CurrentComponent : ItemComponents)
	{
		UAC_Inventory* ParentComponent = CurrentComponent->UniqueID.ParentComponent;
		if(IsValid(ParentComponent))
		{
			FS_InventoryItem ParentItem = ParentComponent->GetItemByUniqueID(CurrentComponent->UniqueID);
			if(ParentItem.IsValid())
			{
				if(ParentComponent->ContainerSettings.IsValidIndex(ParentItem.ContainerIndex))
				{
					if(ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items.IsValidIndex(ParentItem.ItemIndex))
					{
						ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items[ParentItem.ItemIndex].ItemComponents.AddUnique(CurrentComponent);
					}
				}
			}
		}
	}

	if(CallServerDataReceived)
	{
		ServerInventoryDataReceived.Broadcast(GetOwner());
	}
}

void UAC_Inventory::StopComponent_Implementation()
{
	if(!Initialized)
	{
		return;
	}
	
	ComponentPreStop.Broadcast();
	
	//This function can be overriden in Blueprints in case there are any blueprint level widgets that need to be wiped.
	
	//Wipe reference to container widgets so they can be garbage collected.
	RemoveAllContainerWidgets();

	//Set initialized to false so StartComponent can be called again.
	Initialized = false;

	GeneratedItemIcons.Empty();

	//Go through all items in all containers and wipe their attachment widget reference and item components.
	for(auto& CurrentContainer : ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(!IsValid(CurrentItem.ItemAsset))
			{
				continue;
			}
			
			for(auto& CurrentObject : CurrentItem.ItemAsset->TraitsAndComponents)
			{
				if(!IsValid(CurrentObject))
				{
					continue;
				}

				if(UIT_ItemComponentTrait* ItemComponentTrait = Cast<UIT_ItemComponentTrait>(CurrentObject))
				{
					UItemComponent* ItemComponent = GetItemComponent(CurrentItem, ItemComponentTrait, false, GetOwner());
					if(ItemComponent)
					{
						const FGameplayTag StopResponse;
						ItemComponent->StopComponent(StopResponse);
						if(ItemComponent) //Component might have been destroyed instantly.
						{
							ItemComponent->DestroyComponent();
						}
					}
				}
			}

			UW_AttachmentParent* AttachmentWidget = UFL_InventoryFramework::GetItemsAttachmentWidget(CurrentItem, false);
			if(IsValid(AttachmentWidget))
			{
				AttachmentWidget->RemoveFromParent();
				AttachmentWidget->MarkAsGarbage();
			}

			if(IsValid(CurrentItem.Widget))
			{
				II_ExternalObjects::Execute_RemoveWidgetReferences(CurrentItem.Widget);
			}
			
			CurrentItem.Widget = nullptr;
		}

		if(IsValid(CurrentContainer.Widget))
		{
			II_ExternalObjects::Execute_RemoveWidgetReferences(CurrentContainer.Widget);
		}
		
		CurrentContainer.Widget = nullptr;
	}

	TArray<FS_ContainerSettings> NewContainerSettings = GetContainersForSaveState();

	ContainerSettings = NewContainerSettings;
	ComponentStopped.Broadcast();
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainersForSaveState()
{
	TArray<FS_ContainerSettings> CleanedUpContainers;

	for(auto& CurrentContainer : ContainerSettings)
	{
		//Make a copy, we don't want to modify the containers.
		FS_ContainerSettings ContainerCopy = CurrentContainer;
		for(auto& CurrentItem : ContainerCopy.Items)
		{
			//While the save file already can't save object references,
			//if we store object references inside the game instance they
			//won't get garbage collected properly.
			CurrentItem.UniqueID.ParentComponent = nullptr;
			CurrentItem.ItemComponents.Empty();
			CurrentItem.ExternalObjects.Empty();
			CurrentItem.Widget = nullptr;
			CurrentItem.ItemInstance = nullptr;

			/**If the component has already been initialized, we can
			 * safely assume that all validation has been processed,
			 * so we can skip it for when we load the save, and all
			 * loot tables have been processed.*/
			if(Initialized)
			{
				if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, true))
				{
					TagFragment->Tags.AddTagFast(IFP_SkipValidation);
					TagFragment->Tags.RemoveTag(IFP_IncludeLootTables);
				}
			}
		}
		
		ContainerCopy.UniqueID.ParentComponent = nullptr;
		ContainerCopy.Widget = nullptr;
		if(Initialized)
		{
			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ContainerCopy.ContainerFragments, true))
			{
				TagFragment->Tags.AddTagFast(IFP_SkipValidation);
			}
		}
		
		//Data has been cleaned, add it to the array.
		CleanedUpContainers.Add(ContainerCopy);
	}

	return CleanedUpContainers;
}

void UAC_Inventory::ResetAllUniqueIDs()
{
	//Update BelongsToItem directions before we wipe out the UniqueID's
	RefreshIndexes();
	for(auto& CurrentContainer : ContainerSettings)
	{
		for(auto& XContainer : ContainerSettings)
		{
			if(CurrentContainer.BelongsToItem.X == XContainer.UniqueID.IdentityNumber)
			{
				for(auto& YItem : XContainer.Items)
				{
					if(YItem.UniqueID.IdentityNumber == CurrentContainer.BelongsToItem.Y)
					{
						CurrentContainer.BelongsToItem.X = XContainer.ContainerIndex;
						CurrentContainer.BelongsToItem.Y = YItem.ItemIndex;
						break;
					}
				}
				break;
			}
		}
		
	}
	for(auto& CurrentContainer : ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			CurrentItem.UniqueID.IdentityNumber = 0;
			CurrentItem.UniqueID.ParentComponent = nullptr;
		}
		CurrentContainer.UniqueID.IdentityNumber = 0;
		CurrentContainer.UniqueID.ParentComponent = nullptr;
	}
}

TEnumAsByte<EComponentState> UAC_Inventory::GetComponentState()
{
	const bool bInGameplaySession = IsValid(UGameplayStatics::GetGameInstance(this));

	if(GetAllContainerWidgets().IsValidIndex(0))
	{
		return bInGameplaySession ? Gameplay : Editor;
	}
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.UniqueID.IdentityNumber > 0)
		{
			return bInGameplaySession ? Gameplay : Editor;
		}

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.UniqueID.IdentityNumber > 0)
			{
				return bInGameplaySession ? Gameplay : Editor;
			}
		}
	}

	return Raw;
}

void UAC_Inventory::ConvertFromRawStateToEditorState()
{
	const TEnumAsByte<EComponentState> CurrentComponentState = GetComponentState();

	if(CurrentComponentState == Editor)
	{
		return;
	}

	RefreshIndexes();

	for(auto& CurrentContainer : ContainerSettings)
	{
		CurrentContainer.UniqueID = GenerateUniqueID();

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.ItemAssetSoftReference.IsNull())
			{
				continue;
			}

			CurrentItem.ItemAsset = CurrentItem.ItemAssetSoftReference.LoadSynchronous();
			
			CurrentItem.UniqueID = GenerateUniqueID();
			//To optimize updating any containers that belong to this item,
			//we find out if this item can even have containers that belong to it.
			TSubclassOf<UW_AttachmentParent> AttachmentWidget = CurrentItem.ItemAsset->GetAttachmentWidgetClass();
			if(IsValid(AttachmentWidget))
			{
				//GetItemsContainers and UpdateItemsContainers will not work yet because the BelongsToItem is still set to a container index and item index.
				//We manually update them so when we do need to use those functions, they will work.
				TArray<FS_ContainerSettings> ItemsContainers;
				//Find the items containers.
				for(auto& ItemsCurrentContainer : ContainerSettings)
				{
					if(ItemsCurrentContainer.BelongsToItem.X == CurrentItem.ContainerIndex && ItemsCurrentContainer.BelongsToItem.Y == CurrentItem.ItemIndex)
					{
						ItemsContainers.Add(ItemsCurrentContainer);
					}
				}
				//Update them if we found any.
				if(ItemsContainers.IsValidIndex(0))
				{
					for(int32 CurrentContainerIndex = 0; CurrentContainerIndex < ItemsContainers.Num(); CurrentContainerIndex++)
					{
						ContainerSettings[ItemsContainers[CurrentContainerIndex].ContainerIndex].BelongsToItem.X = ContainerSettings[CurrentContainer.ContainerIndex].UniqueID.IdentityNumber;
						ContainerSettings[ItemsContainers[CurrentContainerIndex].ContainerIndex].BelongsToItem.Y = CurrentItem.UniqueID.IdentityNumber;
					}
				}
				else
				{
					UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item was found to have a valid attachment widget but did not have any containers that belonged to it."), true, true);
				}
			}
		}

		RebuildTileMap(CurrentContainer);
	}
}

void UAC_Inventory::ConvertToRawState()
{
	RefreshIndexes();
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			TArray<FS_ContainerSettings> ItemsContainers = GetItemsChildrenContainers(CurrentItem);
			if(ItemsContainers.IsValidIndex(0))
			{
				for(auto& CurrentItemContainer : ItemsContainers)
				{
					FS_ContainerSettings& ContainerRef = ContainerSettings[CurrentItemContainer.ContainerIndex];
					ContainerRef.BelongsToItem.X = CurrentContainer.ContainerIndex;
					ContainerRef.BelongsToItem.Y = CurrentItem.ItemIndex;
				}
			}
			
			//Object references have to be wiped
			CurrentItem.UniqueID.ParentComponent = nullptr;
			CurrentItem.UniqueID.IdentityNumber = 0;
			CurrentItem.ItemAsset = nullptr;
		}
		
		CurrentContainer.UniqueID.ParentComponent = nullptr;
		CurrentContainer.UniqueID.IdentityNumber = 0;
		CurrentContainer.TileMap.Empty();
		CurrentContainer.IndexCoordinates.Empty();
	}

	ID_Map.Empty();
	RemoveAllContainerWidgets();
	Listeners.Empty();
	NetworkQueue.Empty();
	WidgetRef = nullptr;
	Initialized = false;
}

void UAC_Inventory::RefreshIDMap()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RefreshIDMap)
	ID_Map.Empty();

	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.UniqueID.IsValid())
		{
			AddUniqueIDToIDMap(CurrentContainer.UniqueID, FIntPoint(CurrentContainer.ContainerIndex, -1), true);
		}

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.UniqueID.IsValid())
			{
				AddUniqueIDToIDMap(CurrentItem.UniqueID, FIntPoint(CurrentItem.ContainerIndex, CurrentItem.ItemIndex));
			}
		}
	}
}

void UAC_Inventory::AddUniqueIDToIDMap(FS_UniqueID UniqueID, FIntPoint Directions, bool IsContainer)
{
	ID_Map.Add(UniqueID.IdentityNumber, FS_IDMapEntry(IsContainer, Directions));
}

void UAC_Inventory::RemoveUniqueIDFromIDMap(FS_UniqueID UniqueID)
{
	ID_Map.Remove(UniqueID.IdentityNumber);
}

bool UAC_Inventory::ValidateIDMap(TArray<FS_ContainerSettings>& MissingContainers,
	TArray<FS_InventoryItem>& MissingItems, TArray<FS_UniqueID>& UnknownIDs, TArray<FS_UniqueID> &IncorrectDirections)
{
	MissingContainers.Empty();
	MissingItems.Empty();
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(!ID_Map.Contains(CurrentContainer.UniqueID.IdentityNumber))
		{
			MissingContainers.Add(CurrentContainer);
		}

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(!ID_Map.Contains(CurrentItem.UniqueID.IdentityNumber))
			{
				MissingItems.Add(CurrentItem);
			}
		}
	}

	for(auto& CurrentEntry : ID_Map)
	{
		bool MatchFound = false;
		for(auto& CurrentContainer : ContainerSettings)
		{
			if(CurrentContainer.UniqueID.IdentityNumber == CurrentEntry.Key)
			{
				//ID is valid. Validate the directions.
				if(CurrentEntry.Value.Directions != FIntPoint(CurrentContainer.ContainerIndex, -1))
				{
					IncorrectDirections.Add(CurrentContainer.UniqueID);
				}

				MatchFound = true;
				continue;
			}
			
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				if(CurrentItem.UniqueID.IdentityNumber == CurrentEntry.Key)
				{
					//ID is valid. Validate the directions.
					if(CurrentEntry.Value.Directions != FIntPoint(CurrentItem.ContainerIndex, CurrentItem.ItemIndex))
					{
						IncorrectDirections.Add(CurrentItem.UniqueID);
					}

					MatchFound = true;
					break;
				}
			}
			
			if(MatchFound)
			{
				break;
			}
		}

		//No container or item matched the ID
		if(!MatchFound)
		{
			UnknownIDs.Add(FS_UniqueID(CurrentEntry.Key, this));
		}
	}

	return true;
}

void UAC_Inventory::BroadcastNewAssignedUniqueID(FS_UniqueID OldID, FS_UniqueID NewID)
{
}

void UAC_Inventory::LoadItemAssets(TArray<FName> AssetBundles, FItemAssetLoaded OnItemAssetLoaded, FAllItemAssetsLoaded OnAllItemAssetsLoaded)
{
	if(Initialized)
	{
		//Only call this function if the component isn't initialized
		return;
	}
	
	UAssetManager& AssetManager = UAssetManager::Get();

	for(int32 CurrentContainer = 0; CurrentContainer < ContainerSettings.Num(); CurrentContainer++)
	{
		for(int32 CurrentItem = 0; CurrentItem < ContainerSettings[CurrentContainer].Items.Num(); CurrentItem++)
		{
			if(ContainerSettings[CurrentContainer].Items[CurrentItem].ItemAssetSoftReference.IsNull())
			{
				continue;
			}
			
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForPath(ContainerSettings[CurrentContainer].Items[CurrentItem].ItemAssetSoftReference.ToSoftObjectPath());
			TWeakObjectPtr WeakThis(this);
			
			AssetManager.LoadPrimaryAsset(
				AssetId,
				AssetBundles,
				FStreamableDelegate::CreateLambda([WeakThis, CurrentContainer, CurrentItem, OnItemAssetLoaded, OnAllItemAssetsLoaded]()
			{
				if(!WeakThis.IsValid())
				{
					//In case this component got marked as garbage mid async load.
					//Though I am somewhat sure the async load will cancel in this case.
					return;
				}
				
				if(WeakThis->Initialized)
				{
					/**In case the component got started during this async load.
					 * If that happens, StartComponent will force load everything,
					 * leading to this async load being redundant and should be ignored.*/
					return;
				}
				
				if(WeakThis->ContainerSettings.IsValidIndex(CurrentContainer))
				{
					if(WeakThis->ContainerSettings[CurrentContainer].Items.IsValidIndex(CurrentItem))
					{
						if(UDA_CoreItem* Item = WeakThis->ContainerSettings[CurrentContainer].Items[CurrentItem].ItemAssetSoftReference.Get())
						{
							WeakThis->ContainerSettings[CurrentContainer].Items[CurrentItem].ItemAsset = Item;
							OnItemAssetLoaded.ExecuteIfBound(Item, WeakThis->ContainerSettings[CurrentContainer].Items[CurrentItem], CurrentContainer, CurrentItem);
						}
					}
				}

				for(auto& CurrentContainerRef : WeakThis->ContainerSettings)
				{
					for(auto& CurrentItemRef : CurrentContainerRef.Items)
					{
						if(!CurrentItemRef.ItemAsset)
						{
							return;
						}
					}
				}

				//Above return was not hit. Everything is loaded
				OnAllItemAssetsLoaded.ExecuteIfBound();
			}));
		}
	}
	
}

void UAC_Inventory::CleanupNullReferences()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CleanupNullReferences)

	for(auto& CurrentContainer : ContainerSettings)
	{
		/**Remove null fragments*/
		for(int32 CurrentFragment = 0; CurrentFragment < CurrentContainer.ContainerFragments.Num(); CurrentFragment++)
		{
			if(!CurrentContainer.ContainerFragments[CurrentFragment].IsValid())
			{
				CurrentContainer.ContainerFragments.RemoveAt(CurrentFragment);
			}
		}

		/**Cleanup null external objects*/
		for(int32 CurrentObject = 0; CurrentObject < CurrentContainer.ExternalObjects.Num(); CurrentObject++)
		{
			if(!CurrentContainer.ExternalObjects[CurrentObject].Get())
			{
				CurrentContainer.ExternalObjects.RemoveAt(CurrentObject);
			}
		}
		
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			/**Remove null fragments*/
			for(int32 CurrentFragment = 0; CurrentFragment < CurrentItem.ItemFragments.Num(); CurrentFragment++)
			{
				if(!CurrentItem.ItemFragments[CurrentFragment].IsValid())
				{
					CurrentItem.ItemFragments.RemoveAt(CurrentFragment);
				}
			}

			/**Cleanup null external objects*/
			for(int32 CurrentObject = 0; CurrentObject < CurrentItem.ExternalObjects.Num(); CurrentObject++)
			{
				if(!CurrentItem.ExternalObjects[CurrentObject].Get())
				{
					CurrentItem.ExternalObjects.RemoveAt(CurrentObject);
				}
			}

			/**Cleanup null item components*/
			for(int32 CurrentObject = 0; CurrentObject < CurrentItem.ItemComponents.Num(); CurrentObject++)
			{
				if(!IsValid(CurrentItem.ItemComponents[CurrentObject]))
				{
					CurrentItem.ItemComponents.RemoveAt(CurrentObject);
				}
			}
		}
	}
}

void UAC_Inventory::MoveItem(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent, UAC_Inventory* ToComponent,
                             int32 ToContainer, int32 ToIndex, int32 Count, bool CallItemMoved, bool CallItemAdded,  bool SkipCollisionCheck, TEnumAsByte<ERotation> NewRotation)
{
	if(!IsValid(FromComponent) || !IsValid(ToComponent))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either FromComponent or ToComponent is invalid - AC_Inventory.cpp -> MoveItem"), true, true);
		return;
	}

	//Item is trying to move to the same location, just cancel the function.
	//Temporarily disabled, as ItemToMove.Rotated can't be used because of how the default system modifies the item struct while dragging the item.
	// if(ToContainer == ItemToMove.ContainerIndex && ToIndex == ItemToMove.TileIndex && FromComponent == ToComponent && ItemToMove.Rotated == NewRotation)
	// {
	// 	return;
	// }
	
	if(!UKismetSystemLibrary::IsServer(this))
	{
		//We try to do some validity checks before sending the server RPC,
		//as there is a small chance that if it fails, we can skip sending
		//a potentially expensive RPC.
		
		//Only vendors are allowed to move items inside their own container.
		//This will allow players to move items out of or into a vendors container though.
		if(InventoryType != Vendor && ToComponent->InventoryType == Vendor && FromComponent->InventoryType == Vendor)
		{
			return;
		}

		if(NetworkQueue.Contains(ItemToMove.UniqueID))
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to move item that was already in network queue - AC_Inventory.cpp -> MoveItem"));
			return;
		}

		if(ItemToMove.TileIndex == ToIndex && ItemToMove.ContainerIndex == ToContainer && ItemToMove.UniqueID.ParentComponent == ToComponent && ItemToMove.Rotation == NewRotation)
		{
			//Item is in the exact same location and rotation.
			C_RemoveItemFromNetworkQueue(ItemToMove.UniqueID);
			return;
		}
		
		FromComponent->C_AddItemToNetworkQueue(ItemToMove.UniqueID);
	}
	S_MoveItem(ItemToMove.UniqueID, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, GetOwner()->GetLocalRole());
}

bool UAC_Inventory::S_MoveItem_Validate(FS_UniqueID ItemToMove, UAC_Inventory* FromComponent,
	UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex, int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck,
	ERotation NewRotation, ENetRole CallerLocalRole)
{
	FS_InventoryItem Item = ItemToMove.ParentComponent->GetItemByUniqueID(ItemToMove);
	if(!Item.IsValid())
	{
		return true;
	}
	
	//Check if any of the data is dirty
	if(!IsValid(Item.ItemAsset) || Item.ItemIndex < 0 || Item.ContainerIndex < 0)
	{
		return false;
	}
	if(!ToComponent->ContainerSettings.IsValidIndex(ToContainer))
	{
		return false;
	}

	if(!UFL_InventoryFramework::IsTileMapIndexValid(ToIndex, ToComponent->ContainerSettings[ToContainer]))
	{
		return false;
	}

	//Check if From and this are players. If they are, do not allow this to modify another players component.
	if(FromComponent->InventoryType == Player && InventoryType == Player)
	{
		if(FromComponent != this)
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("From and To component are both set to *Player*, system does not allow a player to modify another players inventory. Modify S_MoveItem_Validate if you want to remove or expand this behavior."));
			return false;
		}
	}

	return true;
}

void UAC_Inventory::S_MoveItem_Implementation(FS_UniqueID ItemToMove, UAC_Inventory* FromComponent,
	UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex, int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, ERotation NewRotation, ENetRole CallerLocalRole)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("Move Item - Server")
	FS_InventoryItem Item = ItemToMove.ParentComponent->GetItemByUniqueID(ItemToMove);
	if(!Item.IsValid())
	{
		return;
	}
	
	if(!IsValid(FromComponent) || !IsValid(ToComponent))
	{
		C_RemoveItemFromNetworkQueue(ItemToMove);
		return;
	}

	if(!FromComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		C_RemoveItemFromNetworkQueue(ItemToMove);
		return;
	}

	if(!FromComponent->ContainerSettings[Item.ContainerIndex].Items.IsValidIndex(Item.ItemIndex))
	{
		C_RemoveItemFromNetworkQueue(ItemToMove);
		return;
	}

	/**Item might have been modified while being moved. This is technically a logic error rooted
	 * in the custom networking the system performs. We need to send the entire item struct to
	 * clients, because other clients might not have the needed data to perform the MoveItem
	 * function. This leads to the drag and drop logic holding onto a unique copy of the item the
	 * moment it is dragged. Then when its dropped, it is recreating the item in the new location
	 * by using the unique copy. But if the item count or something else is modified WHILE the
	 * drag and drop logic is happening, the unique copy is outdated.
	 *
	 * Here we try and find the original item and update the unique copy we are holding onto.
	 * TODO: Validate if this is needed anymore
	 */	
	if(FromComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].UniqueID == ItemToMove)
	{
		Item = FromComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex];
	}
	else
	{
		FromComponent->GetItemByUniqueID(ItemToMove);
		if(!ItemToMove.IsValid())
		{
			C_RemoveItemFromNetworkQueue(ItemToMove);
			return;
		}
	}

	if(Item.TileIndex == ToIndex && Item.ContainerIndex == ToContainer && Item.UniqueID.ParentComponent == ToComponent && Item.Rotation == NewRotation)
	{
		//Item is in the exact same location and rotation.
		C_RemoveItemFromNetworkQueue(ItemToMove);
		return;
	}

	//Sometimes people want to move a specific amount.
	//If no amount is specified, move the entire stack.
	if(Count <= 0)
	{
		Count = Item.Count;
	}

	FS_InventoryItem NewlyCreatedItem = Item;
	if(ToComponent->ContainerSettings[ToContainer].Style == DataOnly)
	{
		//Container is data only, skip all tile map interaction.
		ToIndex = -1;
		SkipCollisionCheck = true;
	}
	
	if(ToComponent->ContainerSettings[ToContainer].IsSpacialContainer())
	{
		NewlyCreatedItem.Rotation = NewRotation;
	}
	else
	{
		/**Container isn't spacial, reset rotation to 0*/
		NewlyCreatedItem.Rotation = Zero;
	}
	NewlyCreatedItem.TileIndex = ToIndex;
	NewlyCreatedItem.ContainerIndex = ToContainer;

	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	//If the container is infinite, first find a space. If none is available, expand it.
	TEnumAsByte<EContainerInfinityDirection> InfinityDirection;
	if(ToIndex < 0 && UFL_InventoryFramework::IsContainerInfinite(ToComponent->ContainerSettings[ToContainer], InfinityDirection))
	{
		if(FromComponent == ToComponent && Item.ContainerIndex == ToContainer)
		{
			return;
		}
		int32 AvailableTile;
		FS_InventoryItem ItemInTheWay;
		bool SpotAvailable = false;
		TArray<int32> IndexesToIgnore = ToComponent->GetGenericIndexesToIgnore(ToComponent->ContainerSettings[ToContainer]);
		ToComponent->GetFirstAvailableTile(NewlyCreatedItem, ToComponent->ContainerSettings[ToContainer], IndexesToIgnore, SpotAvailable, AvailableTile, NewlyCreatedItem.Rotation);
		if(SpotAvailable)
		{
			NewlyCreatedItem.TileIndex = AvailableTile;
			ToIndex = AvailableTile;
			NewRotation = NewlyCreatedItem.Rotation;
		}
		else
		{
			//No spot available, expand the container in the appropriate direction.
			FMargin Adjustments;
			if(InfinityDirection == X)
			{
				if(ToComponent->ContainerSettings[ToContainer].Style == Traditional)
				{
					Adjustments.Right = 1;
				}
				else
				{
					FIntPoint ItemDimension = UFL_InventoryFramework::GetItemDimensions(NewlyCreatedItem);
					Adjustments.Right = ItemDimension.X;
				}
			}
			else if(InfinityDirection == Y)
			{
				if(ToComponent->ContainerSettings[ToContainer].Style == Traditional)
				{
					Adjustments.Bottom = 1;
				}
				else
				{
					int32 ItemX;
					int32 ItemY;
					UFL_InventoryFramework::GetItemDimensionsWithContext(NewlyCreatedItem, ToComponent->ContainerSettings[ToContainer], ItemX, ItemY);
					Adjustments.Bottom = ItemY;
				}
			}
			ToComponent->Internal_AdjustContainerSize(ToComponent->ContainerSettings[ToContainer], Adjustments, false, Seed);
			ToComponent->GetFirstAvailableTile(NewlyCreatedItem, ToComponent->ContainerSettings[ToContainer], IndexesToIgnore, SpotAvailable, AvailableTile, NewlyCreatedItem.Rotation);
			NewlyCreatedItem.TileIndex = AvailableTile;
			ToIndex = AvailableTile;
			NewRotation = NewlyCreatedItem.Rotation;
		}
	}
	else
	{
		if(!SkipCollisionCheck)
		{
			int32 AvailableTile;
			TArray<FS_InventoryItem> ItemsInTheWay;
			bool SpotAvailable = false;
			TArray<FS_InventoryItem> ItemsToIgnore;
			TArray<int32> TilesToIgnore = GetGenericIndexesToIgnore(ToComponent->ContainerSettings[ToContainer]);
			ToComponent->CheckForSpace(NewlyCreatedItem, ToComponent->ContainerSettings[ToContainer], ToIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, AvailableTile, ItemsInTheWay);
			if(!SpotAvailable)
			{
				//The spot was not available, immediately return.
				return;
			}
		}
	}

	/**Try and process any containers the item owns, since the server is the only one who is aware of
	 * everyone's containers.
	 * For example, Client1 and Client2 are interacting with a chest and Client1 puts in a backpack,
	 * which has a container inside of it. Client2 won't be able to process the MoveItem request,
	 * because they are not aware of Client1's containers.*/
	TArray<FS_ContainerSettings> ItemContainers;
    if(FromComponent == ToComponent)
    {
    	//We only need the containers belonging to this item
	    ItemContainers = FromComponent->GetItemsChildrenContainers(Item);
    }
    else
    {
    	//We will need to update and move all containers and items belonging to this item in one go.
    	FromComponent->GetAllContainersAssociatedWithItem(Item, ItemContainers);
    }

	//Handle single player
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
		return;
	}

	//Handle replication.
	TArray<UAC_Inventory*> CombinedListeners;
	
	CombinedListeners.Add(ToComponent);
	
	
	for(auto& AppendingListener : FromComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : ToComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	
	for(const auto& CurrentListener : CombinedListeners)
	{
		if(IsValid(CurrentListener))
		{
			//Update all clients that are currently listening to this component's replication calls.
			if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
			{
				CurrentListener->C_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
			}
		}
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				ToComponent->Internal_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count,  CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
			}
			else
			{
				C_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
				ToComponent->Internal_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
			}
		}
		else
		{
			C_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
			ToComponent->Internal_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
		}
	}
	else
	{
		C_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
		ToComponent->Internal_MoveItem(Item, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, ItemContainers, Seed);
	}
}


void UAC_Inventory::C_MoveItem_Implementation(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent,
	UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex, int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, ERotation NewRotation,
	const TArray<FS_ContainerSettings> &ItemContainers, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	TArray<FS_ContainerSettings> NewContainers = ItemContainers;
	for(auto& CurrentContainer : NewContainers)
	{
		FS_ContainerSettings TempContainer = CurrentContainer.UniqueID.ParentComponent->GetContainerByUniqueID(CurrentContainer.UniqueID);
		if(TempContainer.IsValid())
		{
			CurrentContainer = TempContainer;
		}
	}
	
	Internal_MoveItem(ItemToMove, FromComponent, ToComponent, ToContainer, ToIndex, Count, CallItemMoved, CallItemAdded, SkipCollisionCheck, NewRotation, NewContainers, Seed);
	C_RemoveItemFromNetworkQueue(ItemToMove.UniqueID);
}

void UAC_Inventory::Internal_MoveItem(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent,
	UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex, int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck,
	TEnumAsByte<ERotation> NewRotation, TArray<FS_ContainerSettings> ItemContainers, FRandomStream Seed)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("Move Item")
	
	if(!IsValid(FromComponent) || !IsValid(ToComponent))
	{
		return;
	}

	if(Count < 0)
	{
		Count = ItemToMove.Count;
	}

	/**Widgets can't exist on the server and @ItemToMove was passed through the server,
	 * so if we are a client, the ExternalObjects array will have all widgets stripped out of it.
	 * This will try and get the latest item struct that the client has and override
	 * what the server provided to restore those widgets.*/
	if(!UKismetSystemLibrary::IsServer(GetOwner()) && !UKismetSystemLibrary::IsStandalone(GetOwner()))
	{
		ItemToMove.ExternalObjects = ItemToMove.UniqueID.ParentComponent->GetItemByUniqueID(ItemToMove.UniqueID).ExternalObjects;
		ItemToMove.Widget = UFL_InventoryFramework::GetWidgetForItem(ItemToMove);

		if(!ToComponent->ContainerSettings.IsValidIndex(ToContainer))
		{
			ToContainer = 0;
		}
	}
	
	FS_InventoryItem NewlyCreatedItem = ItemToMove;
	FS_ContainerSettings OldContainer = FromComponent->ContainerSettings.IsValidIndex(ItemToMove.ContainerIndex) ? FromComponent->ContainerSettings[ItemToMove.ContainerIndex] : FS_ContainerSettings();
	
	if(ToComponent->ContainerSettings[ToContainer].Style == Grid)
	{
		NewlyCreatedItem.Rotation = NewRotation;
	}
	else
	{
		//Traditional containers should always be 0 rotation
		NewlyCreatedItem.Rotation = Zero;
	}
	NewlyCreatedItem.TileIndex = ToIndex;
	NewlyCreatedItem.ContainerIndex = ToContainer;

	//If we are inside the same component and same container, there's very little we have to do.
	if(FromComponent == ToComponent && ItemToMove.ContainerIndex == ToContainer)
	{
		//Attempt to retrieve the item widget
		UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(FromComponent->ContainerSettings[ItemToMove.ContainerIndex]);
		UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(ItemToMove);
		if(IsValid(ItemWidget))
		{
			//Widget ref is getting reset for clients because widgets can't go through RPC's.
			//Reset both ItemToMove so delegates get the right data and update
			//the new item we are about to create.
			ItemToMove.Widget = ItemWidget;
			NewlyCreatedItem.Widget = ItemWidget;
		}

		if(Count < ItemToMove.Count)
		{
			if(ItemToMove.ItemAsset->CanItemStack())
			{
				TArray<int32> ItemsTiles;
				bool InvalidTilesFound;
				GetItemsTileIndexes(ItemToMove, ItemsTiles, InvalidTilesFound);
				if(!ItemsTiles.Contains(ToIndex))
				{
					NewlyCreatedItem.Count = UKismetMathLibrary::SelectInt(UKismetMathLibrary::Clamp(Count, Count, ItemToMove.Count), ItemToMove.Count, Count > 0);
					
					//We are trying to move an item, but not the entire stack. Split the item here.
					const int32 NewCount = FMath::Clamp(ItemToMove.Count - Count, 0, UFL_InventoryFramework::GetItemMaxStack(ItemToMove));

					//Start creating the new item
					FS_InventoryItem NewStackItem = ItemToMove;
					NewStackItem.Count = NewCount;
					NewStackItem.ItemIndex = ContainerSettings[ToContainer].Items.Num();
					NewStackItem.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);

					ToComponent->AddItemToTileMap(NewStackItem);
					ToComponent->ContainerSettings[ToContainer].Items.Add(NewStackItem);
				
					if(ContainerWidget)
					{
						ContainerWidget->CreateWidgetForItem(NewStackItem, ItemWidget);
					}

					UFL_ExternalObjects::BroadcastItemCountUpdated(ItemToMove, ItemToMove.Count, NewCount);
					
					if(NewCount == 0)
					{
						bool Success;
						Internal_RemoveItemFromInventory(ItemToMove, true, false, true, true, true, Seed, Success);
					}
				}
			}
		}
				
		//Move the item in the tile map.
		FromComponent->RemoveItemFromTileMap(ItemToMove);
		ToComponent->AddItemToTileMap(NewlyCreatedItem);
		ToComponent->ContainerSettings[ToContainer].Items[ItemToMove.ItemIndex] = NewlyCreatedItem;

		ToComponent->RefreshItemsIndexes(ToComponent->ContainerSettings[ToContainer]);

		//Update location, rotation in case we rotated the item during the move, then update
		//the size in case the new container is a different size from the old container.
		if(!IsValid(ItemWidget))
		{
			if(IsValid(ContainerWidget))
			{
				ContainerWidget->CreateWidgetForItem(NewlyCreatedItem, ItemWidget);
			}
		}
		
		UFL_ExternalObjects::BroadcastLocationUpdated(NewlyCreatedItem);
		UFL_ExternalObjects::BroadcastRotationUpdated(NewlyCreatedItem);
		UFL_ExternalObjects::BroadcastSizeUpdated(NewlyCreatedItem);
		UFL_ExternalObjects::BroadcastItemCountUpdated(ItemToMove, ItemToMove.Count, NewlyCreatedItem.Count);
		if(OldContainer.ContainerType == Equipment)
		{
			UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(NewlyCreatedItem, false, TArray<FName>());
		}
		
		if(CallItemMoved)
		{
			ToComponent->ItemMoved.Broadcast(ItemToMove, NewlyCreatedItem, FromComponent->ContainerSettings[ItemToMove.ContainerIndex],
				ToComponent->ContainerSettings[ToContainer], FromComponent, ToComponent);
		}

		if(CallItemAdded)
		{
			ToComponent->ItemAdded.Broadcast(NewlyCreatedItem, ToIndex, ToComponent->ContainerSettings[ToContainer]);
		}
		
		return;
	}

	//Since we are not inside the same component or the same container, there's a lot more we have to do.
	
	
	TArray<FS_ItemAndContainers> ProcessList; //A list of all items and their owning containers.
	bool bNewComponent = FromComponent != ToComponent;
	TArray<FS_ContainerSettings> ContainerWidgetsToUpdate;
	if(bNewComponent)
	{
		//It's easier to resolve the hierarchy if we start with items.
		TArray<FS_InventoryItem> ItemsToProcess;
		ItemsToProcess.Add(ItemToMove);
		UW_AttachmentParent* AttachmentWidget = UFL_InventoryFramework::GetItemsAttachmentWidget(NewlyCreatedItem, false);
		if(!ToComponent->Initialized)
		{
			AttachmentWidget = UFL_InventoryFramework::GetItemsAttachmentWidget(NewlyCreatedItem, false);
			if(IsValid(AttachmentWidget))
			{
				AttachmentWidget->RemoveFromParent();
			}
		}
		
		for(auto& CurrentContainer : ItemContainers)
		{
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				ItemsToProcess.Add(CurrentItem);
				if(!ToComponent->Initialized)
				{
					AttachmentWidget = UFL_InventoryFramework::GetItemsAttachmentWidget(CurrentItem, false);
					if(IsValid(AttachmentWidget))
					{
						AttachmentWidget->RemoveFromParent();
					}
				}
			}
		}

		//Accumulate a list to add to the ToComponent.
			
		for(auto& CurrentItem : ItemsToProcess)
		{
			FS_ItemAndContainers NewProcess;
			NewProcess.bIsRootItem = ProcessList.IsValidIndex(0) ? false : true;
			NewProcess.Item = CurrentItem;
			//Resolve which containers belong to CurrentItem and assign them
			for(auto& CurrentValidatedContainer : ItemContainers) 
			{
				if(CurrentValidatedContainer.BelongsToItem.Y == CurrentItem.UniqueID.IdentityNumber)
				{
					NewProcess.Containers.Add(CurrentValidatedContainer);
				}
			}
			ProcessList.Add(NewProcess);
		}
	}
	else
	{
		//Since the item isn't being moved to a new component, we only need to update the items root containers.
		//We don't need to update any children items containers.
		FS_ItemAndContainers NewProcess;
		NewProcess.bIsRootItem = true;
		NewProcess.Item = ItemToMove;
		NewProcess.Containers = ItemContainers;
		ProcessList.Add(NewProcess);
	}

	if(bNewComponent)
	{
		//Add all the containers to the new component
		TArray<FS_ContainerSettings> AddedContainers;
		for(auto& CurrentContainer : ItemContainers)
		{
			FS_ContainerSettings TempContainer = CurrentContainer.UniqueID.ParentComponent->GetContainerByUniqueID(CurrentContainer.UniqueID);
			if(TempContainer.IsValid())
			{
				CurrentContainer.Widget = TempContainer.Widget;
				CurrentContainer.ExternalObjects = TempContainer.ExternalObjects;
			}
			
			ToComponent->ContainerSettings.Add(CurrentContainer);
			AddedContainers.Add(ToComponent->ContainerSettings[ToComponent->ContainerSettings.Num() - 1]);
		}
	}
		
	for(auto& CurrentProcess : ProcessList)
	{
		//Root item has to be moved manually
		if(CurrentProcess.bIsRootItem)
		{
			bool CallUnequip = OldContainer.ContainerType == Equipment;
			UW_Container* ContainerWidget = nullptr;
			UW_InventoryItem* ItemWidget = nullptr;
			
			if(Count >= ItemToMove.Count)
			{
				//When two players are interacting with the same component, PlayerB does not have the information needed to remove the item
				//from PlayerA's inventory, so we check if we have the necessary data.
				if(FromComponent->ClientReceivedContainerData || UKismetSystemLibrary::IsServer(this) || FromComponent->Initialized)
				{
					bool RemoveItemSuccess;
					FromComponent->Internal_RemoveItemFromInventory(ItemToMove, false,
						CallUnequip, false, false, false, Seed, RemoveItemSuccess);
				}
			}
			else
			{
				if(ItemToMove.ItemAsset->CanItemStack())
				{
					NewlyCreatedItem.Count = UKismetMathLibrary::SelectInt(UKismetMathLibrary::Clamp(Count, Count, ItemToMove.Count), ItemToMove.Count, Count > 0);
					
					//We are trying to move an item, but not the entire stack. Split the item here.
					const int32 NewCount = FMath::Clamp(ItemToMove.Count - Count, 0, UFL_InventoryFramework::GetItemMaxStack(ItemToMove));
					FromComponent->ContainerSettings[ItemToMove.ContainerIndex].Items[ItemToMove.ItemIndex].Count = NewCount;

					if(!bNewComponent)
					{
						//The code after this ensures that if the destination is a new component,
						//we will generate a new ID. But since we are now essentially "splitting"
						//the item, we do this to ensure an ID is always generated
						NewlyCreatedItem.UniqueID = FromComponent->GenerateUniqueIDWithSeed(Seed);
					}

					//Update the count for the original item
					UFL_ExternalObjects::BroadcastItemCountUpdated(ItemToMove, ItemToMove.Count, NewCount);
					
					if(NewCount == 0)
					{
						bool Success;
						FromComponent->Internal_RemoveItemFromInventory(ItemToMove, CallUnequip, true, true, true, true, Seed, Success);
					}
				}
				else
				{
					bool RemoveItemSuccess;
					FromComponent->Internal_RemoveItemFromInventory(ItemToMove, false, CallUnequip, false, false, false, Seed, RemoveItemSuccess);
				}
			}

			//Add the new item to the ContainerSettings.Items array and the containers TileMap.
			NewlyCreatedItem.ItemIndex = ToComponent->ContainerSettings[ToContainer].Items.Num();
			if(bNewComponent)
			{
				NewlyCreatedItem.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);
				Seed.Initialize(Seed.GetInitialSeed() + 1);
			}

			if(OldContainer.ContainerType == Equipment)
			{
				if(FTagFragment* TagFragment = FindFragment<FTagFragment>(NewlyCreatedItem.ItemFragments, false))
				{
					TagFragment->Tags.RemoveTag(EquipTag);
				}
			}
			
			ToComponent->ContainerSettings[ToContainer].Items.Add(NewlyCreatedItem);
			ToComponent->AddItemToTileMap(NewlyCreatedItem);
			
			if(bNewComponent)
			{
				if(UKismetSystemLibrary::IsServer(this))
				{
					//Only the server should update item components ownership.
					for(auto& CurrentComponent : NewlyCreatedItem.ItemComponents)
					{
						//Component might be in the middle of garbage collection
						if(IsValid(CurrentComponent))
						{
							//Only components that are set to follow should follow the item.
							if(CurrentComponent->DataObject->FollowItem)
							{
								CurrentComponent->UniqueID = NewlyCreatedItem.UniqueID;
								CurrentComponent->AssignNewOwner(ToComponent->GetOwner());
							}
						}
					}
				}

				if(UItemInstance* ItemInstance = ItemToMove.ItemInstance)
				{
					ItemInstance->ItemID = NewlyCreatedItem.UniqueID;
					if(UKismetSystemLibrary::IsServer(this))
					{
						ItemInstance->GetOwningActor()->RemoveReplicatedSubObject(ItemInstance);
						ItemInstance->SetOwner(ToComponent->GetOwner());
						AddReplicatedSubObject(ItemInstance, ItemInstance->ReplicationCondition);
						ItemInstance->OnRep_ItemID(ItemToMove.UniqueID);
					}
				}
			}
			
			if(bNewComponent)
			{
				for(auto& CurrentContainer : CurrentProcess.Containers)
				{
					int32 ContainerIndex = ToComponent->ContainerSettings.Find(CurrentContainer);
					if(ContainerIndex == -1)
					{
						continue;
					}
					
					FS_ContainerSettings& ContainerRef =  ToComponent->ContainerSettings[ContainerIndex];
					ContainerRef.BelongsToItem.X = ToComponent->ContainerSettings[ToContainer].UniqueID.IdentityNumber;
					ContainerRef.BelongsToItem.Y = NewlyCreatedItem.UniqueID.IdentityNumber;
					
					ContainerRef.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);
					Seed.Initialize(Seed.GetInitialSeed() + 1);
					ContainerWidgetsToUpdate.Add(ContainerRef);
				}
			}
			else
			{
				for(auto& CurrentItemContainer : CurrentProcess.Containers)
				{
					//Update the containers BelongsToItem.X direction. We don't need to update Y because the item has not been assigned a new UniqueID.
					ToComponent->ContainerSettings[CurrentItemContainer.ContainerIndex].BelongsToItem.X = ToComponent->ContainerSettings[ToContainer].UniqueID.IdentityNumber;
				}
			}
			
			//If widget container is valid, attempt to create the item widget.
			ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(ToComponent->ContainerSettings[ToContainer]);
			if(IsValid(ContainerWidget))
			{
				ContainerWidget->CreateWidgetForItem(NewlyCreatedItem, ItemWidget);
			}
			ToComponent->RefreshItemsIndexes(ToComponent->ContainerSettings[ToContainer]);
		}
		else
		{
			if(bNewComponent)
			{
				//If the current item has containers, we need to find out what container is holding it in the ToComponent,
				//and update it and the containers the item owns.
				if(CurrentProcess.Containers.IsValidIndex(0))
				{
					//TODO: Check if this ContainerLoop is needed.
					for(auto& CurrentContainer : CurrentProcess.Containers)
					{
						if(ToComponent->ContainerSettings.Contains(CurrentContainer))
						{
							for(auto& ParentContainer : ToComponent->ContainerSettings)
							{
								if(ParentContainer.Items.Contains(CurrentProcess.Item))
								{
									//We have found the parent container and parent item. Start updating their data.
									FS_InventoryItem &NewParentItem = ParentContainer.Items[ParentContainer.Items.Find(CurrentProcess.Item)];
									if(NewParentItem.UniqueID.ParentComponent != ToComponent)
									{
										NewParentItem.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);
										Seed.Initialize(Seed.GetCurrentSeed() + 1);
										if(UItemInstance* ItemInstance = NewParentItem.ItemInstance)
										{
											ItemInstance->ItemID = NewParentItem.UniqueID;
										}
									}
																			
									//Parents have been updated, we can now provide the proper data to the containers attached to the NewParentItem.
									for(auto& ChildContainer : ToComponent->ContainerSettings)
									{
										if(ChildContainer.BelongsToItem.Y == CurrentProcess.Item.UniqueID.IdentityNumber)
										{
											int32 ContainerIndex = ToComponent->ContainerSettings.Find(ChildContainer);
											if(ContainerIndex == -1)
											{
												continue;
											}
											
											FS_ContainerSettings &ChildContainerRef = ToComponent->ContainerSettings[ContainerIndex];
											ChildContainerRef.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);
											Seed.Initialize(Seed.GetInitialSeed() + 1);
											ChildContainerRef.BelongsToItem.X = ParentContainer.UniqueID.IdentityNumber;
											ChildContainerRef.BelongsToItem.Y = NewParentItem.UniqueID.IdentityNumber;
											ContainerWidgetsToUpdate.Add(ChildContainerRef);
										}
									}
									break;
								}
							}
						}
					}
				}
				//Item has no containers, update its UniqueID.
				else
				{
					for(auto& ProcessedContainer : ToComponent->ContainerSettings)
					{
						if(ProcessedContainer.Items.Contains(CurrentProcess.Item))
						{
							FS_InventoryItem &NewChildItem = ProcessedContainer.Items[ProcessedContainer.Items.Find(CurrentProcess.Item)];
							NewChildItem.UniqueID = ToComponent->GenerateUniqueIDWithSeed(Seed);
							Seed.Initialize(Seed.GetCurrentSeed() + 1);
							if(UItemInstance* ItemInstance = NewChildItem.ItemInstance)
							{
								ItemInstance->ItemID = NewChildItem.UniqueID;
							}
							break;
						}
					}
				}
			}
		}
	} //End of ProcessList loop

	if(bNewComponent)
	{
		//Remove the containers from the old component
		for(auto& RemovingContainer : ItemContainers)
		{
			FromComponent->ContainerSettings.Remove(RemovingContainer);
		}
	}
		
	//Since the item had containers that were removed from the FromComponent and added to ToComponent,
	//we need to update their indexes.
	if(ItemContainers.IsValidIndex(0))
	{
		FromComponent->RefreshIndexes();
		ToComponent->RefreshIndexes();
	}
				
	//Sort and Refresh item indexes.
	ToComponent->RefreshItemsIndexes(ToComponent->ContainerSettings[ToContainer]);
	if(FromComponent->ContainerSettings.IsValidIndex(ItemToMove.ContainerIndex))
	{
		FromComponent->RefreshItemsIndexes(FromComponent->ContainerSettings[ItemToMove.ContainerIndex]);
	}
	
	if(bNewComponent)
	{
		//Container and item indexes have been refreshed and had their UniqueID's assigned.
		for(auto& CurrentContainer : ToComponent->ContainerSettings)
		{
			ToComponent->RefreshTileMap(CurrentContainer);
		}

		for(auto& CurrentWidget : ContainerWidgetsToUpdate)
		{
			if(CurrentWidget.Widget)
			{
				CurrentWidget.Widget->ConstructContainers(CurrentWidget, ToComponent, true);
			}
		}
	}

	//Call equip dispatcher if @ToContainer was an equipment container.
	if(ToComponent->ContainerSettings[ToContainer].ContainerType == Equipment)
	{
		//Alert any objects that are listening to either the item or container
		//that equipment status has been updated.
		UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(NewlyCreatedItem, true, TArray<FName>());
	}
	 
	if(CallItemMoved)
	{
		ToComponent->ItemMoved.Broadcast(ItemToMove, NewlyCreatedItem, OldContainer,
			ToComponent->ContainerSettings[ToContainer], FromComponent, ToComponent);
	 
		if(ToComponent != FromComponent) //Only trigger broadcast if the To and From components aren't the same so we don't trigger it twice for the same component.
		{
			FromComponent->ItemMoved.Broadcast(ItemToMove, NewlyCreatedItem, OldContainer,
				ToComponent->ContainerSettings[ToContainer], FromComponent, ToComponent);
		}
	}

	if(CallItemAdded)
	{
		ToComponent->ItemAdded.Broadcast(NewlyCreatedItem, ToIndex, ToComponent->ContainerSettings[ToContainer]);
	}
}

void UAC_Inventory::SwapItemLocations(FS_InventoryItem Item1, FS_InventoryItem Item2, bool CallItemMoved)
{
	if(!UFL_InventoryFramework::IsItemValid(Item1) || !UFL_InventoryFramework::IsItemValid(Item2))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either Item1 or Item2 is invalid - AC_Inventory.cpp -> SwapItemLocations"), true, true);
		return;
	}

	TEnumAsByte<ERotation> Item1NeededRotation;
	TEnumAsByte<ERotation> Item2NeededRotation;
	if(!CanSwapItemLocations(Item1, Item2, Item1NeededRotation, Item2NeededRotation))
	{
		return;
	}

	C_AddItemToNetworkQueue(Item1.UniqueID);
	C_AddItemToNetworkQueue(Item2.UniqueID);
	S_SwapItemLocations(Item1, Item2, CallItemMoved, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_SwapItemLocations_Implementation(FS_InventoryItem Item1, FS_InventoryItem Item2,
                                                       bool CallItemMoved, ENetRole CallerLocalRole)
{
	//Validate data before continuing
	if(!UFL_InventoryFramework::IsItemValid(Item1) || !UFL_InventoryFramework::IsItemValid(Item2))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either Item1 or Item2 is invalid - AC_Inventory.cpp -> SwapItemLocations"), true, true);
		C_RemoveItemFromNetworkQueue(Item1.UniqueID);
		C_RemoveItemFromNetworkQueue(Item2.UniqueID);
		return;
	}

	if(Item1.UniqueID.ParentComponent->ContainerSettings[Item1.ContainerIndex].Items[Item1.ItemIndex].UniqueID != Item1.UniqueID)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("UniqueID for Item1 did not match the servers UniqueID - AC_Inventory.cpp -> SwapItemLocations"), true, true);
		C_RemoveItemFromNetworkQueue(Item1.UniqueID);
		C_RemoveItemFromNetworkQueue(Item2.UniqueID);
		return;
	}

	if(Item2.UniqueID.ParentComponent->ContainerSettings[Item2.ContainerIndex].Items[Item2.ItemIndex].UniqueID != Item2.UniqueID)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("UniqueID for Item2 did not match the servers UniqueID - AC_Inventory.cpp -> SwapItemLocations"), true, true);
		C_RemoveItemFromNetworkQueue(Item1.UniqueID);
		C_RemoveItemFromNetworkQueue(Item2.UniqueID);
		return;
	}

	TEnumAsByte<ERotation> Item1NeededRotation;
	TEnumAsByte<ERotation> Item2NeededRotation;
	if(!CanSwapItemLocations(Item1, Item2, Item1NeededRotation, Item2NeededRotation))
	{
		return;
	}
	//end of data validation

	/**Try and process any containers the item owns, since the server is the only one who is aware of
	 * everyone's containers.
	 * For example, Client1 and Client2 are interacting with a chest and Client1 puts in a backpack,
	 * which has a container inside of it. Client2 won't be able to process the MoveItem request,
	 * because they are not aware of Client1's containers.*/
	TArray<FS_ContainerSettings> Item1Containers;
	if(Item1.UniqueID.ParentComponent == Item2.UniqueID.ParentComponent)
	{
		//We only need the containers belonging to this item
		Item1Containers = Item1.UniqueID.ParentComponent->GetItemsChildrenContainers(Item1);
	}
	else
	{
		//We will need to update and move all containers and items belonging to this item in one go.
		Item1.UniqueID.ParentComponent->GetAllContainersAssociatedWithItem(Item1, Item1Containers);
	}

	TArray<FS_ContainerSettings> Item2Containers;
	if(Item1.UniqueID.ParentComponent == Item2.UniqueID.ParentComponent)
	{
		//We only need the containers belonging to this item
		Item2Containers = Item2.UniqueID.ParentComponent->GetItemsChildrenContainers(Item2);
	}
	else
	{
		//We will need to update and move all containers and items belonging to this item in one go.
		Item2.UniqueID.ParentComponent->GetAllContainersAssociatedWithItem(Item2, Item2Containers);
	}

	FRandomStream Item1Seed;
	Item1Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
	FRandomStream Item2Seed;
	Item2Seed.Initialize(Item1Seed.GetCurrentSeed() + 1); //Ensures Seed1 and Seed2 are never the same

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Item2.UniqueID.ParentComponent->Internal_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
				Item1.UniqueID.ParentComponent->Internal_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
			}
			else
			{
				Item2.UniqueID.ParentComponent->Internal_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
				Item1.UniqueID.ParentComponent->Internal_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
				C_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
				C_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
			}
		}
		else
		{
			Item2.UniqueID.ParentComponent->Internal_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
			Item1.UniqueID.ParentComponent->Internal_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
			C_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
			C_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
		}
	}
	else
	{
		Item2.UniqueID.ParentComponent->Internal_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
		Item1.UniqueID.ParentComponent->Internal_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
		C_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
		C_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
	}

	TArray<UAC_Inventory*> CombinedListeners;
	for(auto& AppendingListener : Item1.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item1.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item2.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item2.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	
	for(const auto& CurrentListener : CombinedListeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_MoveItem(Item1, Item1.UniqueID.ParentComponent, Item2.UniqueID.ParentComponent, Item2.ContainerIndex, Item2.TileIndex, Item1.Count,  CallItemMoved, CallItemMoved, true, Item1NeededRotation, Item1Containers, Item1Seed);
			CurrentListener->C_MoveItem(Item2, Item2.UniqueID.ParentComponent, Item1.UniqueID.ParentComponent, Item1.ContainerIndex, Item1.TileIndex, Item2.Count,  CallItemMoved, CallItemMoved, true, Item2NeededRotation, Item2Containers, Item2Seed);
		}
	}
}

bool UAC_Inventory::S_SwapItemLocations_Validate(FS_InventoryItem Item1, FS_InventoryItem Item2, bool CallItemMoved,
	ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::AddItemToTileMap(const FS_InventoryItem Item)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AddItemToTileMap)
	if(!ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		return;
	}
	
	if(!ContainerSettings[Item.ContainerIndex].SupportsTileMap())
	{
		return;
	}
	
	if(!ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(Item.TileIndex))
	{
		return;
	}

	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return;
	}

	bool InvalidTileFound;
	TArray<FIntPoint> ItemsShape = UFL_InventoryFramework::GetItemsShape(Item, InvalidTileFound);

	for(auto& CurrentTile : ItemsShape)
	{
		int32 CurrentIndex = UFL_InventoryFramework::TileToIndex(CurrentTile.X, CurrentTile.Y, ContainerSettings[Item.ContainerIndex]);
		if(ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(CurrentIndex))
		{
			ContainerSettings[Item.ContainerIndex].TileMap[CurrentIndex] = Item.UniqueID.IdentityNumber;
		}
	}
}

void UAC_Inventory::AddItemToUninitializedTileMap(FS_InventoryItem Item, UPARAM(ref) FS_ContainerSettings& Container)
{
	if(!Container.SupportsTileMap())
	{
		return;
	}
	
	if (Container.Style == Grid)
	{
		//Loop through all the tiles of a container with the dimensions of an item, starting at TileIndex.
		int32 XLoop;
		int32 YLoop;
		UFL_InventoryFramework::IndexToTile(Item.TileIndex, Container, XLoop, YLoop);
		
		FIntPoint ItemDimensions = UFL_InventoryFramework::GetItemDimensions(Item);

		for (int32 ColumnY = YLoop; ColumnY < YLoop + ItemDimensions.Y; ColumnY++)
		{
			if (ColumnY < Container.Dimensions.Y)
			{
				for (int32 RowX = XLoop; RowX < XLoop + ItemDimensions.X; RowX++)
				{
					if (RowX < Container.Dimensions.X)
					{
						int32 CurrentTile = UFL_InventoryFramework::TileToIndex(RowX, ColumnY, Container);
						Container.TileMap[CurrentTile] = Item.UniqueID.IdentityNumber;
					}
					else
					{
						//Invalid X Row
						return;
					}
				}
			}
			else
			{
				//Invalid Y row
				return;
			}
		}
	}
	else if(Container.Style == Traditional)
	{
		if (Item.TileIndex != -1 && Item.ContainerIndex != -1)
		{
			Container.TileMap[Item.TileIndex] = Item.UniqueID.IdentityNumber;
		}
	}
}

void UAC_Inventory::RemoveItemFromTileMap(FS_InventoryItem Item)
{
	if (Item.TileIndex == -1 && Item.ContainerIndex == -1)
	{
		return;
	}

	if(!ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		return;
	}

	//Data-Only containers don't have a tilemap
	if(!ContainerSettings[Item.ContainerIndex].SupportsTileMap())
	{
		return;
	}

	if(!ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(Item.TileIndex))
	{
		return;
	}
	
	if (ContainerSettings[Item.ContainerIndex].Style == Grid)
	{
		//Item might have been rotated by a widget or some function.
		//Find it so we have up to date information.
		Item = GetItemByUniqueID(Item.UniqueID);
		if(!Item.IsValid())
		{
			return;
		}
		
		//Loop through all the tiles of a container with the dimensions of an item, starting at TileIndex.
		int32 XLoop;
		int32 YLoop;
		UFL_InventoryFramework::IndexToTile(Item.TileIndex, ContainerSettings[Item.ContainerIndex], XLoop, YLoop);
		
		FIntPoint ItemDimensions = UFL_InventoryFramework::GetItemDimensions(Item);

		for(int32 ColumnY = YLoop; ColumnY < YLoop + ItemDimensions.Y; ColumnY++)
		{
			if(ColumnY < ContainerSettings[Item.ContainerIndex].Dimensions.Y)
			{
				for(int32 RowX = XLoop; RowX < XLoop + ItemDimensions.X; RowX++)
				{
					if(RowX < ContainerSettings[Item.ContainerIndex].Dimensions.X)
					{
						int32 CurrentTile = UFL_InventoryFramework::TileToIndex(RowX, ColumnY, ContainerSettings[Item.ContainerIndex]);
						if(ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(CurrentTile))
						{
							if(ContainerSettings[Item.ContainerIndex].TileMap[CurrentTile] == Item.UniqueID.IdentityNumber)
							{
								ContainerSettings[Item.ContainerIndex].TileMap[CurrentTile] = -1;
							}
						}
					}
					else
					{
						//Invalid X Row
						return;
					}
				} 
			}
			else
			{
				//Invalid Y row
				return;
			}
		}
	}
	else
	{
		if(ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(Item.TileIndex))
		{
			if(ContainerSettings[Item.ContainerIndex].TileMap[Item.TileIndex] == Item.UniqueID.IdentityNumber)
			{
				ContainerSettings[Item.ContainerIndex].TileMap[Item.TileIndex] = -1;
			}
		}
	}
}

void UAC_Inventory::RemoveItemFromInventory(FS_InventoryItem Item, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, bool& Success)
{
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, Success);
		return;
	}
	C_AddItemToNetworkQueue(Item.UniqueID);
	S_RemoveItemFromInventory(Item.UniqueID, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_RemoveItemFromInventory_Implementation(FS_UniqueID ItemID, bool CallItemRemoved, bool CallItemUnequipped,
	bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, ENetRole CallerLocalRole)
{
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	bool RemovalSuccess;
	
	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, RemovalSuccess);
			}
			else
			{
				Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, RemovalSuccess);
				C_RemoveItemFromInventory(ItemID, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed);
			}
		}
		else
		{
			Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, RemovalSuccess);
			C_RemoveItemFromInventory(ItemID, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed);
		}
	}
	else
	{
		Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, RemovalSuccess);
		C_RemoveItemFromInventory(ItemID, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveItemFromInventory(ItemID, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed);
		}
	}
}

bool UAC_Inventory::S_RemoveItemFromInventory_Validate(FS_UniqueID ItemID, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents,
	bool RemoveItemsContainers, bool RemoveItemInstance, ENetRole CallerLocalRole)
{
	if(const FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID); !UFL_InventoryFramework::IsItemValid(Item))
	{
		return false;
	}
	
	return true;
}

void UAC_Inventory::C_RemoveItemFromInventory_Implementation(FS_UniqueID ItemID, bool CallItemRemoved, bool CallItemUnequipped,
	bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	bool RemovalSuccess;
	if(Item.IsValid())
	{
		Internal_RemoveItemFromInventory(Item, CallItemRemoved, CallItemUnequipped, RemoveItemComponents, RemoveItemsContainers, RemoveItemInstance, Seed, RemovalSuccess);
	}
	else
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to remove item, but the item was not found - AC_Inventory.cpp"), true, true);
	}
	C_RemoveItemFromNetworkQueue(Item.UniqueID);
}

void UAC_Inventory::Internal_RemoveItemFromInventory_Implementation(FS_InventoryItem Item, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents,
	bool RemoveItemsContainers, bool RemoveItemInstance, FRandomStream Seed, bool& Success)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}

	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	if(!Item.IsValid())
	{
		return;
	}

	if(CallItemUnequipped)
	{
		UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(Item, false, TArray<FName>(), Item);
	}

	if(RemoveItemComponents)
	{
		for(auto& CurrentObject : Item.ItemAsset->TraitsAndComponents)
		{
			if(UIT_ItemComponentTrait* ComponentTrait = Cast<UIT_ItemComponentTrait>(CurrentObject))
			{
				UItemComponent* ItemComponent = ParentComponent->GetItemComponent(Item, ComponentTrait, false, GetOwner());
				if(IsValid(ItemComponent))
				{
					if(!ItemComponent->bIsBusy)
					{
						ItemComponent->DestroyComponent();
					}
				}
			}		
		}
	}

	if(RemoveItemsContainers)
	{
		TArray<FS_ContainerSettings> ContainersToRemove;
		ParentComponent->GetAllContainersAssociatedWithItem(Item, ContainersToRemove);
		for(auto& RemovingContainer : ContainersToRemove)
		{
			ContainerSettings.Remove(RemovingContainer);
		}

		RefreshIndexes();
	}
	
	ParentComponent->RemoveItemFromTileMap(Item);

	//The rest is handled in Blueprint.
}

void UAC_Inventory::DropItem(FS_InventoryItem Item)
{
	if(!CanItemBeDropped(Item))
	{
		return;
	}
	
	if(GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		Internal_DropItem(Item);
	}
	else
	{
		//Person doesn't have authority, ask server to drop the item.
		C_AddItemToNetworkQueue(Item.UniqueID);
		S_DropItem(Item.UniqueID);
	}
}

void UAC_Inventory::S_DropItem_Implementation(FS_UniqueID ItemID)
{
	FS_InventoryItem Item = GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		//Already checked in DropItem, but it might have been called
		//from a client, check it again on the server
		if(!CanItemBeDropped(Item))
		{
			return;
		}
		
		Internal_DropItem(Item);
	}
	else
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to drop item, but could not find the item on the server - AC_Inventory.cpp -> S_DropItem"), true, true);
	}
}

bool UAC_Inventory::CanItemBeDropped_Implementation(FS_InventoryItem Item)
{
	if(Item.ItemAsset->ItemActor.IsNull())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Item has no actor, can't be dropped");
		return false;
	}
	
	if(Item.UniqueID.ParentComponent->InventoryType == Vendor)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Item belongs to vendor, can't be dropped");
		return false;
	}

	return true;
}

bool UAC_Inventory::CanItemBeDestroyed_Implementation(FS_InventoryItem Item)
{
	return true;
}

void UAC_Inventory::UpdateItemsContainers(FS_InventoryItem Item, FIntPoint OldXY)
{
	FS_InventoryItem TempItem = Item;
	TempItem.ContainerIndex = OldXY.X;
	TempItem.ItemIndex = OldXY.Y;
	TArray<FS_ContainerSettings> Containers = GetItemsChildrenContainers(TempItem);
	if(Containers.IsValidIndex(0))
	{
		for(int32 CurrentContainer = 0; CurrentContainer < Containers.Num(); CurrentContainer++)
		{
			ContainerSettings[Containers[CurrentContainer].ContainerIndex].BelongsToItem.X = ContainerSettings[Item.ContainerIndex].UniqueID.IdentityNumber;
			ContainerSettings[Containers[CurrentContainer].ContainerIndex].BelongsToItem.Y = Item.UniqueID.IdentityNumber;
		}
	}
}

UW_AttachmentParent* UAC_Inventory::CreateAttachmentWidgetForItem(FS_InventoryItem Item, bool ResetData, bool DoNotBind, TArray<FS_ContainerSettings>& AddedContainers)
{
	UW_AttachmentParent* NewWidget = nullptr;
	TSubclassOf<UW_AttachmentParent> AttachmentWidget = Item.ItemAsset->GetAttachmentWidgetClass();
	TArray<FS_ContainerSettings> ItemsContainers = Item.UniqueID.ParentComponent->GetItemsChildrenContainers(Item);
	if(!AttachmentWidget || ItemsContainers.IsEmpty())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Tried to create attachment widget for item, but could not find an attachment widget or could not find any containers that belong to the item.");
		return nullptr;
	}

	//Create the attachment widget and assign it to the item.
	NewWidget = CreateWidget<UW_AttachmentParent>(GetWorld(), AttachmentWidget);
	Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ExternalObjects.Add(NewWidget);
	NewWidget->ParentItemID = Item.UniqueID;
	if(DoNotBind)
	{
		return NewWidget;
	}
	TArray<UW_Container*> WidgetContainers;
	II_InventoryWidgets::Execute_GetContainers(NewWidget, WidgetContainers);
	if(WidgetContainers.IsEmpty())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Tried to create attachment widget for item, but the widget did not return any containers from I_InventoryWidgets -> GetContainers");
	}
	//Loop through the items containers and give the widgets their data and construct them.
	for(int32 WidgetIndex = 0; WidgetIndex < WidgetContainers.Num(); WidgetIndex++)
	{
		bool ContainerFound;
		FS_ContainerSettings FoundContainer;
		int32 ContainerIndex;
		FGameplayTag Identifier = WidgetContainers[WidgetIndex]->TemporaryContainerSettings.ContainerIdentifier;
		Item.UniqueID.ParentComponent->GetContainerByIdentifier(Identifier, ItemsContainers, true, ContainerFound, FoundContainer, ContainerIndex);
		if(ContainerFound)
		{
			AddedContainers.Add(FoundContainer);
			WidgetContainers[WidgetIndex]->ConstructContainers(FoundContainer, this, ResetData);
		}
	}

	return NewWidget;
}

void UAC_Inventory::TryAddNewItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ItemsContainers, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta)
{
	if(!UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(!IsValid(Item.ItemAsset) || !IsValid(DestinationComponent) || Item.Count <= 0)
	{
		Result = false;
		return;
	}
	
	//Server might be trying to add an item to a component no player has interacted with yet.
	if(!DestinationComponent->Initialized && IsValid(UGameplayStatics::GetGameInstance(this)))
	{
		DestinationComponent->StartComponent();
	}
	
	//Generate random min max count here so both client and server have the same data.
	if(Item.ItemAsset->CanItemStack())
	{
		if(FRandomItemCountFragment* RandomCount = FindFragment<FRandomItemCountFragment>(Item.ItemFragments))
		{
			Item.Count = UKismetMathLibrary::RandomIntegerInRange(RandomCount->MinMaxCount.X, RandomCount->MinMaxCount.Y);
			if(Item.Count <= 0)
			{
				UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item random count was 0. Exiting function - AC_Inventory.cpp -> TryAddNewItem"), true, true);
				return;
			}
		}
	}
	else
	{
		Item.Count = Item.ItemAsset->DefaultStack;
	}

	if(ItemsContainers.IsEmpty())
	{
		ItemsContainers = Item.ItemAsset->GetDefaultContainers();
	}
	
	for(auto& CurrentContainer : ItemsContainers)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.ItemAsset->CanItemStack())
			{
				if(FRandomItemCountFragment* RandomCount = FindFragment<FRandomItemCountFragment>(Item.ItemFragments))
				{
					CurrentItem.Count = UKismetMathLibrary::RandomIntegerInRange(RandomCount->MinMaxCount.X, RandomCount->MinMaxCount.Y);
					if(CurrentItem.Count <= 0)
					{
						CurrentContainer.Items.RemoveSingle(CurrentItem);
					}
				}
			}
		}
	}

	UFL_InventoryFramework::AddDefaultTagsToItem(Item, false);
	UFL_InventoryFramework::AddDefaultTagValuesToItem(Item, false, false);

	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_TryAddNewItem(Item, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Seed, Result, NewItem, StackDelta);
		return;
	}
	
	Internal_TryAddNewItem(Item, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Seed, Result, NewItem, StackDelta);
	DestinationComponent->C_TryAddNewItem(Item, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Seed);
	
	for(auto& CurrentListener : DestinationComponent->Listeners)
	{
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			CurrentListener->C_TryAddNewItem(Item, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Seed);
		}
	}
}

void UAC_Inventory::C_TryAddNewItem_Implementation(FS_InventoryItem Item, const TArray<FS_ContainerSettings> &ItemsContainers, UAC_Inventory* DestinationComponent,
	bool CallItemAdded, bool SkipStacking, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(DestinationComponent->GetOwner()->HasAuthority())
	{
		return;
	}
	
	bool Result;
	FS_InventoryItem NewItem;
	int32 StackDelta;
	Internal_TryAddNewItem(Item, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Seed, Result, NewItem, StackDelta);
}

void UAC_Inventory::Internal_TryAddNewItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ItemsContainers, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking,
	FRandomStream Seed, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta)
{
	StackDelta = 0;
	
	if(!IsValid(Item.ItemAsset) || !IsValid(DestinationComponent))
	{
		Result = false;
		return;
	}
	
	FS_InventoryItem UnmodifiedItem = Item;
	TArray<FS_ContainerSettings> ContainersToWorkWith;
	FS_ContainerSettings AvailableContainer;
	TEnumAsByte<ERotation> NeededRotation;
	int32 AvailableTile = -1;
	Item.UniqueID = DestinationComponent->GenerateUniqueIDWithSeed(Seed);
	Seed.Initialize(Seed.GetInitialSeed() + 1);
	Item.ItemIndex = 0; //This might be dangerous, but IsItemValid requires ItemIndex to be higher than -1

	//Resolve what containers we can work with.
	if(Item.ContainerIndex <= -1)
	{
		bool IsCompatibleWithContainer;
		for(auto& CurrentContainer : DestinationComponent->ContainerSettings)
		{
			IsCompatibleWithContainer = DestinationComponent->CheckCompatibility(Item, CurrentContainer);
			bool SpotFound;
			TEnumAsByte<EContainerInfinityDirection> InfinityDirection;
			if(UFL_InventoryFramework::IsContainerInfinite(CurrentContainer, InfinityDirection))
			{
				//Container is infinite, label spot as found and if there is none, we'll expand the container to make a spot.
				SpotFound = true;
			}
			else
			{
				int32  FoundTile;
				GetFirstAvailableTile(Item, CurrentContainer, GetGenericIndexesToIgnore(CurrentContainer), SpotFound, FoundTile, NeededRotation);
			}
			
			if(IsCompatibleWithContainer && SpotFound)
			{
				ContainersToWorkWith.AddUnique(CurrentContainer);
			}
		}
	}
	else
	{
		if(DestinationComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
		{
			bool IsCompatibleWithContainer = IsCompatibleWithContainer = CheckCompatibility(Item, ContainerSettings[Item.ContainerIndex]);;
			if(!IsCompatibleWithContainer)
			{
				UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to add item to container, but item did not meet compatibility check - AC_Inventory.cpp -> InternalTryAddNewItem"), true, true);
				return;
			}
			ContainersToWorkWith.Add(DestinationComponent->ContainerSettings[Item.ContainerIndex]);
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Incompatible container index - AC_Inventory.cpp -> InternalTryAddNewItem"), true, true);
			return;
		}
	}

	if(!ContainersToWorkWith.IsValidIndex(0))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("No compatible containers found - AC_Inventory.cpp -> InternalTryAddNewItem"), true, true);
	}
	
	for(auto& CurrentContainer : ContainersToWorkWith)
	{
		bool SpotFound = false;
		TArray<int32> TilesToIgnore = GetGenericIndexesToIgnore(CurrentContainer);
		if(Item.TileIndex <= -1)
		{
			TArray<int32> ContainersToIgnore;
			//Check if we can stack the item with any items in this container.
			if(Item.ItemAsset->CanItemStack() && !SkipStacking)
			{
				TArray<FS_InventoryItem> FoundItems;
				int32 TotalAmountFound;
				DestinationComponent->GetAllItemsWithDataAsset(Item.ItemAsset, -1, FoundItems, TotalAmountFound);
				if(FoundItems.IsValidIndex(0))
				{
					for(auto& CurrentItem : FoundItems)
					{
						if(UFL_InventoryFramework::CanStackItems(Item, CurrentItem))
						{
							//Since the function StackTwoItems requires some of the data to be valid,
							//we pretend to stack it by just increasing the count of items.
							int32 ItemsNewCount = UKismetMathLibrary::Clamp((CurrentItem.Count + Item.Count) - UFL_InventoryFramework::GetItemMaxStack(CurrentItem), 0, UFL_InventoryFramework::GetItemMaxStack(Item));
							int32 CurrentItemNewCount;
							StackDelta = Item.Count - CurrentItem.Count;
							DestinationComponent->Internal_IncreaseItemCount(CurrentItem.UniqueID, Item.Count, CurrentItemNewCount);
							Item.Count = ItemsNewCount;
							if(Item.Count <= 0)
							{
								//Remaining count is 0, finalize result.
								NewItem = CurrentItem;
								Result = true;
								NewItem = DestinationComponent->GetItemByUniqueID(CurrentItem.UniqueID);
								return;
							}
						}
					}
				}
			}
			//We've attempted to stack the item as much as possible and count is  still above 0. Find a free tile.
			Item.ContainerIndex = CurrentContainer.ContainerIndex;
			DestinationComponent->GetFirstAvailableTile(Item, CurrentContainer, TilesToIgnore, SpotFound, AvailableTile, NeededRotation);
			if(SpotFound)
			{
				AvailableContainer = CurrentContainer;
				break;
			}
		}
		else
		{
			TArray<FS_InventoryItem> ItemsInTheWay;
			TArray<FS_InventoryItem> ItemsToIgnore;
			DestinationComponent->CheckAllRotationsForSpace(Item, CurrentContainer, Item.TileIndex, ItemsToIgnore, TilesToIgnore, SpotFound, Item.Rotation, AvailableTile, ItemsInTheWay);
			if(!SpotFound)
			{
				DestinationComponent->GetFirstAvailableTile(Item, CurrentContainer, TilesToIgnore, SpotFound, AvailableTile, Item.Rotation);
				if(SpotFound)
				{
					AvailableContainer = CurrentContainer;
					break;
				}
			}
			else
			{
				AvailableContainer = CurrentContainer;
				break;
			}
		}

		//No spot has been found, check if container is infinite. If it is, attempt to expand it.
		TEnumAsByte<EContainerInfinityDirection> InfinityDirection;
		if(!SpotFound && UFL_InventoryFramework::IsContainerInfinite(CurrentContainer, InfinityDirection))
		{
			FMargin ContainerAdjustment;
			FIntPoint ItemDimensions;
			UFL_InventoryFramework::GetItemDimensionsWithContext(Item, CurrentContainer, ItemDimensions.X, ItemDimensions.Y);

			//Container might be infinite, but it can only be infinite in one direction.
			//If the container is infinite in the X direction, we have to make sure the
			//item is not too tall and vice versa.
			bool AttemptExpand = true;
			if(CurrentContainer.Style == Grid)
			{
				if(InfinityDirection == X)
				{
					if(CurrentContainer.Dimensions.Y < ItemDimensions.Y)
					{
						AttemptExpand = false;
					}
				}
				if(InfinityDirection == Y)
				{
					if(CurrentContainer.Dimensions.X < ItemDimensions.X)
					{
						AttemptExpand = false;
					}
				}
			}

			if(AttemptExpand)
			{
				if(InfinityDirection == X)
				{
					if(CurrentContainer.Style == Grid)
					{
						ContainerAdjustment.Right = ItemDimensions.X;
					}
					else if(CurrentContainer.Style == Traditional)
					{
						ContainerAdjustment.Right = 1;
					}
				}
				else if(InfinityDirection == Y)
				{
					if(CurrentContainer.Style == Grid)
					{
						ContainerAdjustment.Bottom = ItemDimensions.Y;
					}
					else if(CurrentContainer.Style == Traditional)
					{
						ContainerAdjustment.Bottom = 1;
					}
				}

				// DestinationComponent->Internal_AdjustContainerSize(CurrentContainer, ContainerAdjustment);
				DestinationComponent->GetFirstAvailableTile(Item, DestinationComponent->ContainerSettings[CurrentContainer.ContainerIndex], TilesToIgnore, SpotFound, AvailableTile, Item.Rotation);
				if(SpotFound)
				{
					AvailableContainer = CurrentContainer;
					break;
				}
			}
		}
	}
	
	if(AvailableContainer.ContainerIndex <= -1)
	{
		Result = false;
		return;
	}

	//Container and tile has been found, proceed with adding the item
	Item.ContainerIndex = AvailableContainer.ContainerIndex;
	Item.TileIndex = AvailableTile;
	FS_ContainerSettings& ContainerRef = DestinationComponent->ContainerSettings[AvailableContainer.ContainerIndex];
	Item.Rotation = NeededRotation;
	Item.ItemIndex = ContainerRef.Items.Num();
	ContainerRef.Items.Add(Item);
	DestinationComponent->AddItemToTileMap(Item);
	DestinationComponent->CreateItemInstanceForItem(Item);

	//Start processing the items containers
	TArray<FS_ContainerSettings> AddedContainers;
	if(ItemsContainers.IsValidIndex(0))
	{
		for(auto& CurrentContainer : ItemsContainers)
		{
			//Populate the containers Tile Maps, so we can do proper collision tests.
			CurrentContainer.TileMap.Empty();
			for(int32 ColumnY = 0; ColumnY < CurrentContainer.Dimensions.Y; ColumnY++)
			{
				if(ColumnY < CurrentContainer.Dimensions.Y)
				{
					for(int32 RowX = 0; RowX < CurrentContainer.Dimensions.X; RowX++)
					{
						if(RowX < CurrentContainer.Dimensions.X)
						{
							CurrentContainer.TileMap.Add(-1);
						}
					}
				}
			}
			CurrentContainer.UniqueID = DestinationComponent->GenerateUniqueIDWithSeed(Seed);
			Seed.Initialize(Seed.GetInitialSeed() + 1);
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				bool SubItemSpotFound = false;
				TArray<int32> IndexesToIgnore = GetGenericIndexesToIgnore(CurrentContainer);
				if(CurrentItem.TileIndex <= -1)
				{
					DestinationComponent->GetFirstAvailableTile(CurrentItem, CurrentContainer, IndexesToIgnore, SubItemSpotFound, CurrentItem.TileIndex, CurrentItem.Rotation);
				}
				else
				{
					TArray<FS_InventoryItem> ItemsInTheWay;
					TArray<FS_InventoryItem> ItemsToIgnore;
					DestinationComponent->CheckAllRotationsForSpace(CurrentItem, CurrentContainer, CurrentItem.TileIndex, ItemsToIgnore, IndexesToIgnore, SubItemSpotFound, CurrentItem.Rotation, CurrentItem.TileIndex, ItemsInTheWay);
					if(!SubItemSpotFound) //Specified tile was not free, find a free one.
					{
						DestinationComponent->GetFirstAvailableTile(CurrentItem, CurrentContainer, IndexesToIgnore, SubItemSpotFound, CurrentItem.TileIndex, CurrentItem.Rotation);
					}
				}
				if(SubItemSpotFound)
				{
					CurrentItem.UniqueID = DestinationComponent->GenerateUniqueIDWithSeed(Seed);
					Seed.Initialize(Seed.GetInitialSeed() + 1);
					DestinationComponent->AddItemToUninitializedTileMap(CurrentItem, CurrentContainer);
					DestinationComponent->CreateItemInstanceForItem(CurrentItem);
				}
			}
			AddedContainers.Add(CurrentContainer);
		}

		for(auto& CurrentContainer : AddedContainers)
		{
			if(ItemsContainers.IsValidIndex(CurrentContainer.BelongsToItem.X))
			{
				if(ItemsContainers[CurrentContainer.BelongsToItem.X].Items.IsValidIndex(CurrentContainer.BelongsToItem.Y))
				{
					//First update Y since it's easier to resolve that way.
					//The containers items UniqueID has already been assigned, so we can just use the containers BelongsToItem as directions for what UniqueID's need to be assigned.
					CurrentContainer.BelongsToItem.Y = ItemsContainers[CurrentContainer.BelongsToItem.X].Items[CurrentContainer.BelongsToItem.Y].UniqueID.IdentityNumber;
					CurrentContainer.BelongsToItem.X = ItemsContainers[CurrentContainer.BelongsToItem.X].UniqueID.IdentityNumber;
				}
			}
			else if(CurrentContainer.BelongsToItem.X <= -1 || CurrentContainer.BelongsToItem.Y <= -1)
			{
				CurrentContainer.BelongsToItem.X = AvailableContainer.UniqueID.IdentityNumber;
				CurrentContainer.BelongsToItem.Y = Item.UniqueID.IdentityNumber;
			}
			CurrentContainer.ContainerIndex = DestinationComponent->ContainerSettings.Num();
			DestinationComponent->ContainerSettings.Add(CurrentContainer);
		}
	}
	
	//Designer might need location data from the item we broadcast.
	UnmodifiedItem.TileIndex = Item.TileIndex;
	UnmodifiedItem.ContainerIndex = Item.ContainerIndex;
	
	UW_Container* WidgetContainer = UFL_InventoryFramework::GetWidgetForContainer(ContainerRef);
	if(IsValid(WidgetContainer))
	{
		UW_InventoryItem* ItemWidget;
		WidgetContainer->CreateWidgetForItem(Item, ItemWidget);
	}
	DestinationComponent->RefreshIndexes();
	DestinationComponent->RefreshItemsIndexes(DestinationComponent->ContainerSettings[AvailableContainer.ContainerIndex]);
	if(ItemsContainers.IsValidIndex(0))
	{
		for(auto& CurrentContainer : AddedContainers)
		{
			DestinationComponent->RefreshTileMap(DestinationComponent->ContainerSettings[CurrentContainer.ContainerIndex]);
		}
	}
	Result = true;
	Item = DestinationComponent->GetItemByUniqueID(Item.UniqueID);
	NewItem = Item;
	StackDelta = NewItem.Count;

	if(CallItemAdded)
	{
		//Designer might need things such as item count before it was stacked, for example a widget telling the player they looted 20 arrows,
		//but since we stacked it with another stack of arrows, the new Item will have incorrect data.
		//If the designer needs the original item, they can use the container and tile index.
		DestinationComponent->ItemAdded.Broadcast(Item, Item.TileIndex, DestinationComponent->ContainerSettings[Item.ContainerIndex]);
		for(auto& CurrentContainer : AddedContainers)
		{
			for(auto& CurrentItem : DestinationComponent->ContainerSettings[CurrentContainer.ContainerIndex].Items)
			{
				CurrentItem = DestinationComponent->GetItemByUniqueID(CurrentItem.UniqueID);
				DestinationComponent->ItemAdded.Broadcast(CurrentItem, CurrentItem.TileIndex, DestinationComponent->ContainerSettings[CurrentItem.ContainerIndex]);
			}
		}
	}
	
	//Call equip dispatcher
	if(DestinationComponent->ContainerSettings[Item.ContainerIndex].ContainerType == Equipment)
	{
		UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(Item, true, TArray<FName>());
	}
}

void UAC_Inventory::TryAddNewItemByDataAsset(UDA_CoreItem* ItemAsset, int32 Count,
	UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta, int32 ContainerIndex, int32 TileIndex)
{
	Result = false;
	NewItem = FS_InventoryItem();
	StackDelta = 0;
	
	if(!IsValid(ItemAsset))
	{
		return;
	}
	
	FS_InventoryItem ItemData;
	ItemData.ItemAsset = ItemAsset;
	ItemData.Count = Count;
	ItemData.ContainerIndex = ContainerIndex;
	ItemData.TileIndex = TileIndex;

	TArray<FS_ContainerSettings> ItemsContainers = ItemAsset->GetDefaultContainers();
	TryAddNewItem(ItemData, ItemsContainers, DestinationComponent, CallItemAdded, SkipStacking, Result, NewItem, StackDelta);
}

void UAC_Inventory::StackTwoItems(FS_InventoryItem Item1, FS_InventoryItem Item2, int32& Item1RemainingCount, int32& Item2NewStackCount)
{
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
		return;
	}
	C_AddItemToNetworkQueue(Item1.UniqueID);
	C_AddItemToNetworkQueue(Item2.UniqueID);
	S_StackTwoItems(Item1.UniqueID, Item2.UniqueID, GetOwner()->GetLocalRole());
	Item1RemainingCount = UKismetMathLibrary::Clamp((Item2.Count + Item1.Count) - UFL_InventoryFramework::GetItemMaxStack(Item2), 0, UFL_InventoryFramework::GetItemMaxStack(Item1));
	Item2NewStackCount = UKismetMathLibrary::Clamp(Item2.Count + Item1.Count, 1, UFL_InventoryFramework::GetItemMaxStack(Item2));
}

void UAC_Inventory::S_StackTwoItems_Implementation(FS_UniqueID Item1ID, FS_UniqueID Item2ID, ENetRole CallerLocalRole)
{
	if(!IsValid(Item1ID.ParentComponent) || !IsValid(Item2ID.ParentComponent)) { return; }
	
	FS_InventoryItem Item1 = Item1ID.ParentComponent->GetItemByUniqueID(Item1ID);
	
	FS_InventoryItem Item2 = Item2ID.ParentComponent->GetItemByUniqueID(Item2ID);
	
	if(!Item1.IsValid() || !Item2.IsValid())
	{
		C_RemoveItemFromNetworkQueue(Item1.UniqueID);
		C_RemoveItemFromNetworkQueue(Item2.UniqueID);
		return;
	}

	int32 Item1RemainingCount;
	int32 Item2NewStackCount;

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
			}
			else
			{
				C_StackTwoItems(Item1ID, Item2ID);
				Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
			}
		}
		else
		{
			C_StackTwoItems(Item1ID, Item2ID);
			Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
		}
	}
	else
	{
		C_StackTwoItems(Item1ID, Item2ID);
		Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
	}

	TArray<UAC_Inventory*> CombinedListeners;
	for(auto& AppendingListener : Item1ID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item1ID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item2ID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Item2ID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}

	
	for(const auto& CurrentListener : CombinedListeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_StackTwoItems(Item1ID, Item2ID);
		}
	}
}

bool UAC_Inventory::S_StackTwoItems_Validate(FS_UniqueID Item1ID, FS_UniqueID Item2ID, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::C_StackTwoItems_Implementation(FS_UniqueID Item1ID, FS_UniqueID Item2ID)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item1 = Item1ID.ParentComponent->GetItemByUniqueID(Item1ID);
	
	FS_InventoryItem Item2 = Item2ID.ParentComponent->GetItemByUniqueID(Item2ID);

	if(!Item1.IsValid() || !Item2.IsValid())
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either Item1 or Item2 was not found for stacking - AC_Inventory.cpp"), true, true);
		return;
	}
	
	int32 Item1RemainingCount;
	int32 Item2NewStackCount;
	Internal_StackTwoItems(Item1, Item2, Item1RemainingCount, Item2NewStackCount);
	C_RemoveItemFromNetworkQueue(Item1.UniqueID);
	C_RemoveItemFromNetworkQueue(Item2.UniqueID);
}

void UAC_Inventory::Internal_StackTwoItems(FS_InventoryItem Item1, FS_InventoryItem Item2, int32& Item1RemainingCount, int32& Item2NewStackCount)
{
	if(UFL_InventoryFramework::IsItemValid(Item1) && UFL_InventoryFramework::IsItemValid(Item2))
	{
		FS_InventoryItem& Item1Ref = Item1.UniqueID.ParentComponent->ContainerSettings[Item1.ContainerIndex].Items[Item1.ItemIndex];
		FS_InventoryItem& Item2Ref = Item2.UniqueID.ParentComponent->ContainerSettings[Item2.ContainerIndex].Items[Item2.ItemIndex];
		if(bool StackCheck = UFL_InventoryFramework::CanStackItems(Item1, Item2); StackCheck)
		{
			const int32 Item1Count = Item1.Count;
			const int32 Item2Count = Item2.Count;

			Item1RemainingCount = UKismetMathLibrary::Clamp((Item2Count + Item1Count) - UFL_InventoryFramework::GetItemMaxStack(Item2), 0, UFL_InventoryFramework::GetItemMaxStack(Item1));
			Item2NewStackCount = UKismetMathLibrary::Clamp(Item2Count + Item1Count, 1, UFL_InventoryFramework::GetItemMaxStack(Item2));

			Item1Ref.Count = Item1RemainingCount;
			Item2Ref.Count = Item2NewStackCount;

			//Notify everyone of the new item count
			UFL_ExternalObjects::BroadcastItemCountUpdated(Item1, Item1Count, Item1RemainingCount);
			UFL_ExternalObjects::BroadcastItemCountUpdated(Item2, Item2Count, Item2NewStackCount);

			bool RemoveItemSuccess;
			if(Item1Ref.Count <= 0)
			{
				bool bCallItemRemoved = Item1.UniqueID.ParentComponent != Item2.UniqueID.ParentComponent;
				FRandomStream Seed;
				Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
				Internal_RemoveItemFromInventory(Item1, bCallItemRemoved, true, true, true, true, Seed, RemoveItemSuccess);
			}
			if(Item2Ref.Count <= 0) //Variann: This won't ever be true, right? Count for Item2 is never being deducted, so it won't ever go above 0. Remove this?
			{
				FRandomStream Seed;
				Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
				Internal_RemoveItemFromInventory(Item2, true, true, true, true, true, Seed, RemoveItemSuccess);
			}
		}
		//A designer might use the return of either items to remove them if <= 0, set their count to the items original count so that doesn't happen.
		else
		{
			Item1RemainingCount = Item1.Count;
			Item2NewStackCount = Item2.Count;
		}
	}
}

void UAC_Inventory::SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex,
	int32& Item1RemainingCount, int32& Item2NewStackCount)
{
	Item1RemainingCount = UKismetMathLibrary::Clamp((SplitAmount + Item.Count) - UFL_InventoryFramework::GetItemMaxStack(Item), 0, UFL_InventoryFramework::GetItemMaxStack(Item));
	Item2NewStackCount = UKismetMathLibrary::Clamp(SplitAmount, 0, Item.ItemAsset->MaxStack);
	
	if(!CanSplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Can't split item - AC_Inventory.cpp -> SplitItem"), true, true);
		return;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FS_UniqueID NewStackUniqueID = DestinationComponent->GenerateUniqueID();
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
		return;
	}

	C_AddItemToNetworkQueue(Item.UniqueID);
	S_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_SplitItem_Implementation(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex, ENetRole CallerLocalRole)
{
	int32 Item1RemainingCount;
	int32 Item2NewStackCount;

	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	FS_UniqueID NewStackUniqueID = DestinationComponent->GenerateUniqueID();

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
			}
			else
			{
				Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
				C_SplitItem(Item, SplitAmount, DestinationComponent, Item.ItemAsset, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Seed);
			}
		}
		else
		{
			Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
			C_SplitItem(Item, SplitAmount, DestinationComponent, Item.ItemAsset, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Seed);
		}
	}
	else
	{
		Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
		C_SplitItem(Item, SplitAmount, DestinationComponent, Item.ItemAsset, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Seed);
	}

	TArray<UAC_Inventory*> CombinedListeners;
	for(auto& AppendingListener : Item.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : DestinationComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	
	for(const auto& CurrentListener : CombinedListeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_SplitItem(Item, SplitAmount, DestinationComponent, Item.ItemAsset, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Seed);
		}
	}
}

bool UAC_Inventory::S_SplitItem_Validate(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex, ENetRole CallerLocalRole)
{
	if(!IsValid(Item.UniqueID.ParentComponent) || !IsValid(DestinationComponent)) { return false; }
	if(!DestinationComponent->ContainerSettings.IsValidIndex(NewStackContainerIndex)) { return false; }
	if(!DestinationComponent->ContainerSettings[NewStackContainerIndex].TileMap.IsValidIndex(NewStackTileIndex)) { return false; }
	if(!UFL_InventoryFramework::IsItemValid(Item)) { return false; }
	
	if(!CanSplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Can't split item - AC_Inventory.cpp -> S_SplitItem_Validate"));
		return false;
	}

	return true;
}

void UAC_Inventory::C_SplitItem_Implementation(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, UDA_CoreItem* ItemDataAsset,
	int32 NewStackContainerIndex, int32 NewStackTileIndex, FS_UniqueID NewStackUniqueID, FRandomStream Seed)
{
	int32 Item1RemainingCount;
	int32 Item2NewStackCount;
	Internal_SplitItem(Item, SplitAmount, DestinationComponent, NewStackContainerIndex, NewStackTileIndex, NewStackUniqueID, Item1RemainingCount, Item2NewStackCount, Seed);
	C_RemoveItemFromNetworkQueue(Item.UniqueID);
}

void UAC_Inventory::Internal_SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex,
	FS_UniqueID NewStackUniqueID, int32& Item1RemainingCount, int32& Item2NewStackCount, FRandomStream Seed)
{
	//First check if there's a colliding item. If we can stack with it, attempt to stack.
	bool SpotAvailable;
	int32 AvailableTile;
	TArray<FS_InventoryItem> ItemsInTheWay;
	TArray<FS_InventoryItem> ItemsToIgnore;
	TArray<int32> TilesToIgnore = GetGenericIndexesToIgnore(DestinationComponent->ContainerSettings[NewStackContainerIndex]);
	DestinationComponent->CheckForSpace(Item, DestinationComponent->ContainerSettings[NewStackContainerIndex], NewStackTileIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, AvailableTile, ItemsInTheWay);
	if(ItemsInTheWay.Contains(Item))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Can't split item, colliding with itself - AC_Inventory.cpp -> Internal_SplitItem"));
		return;
	}
	if(!ItemsInTheWay.IsEmpty())
	{
		if(UFL_InventoryFramework::CanStackItems(Item, ItemsInTheWay[0]))
		{
			int32 NewStackCount;
			if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
			{
				Internal_ReduceItemCount(Item, SplitAmount, true, Seed);
			}
			else
			{
				//If this is a client, the item we are splitting might be rooted in a component
				//the client does not have data from. In that case, we can't reduce the count,
				//but we also don't need to. Server will reduce it.
				FS_InventoryItem FoundItem = GetItemByUniqueID(Item.UniqueID);
				if(FoundItem.IsValid())
				{
					Item = FoundItem;
					Internal_ReduceItemCount(Item, SplitAmount, true, Seed);
				}
			}
			Internal_IncreaseItemCount(ItemsInTheWay[0].UniqueID, SplitAmount, NewStackCount);
			Item1RemainingCount = Item.Count - SplitAmount;
			Item.Count -= SplitAmount;
			Item2NewStackCount = ItemsInTheWay[0].Count;
			if(Item1RemainingCount <= 0)
			{
				return;
			}
		}
		
		return;
	}
	
	//Reduce the original items count
	if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
	{
		Internal_ReduceItemCount(Item, SplitAmount, true, Seed);
	}
	else
	{
		//If this is a client, the item we are splitting might be rooted in a component
		//the client does not have data from. In that case, we can't reduce the count,
		//but we also don't need to. Server will reduce it.
		FS_InventoryItem FoundItem = Item.UniqueID.ParentComponent->GetItemByUniqueID(Item.UniqueID);
		if(FoundItem.IsValid())
		{
			Item = FoundItem;
			Internal_ReduceItemCount(Item, SplitAmount, true, Seed);
		}
	}
	
	Item1RemainingCount = Item.Count;

	//Start creating the new item
	FS_InventoryItem NewStackItem;
	NewStackItem.ItemAsset = Item.ItemAsset;
	NewStackItem.Rotation = Item.Rotation;
	NewStackItem.ContainerIndex = NewStackContainerIndex;
	NewStackItem.TileIndex = NewStackTileIndex;
	NewStackItem.Count = UKismetMathLibrary::Clamp(SplitAmount, 0, Item.ItemAsset->MaxStack);
	NewStackItem.ItemIndex = DestinationComponent->ContainerSettings[NewStackContainerIndex].Items.Num();
	NewStackItem.UniqueID = NewStackUniqueID;
	UFL_InventoryFramework::AddDefaultTagsToItem(NewStackItem, false);
	UFL_InventoryFramework::AddDefaultTagValuesToItem(NewStackItem, false, false);
	Item2NewStackCount = NewStackItem.Count;

	//Add the item before adding the widget, so indexes can be refreshed for both simultaneously
	DestinationComponent->AddItemToTileMap(NewStackItem);
	DestinationComponent->ContainerSettings[NewStackContainerIndex].Items.Add(NewStackItem);

	if(UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(DestinationComponent->ContainerSettings[NewStackContainerIndex]))
	{
		UW_InventoryItem* ItemWidget = nullptr;
		ContainerWidget->CreateWidgetForItem(NewStackItem, ItemWidget);
	}
	
	DestinationComponent->RefreshItemsIndexes(ContainerSettings[NewStackContainerIndex]);
	DestinationComponent->RefreshItemsIndexes(DestinationComponent->ContainerSettings[NewStackContainerIndex]);
}

void UAC_Inventory::IncreaseItemCount(FS_InventoryItem Item, int32 Count, int32& NewCount)
{
	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_IncreaseItemCount(Item.UniqueID, Count, NewCount);
		return;
	}
	S_IncreaseItemCount(Item.UniqueID, Count, GetOwner()->GetLocalRole());
	if(Item.ItemAsset->CanItemStack())
	{
		NewCount = FMath::Clamp(Item.Count + Count, 1, UFL_InventoryFramework::GetItemMaxStack(Item));
	}
	else
	{
		NewCount = Item.Count;
	}
	C_AddItemToNetworkQueue(Item.UniqueID);
}

void UAC_Inventory::S_IncreaseItemCount_Implementation(FS_UniqueID ItemID, int32 Count, ENetRole CallerLocalRole)
{
	int32 NewStackCount;

	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_IncreaseItemCount(ItemID, Count, NewStackCount);
			}
			else
			{
				Internal_IncreaseItemCount(ItemID, Count, NewStackCount);
				C_IncreaseItemCount(ItemID, Count);
			}
		}
		else
		{
			Internal_IncreaseItemCount(ItemID, Count, NewStackCount);
			C_IncreaseItemCount(ItemID, Count);
		}
	}
	else
	{
		Internal_IncreaseItemCount(ItemID, Count, NewStackCount);
		C_IncreaseItemCount(ItemID, Count);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_IncreaseItemCount(ItemID, Count);
		}
	}
}

bool UAC_Inventory::S_IncreaseItemCount_Validate(FS_UniqueID ItemID, int32 Count, ENetRole CallerLocalRole)
{
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	
	//Check if any of the data is dirty
	if(!UFL_InventoryFramework::IsItemValid(Item)) { return false; }

	return true;
}

void UAC_Inventory::C_IncreaseItemCount_Implementation(FS_UniqueID ItemID, int32 Count)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		int32 NewStackCount;
		Internal_IncreaseItemCount(ItemID, Count, NewStackCount);
		C_RemoveItemFromNetworkQueue(Item.UniqueID);
	}
}

void UAC_Inventory::Internal_IncreaseItemCount(FS_UniqueID ItemID, int32 Count, int32& NewCount)
{
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}

	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	if(IsValid(Item.ItemAsset))
	{
		if(Item.ItemAsset->CanItemStack())
		{
			int32 OldCount = Item.Count;
			NewCount = FMath::Clamp(Item.Count + Count, 1, UFL_InventoryFramework::GetItemMaxStack(Item));
			ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].Count = NewCount;

			UFL_ExternalObjects::BroadcastItemCountUpdated(Item, OldCount, NewCount);
		}
	}
}

void UAC_Inventory::ReduceItemCount(FS_InventoryItem Item, int32 Count, bool RemoveItemIf0, int32& NewCount)
{
	NewCount = FMath::Clamp(Item.Count - Count, 0, UFL_InventoryFramework::GetItemMaxStack(Item));

	if(!IsValid(Item.UniqueID.ParentComponent))
	{
		return;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
		return;
	}
	C_AddItemToNetworkQueue(Item.UniqueID);
	S_ReduceItemCount(Item.UniqueID, Count, RemoveItemIf0, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_ReduceItemCount_Implementation(FS_UniqueID ItemID, int32 Count, bool RemoveItemIf0, ENetRole CallerLocalRole)
{
	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}
	
	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
			}
			else
			{
				Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
				C_ReduceItemCount(ItemID, Count, RemoveItemIf0, Seed);
			}
		}
		else
		{
			Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
			C_ReduceItemCount(ItemID, Count, RemoveItemIf0, Seed);
		}
	}
	else
	{
		Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
		C_ReduceItemCount(ItemID, Count, RemoveItemIf0, Seed);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_ReduceItemCount(ItemID, Count, RemoveItemIf0, Seed);
		}
	}
}

bool UAC_Inventory::S_ReduceItemCount_Validate(FS_UniqueID ItemID, int32 Count, bool RemoveItemIf0, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::C_ReduceItemCount_Implementation(FS_UniqueID ItemID, int32 Count, bool RemoveItemIf0, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_ReduceItemCount(Item, Count, RemoveItemIf0, Seed);
		C_RemoveItemFromNetworkQueue(Item.UniqueID);
	}
}

void UAC_Inventory::Internal_ReduceItemCount(FS_InventoryItem Item, int32 Count, bool RemoveItemIf0, FRandomStream Seed)
{
	if(!IsValid(Item.UniqueID.ParentComponent))
	{
		return;
	}

	UFL_InventoryFramework::UpdateItemStruct(Item);
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}

	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	int32 OldCount = Item.Count;
	const int32 NewCount = FMath::Clamp(Item.Count - Count, 0, UFL_InventoryFramework::GetItemMaxStack(Item));
	ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].Count = NewCount;

	UFL_ExternalObjects::BroadcastItemCountUpdated(Item, OldCount, NewCount);
	
	if(NewCount == 0 && RemoveItemIf0)
	{
		bool Success;
			
		Internal_RemoveItemFromInventory(Item, true, true, RemoveItemIf0, true, true, Seed, Success);
	}
}

void UAC_Inventory::MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, bool RemoveItemsIf0, int32& RemainingCount)
{
	int32 FoundTotalCount;
	TArray<FS_ItemCount> MatchingItems = TargetComponent->GetListOfItemsByCount(Item, Count, ContainerIndex, FoundTotalCount);
	if(!MatchingItems.IsValidIndex(0))
	{
		RemainingCount = 0;
		return;
	}

	//Isn't this redundant? We already have FoundTotalCount,
	//why not just deduct that?
	//Come back to this for 2.3
	RemainingCount = Count;
	for(const auto& CurrentItem : MatchingItems)
	{
		RemainingCount -= CurrentItem.Count;
	}
	
	if(!UKismetSystemLibrary::IsServer(this))
	{
		//Add the items we will be working with to the network queue
		for(const auto& CurrentItem : MatchingItems)
		{
			C_AddItemToNetworkQueue(CurrentItem.Item.UniqueID);
		}
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
		return;
	}
	
	S_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, RemoveItemsIf0, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_MassReduceCount_Implementation(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, bool RemoveItemsIf0, ENetRole CallerLocalRole)
{
	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
			}
			else
			{
				Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
				C_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
			}
		}
		else
		{
			Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
			C_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
		}
	}
	else
	{
		Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
		C_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
	}
	
	for(const auto& CurrentListener : TargetComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
		}
	}
}

void UAC_Inventory::C_MassReduceCount_Implementation(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex,
	FRandomStream Seed, bool RemoveItemsIf0)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	int32 FoundTotalCount;
	TArray<FS_ItemCount> MatchingItems = TargetComponent->GetListOfItemsByCount(Item, Count, ContainerIndex, FoundTotalCount);
	Internal_MassReduceCount(Item, Count, TargetComponent, ContainerIndex, Seed, RemoveItemsIf0);
	
	if(!MatchingItems.IsValidIndex(0))
	{
		return;
	}
	
	if(!UKismetSystemLibrary::IsServer(this))
	{
		//Add the items we will be working with to the network queue
		for(const auto& CurrentItem : MatchingItems)
		{
			C_RemoveItemFromNetworkQueue(CurrentItem.Item.UniqueID);
		}
	}
}

void UAC_Inventory::Internal_MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, FRandomStream Seed, bool RemoveItemsIf0)
{
	int32 FoundTotalCount;
	TArray<FS_ItemCount> MatchingItems = TargetComponent->GetListOfItemsByCount(Item, Count, ContainerIndex, FoundTotalCount);
	
	if(!MatchingItems.IsValidIndex(0))
	{
		return;
	}

	for(auto& CurrentItem : MatchingItems)
	{
		TargetComponent->Internal_ReduceItemCount(CurrentItem.Item, CurrentItem.Count, RemoveItemsIf0, Seed);
		Seed.Initialize(Seed.GetInitialSeed() + 1);
	}
}

void UAC_Inventory::NotifyItemSold(FS_InventoryItem Item, UIDA_Currency* Currency, int32 Amount, UAC_Inventory* Buyer, UAC_Inventory* Seller)
{
	if(!UKismetSystemLibrary::IsServer(this))
	{
		Seller->SoldItem.Broadcast(Item, Buyer, Currency, Amount);
		Buyer->BoughtItem.Broadcast(Item, Seller, Currency, Amount);
	}
	S_NotifyItemSold(Item, Currency, Amount, Buyer, Seller);
}

void UAC_Inventory::UpdateItemsOverrideSettings(FS_InventoryItem Item, FItemOverrideSettings NewSettings, AActor* ActorRequestingChange)
{
	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return;
	}

	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}
	
	if(!CanActorChangeItemOverrideSettings(ActorRequestingChange, Item))
	{
		return;
	}

	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_UpdateItemsOverrideSettings(Item, NewSettings);
		return;
	}

	C_AddItemToNetworkQueue(Item.UniqueID);
	S_UpdateItemsOverrideSettings(Item.UniqueID, NewSettings, GetOwner()->GetLocalRole(), ActorRequestingChange);
}

void UAC_Inventory::S_UpdateItemsOverrideSettings_Implementation(FS_UniqueID ItemID, FItemOverrideSettings NewSettings, ENetRole CallerLocalRole, AActor* ActorRequestingChange)
{
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);

	if(!Item.IsValid())
	{
		return;
	}

	if(!CanActorChangeItemOverrideSettings(ActorRequestingChange, Item))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_UpdateItemsOverrideSettings(Item, NewSettings);
			}
			else
			{
				Internal_UpdateItemsOverrideSettings(Item, NewSettings);
				C_UpdateItemsOverrideSettings(Item.UniqueID, NewSettings);
			}
		}
		else
		{
			Internal_UpdateItemsOverrideSettings(Item, NewSettings);
			C_UpdateItemsOverrideSettings(Item.UniqueID, NewSettings);
		}
	}
	else
	{
		Internal_UpdateItemsOverrideSettings(Item, NewSettings);
		C_UpdateItemsOverrideSettings(Item.UniqueID, NewSettings);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_UpdateItemsOverrideSettings(Item.UniqueID, NewSettings);
		}
	}
}

bool UAC_Inventory::S_UpdateItemsOverrideSettings_Validate(FS_UniqueID ItemID, FItemOverrideSettings NewSettings, ENetRole CallerLocalRole, AActor* ActorRequestingChange)
{
	return true;
}

void UAC_Inventory::C_UpdateItemsOverrideSettings_Implementation(FS_UniqueID ItemID,
	FItemOverrideSettings NewSettings)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_UpdateItemsOverrideSettings(Item, NewSettings);
		C_RemoveItemFromNetworkQueue(Item.UniqueID);
	}
}

void UAC_Inventory::Internal_UpdateItemsOverrideSettings(FS_InventoryItem Item, FItemOverrideSettings NewSettings)
{
	//Store old settings for interface call
	
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		//Item directions are invalid, can't update item
		return;
	}

	FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, true);
	FItemOverrideSettings OldOverride = *OverrideFragment;
	*OverrideFragment = NewSettings;
	
	UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(Item);
	
	//Update the item widget
	if(IsValid(ItemWidget))
	{
		II_ExternalObjects::Execute_OverrideSettingsUpdated(ItemWidget, Item, OldOverride, NewSettings);
	}
	
	//Update the items external widgets
	for(const auto& CurrentWidget : UFL_InventoryFramework::GetExternalObjectsFromItem(Item))
	{
		if(IsValid(CurrentWidget))
		{
			if(UKismetSystemLibrary::DoesImplementInterface(CurrentWidget, UI_ExternalObjects::StaticClass()))
			{
				II_ExternalObjects::Execute_OverrideSettingsUpdated(CurrentWidget, Item, OldOverride, NewSettings);
			}
		}
	}
}

bool UAC_Inventory::AddTagToItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue)
{
	if(UFL_InventoryFramework::GetItemsTags(Item, false).HasTagExact(Tag))
	{
		return false;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagBeAddedToItem(Tag, Item))
		{
			Internal_AddTagToItem(Item, Tag);
			return true;
		}
		return false;
	}
	
	S_AddTagToItem(Item.UniqueID, Tag, GetOwner()->GetLocalRole());
	if(!IgnoreNetworkQueue)
	{
		C_AddItemToNetworkQueue(Item.UniqueID);
	}
	return true;
}

void UAC_Inventory::S_AddTagToItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	FS_InventoryItem OriginalItem = ItemID.ParentComponent->GetItemByUniqueID(ItemID);

	if(!OriginalItem.IsValid())
	{
		return;
	}

	if(UFL_InventoryFramework::GetItemsTags(OriginalItem, false).HasTagExact(Tag))
	{
		//Item already has tag, skip RPC's.
		if(!IgnoreNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(ItemID);	
		}
		return;
	}

	if(!CanTagBeAddedToItem(Tag, OriginalItem))
	{
		//Tag is not allowed
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_AddTagToItem(OriginalItem, Tag);
			}
			else
			{
				Internal_AddTagToItem(OriginalItem, Tag);
				C_AddTagToItem(ItemID, Tag, IgnoreNetworkQueue);
			}
		}
		else
		{
			Internal_AddTagToItem(OriginalItem, Tag);
			C_AddTagToItem(ItemID, Tag, IgnoreNetworkQueue);
		}
	}
	else
	{
		Internal_AddTagToItem(OriginalItem, Tag);
		C_AddTagToItem(ItemID, Tag, IgnoreNetworkQueue);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_AddTagToItem(ItemID, Tag, IgnoreNetworkQueue);
		}
	}
}

bool UAC_Inventory::S_AddTagToItem_Validate(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	return true;
}

void UAC_Inventory::C_AddTagToItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, bool IgnoreNetworkQueue)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_AddTagToItem(Item, Tag);
		if(!IgnoreNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
		}
	}
}

void UAC_Inventory::Internal_AddTagToItem(FS_InventoryItem Item, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}

	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, true);
	if(TagFragment->Tags.HasTagExact(Tag))
	{
		return;
	}

	if(TagFragment)
	{
		TagFragment->Tags.AddTag(Tag);
		UFL_ExternalObjects::BroadcastTagsUpdated(Tag, true, Item, FS_ContainerSettings());
	}
}

bool UAC_Inventory::CanTagBeAddedToItem_Implementation(FGameplayTag Tag, FS_InventoryItem Item)
{
	return true;
}

bool UAC_Inventory::RemoveTagFromItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue)
{
	if(!UFL_InventoryFramework::GetItemsTags(Item, false).HasTagExact(Tag))
	{
		return false;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagBeRemovedFromItem(Tag, Item))
		{
			Internal_RemoveTagFromItem(Item, Tag);
			return true;
		}
		return false;
	}
	
	S_RemoveTagFromItem(Item.UniqueID, Tag, GetOwner()->GetLocalRole());
	if(!IgnoreNetworkQueue)
	{
		C_AddItemToNetworkQueue(Item.UniqueID);
	}
	return true;
}


void UAC_Inventory::S_RemoveTagFromItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	FS_InventoryItem OriginalItem = ItemID.ParentComponent->GetItemByUniqueID(ItemID);

	if(!OriginalItem.IsValid())
	{
		return;
	}

	if(!UFL_InventoryFramework::GetItemsTags(OriginalItem, false).HasTagExact(Tag))
	{
		//Item already doesn't have tag, skip RPC's.
		if(!IgnoreNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(OriginalItem.UniqueID);
		}
		return;
	}

	if(!CanTagBeRemovedFromItem(Tag, OriginalItem))
	{
		//Tag is not allowed to be removed.
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveTagFromItem(OriginalItem, Tag);
			}
			else
			{
				Internal_RemoveTagFromItem(OriginalItem, Tag);
				C_RemoveTagFromItem(ItemID, Tag, IgnoreNetworkQueue);
			}
		}
		else
		{
			Internal_RemoveTagFromItem(OriginalItem, Tag);
			C_RemoveTagFromItem(ItemID, Tag, IgnoreNetworkQueue);
		}
	}
	else
	{
		Internal_RemoveTagFromItem(OriginalItem, Tag);
		C_RemoveTagFromItem(ItemID, Tag, IgnoreNetworkQueue);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveTagFromItem(ItemID, Tag, IgnoreNetworkQueue);
		}
	}
}

bool UAC_Inventory::S_RemoveTagFromItem_Validate(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	return true;
}

void UAC_Inventory::C_RemoveTagFromItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, bool DoNotAddToNetworkQueue)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_RemoveTagFromItem(Item, Tag);
		if(!DoNotAddToNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
		}
	}
}

void UAC_Inventory::Internal_RemoveTagFromItem(FS_InventoryItem Item, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}
	
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, true);

	if(!TagFragment->Tags.HasTagExact(Tag))
	{
		return;
	}

	if(TagFragment)
	{
		TagFragment->Tags.RemoveTag(Tag);
		UFL_ExternalObjects::BroadcastTagsUpdated(Tag, false, Item, FS_ContainerSettings());
	}
}

bool UAC_Inventory::CanTagBeRemovedFromItem_Implementation(FGameplayTag Tag, FS_InventoryItem Item)
{
	return true;
}

float UAC_Inventory::GetTotalValueOfTag(FGameplayTag Tag, TArray<TEnumAsByte<EContainerType>> ContainersToCheck, bool Items, bool Containers, bool Component)
{
	float AccumulatedValue = 0;

	for(auto& CurrentContainer : ContainerSettings)
	{
		if(ContainersToCheck.Contains(CurrentContainer.ContainerType))
		{
			if(Containers)
			{
				FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentContainer.ContainerFragments, false);
				AccumulatedValue += UFL_InventoryFramework::GetValueForTag(TagFragment->TagValues, Tag).Value;
			}

			if(Items)
			{
				for(auto& CurrentItem : CurrentContainer.Items)
				{
					FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, false);
					AccumulatedValue += UFL_InventoryFramework::GetValueForTag(TagFragment->TagValues, Tag).Value;
				}
			}
		}
	}

	if(Component)
	{
		AccumulatedValue += UFL_InventoryFramework::GetValueForTag(TagValuesContainer, Tag).Value;
	}

	return AccumulatedValue;
}

bool UAC_Inventory::SetTagValueForItem(FS_InventoryItem Item, FGameplayTag Tag, float Value, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool IgnoreNetworkQueue)
{
	bool Success = false;
	FS_TagValue TagValue;
	TagValue.Tag = Tag;
	TagValue.Value = Value;
	if(!AddIfNotFound)
	{
		FS_TagValue FoundTagValues;
		int32 TagIndex;
		if(!UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetItemsTagValues(Item, false), Tag, FoundTagValues, TagIndex))
		{
			//We don't want to add the tag, and the tag value can not be found. Abort.
			return false;
		}
		else
		{
			if(!CanTagValueBeSetForItem(TagValue, Item))
			{
				//Item has tag, but we aren't allowed to set it.
				return false;
			}
		}
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagValueBeAddedToItem(TagValue, Item))
		{
			Internal_SetTagValueForItem(Item, Tag, Value, AddIfNotFound, CalculationClass, Success);
			return Success;
		}
		return false;
	}
	
	S_SetTagValueForItem(Item.UniqueID, Tag, Value, GetOwner()->GetLocalRole(), AddIfNotFound, CalculationClass);
	if(!IgnoreNetworkQueue)
	{
		C_AddItemToNetworkQueue(Item.UniqueID);	
	}
	return true;
}

void UAC_Inventory::S_SetTagValueForItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool IgnoreNetworkQueue)
{
	FS_TagValue TagValue;
	TagValue.Tag = Tag;
	TagValue.Value = Value;
	FS_InventoryItem OriginalItem = ItemID.ParentComponent->GetItemByUniqueID(ItemID);

	if(!OriginalItem.IsValid())
	{
		return;
	}
	
	if(!AddIfNotFound)
	{
		FS_TagValue FoundTagValues;
		int32 TagIndex;
		if(!UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetItemsTagValues(OriginalItem, false), Tag, FoundTagValues, TagIndex))
		{
			//We don't want to add the tag, and the tag value can not be found. Abort.
			if(!IgnoreNetworkQueue)
			{
				C_RemoveItemFromNetworkQueue(OriginalItem.UniqueID);	
			}
			return;
		}
		else
		{
			if(!CanTagValueBeSetForItem(TagValue, OriginalItem))
			{
				//Item has tag, but we aren't allowed to set it.
				return;
			}
		}
	}

	if(!CanTagValueBeAddedToItem(TagValue, OriginalItem))
	{
		return;
	}

	//Run through calculation class
	if(IsValid(CalculationClass))
	{
		Value = PreItemTagValueCalculation(TagValue, OriginalItem, CalculationClass);
		UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass.GetDefaultObject());
		if(IsValid(Calculator))
		{
			//Find out if the new value has hit some sort of threshold or limit.
			if(Calculator->ShouldValueBeRemovedFromItem(TagValue, OriginalItem))
			{
				//New value has hit some sort of defined limit and should be removed,
				//cancel this function and start removing the item.
				S_RemoveTagValueFromItem(ItemID, Tag, CallerLocalRole, IgnoreNetworkQueue);
				return;
			}
		}
	}	
	
	bool Success;


	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_SetTagValueForItem(OriginalItem, Tag, Value, AddIfNotFound, CalculationClass, Success);
			}
			else
			{
				Internal_SetTagValueForItem(OriginalItem, Tag, Value, AddIfNotFound, CalculationClass, Success);
				C_SetTagValueForItem(ItemID, Tag, Value, AddIfNotFound, IgnoreNetworkQueue);
			}
		}
		else
		{
			Internal_SetTagValueForItem(OriginalItem, Tag, Value, AddIfNotFound, CalculationClass, Success);
			C_SetTagValueForItem(ItemID, Tag, Value, AddIfNotFound, IgnoreNetworkQueue);
		}
	}
	else
	{
		Internal_SetTagValueForItem(OriginalItem, Tag, Value, AddIfNotFound, CalculationClass, Success);
		C_SetTagValueForItem(ItemID, Tag, Value, AddIfNotFound, IgnoreNetworkQueue);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_SetTagValueForItem(ItemID, Tag, Value, AddIfNotFound, IgnoreNetworkQueue);
		}
	}
}

bool UAC_Inventory::S_SetTagValueForItem_Validate(FS_UniqueID ItemID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool IgnoreNetworkQueue)
{
	return true;
}

void UAC_Inventory::C_SetTagValueForItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, float Value, bool AddIfNotFound, bool IgnoreNetworkQueue)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		bool Success;
		//Don't pass in a calculation class since we are receiving this RPC from the server, which has already done the calculation
		Internal_SetTagValueForItem(Item, Tag, Value, AddIfNotFound, nullptr, Success);
		if(!IgnoreNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
		}
	}
}

void UAC_Inventory::Internal_SetTagValueForItem(FS_InventoryItem Item, FGameplayTag Tag, float Value, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool& Success)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	Success = false;
	
	if(!IsValid(ParentComponent))
	{
		return;
	}

	FS_TagValue NewTagValue;
	NewTagValue.Tag = Tag;
	NewTagValue.Value = Value;

	//Only run through the calculation class if we are playing standalone.
	//If this is multiplayer, the server has already calculated the value.
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(IsValid(CalculationClass))
		{
			Value = PreItemTagValueCalculation(NewTagValue, Item, CalculationClass);
			NewTagValue.Value = Value;
			if(UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass.GetDefaultObject()); IsValid(Calculator))
			{
				//Find out if the new value has hit some sort of threshold or limit.
				if(Calculator->ShouldValueBeRemovedFromItem(NewTagValue, Item))
				{
					//New value has hit some sort of defined limit and should be removed,
					//cancel this function and start removing the item.
					Internal_RemoveTagValueFromItem(Item, Tag);
					return;
				}
			}
		}
	}

	FS_TagValue FoundTagValue;
	int32 TagIndex;
	FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, AddIfNotFound);
	if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagFragment->TagValues, Tag, FoundTagValue, TagIndex))
	{
		TagFragment->TagValues[TagIndex].Value = Value;
		ParentComponent->ItemTagValueUpdated.Broadcast(Item, NewTagValue, NewTagValue.Value - FoundTagValue.Value);
		Success = true;
	}
	else
	{
		if(AddIfNotFound)
		{
			TagFragment->TagValues.AddUnique(NewTagValue);
			ParentComponent->ItemTagValueUpdated.Broadcast(Item, NewTagValue, NewTagValue.Value);
			Success = true;
		}
		else
		{
			//Item doesn't have tag and @AddIfNotFound is set to false.
			//End the logic here so we don't send any false widget updates.
			return;
		}
	}

	UFL_ExternalObjects::BroadcastTagValueUpdated(NewTagValue, true, NewTagValue.Value - FoundTagValue.Value, Item, FS_ContainerSettings());
}

float UAC_Inventory::PreItemTagValueCalculation(FS_TagValue TagValue, FS_InventoryItem Item,
	TSubclassOf<UO_TagValueCalculation> CalculationClass)
{
	if(!IsValid(CalculationClass))
	{
		return TagValue.Value;
	}
	
	UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass->GetDefaultObject());
	return Calculator->CalculateItemTagValue(TagValue, Item);
}

bool UAC_Inventory::CanTagValueBeAddedToItem_Implementation(FS_TagValue TagValue, FS_InventoryItem Item)
{
	return true;
}

bool UAC_Inventory::CanTagValueBeSetForItem_Implementation(FS_TagValue TagValue, FS_InventoryItem Item)
{
	return true;
}

bool UAC_Inventory::RemoveTagValueFromItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue)
{
	FS_TagValue FoundTagValue;
	int32 TagIndex;
	if(!UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetItemsTagValues(Item, false), Tag, FoundTagValue, TagIndex))
	{
		return false;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagValueBeRemovedFromItem(Tag, Item))
		{
			Internal_RemoveTagValueFromItem(Item, Tag);
			return true;
		}
		return false;
	}
	
	S_RemoveTagValueFromItem(Item.UniqueID, Tag, GetOwner()->GetLocalRole());
	if(!IgnoreNetworkQueue)
	{
		C_AddItemToNetworkQueue(Item.UniqueID);
	}
	return true;
}


void UAC_Inventory::S_RemoveTagValueFromItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	FS_InventoryItem OriginalItem = ItemID.ParentComponent->GetItemByUniqueID(ItemID);

	if(!OriginalItem.IsValid())
	{
		return;
	}
	
	if(!CanTagValueBeRemovedFromItem(Tag, OriginalItem))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveTagValueFromItem(OriginalItem, Tag);
			}
			else
			{
				Internal_RemoveTagValueFromItem(OriginalItem, Tag);
				C_RemoveTagValueFromItem(ItemID, Tag, IgnoreNetworkQueue);
			}
		}
		else
		{
			Internal_RemoveTagValueFromItem(OriginalItem, Tag);
			C_RemoveTagValueFromItem(ItemID, Tag, IgnoreNetworkQueue);
		}
	}
	else
	{
		Internal_RemoveTagValueFromItem(OriginalItem, Tag);
		C_RemoveTagValueFromItem(ItemID, Tag, IgnoreNetworkQueue);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveTagValueFromItem(ItemID, Tag);
		}
	}
}

bool UAC_Inventory::S_RemoveTagValueFromItem_Validate(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue)
{
	return true;
}

void UAC_Inventory::C_RemoveTagValueFromItem_Implementation(FS_UniqueID ItemID, FGameplayTag Tag, bool IgnoreNetworkQueue)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_RemoveTagValueFromItem(Item, Tag);
		if(!IgnoreNetworkQueue)
		{
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
		}
	}
}

void UAC_Inventory::Internal_RemoveTagValueFromItem(FS_InventoryItem Item, FGameplayTag Tag)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	
	if(!IsValid(ParentComponent))
	{
		return;
	}

	if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments))
	{
		FS_TagValue FoundTagValue;
		int32 TagIndex;
		if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagFragment->TagValues, Tag, FoundTagValue, TagIndex))
		{
			TagFragment->TagValues.RemoveAt(TagIndex);
			ParentComponent->ItemTagValueUpdated.Broadcast(Item, FoundTagValue, FoundTagValue.Value * -1);
		
			UFL_ExternalObjects::BroadcastTagValueUpdated(FoundTagValue, true, FoundTagValue.Value * -1, Item, FS_ContainerSettings());
		}
	}
}

bool UAC_Inventory::CanTagValueBeRemovedFromItem_Implementation(FGameplayTag TagValue, FS_InventoryItem Item)
{
	return true;
}

TArray<FS_InventoryItem> UAC_Inventory::GetItemsByTag(FGameplayTag Tag, int32 ContainerIndex)
{
	TArray<FS_ContainerSettings> ContainersToSearch;
	TArray<FS_InventoryItem> FoundItems;
	if(ContainerIndex == -1)
	{
		ContainersToSearch = ContainerSettings;
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			ContainersToSearch.Add(ContainerSettings[ContainerIndex]);
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid container index - AC_Inventory -> GetItemsByTag"), true, true);
			return FoundItems;
		}
	}

	for(auto& CurrentContainer : ContainersToSearch)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			FGameplayTagContainer ItemsTags = UFL_InventoryFramework::GetItemsTags(CurrentItem);
			if(ItemsTags.HasTagExact(Tag))
			{
				FoundItems.Add(CurrentItem);
			}
		}
	}

	return FoundItems;
}

TArray<FS_InventoryItem> UAC_Inventory::GetItemsByTagValue(FGameplayTag Tag, int32 ContainerIndex)
{
	TArray<FS_ContainerSettings> ContainersToSearch;
	TArray<FS_InventoryItem> FoundItems;
	if(ContainerIndex == -1)
	{
		ContainersToSearch = ContainerSettings;
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			ContainersToSearch.Add(ContainerSettings[ContainerIndex]);
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid container index - AC_Inventory -> GetItemsByTag"), true, true);
			return FoundItems;
		}
	}

	for(auto& CurrentContainer : ContainersToSearch)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			FS_TagValue FoundTagValue;
			int32 TagIndex;
			if(UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetItemsTagValues(CurrentItem), Tag, FoundTagValue, TagIndex))
			{
				FoundItems.Add(CurrentItem);
			}
		}
	}

	return FoundItems;
}

TArray<FS_InventoryItem> UAC_Inventory::GetItemsByType(FGameplayTag Tag, int32 ContainerIndex)
{
	TArray<FS_ContainerSettings> ContainersToSearch;
	TArray<FS_InventoryItem> FoundItems;
	if(ContainerIndex == -1)
	{
		ContainersToSearch = ContainerSettings;
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			ContainersToSearch.Add(ContainerSettings[ContainerIndex]);
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid container index - AC_Inventory -> GetItemsByTag"), true, true);
			return FoundItems;
		}
	}

	for(auto& CurrentContainer : ContainersToSearch)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.ItemAsset->ItemType == Tag)
			{
				FoundItems.Add(CurrentItem);
			}
		}
	}

	return FoundItems;
}

TArray<FS_InventoryItem> UAC_Inventory::GetItemsByTagQuery(FGameplayTagQuery TagQuery, int32 ContainerIndex)
{
	TArray<FS_ContainerSettings> ContainersToSearch;
	TArray<FS_InventoryItem> FoundItems;

	if(TagQuery.IsEmpty())
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid TagQuery - AC_Inventory -> GetItemsByTagTagQuery"));
		return FoundItems;
	}
	
	if(ContainerIndex == -1)
	{
		ContainersToSearch = ContainerSettings;
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			ContainersToSearch.Add(ContainerSettings[ContainerIndex]);
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid container index - AC_Inventory -> GetItemsByTag"));
			return FoundItems;
		}
	}

	for(auto& CurrentContainer : ContainersToSearch)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			FGameplayTagContainer ItemsTags = UFL_InventoryFramework::GetItemsTags(CurrentItem);
			if(TagQuery.Matches(ItemsTags))
			{
				FoundItems.Add(CurrentItem);
			}
		}
	}

	return FoundItems;
}

AActor* UAC_Inventory::GetItemComponentOwner_Implementation()
{
	return GetOwner();
}

void UAC_Inventory::SortAndMoveItems(TEnumAsByte<ESortingType> SortType, FS_ContainerSettings Container, float StaggerTimer)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		return;
	}

	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
		return;
	}

	Container.UniqueID.ParentComponent->C_AddAllContainerItemsToNetworkQueue(Container.UniqueID);
	S_SortAndMoveItems(SortType, Container.UniqueID, StaggerTimer, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_SortAndMoveItems_Implementation(ESortingType SortType, FS_UniqueID ContainerID, float StaggerTimer, ENetRole CallerLocalRole)
{
	UAC_Inventory* ParentComponent = ContainerID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return;
	}
	
	FS_ContainerSettings Container = ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid() || Container.Items.IsEmpty())
	{
		return;
	}

	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
			}
			else
			{
				C_SortAndMoveItems(SortType, ContainerID, StaggerTimer, Seed);
				Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
			}
		}
		else
		{
			C_SortAndMoveItems(SortType, ContainerID, StaggerTimer, Seed);
			Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
		}
	}
	else
	{
		C_SortAndMoveItems(SortType, ContainerID, StaggerTimer, Seed);
		Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_SortAndMoveItems(SortType, ContainerID, StaggerTimer, Seed);
		}
	}
}

void UAC_Inventory::C_SortAndMoveItems_Implementation(ESortingType SortType, FS_UniqueID ContainerID, float StaggerTimer, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	UAC_Inventory* ParentComponent = ContainerID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return;
	}
	
	FS_ContainerSettings Container = ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid() || Container.Items.IsEmpty())
	{
		return;
	}
	
	Internal_SortAndMoveItems(SortType, Container, StaggerTimer, Seed);
	Container.UniqueID.ParentComponent->C_RemoveAllContainerItemsFromNetworkQueue(ContainerID);
}

void UAC_Inventory::Internal_SortAndMoveItems(TEnumAsByte<ESortingType> SortType, const FS_ContainerSettings& Container, float StaggerTimer, FRandomStream Seed)
{
	UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return;
	}
	
	FS_ContainerSettings FoundContainer = ParentComponent->GetContainerByUniqueID(Container.UniqueID);
	if(!FoundContainer.IsValid() || FoundContainer.Items.IsEmpty())
	{
		return;
	}
	
	FS_ContainerSettings& ContainerRef = ParentComponent->ContainerSettings[Container.ContainerIndex];

	//Init tile map will clear all items from the tile map
	ParentComponent->InitializeTileMap(ContainerRef);
	TArray<FS_InventoryItem> SortedItems;
	
	//This enum will be extended in the future.
	//If you add any entries to ESortingType, this is where
	//you call your custom sorting function.
	switch (SortType)
	{
	case Name:
		{
			SortedItems = UFL_InventoryFramework::SortItemStructsAlphabetically(ContainerRef.Items);
			break;
		}
	case Type:
		{
			SortedItems = UFL_InventoryFramework::SortItemStructsByType(ContainerRef.Items);
		}
	default:
		break;
	}

	if(!ContainerRef.SupportsTileMap())
	{
		/**The container might be data-only, thus MoveItem
		 * doesn't need to be called. The array has been sorted
		 * already.*/
		ContainerRef.Items = SortedItems;
		ParentComponent->RefreshItemsIndexes(ContainerRef);
		SortingFinished.Broadcast();
		return;
	}
	
	ContainerRef.Items.Empty();
	if(StaggerTimer > 0)
	{
		//Store the original value so we can stagger it correctly
		const float OriginalStaggerTime = StaggerTimer;
		for(int32 CurrentItem = 0; CurrentItem < SortedItems.Num(); CurrentItem++)
		{
			FTimerHandle MoveItemTimer;
			FTimerDelegate TimerDelegate;
			//Helps evaluate when all items have been sorted
			bool IsLastItem = CurrentItem == SortedItems.Num() - 1;
			TimerDelegate.BindUFunction(this, "T_SortAndMoveItems", ParentComponent, ContainerRef, SortedItems[CurrentItem], Seed, IsLastItem);
			GetWorld()->GetTimerManager().SetTimer(MoveItemTimer, TimerDelegate, StaggerTimer, false);
			//Set timer for next item in the loop
			StaggerTimer += OriginalStaggerTime;
		}
	}
	else
	{
		for(auto& CurrentItem : SortedItems)
		{
			ContainerRef.Items.Add(CurrentItem);
			CurrentItem.ItemIndex = ContainerRef.Items.Num() - 1;
			bool SpotFound;
			int32 AvailableTile;
			TEnumAsByte<ERotation> NeededRotation;
			ParentComponent->GetFirstAvailableTile(CurrentItem, ContainerRef, GetGenericIndexesToIgnore(ContainerRef), SpotFound, AvailableTile, NeededRotation);
			if(SpotFound)
			{
				TArray<FS_ContainerSettings> ItemsContainers = ParentComponent->GetItemsChildrenContainers(CurrentItem);
				ParentComponent->Internal_MoveItem(CurrentItem, ParentComponent, ParentComponent	, ContainerRef.ContainerIndex, AvailableTile, CurrentItem.Count, true, false, true, NeededRotation, ItemsContainers, Seed);
			}
			else
			{
				//Spot was not found in it's current container, find another container
				FS_ContainerSettings CompatibleContainer;
				ParentComponent->GetFirstAvailableContainerAndTile(CurrentItem, TArray<int32>(), SpotFound, CompatibleContainer, AvailableTile, NeededRotation);
				if(SpotFound)
				{
					TArray<FS_ContainerSettings> ItemsContainers = ParentComponent->GetItemsChildrenContainers(CurrentItem);
					ParentComponent->Internal_MoveItem(CurrentItem, ParentComponent, ParentComponent	, ContainerRef.ContainerIndex, AvailableTile, CurrentItem.Count, true, false, true, NeededRotation, ItemsContainers, Seed);
				}
				else
				{
					//There's no spots left in the entire inventory
					ParentComponent->DropItem(CurrentItem);
				}
			}
		}
		SortingFinished.Broadcast();
	}
}

void UAC_Inventory::T_SortAndMoveItems(UAC_Inventory* ParentComponent, const FS_ContainerSettings& Container, FS_InventoryItem Item, FRandomStream Seed, bool bLastItem)
{
	ParentComponent->ContainerSettings[Container.ContainerIndex].Items.Add(Item);
	Item.ItemIndex = ParentComponent->ContainerSettings[Container.ContainerIndex].Items.Num() - 1;
	bool SpotFound;
	int32 AvailableTile;
	TEnumAsByte<ERotation> NeededRotation;
	ParentComponent->GetFirstAvailableTile(Item, ParentComponent->ContainerSettings[Container.ContainerIndex], GetGenericIndexesToIgnore(Container), SpotFound, AvailableTile, NeededRotation);
	if(SpotFound)
	{
		TArray<FS_ContainerSettings> ItemsContainers = ParentComponent->GetItemsChildrenContainers(Item);
		ParentComponent->Internal_MoveItem(Item, ParentComponent, ParentComponent	, Container.ContainerIndex, AvailableTile, Item.Count, true, false, true, NeededRotation, ItemsContainers, Seed);
	}
	else
	{
		//Spot was not found in it's current container, find another container
		FS_ContainerSettings CompatibleContainer;
		ParentComponent->GetFirstAvailableContainerAndTile(Item, TArray<int32>(), SpotFound, CompatibleContainer, AvailableTile, NeededRotation);
		if(SpotFound)
		{
			TArray<FS_ContainerSettings> ItemsContainers = ParentComponent->GetItemsChildrenContainers(Item);
			ParentComponent->Internal_MoveItem(Item, ParentComponent, ParentComponent	, Container.ContainerIndex, AvailableTile, Item.Count, true, false, true, NeededRotation, ItemsContainers, Seed);
		}
		else
		{
			//There's no spots left in the entire inventory
			ParentComponent->DropItem(Item);
		}
	}
	
	if(!UKismetSystemLibrary::IsServer(this))
	{
		Container.UniqueID.ParentComponent->C_RemoveItemFromNetworkQueue(Item.UniqueID);
	}

	if(bLastItem)
	{
		SortingFinished.Broadcast();
	}
}

void UAC_Inventory::S_NotifyItemSold_Implementation(FS_InventoryItem Item, UIDA_Currency* Currency, int32 Amount, UAC_Inventory* Buyer,
                                                    UAC_Inventory* Seller)
{
	Seller->SoldItem.Broadcast(Item, Buyer, Currency, Amount);
	Buyer->BoughtItem.Broadcast(Item, Seller, Currency, Amount);
}

bool UAC_Inventory::CanAffordItem_Implementation(FS_InventoryItem ItemToPurchase, UIDA_Currency*& Currency)
{
	Currency = nullptr;
	
	if(ItemToPurchase.UniqueID.ParentComponent == this)
	{
		return true;
	}
	
	TArray<UIDA_Currency*> AcceptedCurrencies = UFL_InventoryFramework::GetAcceptedCurrencies(ItemToPurchase);
	if(!AcceptedCurrencies.IsValidIndex(0))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item has no accepted currencies set up."));
		return false;
	}

	for(const auto& CurrentCurrency : AcceptedCurrencies)
	{
		if(UKismetSystemLibrary::DoesImplementInterface(GetOwner(), UI_Inventory::StaticClass()))
		{
			if(II_Inventory::Execute_MeetsCurrencyCheck(GetOwner(), this, Currency, UFL_InventoryFramework::GetItemPrice(ItemToPurchase)))
			{
				Currency = CurrentCurrency;
				return true;
			}
		}
		
		int32 TotalAmountFound;
		GetListOfItemsByCount(CurrentCurrency, UFL_InventoryFramework::GetItemPrice(ItemToPurchase), -1, TotalAmountFound);
		if(TotalAmountFound >= UFL_InventoryFramework::GetItemPrice(ItemToPurchase))
		{
			Currency = CurrentCurrency;
			return true;
		}
	}

	//No valid items were found.
	return false;
}

bool UAC_Inventory::CanItemBeRotated_Implementation(FS_InventoryItem Item)
{
	return true;
}

UItemComponent* UAC_Inventory::GetItemComponent(FS_InventoryItem Item, UIT_ItemComponentTrait* Trait, bool CreateComponent, AActor* Instigator, FGameplayTag Event, FItemComponentPayload Payload)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("GetItemComponent")
	
	if(!IsValid(Trait))
	{
		return nullptr;
	}

	if(Trait->ItemComponent.IsNull() || !IsValid(Item.ItemAsset) || !GetItemComponentOwner())
	{
		return nullptr;
	}

	if(!UKismetSystemLibrary::IsServer(this) && !UKismetSystemLibrary::IsStandalone(this))
	{
		C_AddItemToNetworkQueue(Item.UniqueID);
	}

	if(Trait->ConstructionPolicy == PerUse)
	{
		if(!CreateComponent)
		{
			UItemComponent* LatestItemComponent = nullptr;
			for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
			{
				if(IsValid(CurrentComponent))
				{
					if(Cast<UItemComponent>(CurrentComponent))
					{
						if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
						{
							LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
						}
					}
				}
			}
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
			return LatestItemComponent;
		}
		//CreateComponent is true and the component is PerUse. Don't bother to find one, immediately create a new component.
		if(Trait->NetworkingMethod == Server || Trait->NetworkingMethod == Both)
		{
			if(UKismetSystemLibrary::IsServer(this) || Trait->NetworkingMethod == Both)
			{
				S_ConstructServerItemComponent(Item.ItemAsset, Item.UniqueID, Trait, Instigator, Trait->ItemComponent.LoadSynchronous(), Event, Payload);
				if(UKismetSystemLibrary::IsServer(this))
				{
					//Server has created the item component, try and find it and return it.
					UItemComponent* LatestItemComponent = nullptr;
					for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
					{
						if(IsValid(CurrentComponent))
						{
							if(Cast<UItemComponent>(CurrentComponent))
							{
								if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
								{
									LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
								}
							}
						}
					}
					if(!LatestItemComponent)
					{
						UFL_InventoryFramework::LogIFPMessage(this, TEXT("Could not find component - AC_Inventory -> GetItemComponent"), true, true);
					}
					return LatestItemComponent;
				}
				return nullptr;
			}
		}
		else if(Trait->NetworkingMethod == Client)
		{
			if(!UKismetSystemLibrary::IsDedicatedServer(this))
			{
				C_ConstructClientItemComponent(Item.ItemAsset, Item.UniqueID, Trait, Instigator, Trait->ItemComponent.LoadSynchronous(), Event, Payload);
				UItemComponent* LatestItemComponent = nullptr;
				//Item Struct might not be valid, so search all components
				for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
				{
					if(IsValid(CurrentComponent))
					{
						if(Cast<UItemComponent>(CurrentComponent))
						{
							if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
							{
								LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
							}
						}
					}
				}
				return LatestItemComponent;
			}
		}
	}
	
	if(Trait->ConstructionPolicy == PerActor)
	{
		//Find the item component.
		UItemComponent* LatestItemComponent = nullptr;
		for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
		{
			if(IsValid(CurrentComponent))
			{
				if(Cast<UItemComponent>(CurrentComponent))
				{
					if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
					{
						LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
						C_RemoveItemFromNetworkQueue(Item.UniqueID);
						return LatestItemComponent;
					}
				}
			}
		}
		
		//No item Component was found, and we have CreateDrive set to true. Resolve whether we are creating a server, client or both.
		if(CreateComponent && LatestItemComponent == nullptr)
		{
			if(Trait->NetworkingMethod == Server || Trait->NetworkingMethod == Both)
			{
				if(UKismetSystemLibrary::IsServer(this) || Trait->NetworkingMethod == Both)
				{
					S_ConstructServerItemComponent(Item.ItemAsset, Item.UniqueID, Trait, Instigator, Trait->ItemComponent.LoadSynchronous(), Event, Payload);
					//Item Component should now exist, so search for it again.
					//Item Struct might not be valid, so search all components
					for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
					{
						if(IsValid(CurrentComponent))
						{
							if(Cast<UItemComponent>(CurrentComponent))
							{
								if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
								{
									return LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
								}
							}
						}
					}
				}
			}
			else if(Trait->NetworkingMethod == Client)
			{
				C_ConstructClientItemComponent(Item.ItemAsset, Item.UniqueID, Trait, Instigator, Trait->ItemComponent.LoadSynchronous(), Event, Payload);
				//In the case a server calls this through an RPC, the server won't be able to return
				//a copy, because it only exists on the client. Hence why we skip searching for
				//the recently added component if this is the server.
				if(!UKismetSystemLibrary::IsServer(this) || UKismetSystemLibrary::IsStandalone(this))
				{
					for(auto& CurrentComponent : GetItemComponentOwner()->GetComponents())
					{
						if(IsValid(CurrentComponent))
						{
							if(Cast<UItemComponent>(CurrentComponent))
							{
								if(Cast<UItemComponent>(CurrentComponent)->UniqueID == Item.UniqueID && Cast<UItemComponent>(CurrentComponent)->DataObject == Trait)
								{
									LatestItemComponent = Cast<UItemComponent>(CurrentComponent);
								}
							}
						}
					}
					return LatestItemComponent;
				}
				UFL_InventoryFramework::LogIFPMessage(this, TEXT("Server created item Component for client, can't return a copy  - AC_Inventory -> GetItemComponent"), true, true);
				return nullptr;
			}
		}
		return LatestItemComponent;
	}

	if(Trait->ConstructionPolicy == Custom)
	{
		if(Trait->AllowNewInstance())
		{
			C_RemoveItemFromNetworkQueue(Item.UniqueID);
			return NewObject<UItemComponent>(GetItemComponentOwner(), Trait->GetClass());
		}
	}

	return nullptr;
}

UItemInstance* UAC_Inventory::CreateItemInstanceForItem(FS_InventoryItem& Item)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CreateItemInstance)
	
	//Only server should be creating objects
	if(!GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	
	if(!Initialized || !Item.ItemAsset)
	{
		return nullptr;
	}

	/**Don't create objects while in the editor.*/
	if(!UGameplayStatics::GetGameInstance(this))
	{
		return nullptr;
	}

	if(!Item.ItemAsset->ItemInstance.IsNull() || IsValid(Item.ItemInstance))
	{
		UItemInstance* Template = Item.ItemInstance ? Item.ItemInstance : Cast<UItemInstance>(Item.ItemAsset->ItemInstance.LoadSynchronous()->GetDefaultObject());
		if(Template->ItemID.IsValid())
		{
			return Template;
		}
		//Create a new object and use the instanced copy as a template, so all the edited variables are inherited by the new object.
		UItemInstance* ItemInstance = NewObject<UItemInstance>(GetOwner(), Template->GetClass(),
			NAME_None, RF_NoFlags, Template);
		ItemInstance->ItemID = Item.UniqueID;
		ItemInstance->ItemAsset = Item.ItemAsset;
		ItemInstance->SetOwner(GetOwner());
		
		Item.ItemInstance = ItemInstance; //Update the ref
		
		//Update the item struct in the container settings
		FS_InventoryItem ItemStruct = GetItemByUniqueID(Item.UniqueID);
		if(ItemStruct.IsValid())
		{
			ContainerSettings[ItemStruct.ContainerIndex].Items[ItemStruct.ItemIndex].ItemInstance = ItemInstance;
		}

		ItemInstance->StartPlay();

		//Label object for replication
		AddReplicatedSubObject(ItemInstance, ItemInstance->ReplicationCondition);

		return ItemInstance;
	}

	return nullptr;
}

int32 UAC_Inventory::GetItemCount(UDA_CoreItem* ItemAsset, TArray<FS_ContainerSettings> OptionalFilter)
{
	if(!OptionalFilter.IsValidIndex(0))
	{
		OptionalFilter = ContainerSettings;
	}

	int32 TotalCount = 0;

	for(auto& CurrentContainer : OptionalFilter)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.ItemAsset != ItemAsset)
			{
				continue;
			}

			TotalCount += CurrentItem.Count;
		}
	}

	return TotalCount;
}

void UAC_Inventory::UpdateItemsEquipStatus(FS_InventoryItem Item, bool IsEquipped, TArray<FName> CustomTriggerFilters)
{
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
		return;
	}

	C_AddItemToNetworkQueue(Item.UniqueID);
	S_UpdateItemsEquipStatus(Item.UniqueID, IsEquipped, CustomTriggerFilters, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_UpdateItemsEquipStatus_Implementation(FS_UniqueID ItemID, bool IsEquipped,
                                                            const TArray<FName>& CustomTriggerFilters, ENetRole CallerLocalRole)
{
	if(!ItemID.IsValid())
	{
		return;
	}

	FS_InventoryItem Item = GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
			}
			else
			{
				Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
				C_UpdateItemsEquipStatus(ItemID, IsEquipped, CustomTriggerFilters);
			}
		}
		else
		{
			Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
			C_UpdateItemsEquipStatus(ItemID, IsEquipped, CustomTriggerFilters);
		}
	}
	else
	{
		Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
		C_UpdateItemsEquipStatus(ItemID, IsEquipped, CustomTriggerFilters);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_UpdateItemsEquipStatus(ItemID, IsEquipped, CustomTriggerFilters);
		}
	}
}

void UAC_Inventory::C_UpdateItemsEquipStatus_Implementation(FS_UniqueID ItemID, bool IsEquipped,
	const TArray<FName>& CustomTriggerFilters)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(!IsValid(ItemID.ParentComponent))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemID.ParentComponent->GetItemByUniqueID(ItemID);
	if(Item.IsValid())
	{
		Internal_UpdateItemsEquipStatus(Item, IsEquipped, CustomTriggerFilters);
		C_RemoveItemFromNetworkQueue(Item.UniqueID);
	}
}

void UAC_Inventory::Internal_UpdateItemsEquipStatus(FS_InventoryItem Item, bool IsEquipped,
	TArray<FName> CustomTriggerFilters)
{
	UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(Item, IsEquipped, CustomTriggerFilters, IsEquipped ? FS_InventoryItem() : Item);
}

void UAC_Inventory::MassSplitStack(FS_InventoryItem Item, int32 StackSize, int32 SplitAmount, FS_ContainerSettings DestinationContainer)
{
	if(!Item.IsValid() || !DestinationContainer.IsValid() || StackSize <= 0 || SplitAmount < 1 || !Item.ItemAsset->CanItemStack())
	{
		return;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		Internal_MassSplitStack(Item, StackSize, SplitAmount, DestinationContainer, Seed, 0);
		return;
	}

	C_AddItemToNetworkQueue(Item.UniqueID);
	S_MassSplitStack(Item.UniqueID, SplitAmount, StackSize, DestinationContainer.UniqueID, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_MassSplitStack_Implementation(FS_UniqueID ItemID, int32 StackSize, int32 SplitAmount,
                                                    FS_UniqueID ContainerID, ENetRole CallerLocalRole)
{
	if(!ItemID.IsValid() || !ContainerID.IsValid())
	{
		return;
	}

	FS_InventoryItem Item = GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}

	FS_ContainerSettings Container = GetContainerByUniqueID(ContainerID);
	if(!Container.IsValid())
	{
		return;
	}

	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	int32 AmountReduced = Internal_MassSplitStack(Item, StackSize, SplitAmount, Container, Seed, 0);

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(!GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				C_MassSplitStack(ItemID, StackSize, SplitAmount, ContainerID, Seed, AmountReduced);
			}
		}
		else
		{
			C_MassSplitStack(ItemID, StackSize, SplitAmount, ContainerID, Seed, AmountReduced);
		}
	}
	else
	{
		C_MassSplitStack(ItemID, StackSize, SplitAmount, ContainerID, Seed, AmountReduced);
	}
	
	for(const auto& CurrentListener : ItemID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_MassSplitStack(ItemID, StackSize, SplitAmount, ContainerID, Seed, AmountReduced);;
		}
	}
}

void UAC_Inventory::C_MassSplitStack_Implementation(FS_UniqueID ItemID, int32 StackSize, int32 SplitAmount, FS_UniqueID ContainerID, FRandomStream Seed, int32
                                                    ItemCountReduction)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(!ItemID.IsValid() || !ContainerID.IsValid())
	{
		return;
	}

	FS_InventoryItem Item = GetItemByUniqueID(ItemID);
	if(!Item.IsValid())
	{
		return;
	}

	FS_ContainerSettings Container = GetContainerByUniqueID(ContainerID);
	
	Internal_MassSplitStack(Item, SplitAmount, SplitAmount, Container, Seed, ItemCountReduction);
	C_RemoveItemFromNetworkQueue(Item.UniqueID);
}

int32 UAC_Inventory::Internal_MassSplitStack(FS_InventoryItem Item, int32 StackSize,
                                            int32 SplitAmount, FS_ContainerSettings DestinationContainer, FRandomStream Seed,
                                            int32 ItemCountReduction)
{
	int32 AmountReduced = 0;
	StackSize = FMath::Clamp(StackSize, 0, Item.Count);
	FS_ContainerSettings ContainerRef = DestinationContainer.ParentComponent()->GetContainerByUniqueID(DestinationContainer.UniqueID);
	if(!ContainerRef.IsValid())
	{
		/* If the destination container can't be found, we are most likely a client
		 * that doesn't have the information to fully complete this function.
		 * In that case, just reduce the item count by the total that the server
		 * was able to reduce it by. */
		
		if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
		{
			Internal_ReduceItemCount(Item, ItemCountReduction, true, Seed);
		}
		else
		{
			//If this is a client, the item we are splitting might be rooted in a component
			//the client does not have data from. In that case, we can't reduce the count,
			//but we also don't need to. Server will reduce it.
			FS_InventoryItem FoundItem = GetItemByUniqueID(Item.UniqueID);
			if(FoundItem.IsValid())
			{
				Item = FoundItem;
				Internal_ReduceItemCount(Item, ItemCountReduction, true, Seed);
			}
		}

		return ItemCountReduction;
	}
	
	//Low-cost way of tracking the item count without constantly fetching the latest item copy
	int32 Count = Item.Count;
	for(int32 CurrentSplit = 0; CurrentSplit < SplitAmount; CurrentSplit++)
	{
		if(Count == 0)
		{
			return AmountReduced;
		}
		
		bool SpotFound = true;
		int32 TileFound;
		TEnumAsByte<ERotation> NeededRotation = Zero;
		GetFirstAvailableTile(Item, DestinationContainer.ParentComponent()->ContainerSettings[DestinationContainer.ContainerIndex], GetGenericIndexesToIgnore(DestinationContainer), SpotFound, TileFound, NeededRotation);
		if(!SpotFound)
		{
			//No more spots were found, not possible to continue
			return AmountReduced;
		}
		
		//Reduce the original items count
		if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
		{
			Internal_ReduceItemCount(Item, StackSize, true, Seed);
		}
		else
		{
			//If this is a client, the item we are splitting might be rooted in a component
			//the client does not have data from. In that case, we can't reduce the count,
			//but we also don't need to. Server will reduce it.
			FS_InventoryItem FoundItem = GetItemByUniqueID(Item.UniqueID);
			if(FoundItem.IsValid())
			{
				Item = FoundItem;
				Internal_ReduceItemCount(Item, StackSize, true, Seed);
			}
		}

		//Add the new split item
		FS_InventoryItem NewStackItem;
		NewStackItem.ItemAsset = Item.ItemAsset;
		NewStackItem.Rotation = NeededRotation;
		NewStackItem.ContainerIndex = ContainerRef.ContainerIndex;
		NewStackItem.TileIndex = TileFound;
		NewStackItem.Count = StackSize;
		NewStackItem.ItemIndex = ContainerRef.ParentComponent()->ContainerSettings[ContainerRef.ContainerIndex].Items.Num();
		NewStackItem.UniqueID = GenerateUniqueIDWithSeed(Seed);
		Seed.Initialize(Seed.GetInitialSeed() + 1);
		UFL_InventoryFramework::AddDefaultTagsToItem(NewStackItem, false);
		UFL_InventoryFramework::AddDefaultTagValuesToItem(NewStackItem, false, false);

		ContainerRef.ParentComponent()->AddItemToTileMap(NewStackItem);
		ContainerRef.ParentComponent()->ContainerSettings[ContainerRef.ContainerIndex].Items.Add(NewStackItem);
		
		if(UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(ContainerRef.ParentComponent()->ContainerSettings[ContainerRef.ContainerIndex]))
		{
			UW_InventoryItem* ItemWidget = nullptr;
			ContainerWidget->CreateWidgetForItem(NewStackItem, ItemWidget);
		}

		Count = FMath::Clamp(Count - StackSize, 0, Item.Count);
		if(Count < StackSize)
		{
			StackSize = Count;
		}
		AmountReduced = FMath::Clamp(AmountReduced + StackSize, 0, Item.Count);
	}

	/* Refresh the indexes, but also don't double call refresh on the same container if the
	 * destination container is the same as the original items container. */
	if(Count == 0 && DestinationContainer != Item.ParentComponent()->ContainerSettings[Item.ContainerIndex])
	{
		Item.ParentComponent()->RefreshItemsIndexes(Item.ParentComponent()->ContainerSettings[Item.ContainerIndex]);
	}
	DestinationContainer.ParentComponent()->RefreshItemsIndexes(DestinationContainer);
	return AmountReduced;
}

TArray<TSoftClassPtr<UItemInstance>> UAC_Inventory::GetUnloadedItemInstances()
{
	TArray<TSoftClassPtr<UItemInstance>> InstanceClasses;

	for(auto& CurrentContainer : ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			
			if(CurrentItem.ItemInstance || CurrentItem.ItemAsset->ItemInstance.IsNull())
			{
				/**If the ItemInstance is valid, that means this component is hard-referencing
				 * it, so no need to load it.
				 * If the item asset doesn't have an ItemInstance reference, then there's nothing to load.*/
				continue;
			}
			
			if(CurrentItem.ItemAsset->ItemInstance.Get() || CurrentItem.ItemAsset->ItemInstance.IsPending())
			{
				/**ItemInstance is valid, but the class is already loaded or is being loaded*/
				continue;
			}

			InstanceClasses.AddUnique(CurrentItem.ItemAsset->ItemInstance);
		}
	}
	
	return InstanceClasses;
}


void UAC_Inventory::S_ConstructServerItemComponent_Implementation(UDA_CoreItem* CoreItem, FS_UniqueID UniqueID, UIT_ItemComponentTrait* Trait, AActor* Instigator, TSubclassOf<UItemComponent> ItemComponent, FGameplayTag Event, FItemComponentPayload Payload)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("Construct item Component")
	
	if(Trait->NetworkingMethod == Client)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Server just tried to construct an object that was set to client. Aborting item Component creation."), true, true);
		return;
	}
	
	UAC_Inventory* ParentComponent = UniqueID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return;
	}

	FS_InventoryItem ParentItem = ParentComponent->GetItemByUniqueID(UniqueID);
	if(!ParentItem.IsValid())
	{
		//UniqueID not found in the parent component.
		return;
	}
	
	UItemComponent* NewItemComponent = NewObject<UItemComponent>(GetItemComponentOwner(), ItemComponent);
	NewItemComponent->UniqueID = UniqueID;
	NewItemComponent->DataObject = Trait;
	NewItemComponent->Instigator = Instigator;
	NewItemComponent->CreationTime = GetOwner()->GetGameTimeSinceCreation();
	NewItemComponent->RegisterComponent();
	ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items[ParentItem.ItemIndex].ItemComponents.AddUnique(NewItemComponent);
	
	if(Trait->NetworkingMethod == Both)
	{
		NewItemComponent->SetIsReplicated(true);
	}
	else //Item Components should always by default not replicate. This is just a safety mechanism.
	{
		NewItemComponent->SetIsReplicated(false);
	}

	C_RemoveItemFromNetworkQueue(UniqueID);

	ItemComponentStarted.Broadcast(UniqueID, NewItemComponent, Instigator, Event);
	if(Event.IsValid())
	{
		NewItemComponent->ActivateEvent(Event, Payload);
	}
}

bool UAC_Inventory::S_ConstructServerItemComponent_Validate(UDA_CoreItem* CoreItem, FS_UniqueID UniqueID, UIT_ItemComponentTrait* Trait, AActor* Instigator, TSubclassOf<UItemComponent> ItemComponent, FGameplayTag Event, FItemComponentPayload Payload)
{
	return CoreItem->TraitsAndComponents.Contains(Trait) && UniqueID.ParentComponent == this;
}

void UAC_Inventory::C_ConstructClientItemComponent_Implementation(UDA_CoreItem* CoreItem, FS_UniqueID UniqueID,
	UIT_ItemComponentTrait* Trait, AActor* Instigator, TSubclassOf<UItemComponent> ItemComponent, FGameplayTag Event, FItemComponentPayload Payload)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	TRACE_CPUPROFILER_EVENT_SCOPE("Construct item Component")
	
	if(Trait->NetworkingMethod != Client)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Client just tried to construct an object that was not set to Client. Aborting item Component creation."), true, true);
		return;
	}

	UAC_Inventory* ParentComponent = UniqueID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return;
	}

	FS_InventoryItem ParentItem = ParentComponent->GetItemByUniqueID(UniqueID);
	if(!ParentItem.IsValid())
	{
		//UniqueID not found in the parent component.
		return;
	}
	
	UItemComponent* NewItemComponent = NewObject<UItemComponent>(GetItemComponentOwner(), ItemComponent);
	NewItemComponent->UniqueID = UniqueID;
	NewItemComponent->DataObject = Trait;
	NewItemComponent->Instigator = Instigator;
	NewItemComponent->CreationTime = GetOwner()->GetGameTimeSinceCreation();
	NewItemComponent->RegisterComponent();
	ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items[ParentItem.ItemIndex].ItemComponents.AddUnique(NewItemComponent);

	C_RemoveItemFromNetworkQueue(UniqueID);

	ItemComponentStarted.Broadcast(UniqueID, NewItemComponent, Instigator, Event);
	if(Event.IsValid())
	{
		NewItemComponent->ActivateEvent(Event, Payload);
	}
}

void UAC_Inventory::UpdateItemComponentsUniqueID(FS_UniqueID OldUniqueID, FS_UniqueID NewUniqueID)
{
	FS_InventoryItem ItemData = GetItemByUniqueID(OldUniqueID);
	if(ItemData.IsValid())
	{
		for(auto& CurrentObject : ItemData.ItemAsset->TraitsAndComponents)
		{
			if(UIT_ItemComponentTrait* ComponentTrait = Cast<UIT_ItemComponentTrait>(CurrentObject))
			{
				if(UItemComponent* ItemComponent = GetItemComponent(ItemData, ComponentTrait, false, GetOwner()))
				{
					ItemComponent->UniqueID = NewUniqueID;
				}
			}
		}
	}
}

void UAC_Inventory::CheckForSpace(FS_InventoryItem Item, FS_ContainerSettings Container, int32 TopLeftIndex, TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable, int32& AvailableTile, TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CheckForSpace)
	ItemsInTheWay.Empty();
	if(!IsValid(Container.UniqueID.ParentComponent))
	{
		SpotAvailable = false;
		AvailableTile = -1;
		return;		
	}

	if(Container.Style == DataOnly)
	{
		SpotAvailable = true;
		AvailableTile = 1;
		return;
	}

	constexpr int32 ItemX = 0;
	constexpr int32 ItemY = 0;
	FIntPoint ItemDimensions = FIntPoint(ItemX, ItemY);
	if(Container.Style ==  Traditional || Container.ContainerType == Equipment)
	{
		ItemDimensions.X = 1;
		ItemDimensions.Y = 1;
	}
	else
	{
		UFL_InventoryFramework::GetItemDimensionsWithContext(Item, Container, ItemDimensions.X, ItemDimensions.Y);

		//Item is larger than the container, immediately fail.
		if(ItemDimensions.X > Container.Dimensions.X || ItemDimensions.Y > Container.Dimensions.Y)
		{
			SpotAvailable = false;
			AvailableTile = -1;
			return;
		}
	}

	bool InvalidTileFound;
	
	//Do a little bit of trickery to get GetItemsShape to do the correct calculations for us.
	Item.TileIndex += TopLeftIndex - Item.TileIndex;
	
	TArray<FIntPoint> ItemsShape = UFL_InventoryFramework::GetItemsShapeWithContext(Item, Container, InvalidTileFound);

	if(InvalidTileFound)
	{
		SpotAvailable = false;
		AvailableTile = Item.TileIndex;
		return;
	}

	if(ItemsShape.IsEmpty())
	{
		SpotAvailable = false;
		AvailableTile = -1;
		return;
	}
	
	for(auto& CurrentTile : ItemsShape)
	{
		int32 CurrentIndex = UFL_InventoryFramework::TileToIndex(CurrentTile.X, CurrentTile.Y, Container);
		
		if(TilesToIgnore.Contains(CurrentIndex))
		{
			SpotAvailable = false;
			AvailableTile = -1;
			return;
		}
		
		if(Container.TileMap.IsValidIndex(CurrentIndex))
		{
			//Check if CurrentTile is free on the TileMap and make sure CurrentTile is not either hidden or locked.
			if(Container.TileMap[CurrentIndex] != -1 && Container.TileMap[CurrentIndex] != Item.UniqueID.IdentityNumber)
			{
				if(Optimize)
				{
					SpotAvailable = false;
					AvailableTile = -1;
					return;
				}
				else
				{
					FS_InventoryItem BlockingItem = GetItemAtSpecificIndex(Container, CurrentIndex);
					if(BlockingItem.IsValid())
					{
						if(!ItemsToIgnore.Contains(BlockingItem))
						{
							SpotAvailable = false;
							AvailableTile = -1;
							ItemsInTheWay.AddUnique(BlockingItem);
						}
					}
				}
			}
		}
	}
	
	if(ItemsInTheWay.IsValidIndex(0))
	{
		SpotAvailable = false;
		AvailableTile = -1;
	}
	else
	{
		AvailableTile = TopLeftIndex;
		SpotAvailable = true;
	}
}

void UAC_Inventory::CheckAllRotationsForSpace(FS_InventoryItem Item, const FS_ContainerSettings Container,
	const int32 TopLeftIndex, TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable,
	TEnumAsByte<ERotation>& NeededRotation, int32& AvailableTile, TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CheckAllRotationsForSpace)
	ItemsInTheWay.Empty();
	AvailableTile = -1;
	NeededRotation = Item.Rotation;
	SpotAvailable = false;
	if(!CanItemBeRotated(Item) || !Container.IsSpacialContainer())
	{
		//Item or container doesn't support rotations, just do a simple check.
		CheckForSpace(Item, Container, TopLeftIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, AvailableTile, ItemsInTheWay, Optimize);
		if(SpotAvailable)
		{
			NeededRotation = Item.Rotation;
			return;
		}
	
		return;
	}
	
	//Setup start enum
	const ERotation StartingRotation = Item.Rotation;
	ERotation CurrentRotation = StartingRotation;
	constexpr int32 MaxEnumSize = static_cast<int32>(ERotation::TwoSeventy);

	/**The way this loop works is by starting off on the items default rotation,
	 * then it figures out what enum entry is after that, and it then does
	 * some math in case it hits the final entry to loop back around,
	 * then stopping once it hits the original rotation again.*/
	do
	{
		Item.Rotation = CurrentRotation;
		CheckForSpace(Item, Container, TopLeftIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, AvailableTile, ItemsInTheWay, Optimize);
		if(SpotAvailable)
		{
			NeededRotation = Item.Rotation;
			return;
		}

		//Figure out the next enum
		const int32 NextRotation = static_cast<int32>(CurrentRotation) + 1;
		CurrentRotation = static_cast<ERotation>((NextRotation % (MaxEnumSize + 1)));
	} while (CurrentRotation != StartingRotation);

	//Above return was hit, and all other outputs are
	//reset by CheckForSpace, except NeededRotation.
	//If no rotations were available, this would return
	//the rotation of the last item that succeeded.
	//Reset it here just in case.
	NeededRotation = Item.Rotation;
}

void UAC_Inventory::CheckForSpaceForShape(TArray<FIntPoint> Shape, FS_ContainerSettings Container, int32 TopLeftIndex,
	TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable, int32& AvailableTile,
	TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CheckForSpaceForShape)
	if(Shape.IsEmpty())
	{
		SpotAvailable = false;
		AvailableTile = -1;
		return;
	}
	
	ItemsInTheWay.Empty();
	if(!IsValid(Container.UniqueID.ParentComponent))
	{
		SpotAvailable = false;
		AvailableTile = -1;
		return;		
	}

	if(Container.Style == DataOnly)
	{
		SpotAvailable = true;
		AvailableTile = 1;
		return;
	}

	FIntPoint IndexTile;
	UFL_InventoryFramework::IndexToTile(TopLeftIndex, Container, IndexTile.X, IndexTile.Y);
	
	for(auto& CurrentTile : Shape)
	{
		//Shape indexes are in local space. Offset it to the correct tile.
		CurrentTile.X += IndexTile.X;
		CurrentTile.Y += IndexTile.Y;
		
		int32 CurrentIndex = UFL_InventoryFramework::TileToIndex(CurrentTile.X, CurrentTile.Y, Container);

		if(!UFL_InventoryFramework::IsTileValid(CurrentTile.X, CurrentTile.Y, Container))
		{
			SpotAvailable = false;
			AvailableTile = -1;
			return;
		}
			
		if(TilesToIgnore.Contains(CurrentIndex))
		{
			SpotAvailable = false;
			AvailableTile = -1;
			return;
		}
					
		if(Container.TileMap.IsValidIndex(CurrentIndex))
		{
			//Check if CurrentTile is free on the TileMap and make sure CurrentTile is not either hidden or locked.
			if(Container.TileMap[CurrentIndex] != -1)
			{
				if(Optimize)
				{
					SpotAvailable = false;
					AvailableTile = -1;
					return;
				}
				else
				{
					FS_InventoryItem BlockingItem;
					if(GetItemAtSpecificIndex(Container, CurrentIndex).IsValid())
					{
						if(!ItemsToIgnore.Contains(BlockingItem))
						{
							SpotAvailable = false;
							AvailableTile = -1;
							ItemsInTheWay.AddUnique(BlockingItem);
						}
					}
				}
			}
		}
	}
	
	if(ItemsInTheWay.IsValidIndex(0))
	{
		SpotAvailable = false;
		AvailableTile = -1;
	}
	else
	{
		AvailableTile = TopLeftIndex;
		SpotAvailable = true;
	}
}

bool UAC_Inventory::CanSplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent,
                                 int32 NewStackContainerIndex, int32 NewStackTileIndex)
{
	if(!Item.ItemAsset->CanItemStack()) { return false; }
	if(!DestinationComponent) { return false; }
	if(SplitAmount < 1) { return false; }
	if(!DestinationComponent->ContainerSettings.IsValidIndex(NewStackContainerIndex)) { return false; }
	if(!DestinationComponent->ContainerSettings[NewStackContainerIndex].TileMap.IsValidIndex(NewStackTileIndex)) { return false; }
	if(DestinationComponent->ContainerSettings[NewStackContainerIndex].TileMap[NewStackTileIndex] == Item.UniqueID.IdentityNumber) { return false; }
	if(!UFL_InventoryFramework::IsItemValid(Item)) { return false; }
	
	return true;
}

bool UAC_Inventory::CanSwapItemLocations(FS_InventoryItem Item1, FS_InventoryItem Item2, TEnumAsByte<ERotation>& Item1RequiredRotation,
	TEnumAsByte<ERotation>& Item2RequiredRotation)
{
	//First validate all data
	if(!UFL_InventoryFramework::IsItemValid(Item1) || !UFL_InventoryFramework::IsItemValid(Item2))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either Item1 or Item2 is invalid - AC_Inventory -> CanSwapItemLocations"), true, true);
		return false;
	}

	UAC_Inventory* Item1ParentComponent = Item1.UniqueID.ParentComponent;
	UAC_Inventory* Item2ParentComponent = Item2.UniqueID.ParentComponent;
	if(!Item1ParentComponent->ContainerSettings.IsValidIndex(Item1.ContainerIndex) || !Item2ParentComponent->ContainerSettings.IsValidIndex(Item2.ContainerIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Either Item1 container index or Item2 container index is invalid - AC_Inventory -> CanSwapItemLocations"), true, true);
		return false;
	}

	if(UFL_InventoryFramework::ItemEqualsItem(Item1, Item2))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item tried to swap locations with itself - AC_Inventory -> CanSwapItemLocations"), true, true);
		return false;
	}

	//Figure out if Item1 or Item2 is a child of the other.
	TArray<FS_ItemSubLevel> ChildrenItems;
	GetChildrenItems(Item1, ChildrenItems);
	for(auto& CurrentChild : ChildrenItems)
	{
		if(CurrentChild.Item == Item2)
		{
			return false;
		}
	}

	ChildrenItems.Empty();
	GetChildrenItems(Item2, ChildrenItems);
	for(auto& CurrentChild : ChildrenItems)
	{
		if(CurrentChild.Item == Item1)
		{
			return false;
		}
	}

	if(!Item2ParentComponent->CheckCompatibility(Item1, Item2ParentComponent->ContainerSettings[Item2.ContainerIndex]))
	{
		return false;
	}

	if(!Item1ParentComponent->CheckCompatibility(Item2, Item1ParentComponent->ContainerSettings[Item1.ContainerIndex]))
	{
		return false;
	}
	//End data validation
	
	//Start by checking if Item1 can swap with Item2
	FS_InventoryItem SimulatedItem1 = Item1;
	SimulatedItem1.ContainerIndex = Item2.ContainerIndex;
	SimulatedItem1.TileIndex = Item2.TileIndex;
	SimulatedItem1.UniqueID = FS_UniqueID(UKismetMathLibrary::RandomIntegerInRange(1, 2147483647), Item1.UniqueID.ParentComponent);
	
	FS_InventoryItem SimulatedItem2 = Item2;
	SimulatedItem2.ContainerIndex = Item1.ContainerIndex;
	SimulatedItem2.TileIndex = Item1.TileIndex;
	SimulatedItem2.UniqueID = FS_UniqueID(UKismetMathLibrary::RandomIntegerInRange(1, 2147483647), Item2.UniqueID.ParentComponent);
	
	bool SpotAvailable = false;
	int32 AvailableTile;
	TArray<FS_InventoryItem> CollidingItems;
	TArray<FS_InventoryItem> ItemsToIgnore;
	ItemsToIgnore.Add(Item2);
	TArray<int32> TilesToIgnore = GetGenericIndexesToIgnore(Item2ParentComponent->ContainerSettings[Item2.ContainerIndex]);
	Item2ParentComponent->CheckAllRotationsForSpace(SimulatedItem1, Item2ParentComponent->ContainerSettings[SimulatedItem1.ContainerIndex], SimulatedItem1.TileIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, Item1RequiredRotation, AvailableTile, CollidingItems);
	//If colliding items is greater than 1, we know another item that is NOT Item2 is in the way.
	if(CollidingItems.Num() != 0 || !SpotAvailable)
	{
		return false;
	}
	
	CollidingItems.Empty();
	ItemsToIgnore.Empty();
	ItemsToIgnore.Add(Item1);

	TilesToIgnore = GetGenericIndexesToIgnore(Item1ParentComponent->ContainerSettings[Item1.ContainerIndex]);
	Item1ParentComponent->CheckAllRotationsForSpace(SimulatedItem2, Item1ParentComponent->ContainerSettings[SimulatedItem2.ContainerIndex], SimulatedItem2.TileIndex, ItemsToIgnore, TilesToIgnore, SpotAvailable, Item2RequiredRotation, AvailableTile, CollidingItems);
	//If colliding items is greater than 1, we know another item that is NOT Item1 is in the way.
	if(CollidingItems.Num() != 0 || !SpotAvailable)
	{
		return false;
	}

	return true;
}

bool UAC_Inventory::CheckCompatibility_Implementation(FS_InventoryItem Item, FS_ContainerSettings Container)
{
	if(!IsValid(Item.ItemAsset))
	{
    	return false;
	}

	FCompatibilitySettingsFragment* CompatibilitySettingsFragment = FindFragment<FCompatibilitySettingsFragment>(Container.ContainerFragments);
	if(!CompatibilitySettingsFragment)
	{
		//No compatibility settings. Assuming everything is compatible
		return true;
	}

	if(IsValid(Item.UniqueID.ParentComponent))
	{
		TArray<FS_ContainerSettings> ItemsContainers;
		Item.UniqueID.ParentComponent->GetAllContainersAssociatedWithItem(Item, ItemsContainers);
		//Items are not allowed to go inside containers that are attached to them.
		if(ItemsContainers.Contains(Container))
		{
			return false;
		}
	}

	FGameplayTagContainer ItemTags = UFL_InventoryFramework::GetItemsTags(Item, true);

	if(CompatibilitySettingsFragment->RequiredTags.IsValidIndex(0))
	{
		if(!ItemTags.HasAnyExact(CompatibilitySettingsFragment->RequiredTags))
		{
			return false;
		}
	}

	if(CompatibilitySettingsFragment->BlockingTags.IsValidIndex(0))
	{		    
		if(ItemTags.HasAnyExact(CompatibilitySettingsFragment->BlockingTags))
		{
			return false;
		}	
	}

	if(!CompatibilitySettingsFragment->ItemTypes.IsEmpty())
	{
		if(!Item.ItemAsset->ItemType.MatchesAny(CompatibilitySettingsFragment->ItemTypes))
		{
			return false;
		}
	}
		
	//Whitelist check.
	if(CompatibilitySettingsFragment->ItemWhitelist.IsValidIndex(0))
	{
		if(!CompatibilitySettingsFragment->ItemWhitelist.Contains(Item.ItemAssetSoftReference))
		{
			return false;
		}
	}

	//Blacklist check.
	if(CompatibilitySettingsFragment->ItemBlacklist.IsValidIndex(0))
	{
		if(CompatibilitySettingsFragment->ItemBlacklist.Contains(Item.ItemAssetSoftReference))
		{
			return false;
		}
	}
		
	return true;
}

bool UAC_Inventory::HasAnyItems()
{
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.ContainerType == ThisActor)
		{
			continue;
		}
		
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(UFL_InventoryFramework::IsItemValid(CurrentItem))
			{
				return true;
			}
		}
	}

	return false;
}

void UAC_Inventory::GetContainerByIdentifier(FGameplayTag ContainerIdentifier,
                                             TArray<FS_ContainerSettings> ContainerList, bool AllowItemsContainers, bool& FoundContainer,
                                             FS_ContainerSettings& Container, int32& ContainerIndex)
{
	if(ContainerList.Num() > 0)
	{
		for(int32 Index=0; Index < ContainerList.Num(); Index++)
		{
			if(ContainerList[Index].ContainerIdentifier.MatchesTagExact(ContainerIdentifier) && (AllowItemsContainers || ContainerList[Index].BelongsToItem.X == -1))
			{
				FoundContainer = true;
				Container = ContainerList[Index];
				ContainerIndex = Index;
				return;
			}
		}
	}

	FS_ContainerSettings EmptyContainer = {};
	Container = EmptyContainer;
	ContainerIndex = -1;
	FoundContainer = false;
}

FS_InventoryItem UAC_Inventory::GetItemByUniqueID(FS_UniqueID UniqueID)
{
	TRACE_CPUPROFILER_EVENT_SCOPE("GetItemByUniqueID")
	if(!UniqueID.IsValid())
	{
		return FS_InventoryItem();
	}

	/**Attempt to get the items directions through the ID map.*/
	if(const FS_IDMapEntry* Entry = ID_Map.Find(UniqueID.IdentityNumber))
	{
		if(UFL_InventoryFramework::AreItemDirectionsValid(UniqueID, Entry->Directions.X, Entry->Directions.Y))
		{
			return ContainerSettings[Entry->Directions.X].Items[Entry->Directions.Y];
		}
	}

	//Directions were invalid. Brute force through everything.
	for(FS_ContainerSettings CurrentContainer : ContainerSettings)
	{
		for(FS_InventoryItem CurrentItem : CurrentContainer.Items)
		{
			if(UniqueID == CurrentItem.UniqueID)
			{
				return CurrentItem;
			}
		}
	}

	return FS_InventoryItem();
}

void UAC_Inventory::GetItemByUniqueIDInContainer(FS_UniqueID UniqueID, int32 ContainerIndex, bool& ItemFound, FS_InventoryItem& Item)
{
	ItemFound = false;
	Item = FS_InventoryItem();
	if(!ContainerSettings.IsValidIndex(ContainerIndex))
	{
		return;
	}
	
	for(auto& CurrentItem : ContainerSettings[ContainerIndex].Items)
	{
		if(UniqueID == CurrentItem.UniqueID)
		{
			Item = CurrentItem;
			ItemFound = true;
			return;
		}
	}
}

TArray<FS_ItemCount> UAC_Inventory::GetListOfItemsByCount(UDA_CoreItem* Item, int32 Count, int32 ContainerIndex, int32& TotalFoundCount)
{
	TotalFoundCount = 0;
	TArray<FS_ItemCount> FoundItems;
	TArray<FS_ContainerSettings> ContainersToCheck;
	if(ContainerIndex == -1)
	{
		for(auto& CurrentContainer : ContainerSettings)
		{
			ContainersToCheck.Add(CurrentContainer);
		}
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			ContainersToCheck.Add(ContainerSettings[ContainerIndex]);
		}
		else
		{
			return FoundItems;
		}
	}
	
	for(const auto& CurrentContainer : ContainersToCheck)
	{
		if(Count == 0)
		{
			break;
		}
		
		TArray<FS_InventoryItem> MatchingItems;
		int32 TotalAmountFound;
		GetAllItemsWithDataAsset(Item, CurrentContainer.ContainerIndex, MatchingItems, TotalAmountFound);
		if(MatchingItems.IsValidIndex(0))
		{
			for(const auto& CurrentItem : MatchingItems)
			{
				if(Count == 0)
				{
					break;
				}
				
				FS_ItemCount ItemCount;
				ItemCount.Item = CurrentItem;
				ItemCount.Count = FMath::Clamp(CurrentItem.Count, 0, Count);
				FoundItems.Add(ItemCount);
				Count = FMath::Clamp(Count - CurrentItem.Count, 0, Count);
				TotalFoundCount += ItemCount.Count;
			}
		}
	}

	return FoundItems;
}

FS_InventoryItem UAC_Inventory::GetItemAtSpecificIndex(FS_ContainerSettings Container, int32 TileIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetItemAtSpecificIndex)
	if(Container.TileMap.IsValidIndex(TileIndex))
	{
		if(Container.TileMap[TileIndex] != 1)
		{
			FS_UniqueID TileMapUniqueID;
			TileMapUniqueID.IdentityNumber = Container.TileMap[TileIndex];
			TileMapUniqueID.ParentComponent = this;

			FS_InventoryItem Item = GetItemByUniqueID(TileMapUniqueID);
			if(Item.IsValid())
			{
				return Item;
			}
		}
	}
	
	return FS_InventoryItem();
}

void UAC_Inventory::GetFirstAvailableTile(FS_InventoryItem Item, FS_ContainerSettings Container,
	const TArray<int32>& IndexesToIgnore, bool& SpotFound, int32& AvailableTile, TEnumAsByte<ERotation>& NeededRotation)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetFirstAvailableTile)
	AvailableTile = -1;
	SpotFound = false;
	NeededRotation = Item.Rotation;

	if(!Container.SupportsTileMap() || !IsValid(Container.UniqueID.ParentComponent))
	{
		SpotFound = true;
		return;
	}
	
	if(Container.UniqueID.ParentComponent != this)
	{
		Container.UniqueID.ParentComponent->GetFirstAvailableTile(Item, Container, IndexesToIgnore, SpotFound, AvailableTile, NeededRotation);
        return;
	}

	bool PerformComplexCalculation = CanItemBeRotated(Item) && Container.IsSpacialContainer();

	//If we are about to do complex collision checks, pre-perform some calculations.
	TArray<FRotationAndShape> Shapes;
	if(PerformComplexCalculation)
	{
		/**Item can be rotated and is inside a spacial container.
		 * Instead of using CheckAllRotationsForSpace, we will cache
		 * each shape for each rotation, then run a very simple
		 * and performant check.
		 * Where as with spamming CheckAllRotationsForSpace, we'd
		 * be re-calculating the shape 4 times per tile.*/
		const ERotation StartingRotation = Item.Rotation;
		ERotation CurrentRotation = StartingRotation;
		constexpr int32 MaxEnumSize = static_cast<int32>(ERotation::TwoSeventy);

		/**The way this loop works is by starting off on the items default rotation,
		 * then it figures out what enum entry is after that, and it then does
		 * some math in case it hits the final entry to loop back around,
		 * then stopping once it hits the original rotation again.*/
		do
		{
			Item.Rotation = CurrentRotation;
			TArray<FIntPoint> ItemsShape = Item.ItemAsset->GetItemsPureShape(Item.Rotation);
			Shapes.Add(FRotationAndShape(Item.Rotation, ItemsShape));
			
			//Figure out the next enum
			const int32 NextRotation = static_cast<int32>(CurrentRotation) + 1;
			CurrentRotation = static_cast<ERotation>((NextRotation % (MaxEnumSize + 1)));
		} while (CurrentRotation != StartingRotation);
	}

	if(Container.ContainerType == Inventory)
	{
		for(int32 CurrentTile = 0; CurrentTile < Container.TileMap.Num(); CurrentTile++)
		{
			if(Container.TileMap[CurrentTile] != -1)
			{
				continue;
			}

			if(IndexesToIgnore.Contains(CurrentTile))
			{
				continue;
			}

			//Use the simple CheckForSpace if item can't be rotated
			//or if the container isn't spacial.
			if(PerformComplexCalculation)
			{
				/**We've pre-calculated each rotations shape*/
				for(auto& CurrentShape : Shapes)
				{
					TArray<FS_InventoryItem> ItemsInTheWay;
					TArray<FS_InventoryItem> ItemsToIgnore;
					NeededRotation = CurrentShape.Rotation;
					CheckForSpaceForShape(CurrentShape.Shape, Container, CurrentTile, ItemsToIgnore, IndexesToIgnore, SpotFound, AvailableTile, ItemsInTheWay, true);
					if(SpotFound)
					{
						AvailableTile = CurrentTile;
						return;
					}
				}
			}
			else
			{
				TArray<FS_InventoryItem> ItemsInTheWay;
				TArray<FS_InventoryItem> ItemsToIgnore;
				CheckForSpace(Item, Container, CurrentTile, ItemsToIgnore, IndexesToIgnore, SpotFound, AvailableTile, ItemsInTheWay, true);
				if(SpotFound)
				{
					AvailableTile = CurrentTile;
					return;
				}
			}
		}
	}
	else if(Container.ContainerType == Equipment)
	{
		if(Container.TileMap[0] == -1)
		{
			SpotFound = true;
			AvailableTile = 0;
			NeededRotation = Item.Rotation;
		}
	}
}

void UAC_Inventory::GetFirstAvailableContainerAndTile_Implementation(FS_InventoryItem Item,
	const TArray<int32>& ContainersToIgnore, bool& SpotFound, FS_ContainerSettings& AvailableContainer, int32& AvailableTile,
	TEnumAsByte<ERotation>& NeededRotation)
{
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(!ContainersToIgnore.Contains(CurrentContainer.ContainerIndex))
		{
			bool IsAllowed = CheckCompatibility(Item, CurrentContainer);
			if(IsAllowed)
			{
				bool SpaceFound = false;
				const TArray<int32> IndexesToIgnore = GetGenericIndexesToIgnore(CurrentContainer);
				GetFirstAvailableTile(Item, CurrentContainer, IndexesToIgnore, SpaceFound, AvailableTile, NeededRotation);
				if(SpaceFound)
				{
					SpotFound = true;
					AvailableContainer = CurrentContainer;
					return;
				}
				
				TEnumAsByte<EContainerInfinityDirection> InfinityDirection;
				if(UFL_InventoryFramework::IsContainerInfinite(CurrentContainer, InfinityDirection))
				{
					SpotFound = true;
					AvailableContainer = CurrentContainer;
					AvailableTile = -1;
					return;
				}
			}
		}
	}
	//above return was never hit, we can assume no spot is free.
	SpotFound = false;
	AvailableTile = -1;
	NeededRotation = Item.Rotation;
}


TArray<FS_ContainerSettings> UAC_Inventory::GetItemsChildrenContainers(FS_InventoryItem Item)
{
	TArray<FS_ContainerSettings> Containers;
	
	if(Item.ItemAsset)
	{
		if(Item.ItemAsset->GetDefaultContainers().IsEmpty())
		{
			return Containers;
		}
	}
	
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;

	if(!IsValid(ParentComponent))
	{
		return Containers;
	}

	//Item might not be initialized properly yet.
	if(!ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		return Containers;
	}
	
	int32 X;
	int32 Y;
	if(Item.UniqueID.IdentityNumber > 0)
	{
		X = ParentComponent->ContainerSettings[Item.ContainerIndex].UniqueID.IdentityNumber;
		Y = Item.UniqueID.IdentityNumber;
	}
	else
	{
		//Item is uninitialized (most likely still in editor).
		//Try to find any containers if @Item has correct directions.
		if(Item.ContainerIndex < 0 || Item.ItemIndex < 0)
		{
			return Containers;
		}
		X = Item.ContainerIndex;
		Y = Item.ItemIndex;
	}

	//TODO: Update this to use GetContainerByUniqueID
	for(auto& CurrentContainer : ParentComponent->ContainerSettings)
	{
		if(CurrentContainer.BelongsToItem.X == X && CurrentContainer.BelongsToItem.Y == Y)
		{
			Containers.Add(CurrentContainer);
		}
	}

	return Containers;
}

void UAC_Inventory::GetWidgetForContainer(FS_ContainerSettings Container, UW_Container*& Widget)
{
	Widget = UFL_InventoryFramework::GetWidgetForContainer(Container);
}

void UAC_Inventory::GetWidgetForItem(FS_InventoryItem Item, UW_InventoryItem*& Widget)
{
	Widget = UFL_InventoryFramework::GetWidgetForItem(Item);
}

TArray<UW_Container*> UAC_Inventory::GetAllContainerWidgets()
{
	TArray<UW_Container*> FoundContainerWidgets;
	
	for(const auto& CurrentContainer : ContainerSettings)
	{
		UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(CurrentContainer);
		if(IsValid(ContainerWidget))
		{
			FoundContainerWidgets.AddUnique(ContainerWidget);
		}
	}

	return FoundContainerWidgets;
}

void UAC_Inventory::RemoveAllContainerWidgets()
{
	for(auto& CurrentContainer : ContainerSettings)
	{
		UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(CurrentContainer);
		if(IsValid(ContainerWidget))
		{
			II_ExternalObjects::Execute_RemoveWidgetReferences(ContainerWidget);
			CurrentContainer.Widget = nullptr;
		}
	}
}

void UAC_Inventory::GetChildrenItems(FS_InventoryItem Item, TArray<FS_ItemSubLevel>& AssociatedItems)
{
	CurrentSubLevel++;
	TArray<FS_ContainerSettings> ItemContainers = GetItemsChildrenContainers(Item);
	if(ItemContainers.IsValidIndex(0))
	{
		for(auto& CurrentContainer : ItemContainers)
		{
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				FS_ItemSubLevel CurrentSubItem;
				CurrentSubItem.SubLevel = CurrentSubLevel;
				CurrentSubItem.Item = CurrentItem;
				AssociatedItems.Add(CurrentSubItem);
				GetChildrenItems(CurrentItem, AssociatedItems);
			}
		}
	}
	CurrentSubLevel--;
}

void UAC_Inventory::GetAllContainersAssociatedWithItem(FS_InventoryItem Item, TArray<FS_ContainerSettings>& Containers)
{
	TArray<FS_ItemSubLevel> AssociatedItems;
	Containers = GetItemsChildrenContainers(Item);
	GetChildrenItems(Item, AssociatedItems);
	if(AssociatedItems.IsValidIndex(0))
	{
		for(auto& CurrentItem : AssociatedItems)
		{
			TArray<FS_ContainerSettings> TempContainers = GetItemsChildrenContainers(CurrentItem.Item);
			if(TempContainers.IsValidIndex(0))
			{
				Containers.Append(TempContainers);
			}
		}
	}
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainerSettingsForSpawningItemActor(FS_InventoryItem Item)
{
	TArray<FS_ContainerSettings> Containers;

	if(!Item.IsValid())
	{
		return Containers;
	}

	FS_InventoryItem DefaultItem = Item;
	DefaultItem.ContainerIndex = -1;
	DefaultItem.ItemIndex = -1;
	DefaultItem.TileIndex = 1;
	DefaultItem.Rotation = Zero;
	
	FS_ContainerSettings DefaultContainer;
	DefaultContainer.ContainerType = ThisActor;
	DefaultContainer.Items.Add(DefaultItem);
	DefaultContainer.UniqueID = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].UniqueID;

	Containers.Add(DefaultContainer);
	TArray<FS_ContainerSettings> ItemsContainers;
	GetAllContainersAssociatedWithItem(Item, ItemsContainers);
	if(ItemsContainers.IsValidIndex(0))
	{
		Containers.Append(ItemsContainers);
	}

	return Containers;
}

void UAC_Inventory::GetParentItems(FS_InventoryItem Item, TArray<FS_ItemSubLevel>& ParentItems)
{
	if(!Item.IsValid())
	{
		return;
	}
	
	CurrentSubLevel++;
	
	FS_UniqueID ParentItemID;
	ParentItemID.ParentComponent = Item.UniqueID.ParentComponent;
	ParentItemID.IdentityNumber = ContainerSettings[Item.ContainerIndex].BelongsToItem.Y;
	FS_InventoryItem ParentItem = GetItemByUniqueID(ParentItemID);
	if(ParentItem.IsValid())
	{
		FS_ItemSubLevel CurrentSubItem;
		CurrentSubItem.SubLevel = CurrentSubLevel;
		CurrentSubItem.Item = ParentItem;
		ParentItems.Add(CurrentSubItem);
		GetParentItems(ParentItem, ParentItems);
	}
	CurrentSubLevel--;
}

FS_InventoryItem UAC_Inventory::GetParentItem(FS_InventoryItem Item)
{
	if(!Item.IsValid())
	{
		return FS_InventoryItem();
	}
	
	FS_UniqueID ParentItemID;
	ParentItemID.ParentComponent = Item.UniqueID.ParentComponent;
	ParentItemID.IdentityNumber = ContainerSettings[Item.ContainerIndex].BelongsToItem.Y;
	FS_InventoryItem FoundParentItem = GetItemByUniqueID(ParentItemID);
	if(FoundParentItem.IsValid())
	{
		return FoundParentItem;
	}

	return FS_InventoryItem();
}

TArray<FS_InventoryItem> UAC_Inventory::GetAllItems(int32 ContainerIndex)
{
	TArray<FS_InventoryItem> FoundItems;
	if(ContainerIndex <= -1)
	{
		for(auto& CurrentContainer : ContainerSettings)
		{
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				FoundItems.Add(CurrentItem);
			}
		}
	}
	else
	{
		for(auto& CurrentItem : ContainerSettings[ContainerIndex].Items)
		{
			FoundItems.Add(CurrentItem);
		}
	}
	
	return FoundItems;
}

void UAC_Inventory::GetAllItemsWithDataAsset(UDA_CoreItem* DataAsset, int32 ContainerIndex, TArray<FS_InventoryItem>& Items, int32& TotalCountFound)
{
	TArray<FS_InventoryItem> ReturnedItems;
	TotalCountFound = 0;
	
	if(ContainerIndex == -1)
	{
		for(auto& CurrentContainer : ContainerSettings)
		{
			for(auto& CurrentItem : CurrentContainer.Items)
			{
				if(DataAsset == CurrentItem.ItemAsset)
				{
					TotalCountFound += CurrentItem.Count;
					ReturnedItems.Add(CurrentItem);
				}
			}
		}
	}
	else
	{
		if(ContainerSettings.IsValidIndex(ContainerIndex))
		{
			for(auto& CurrentItem : ContainerSettings[ContainerIndex].Items)
			{
				if(DataAsset == CurrentItem.ItemAsset)
				{
					TotalCountFound += CurrentItem.Count;
					ReturnedItems.Add(CurrentItem);
				}
			}
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Invalid container index used for GetAllItemsWithDataAsset."), true, true);
		}
	}

	Items = ReturnedItems;
}

void UAC_Inventory::GetItemsTileIndexes(FS_InventoryItem Item, TArray<int32>& Indexes, bool& InvalidTileFound)
{
	InvalidTileFound = false;
	Indexes.Empty();
	
	//Item must be initialized properly for this function to work.
	if(!ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		InvalidTileFound = true;
		return;
	}
	
	if(ContainerSettings[Item.ContainerIndex].Style != Grid)
	{
		if(!ContainerSettings[Item.ContainerIndex].TileMap.IsValidIndex(Item.TileIndex))
		{
			InvalidTileFound = true;
			return;
		}

		
		Indexes.Add(Item.TileIndex);
		InvalidTileFound = false;
		return;
	}

	TArray<FIntPoint> ItemsShape = UFL_InventoryFramework::GetItemsShape(Item, InvalidTileFound);
	FS_ContainerSettings Container = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex];

	for(auto& CurrentTile : ItemsShape)
	{
		if(UFL_InventoryFramework::IsTileValid(CurrentTile.X, CurrentTile.Y, Container))
		{
			Indexes.Add(UFL_InventoryFramework::TileToIndex(CurrentTile.X, CurrentTile.Y, Container));
		}
	}
}

TArray<FS_InventoryItem> UAC_Inventory::GetNearbyItems(FS_InventoryItem Item, int32 Range)
{
	TArray<FS_InventoryItem> FoundItems;

	if(!UFL_InventoryFramework::IsItemValid(Item))
	{
		return FoundItems;
	}
	if(Item.UniqueID.ParentComponent != this)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item does not belong to target component -> GetNearbyItems"), true, true);
		return FoundItems;
	}
	if(!Item.UniqueID.ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		return FoundItems;
	}

	FS_ContainerSettings ParentContainer = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex];

	TArray<int32> ScannedTiles; //Small optimization so we don't scan the same tile multiple times.
	TArray<int32> ItemsTiles;
	bool InvalidTileFound = false;
	GetItemsTileIndexes(Item, ItemsTiles, InvalidTileFound);
	
	if(!InvalidTileFound && ItemsTiles.IsValidIndex(0))
	{
		for(const auto& CurrentItemTile : ItemsTiles)
		{
			//Get the items X and Y
			FIntPoint ItemsTile;
			UFL_InventoryFramework::IndexToTile(CurrentItemTile, ParentContainer, ItemsTile.X, ItemsTile.Y);

			//Start looping around the tiles around the tiles location.
			//This technically scans out of bounds as well, but we check
			//if the tile is valid.
			//This can technically be optimized to be clamped inside the container.
			TArray<int32> Tiles;
			for(int32 ColumnY = ItemsTile.Y - Range; ColumnY <= ItemsTile.Y + Range; ColumnY++)
			{
				if(ColumnY < ParentContainer.Dimensions.Y)
				{
					for(int32 RowX = ItemsTile.X - Range; RowX <= ItemsTile.X + Range; RowX++)
					{
						if(RowX < ParentContainer.Dimensions.X)
						{
							int32 CurrentIndex = UFL_InventoryFramework::TileToIndex(RowX, ColumnY, ParentContainer);;
							if(ParentContainer.TileMap.IsValidIndex(CurrentIndex) && UFL_InventoryFramework::IsTileValid(RowX, ColumnY, ParentContainer) && !ScannedTiles.Contains(CurrentIndex))
							{
								Tiles.Add(CurrentIndex);
							}
						}
					} 
				}
			}

			for(const auto& CurrentTile : Tiles)
			{
				if(ScannedTiles.Contains(CurrentTile))
				{
					//Tile has already been scanned, skip this element.
					continue;
				}
				
				ScannedTiles.Add(CurrentTile);
				if(ParentContainer.TileMap.IsValidIndex(CurrentTile))
				{
					if(ParentContainer.TileMap[CurrentTile] != -1 && ParentContainer.TileMap[CurrentTile] != Item.UniqueID.IdentityNumber)
					{
						FS_UniqueID ItemID;
						ItemID.IdentityNumber = ParentContainer.TileMap[CurrentTile];
						ItemID.ParentComponent = Item.UniqueID.ParentComponent;
						FS_InventoryItem CollidingItem = GetItemByUniqueID(ItemID);
						if(CollidingItem.IsValid())
						{
							FoundItems.AddUnique(CollidingItem);
						}
					}
				}
			}
		}
	}

	return FoundItems;
}

	TArray<FS_InventoryItem> UAC_Inventory::GetNearbyItemsDirectional(FS_InventoryItem Item, FIntPoint Direction, TArray<int32>& Tiles)
	{
		TArray<FS_InventoryItem> FoundItems;

		if(!UFL_InventoryFramework::IsItemValid(Item))
		{
			return FoundItems;
		}
		if(Item.UniqueID.ParentComponent != this)
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Item does not belong to target component -> GetNearbyItems"), true, true);
			return FoundItems;
		}
		if(!Item.UniqueID.ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
		{
			return FoundItems;
		}

		FS_ContainerSettings ParentContainer = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex];
		
		TArray<int32> ItemsTiles;
		bool InvalidTileFound = false;
		Item.UniqueID.ParentComponent->GetItemsTileIndexes(Item, ItemsTiles, InvalidTileFound);

		TArray<FIntPoint> ItemsShape = UFL_InventoryFramework::GetItemsShape(Item, InvalidTileFound);
		TArray<int32> TracedTiles;
		
		if(!InvalidTileFound && ItemsTiles.IsValidIndex(0))
		{
			for(const auto& CurrentTile : ItemsShape)
			{
				int32 DeltaX = FMath::Abs((CurrentTile.X + Direction.X) - CurrentTile.X);
				int32 DeltaY = -FMath::Abs((CurrentTile.Y + Direction.Y) - CurrentTile.Y);
				int32 DeltaError = DeltaX + DeltaY;
				int32 X = CurrentTile.X;
				int32 Y = CurrentTile.Y;
				int32 XStep = (CurrentTile.X < (CurrentTile.X + Direction.X)) ? 1 : -1;
				int32 YStep = (CurrentTile.Y < (CurrentTile.Y + Direction.Y)) ? 1 : -1;

				while (true)
				{
					int32 DoubleError = 2 * DeltaError;

					//Small optimization. If we update the steps first,
					//we reduce the amount of scanning tiles we know are already
					//populated by the item.
					//If this optimization is causing issues, might be worth
					//putting this after the X and Y break loop check below.
					if (DoubleError >= DeltaY)
					{
						DeltaError += DeltaY;
						X += XStep;
					}

					if (DoubleError <= DeltaX)
					{
						DeltaError += DeltaX;
						Y += YStep;
					}

					//Check if the point is within the bounds of the container
					if (X >= 0 && X < ParentContainer.Dimensions.X && Y >= 0 && Y < ParentContainer.Dimensions.Y)
					{
						int32 Index = Y * ParentContainer.Dimensions.X + X;
						TracedTiles.AddUnique(Index);
					}
					else
					{
						break;
					}

					//X and Y have reached their end destination, break the loop
					if (X == (CurrentTile.X + Direction.X) && Y == (CurrentTile.Y + Direction.Y))
						break;
				}
			}
		}

		for(auto& CurrentTile : TracedTiles)
		{
			FS_InventoryItem FoundItem = Item.UniqueID.ParentComponent->GetItemAtSpecificIndex(ParentContainer, CurrentTile);
			if(FoundItem != Item)
			{
				FoundItems.AddUnique(FoundItem);
			}
		}

		Tiles = TracedTiles;
		return FoundItems;
	}

TArray<FS_ContainerSettings> UAC_Inventory::GetAvailableEquipmentContainers()
{
	TArray<FS_ContainerSettings> FoundContainers;
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.ContainerType == Equipment && !CurrentContainer.Items.IsValidIndex(0))
		{
			FoundContainers.Add(CurrentContainer);
		}
	}
	
	return FoundContainers;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetCompatibleEquipContainersForItem(FS_InventoryItem Item,
	bool OnlyEmptyContainers)
{
	TArray<FS_ContainerSettings> FoundContainers;
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.ContainerType == Equipment)
		{
			if(!CheckCompatibility(Item, CurrentContainer))
			{
				continue;
			}
			
			if(OnlyEmptyContainers && !CurrentContainer.Items.IsValidIndex(0))
			{
				FoundContainers.Add(CurrentContainer);
				continue;
			}

			if(!OnlyEmptyContainers)
			{
				FoundContainers.Add(CurrentContainer);
			}
		}
	}
	
	return FoundContainers;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetCompatibleContainersForItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ContainersFilter)
{
	TArray<FS_ContainerSettings> FoundContainers;
	if(ContainersFilter.IsEmpty())
	{
		ContainersFilter = ContainerSettings;
	}
	
	for(auto& CurrentContainer : ContainersFilter)
	{
		if(!CheckCompatibility(Item, CurrentContainer))
		{
			continue;
		}

		FoundContainers.Add(CurrentContainer);
	}

	return FoundContainers;
}

void UAC_Inventory::AddTagsToTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags)
{
	if(!Container.TileMap.IsValidIndex(TileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Can't add tags to tile, tile index is invalid");
		return;
	}

	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_AddTagsToTile(Container, TileIndex, Tags);
		return;
	}

	if(UFL_InventoryFramework::IsContainerValid(Container))
	{
		//We don't want to bother with RPC's if the container is invalid.
		S_AddTagsToTile(Container.UniqueID, TileIndex, Tags, GetOwner()->GetLocalRole());
	}
}

void UAC_Inventory::S_AddTagsToTile_Implementation(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags,
                                                   ENetRole CallerLocalRole)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	
	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_AddTagsToTile(OriginalContainer, TileIndex, Tags);
			}
			else
			{
				Internal_AddTagsToTile(OriginalContainer, TileIndex, Tags);
				C_AddTagsToTile(ContainerID, TileIndex, Tags);
			}
		}
		else
		{
			Internal_AddTagsToTile(OriginalContainer, TileIndex, Tags);
			C_AddTagsToTile(ContainerID, TileIndex, Tags);
		}
	}
	else
	{
		Internal_AddTagsToTile(OriginalContainer, TileIndex, Tags);
		C_AddTagsToTile(ContainerID, TileIndex, Tags);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_AddTagsToTile(ContainerID, TileIndex, Tags);
		}
	}
}

void UAC_Inventory::C_AddTagsToTile_Implementation(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);

	if(OriginalContainer.IsValid())
	{
		Internal_AddTagsToTile(OriginalContainer, TileIndex, Tags);
	}
}

void UAC_Inventory::Internal_AddTagsToTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags)
{
	if(!Container.TileMap.IsValidIndex(TileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Can't add tags to tile, tile index is invalid");
		return;
	}

	UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
	if(!IsValid(ParentComponent))
	{
		return;
	}

	if(!ParentComponent->ContainerSettings.IsValidIndex(Container.ContainerIndex))
	{
		return;
	}

	if(ParentComponent->ContainerSettings[Container.ContainerIndex].UniqueID != Container.UniqueID)
	{
		//Index is not correct
		return;
	}
	
	FTileTagsFragment* TileTagsFragment = FindFragment<FTileTagsFragment>(ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments);
	int32 TileTagIndex = UCF_TileTags::GetTileTagIndexForTile(Container, TileIndex);
	if(TileTagIndex == -1)
	{
		//TileTag index not found, add it to the TileTag array
		TileTagsFragment->TileTags.Add(FS_TileTag(TileIndex, Tags));
	}
	else
	{
		//TileTag index found, update it
		TileTagsFragment->TileTags[TileTagIndex].Tags.AppendTags(Tags);
	}

	ParentComponent->TileTagsAdded.Broadcast(ParentComponent->ContainerSettings[Container.ContainerIndex], TileIndex, Tags);
}

void UAC_Inventory::RemoveTagsFromTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags)
{
	if(!Container.TileMap.IsValidIndex(TileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Can't add tags to tile, tile index is invalid");
		return;
	}

	if(UKismetSystemLibrary::IsStandalone(this))
	{
		Internal_RemoveTagsFromTile(Container, TileIndex, Tags);
		return;
	}

	if(UFL_InventoryFramework::IsContainerValid(Container))
	{
		S_RemoveTagsFromTile(Container.UniqueID, TileIndex, Tags, GetOwner()->GetLocalRole());
	}
}

void UAC_Inventory::S_RemoveTagsFromTile_Implementation(FS_UniqueID ContainerID, int32 TileIndex,
														FGameplayTagContainer Tags, ENetRole CallerLocalRole)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	
	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveTagsFromTile(OriginalContainer, TileIndex, Tags);
			}
			else
			{
				Internal_RemoveTagsFromTile(OriginalContainer, TileIndex, Tags);
				C_RemoveTagsFromTile(ContainerID, TileIndex, Tags);
			}
		}
		else
		{
			Internal_RemoveTagsFromTile(OriginalContainer, TileIndex, Tags);
			C_RemoveTagsFromTile(ContainerID, TileIndex, Tags);
		}
	}
	else
	{
		Internal_RemoveTagsFromTile(OriginalContainer, TileIndex, Tags);
		C_RemoveTagsFromTile(ContainerID, TileIndex, Tags);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveTagsFromTile(ContainerID, TileIndex, Tags);
		}
	}
}

void UAC_Inventory::C_RemoveTagsFromTile_Implementation(FS_UniqueID ContainerID, int32 TileIndex,
	FGameplayTagContainer Tags)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);

	if(OriginalContainer.IsValid())
	{
		Internal_RemoveTagsFromTile(OriginalContainer, TileIndex, Tags);
	}
}

void UAC_Inventory::Internal_RemoveTagsFromTile(FS_ContainerSettings Container, int32 TileIndex,
	FGameplayTagContainer Tags)
{
	if(!Container.TileMap.IsValidIndex(TileIndex))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Can't add tags to tile, tile index is invalid");
		return;
	}

	UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
	if(!IsValid(ParentComponent))
	{
		return;
	}

	if(!ParentComponent->ContainerSettings.IsValidIndex(Container.ContainerIndex))
	{
		return;
	}

	if(ParentComponent->ContainerSettings[Container.ContainerIndex].UniqueID != Container.UniqueID)
	{
		//Index is not correct
		return;
	}
	
	FTileTagsFragment* TileTagsFragment = FindFragment<FTileTagsFragment>(ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments);
	int32 TileTagIndex = UCF_TileTags::GetTileTagIndexForTile(Container, TileIndex);
	if(TileTagIndex == -1)
	{
		//TileTag index is invalid, no need to continue
		return;
	}
	else
	{
		//TileTag index found, update it
		TileTagsFragment->TileTags[TileIndex].Tags.RemoveTags(Tags);

		//Array must be kept 1:1 if it's optimized for game thread.
		if(!TileTagsFragment->OptimizeForGameThread)
		{
			//Check if it's empty, if so, remove the index
			if(TileTagsFragment->TileTags[TileIndex].Tags.IsEmpty())
			{
				TileTagsFragment->TileTags.RemoveAt(TileIndex);
			}
		}
	}

	ParentComponent->TileTagsRemoved.Broadcast(ParentComponent->ContainerSettings[Container.ContainerIndex], TileIndex, Tags);
}

TArray<int32> UAC_Inventory::GetGenericIndexesToIgnore_Implementation(FS_ContainerSettings Container)
{
	return TArray<int32>();
}

void UAC_Inventory::BindContainerWithWidget(FS_ContainerSettings Container, UW_Container* Widget, bool& Success)
{
	if(IsValid(Widget) && UFL_InventoryFramework::IsContainerValid(Container))
	{
		Widget->ConstructContainers(Container, this, true);
		Success = true;
		return;
	}
	
	Success = false;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetComponentOwningContainers()
{
	TArray<FS_ContainerSettings> Containers;

	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.BelongsToItem.X < 0 || CurrentContainer.BelongsToItem.Y < 0)
		{
			Containers.Add(CurrentContainer);
		}
		else
		{
			/**Since new containers are appended to the ContainerSettings
			 * array, we can assume that once we hit one that belongs
			 * to an item, we have found all the containers.*/
			return Containers;
		}
	}
	
	return Containers;
}

FS_ContainerSettings UAC_Inventory::GetContainerByUniqueID(FS_UniqueID UniqueID)
{
	/**Attempt to get the items directions through the ID map.*/
	if(const FS_IDMapEntry* Entry = ID_Map.Find(UniqueID.IdentityNumber))
	{
		if(ContainerSettings.IsValidIndex(Entry->Directions.X))
		{
			if(UniqueID == ContainerSettings[Entry->Directions.X].UniqueID)
			{
				return ContainerSettings[Entry->Directions.X];;
			}
		}
	}
	
	if(ContainerSettings.IsValidIndex(0))
	{
		for(int32 CurrentIndex = 0; CurrentIndex < ContainerSettings.Num(); CurrentIndex++)
		{
			if(UniqueID == ContainerSettings[CurrentIndex].UniqueID)
			{
				return ContainerSettings[CurrentIndex];
			}
		}
	}

	return FS_ContainerSettings();
}

bool UAC_Inventory::DoesContainerBelongToComponent(FS_ContainerSettings Container)
{
	return Container.BelongsToItem.X == -1 || Container.BelongsToItem.Y == -1;
}

void UAC_Inventory::AdjustContainerSize(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems)
{
	UAC_Inventory* TargetComponent = Container.UniqueID.ParentComponent;

	if(!Container.SupportsTileMap())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Tried to expand a container that doesn't support it - AC_Inventory.cpp -> AdjustContainerSize");
		return;
	}
	
	if(!IsValid(TargetComponent))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Target component not valid - AC_Inventory.cpp -> AdjustContainerSize"), true, true);
		return;
	}

	if(!IsValid(Container.UniqueID.ParentComponent))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container UniqueID not valid - AC_Inventory.cpp -> AdjustContainerSize"), true, true);
		return;
	}

	if(TargetComponent->Initialized == false && IsValid(UGameplayStatics::GetGameInstance(this)))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to adjust container size on component that is not initialized yet - AC_Inventory.cpp -> AdjustContainerSize"), true, true);
		return;
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		FRandomStream Seed;
		Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
		TargetComponent->Internal_AdjustContainerSize(Container, Adjustments, ClampToItems, Seed);
		return;
	}
	
	S_AdjustContainerSize(Container, Adjustments, ClampToItems, GetOwner()->GetLocalRole());
}

void UAC_Inventory::S_AdjustContainerSize_Implementation(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems, ENetRole CallerLocalRole)
{
	UAC_Inventory* TargetComponent = Container.UniqueID.ParentComponent;
	if(!IsValid(TargetComponent))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container UniqueID not valid - AC_Inventory.cpp -> AdjustContainerSize"), true, true);
		return;
	}

	if(Initialized == false)
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Tried to adjust container size on component that is not initialized yet - AC_Inventory.cpp -> S_AdjustContainerSize"), true, true);
		return;
	}

	//Validate the passed in @Container in case client fed bad data.
	FS_ContainerSettings FoundContainer = GetContainerByUniqueID(Container.UniqueID);
	if(FoundContainer.IsValid())
	{
		bool IsContainerValid = true;
		
		if(Container.Items.Num() != FoundContainer.Items.Num())
		{
			IsContainerValid = false;
		}

		if(Container.ContainerIndex != FoundContainer.ContainerIndex)
		{
			IsContainerValid = false;
		}

		if(!IsContainerValid)
		{
			UFL_InventoryFramework::LogIFPMessage(this, TEXT("Some container data from client did not match with server version of container - AC_Inventory.cpp -> S_AdjustContainerSize"), true, true);
			return;
		}
	}
	
	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));

	TArray<UAC_Inventory*> CombinedListeners;
	for(auto& AppendingListener : Container.UniqueID.ParentComponent->Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}
	for(auto& AppendingListener : Listeners)
	{
		CombinedListeners.AddUnique(AppendingListener);
	}

	for(const auto& CurrentListener : CombinedListeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_AdjustContainerSize(Container.UniqueID, Adjustments, ClampToItems, Seed);
		}
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				TargetComponent->Internal_AdjustContainerSize(Container, Adjustments, ClampToItems, Seed);
			}
			else
			{
				TargetComponent->Internal_AdjustContainerSize(Container, Adjustments, ClampToItems, Seed);
				C_AdjustContainerSize(Container.UniqueID, Adjustments, ClampToItems, Seed);
			}
		}
		else
		{
			TargetComponent->Internal_AdjustContainerSize(Container, Adjustments, ClampToItems, Seed);
			C_AdjustContainerSize(Container.UniqueID, Adjustments, ClampToItems, Seed);
		}
	}
	else
	{
		TargetComponent->Internal_AdjustContainerSize(Container, Adjustments, ClampToItems, Seed);
		C_AdjustContainerSize(Container.UniqueID, Adjustments, ClampToItems, Seed);
	}
}

void UAC_Inventory::C_AdjustContainerSize_Implementation(FS_UniqueID ContainerID, FMargin Adjustments, bool ClampToItems, FRandomStream Seed)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	UAC_Inventory* TargetComponent = ContainerID.ParentComponent;
	if(!IsValid(TargetComponent))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container UniqueID not valid - AC_Inventory.cpp -> C_AdjustContainerSize"), true, true);
		return;
	}
	
	FS_ContainerSettings FoundContainer = TargetComponent->GetContainerByUniqueID(ContainerID);
	if(FoundContainer.IsValid())
	{
		TargetComponent->Internal_AdjustContainerSize(FoundContainer, Adjustments, ClampToItems, Seed);
	}
	
}

bool UAC_Inventory::Internal_AdjustContainerSize(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems, FRandomStream Seed)
{
	if(Container.UniqueID.IdentityNumber < 1)
	{
		return false;
	}

	FS_ContainerSettings& ContainerRef = Container.UniqueID.ParentComponent->ContainerSettings[Container.ContainerIndex];
	UAC_Inventory* ParentComponent = ContainerRef.UniqueID.ParentComponent;

	UW_Container* ContainerWidget = UFL_InventoryFramework::GetWidgetForContainer(Container);

	/**Start modifying the size of the container. While it could be cheaper to shrink first, then expand,
	 * we expand first because if an item has to be moved because it's occupying a tile we are removing, if there's
	 * not enough space in the container, we have to move it to a new container. By expanding first, we can check
	 * if the newly added space is enough to hold the item that needs to be moved.*/

	//TODO: Stop using lazy for loops with InRange, use GetTilesWithinDimension instead.
	TArray<int32> TilesToRemove;
	if(Adjustments.Right > 0)
	{
		Adjustments.Right = UKismetMathLibrary::FTrunc(Adjustments.Right);
		ContainerRef.Dimensions.X += Adjustments.Right;
		for(int32 CurrentIndex = 0; CurrentIndex < ContainerRef.Dimensions.X * ContainerRef.Dimensions.Y; CurrentIndex++)
		{
			int32 X;
			int32 Y;
			UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef))
			{
				if(UKismetMathLibrary::InRange_IntInt(X, ContainerRef.Dimensions.X - Adjustments.Right, ContainerRef.Dimensions.X, true, true))
				{
					ContainerRef.TileMap.Insert(-1, CurrentIndex);
				}
			}
		}
	}
	
	if(Adjustments.Bottom > 0)
	{
		Adjustments.Bottom = UKismetMathLibrary::FTrunc(Adjustments.Bottom);
		ContainerRef.Dimensions.Y += Adjustments.Bottom;
		int32 MinRange = UFL_InventoryFramework::TileToIndex(0, ContainerRef.Dimensions.Y - Adjustments.Bottom, ContainerRef);
		for(int32 CurrentIndex = MinRange; CurrentIndex < ContainerRef.Dimensions.X * ContainerRef.Dimensions.Y; CurrentIndex++)
		{
			int32 X;
			int32 Y;
			UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef))
			{
				ContainerRef.TileMap.Insert(-1, CurrentIndex);
			}
		}
	}
	
	if(Adjustments.Left > 0)
	{
		Adjustments.Left = UKismetMathLibrary::FTrunc(Adjustments.Left);
		TArray<FIntPoint> AddedTiles;
		ContainerRef.Dimensions.X += Adjustments.Left;
		for(int32 CurrentIndex = 0; CurrentIndex < ContainerRef.Dimensions.X * ContainerRef.Dimensions.Y - 1; CurrentIndex++)
		{
			int32 X;
			int32 Y;
			UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef))
			{
				if(UKismetMathLibrary::InRange_IntInt(X, 0, Adjustments.Left - 1, true, true))
				{
					ContainerRef.TileMap.Insert(-1, CurrentIndex);
					FIntPoint NewTile;
					NewTile.X = X;
					NewTile.Y = Y;
					AddedTiles.Add(NewTile);
				}
			}
		}
	}
	
	if(Adjustments.Top > 0)
	{
		Adjustments.Top = UKismetMathLibrary::FTrunc(Adjustments.Top);
		TArray<FIntPoint> AddedTiles;
		ContainerRef.Dimensions.Y += Adjustments.Top;
		int32 MaxRange = UFL_InventoryFramework::TileToIndex(ContainerRef.Dimensions.X, Adjustments.Top - 1, ContainerRef);
		for(int32 CurrentIndex = 0; CurrentIndex < MaxRange; CurrentIndex++)
		{
			int32 X;
			int32 Y;
			UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef))
			{
				if(UKismetMathLibrary::InRange_IntInt(Y, 0, Adjustments.Top - 1, true, true))
				{
					ContainerRef.TileMap.Insert(-1, CurrentIndex);
					FIntPoint NewTile;
					NewTile.X = X;
					NewTile.Y = Y;
					AddedTiles.Add(NewTile);
				}
			}
		}
	}

	//Apply expansion adjustments to items so they'll offset correctly.
	for(auto& CurrentItem : ContainerRef.Items)
	{
		int32 X = 0;
		int32 Y = 0;
		FS_InventoryItem OldItem = Container.Items[CurrentItem.ItemIndex];
		UFL_InventoryFramework::IndexToTile(OldItem.TileIndex, Container, X, Y);
		if(Adjustments.Left > 0)
		{
			X += Adjustments.Left;
		}
		if(Adjustments.Top > 0)
		{
			Y += Adjustments.Top;
		}
		int32 NewTile = UFL_InventoryFramework::TileToIndex(X, Y, ContainerRef);
		CurrentItem.TileIndex = NewTile;
	}

	/**Start shrinking. First we gather a list of items that need to be moved and what tiles will be removed.
	 * Then the tile map is reset, excluding the items that need to be moved.
	 * Then we shrink the container and remove the tiles, then we are ready to move the items.
	 * Shrinking first and then moving the items creates multiple weird scenarios where other functions won't
	 * function correctly, because tile map, item indexes and sizing doesn't make sense to some of the validation.*/
	
	TArray<FS_InventoryItem> ItemsToMove;
	TArray<int32> IndexesToRemove;
	TArray<int32> IndexesToIgnore;

	if(Adjustments.Right < 0)
	{
		FIntPoint Range;
		if(ClampToItems)
		{
			/**Clamping to the right is a bit more complicated. Essentially, the for loops
			 * scan left to right, then go down a row and repeat. But for shrinking the right
			 * side, we need to scan columns top to bottom, then move right to the next column.*/
			
			int32 ColumnsToScan = Adjustments.Right * -1;
			Range.X = 1;
			Range.Y = ContainerRef.Dimensions.Y;
			TArray<int32> TilesPendingApproval;
			int32 StartingIndex = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.X + Adjustments.Right, 1, ContainerRef.Dimensions.X);
			//Scan over each column
			
			for(int32 CurrentColumn = 0; CurrentColumn < ColumnsToScan; CurrentColumn++)
			{
				bool ColumnDirty = false;
				TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, StartingIndex + CurrentColumn);
				for(auto& CurrentIndex : Tiles)
				{
					FIntPoint CurrentTile;
					UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, CurrentTile.X, CurrentTile.Y);

					//Fresh column. Reset.
					if(CurrentTile.Y == 0)
					{
						TilesPendingApproval.Empty();
						ColumnDirty = false;
					}

					if(!ColumnDirty)
					{
						if(ContainerRef.TileMap[CurrentIndex] != -1)
						{
							FS_UniqueID ItemID;
							ItemID.ParentComponent = Container.UniqueID.ParentComponent;
							ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
							FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
							if(FoundItem.IsValid())
							{
								//An item was found in this row, label it dirty and disregard all the tiles.
								TilesPendingApproval.Empty();
								ColumnDirty = true;
								FIntPoint FoundItemTile;
								UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, FoundItemTile.X, FoundItemTile.Y);
								Adjustments.Right = (ContainerRef.Dimensions.X - 1 - FoundItemTile.X) * -1;
							}
						}

						if(!ColumnDirty)
						{
							TilesPendingApproval.AddUnique(CurrentIndex);
						}
					}
				}
			}
			//Append the approved tiles, but don't have any duplicates.
			for(auto& CurrentTile : TilesPendingApproval)
			{
				IndexesToRemove.AddUnique(CurrentTile);
			}
		}
		
		Range.X = Adjustments.Right * -1;
		Range.Y = ContainerRef.Dimensions.Y;
		int32 StartingIndex = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.X + Adjustments.Right, 1, ContainerRef.Dimensions.X);
		TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, StartingIndex);
		
		for(int32 CurrentIndex : Tiles)
		{
			if(ContainerRef.TileMap[CurrentIndex] != -1)
			{
				FS_UniqueID ItemID;
				ItemID.ParentComponent = Container.UniqueID.ParentComponent;
				ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
				FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
				if(FoundItem.IsValid())
				{
					ItemsToMove.AddUnique(FoundItem);
				}
			}
			IndexesToRemove.AddUnique(CurrentIndex);
		}
	}

	if(Adjustments.Bottom < 0)
	{
		int32 MinRange = UFL_InventoryFramework::TileToIndex(0, ContainerRef.Dimensions.Y + Adjustments.Bottom, ContainerRef);
		bool RowDirty = false;
		TArray<int32> TilesPendingApproval;
		for(int32 CurrentIndex = MinRange; CurrentIndex < ContainerRef.Dimensions.X * ContainerRef.Dimensions.Y; CurrentIndex++)
		{
			int32 X;
			int32 Y;
			UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef) && ContainerRef.TileMap.IsValidIndex(CurrentIndex))
			{
				if(Y != 0)
				{
					if(ClampToItems)
					{
						FIntPoint CurrentTile;
						UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, CurrentTile.X, CurrentTile.Y);

						//Fresh row. Reset.
						if(CurrentTile.X == 0)
						{
							TilesPendingApproval.Empty();
							RowDirty = false;
						}

						if(!RowDirty)
						{
							if(ContainerRef.TileMap[CurrentIndex] != -1)
							{
								FS_UniqueID ItemID;
								ItemID.ParentComponent = Container.UniqueID.ParentComponent;
								ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
								FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
								if(FoundItem.IsValid())
								{
									//An item was found in this row, label it dirty and disregard all the tiles.
									TilesPendingApproval.Empty();
									RowDirty = true;
									FIntPoint FoundItemTile;
									UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, FoundItemTile.X, FoundItemTile.Y);
									Adjustments.Bottom = (ContainerRef.Dimensions.Y - 1 - FoundItemTile.Y) * -1;
								}
							}

							if(!RowDirty)
							{
								TilesPendingApproval.AddUnique(CurrentIndex);
							}
						}
					}
					else
					{
						if(ContainerRef.TileMap[CurrentIndex] != -1)
						{
							FS_UniqueID ItemID;
							ItemID.ParentComponent = Container.UniqueID.ParentComponent;
							ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
							FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
							if(FoundItem.IsValid())
							{
								ItemsToMove.AddUnique(FoundItem);
							}
						}
						IndexesToRemove.AddUnique(CurrentIndex);
					}
				}
			}
		}
		if(ClampToItems)
		{
			//Append the approved tiles, but don't have any duplicates.
			for(auto& CurrentTile : TilesPendingApproval)
			{
				IndexesToRemove.AddUnique(CurrentTile);
			}
		}
	}
	
	if(Adjustments.Left < 0 && ContainerRef.Dimensions.X - 1 > 0)
	{
		FIntPoint Range;
		if(ClampToItems)
		{
			/**Clamping to the left is a bit more complicated. Essentially, the for loops
			 * scan left to right, then go down a row and repeat. But for shrinking the left
			 * side, we need to scan columns top to bottom, then move right to the next column.*/
			
			int32 ColumnsToScan = Adjustments.Left * -1;
			Range.X = 1;
			Range.Y = ContainerRef.Dimensions.Y;
			TArray<int32> TilesPendingApproval;
			int32 StartingIndex = 0;
			//Scan over each column
			
			for(int32 CurrentColumn = 0; CurrentColumn < ColumnsToScan; CurrentColumn++)
			{
				bool ColumnDirty = false;
				TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, StartingIndex + CurrentColumn);
				for(auto& CurrentIndex : Tiles)
				{
					FIntPoint CurrentTile;
					UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, CurrentTile.X, CurrentTile.Y);

					//Fresh column. Reset.
					if(CurrentTile.Y == 0)
					{
						TilesPendingApproval.Empty();
						ColumnDirty = false;
					}

					if(!ColumnDirty)
					{
						if(ContainerRef.TileMap[CurrentIndex] != -1)
						{
							FS_UniqueID ItemID;
							ItemID.ParentComponent = Container.UniqueID.ParentComponent;
							ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
							FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
							if(FoundItem.IsValid())
							{
								//An item was found in this row, label it dirty and disregard all the tiles.
								TilesPendingApproval.Empty();
								ColumnDirty = true;
								FIntPoint FoundItemTile;
								UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, FoundItemTile.X, FoundItemTile.Y);
								Adjustments.Left = FoundItemTile.X * -1;
								break;
							}
						}

						if(!ColumnDirty)
						{
							TilesPendingApproval.AddUnique(CurrentIndex);
						}
					}
				}
				//Since we are scanning from the left, we can end
				//the loop if a colliding item was found.
				if(ColumnDirty)
				{
					break;
				}
			}
			if(ClampToItems)
			{
				//Append the approved tiles, but don't have any duplicates.
				for(auto& CurrentTile : TilesPendingApproval)
				{
					IndexesToRemove.AddUnique(CurrentTile);
				}
			}
		}
		
		Range.X = UKismetMathLibrary::Clamp(Adjustments.Left * -1 - 1, 0, ContainerRef.Dimensions.X - 1);
		Range.Y = ContainerRef.Dimensions.Y;
		TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, 0);
		for(int32 CurrentIndex : Tiles)
		{
			if(ContainerRef.TileMap[CurrentIndex] != -1)
			{
				FS_UniqueID ItemID;
				ItemID.ParentComponent = Container.UniqueID.ParentComponent;
				ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
				FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
				if(FoundItem.IsValid())
				{
					ItemsToMove.AddUnique(FoundItem);
					TArray<int32> ItemsIndexes;
					bool InvalidTileFound;
					GetItemsTileIndexes(FoundItem, ItemsIndexes, InvalidTileFound);
					IndexesToIgnore.Append(ItemsIndexes);
				}
			}
			IndexesToRemove.AddUnique(CurrentIndex);
		}
	}

	if(Adjustments.Top < 0 && ContainerRef.Dimensions.Y - 1 > 0)
	{
		FIntPoint Range;
		if(ClampToItems)
		{
			bool RowDirty = false;
			TArray<int32> TilesPendingApproval;
			Range.X = ContainerRef.Dimensions.X;
			Range.Y = UKismetMathLibrary::Clamp(Adjustments.Top * -1, 0, ContainerRef.Dimensions.Y - 1);
			TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, 0);
		
			for(auto& CurrentIndex : Tiles)
			{
				FIntPoint CurrentTile;
				UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, CurrentTile.X, CurrentTile.Y);

				//Fresh row. Reset.
				if(CurrentTile.X == 0)
				{
					TilesPendingApproval.Empty();
					RowDirty = false;
				}

				if(!RowDirty)
				{
					if(ContainerRef.TileMap[CurrentIndex] != -1)
					{
						FS_UniqueID ItemID;
						ItemID.ParentComponent = Container.UniqueID.ParentComponent;
						ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
						FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
						if(FoundItem.IsValid())
						{
							//An item was found in this row, label it dirty and disregard all the tiles.
							TilesPendingApproval.Empty();
							RowDirty = true;
							FIntPoint FoundItemTile;
							UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, FoundItemTile.X, FoundItemTile.Y);
							Adjustments.Top = FoundItemTile.Y * -1;
							break;
						}
					}

					if(!RowDirty)
					{
						TilesPendingApproval.AddUnique(CurrentIndex);
					}
				}
			}
			//Append the approved tiles, but don't have any duplicates.
			for(auto& CurrentTile : TilesPendingApproval)
			{
				IndexesToRemove.AddUnique(CurrentTile);
			}
		}
		else
		{
			Range.X = ContainerRef.Dimensions.X;
			Range.Y = UKismetMathLibrary::Clamp(Adjustments.Top * -1, 0, ContainerRef.Dimensions.Y - 1);
			TArray<int32> Tiles = UFL_InventoryFramework::GetTilesWithinDimension(Range, ContainerRef, 0);
		
			for(auto& CurrentIndex : Tiles)
			{
				int32 X;
				int32 Y;
				UFL_InventoryFramework::IndexToTile(CurrentIndex, ContainerRef, X, Y);
			
				if(UFL_InventoryFramework::IsTileValid(X, Y, ContainerRef))
				{
					if(ContainerRef.TileMap[CurrentIndex] != -1)
					{
						FS_UniqueID ItemID;
						ItemID.ParentComponent = Container.UniqueID.ParentComponent;
						ItemID.IdentityNumber = ContainerRef.TileMap[CurrentIndex];
						FS_InventoryItem FoundItem = GetItemByUniqueID(ItemID);
						if(FoundItem.IsValid())
						{
							ItemsToMove.AddUnique(FoundItem);
							TArray<int32> ItemsIndexes;
							bool InvalidTileFound;
							GetItemsTileIndexes(FoundItem, ItemsIndexes, InvalidTileFound);
							IndexesToIgnore.Append(ItemsIndexes);
						}
					}
					IndexesToRemove.AddUnique(CurrentIndex);
				}
			}
		}
	}
	
	//Finally shrink the dimensions
	if(IndexesToRemove.IsValidIndex(0))
	{
		if(Adjustments.Right < 0)
		{
			Adjustments.Right = UKismetMathLibrary::FTrunc(Adjustments.Right);
			ContainerRef.Dimensions.X = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.X + Adjustments.Right, 1, ContainerRef.Dimensions.X);
		}

		if(Adjustments.Bottom < 0)
		{
			Adjustments.Bottom = UKismetMathLibrary::FTrunc(Adjustments.Bottom);
			ContainerRef.Dimensions.Y = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.Y + Adjustments.Bottom, 1, ContainerRef.Dimensions.Y);
		}

		if(Adjustments.Left < 0)
		{
			Adjustments.Left = UKismetMathLibrary::FTrunc(Adjustments.Left);
			ContainerRef.Dimensions.X = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.X + Adjustments.Left, 1, ContainerRef.Dimensions.X);
		}

		if(Adjustments.Top < 0)
		{
			Adjustments.Top = UKismetMathLibrary::FTrunc(Adjustments.Top);
			ContainerRef.Dimensions.Y = UKismetMathLibrary::Clamp(ContainerRef.Dimensions.Y + Adjustments.Top, 1, ContainerRef.Dimensions.Y);
		}
	}

	//Reset the tile map before moving the items
	InitializeTileMap(ContainerRef);
	
	//Update items positions.
	for(auto& CurrentItem : ContainerRef.Items)
	{
		//Don't bother to update the position of items that need to be moved.
		if(!ItemsToMove.Contains(CurrentItem))
		{
			int32 X = 0;
			int32 Y = 0;
			FS_InventoryItem OldItem = Container.Items[CurrentItem.ItemIndex];
			UFL_InventoryFramework::IndexToTile(OldItem.TileIndex, Container, X, Y);
			if(Adjustments.Left < 0 && ContainerRef.Dimensions.X - 1 > 0)
			{
				if(Y == 0)
				{
					CurrentItem.TileIndex += Adjustments.Left;
				}
				else
				{
					CurrentItem.TileIndex += (Y + 1) * Adjustments.Left;
				}
			}
			if(Adjustments.Right < 0)
			{
				
				CurrentItem.TileIndex += Y * Adjustments.Right;
			}
			if(Adjustments.Top < 0)
			{
				CurrentItem.TileIndex -= ContainerRef.Dimensions.X;
			}
			AddItemToTileMap(CurrentItem);
		}
	}
	//Move the items that were inside tiles we are removing.
	if(ItemsToMove.IsValidIndex(0))
	{
		/** The default behaviour for how items that need to be moved is as follows:
		 * 1. Attempt to stack the item with other items inside the same container.
		 * 2. If any stack count is left, try to move the remaining to a new tile
		 * inside the same container.
		 * 3. If no tile was found, try to stack with all stackable items inside
		 * any container (Excluding the original container since we have already
		 * attempted to stack with items in there).
		 * 4. If it has been stacked as much as possible and any count is left, find
		 * the first available tile and container.
		 * 5. If no available container and no tile was found, the item is dropped.*/
		for(auto& CurrentItem : ItemsToMove)
		{
			//Start by searching the items current container as it makes the most sense to try and keep items inside their original containers.
			TArray<int32> CurrentContainerIndexesToIgnore = GetGenericIndexesToIgnore(ContainerRef);

			//MoveItem calls RefreshIndexes, which will update the items ItemIndex in this container.
			//Since ItemsToMove did not get updated, it still has the old ItemIndex and the validation
			//will fail for MoveItem.
			//We find the updated item with the UniqueID and move that.
			FS_InventoryItem ItemRef;
			for(auto& ValidatedItem : ContainerRef.Items)
			{
				if(ValidatedItem.UniqueID == CurrentItem.UniqueID)
				{
					ItemRef = ValidatedItem;
					break;
				}
			}

			int32 ItemRefRemainingCount = 0;
			int32 Item2NewCount = 0;

			//1. Attempt to stack the item with other items inside the same container.
			if(ItemRef.ItemAsset->CanItemStack())
			{
				TArray<FS_InventoryItem> StackableItems;
				int32 TotalAmountFound;
				GetAllItemsWithDataAsset(ItemRef.ItemAsset, ItemRef.ContainerIndex, StackableItems, TotalAmountFound);
				{
					if(StackableItems.IsValidIndex(0))
					{
						for(auto& StackingItem : StackableItems)
						{
							if(UFL_InventoryFramework::CanStackItems(ItemRef, StackingItem))
							{
								Internal_StackTwoItems(ItemRef, StackingItem, ItemRefRemainingCount, Item2NewCount);
								ItemRef.Count = ItemRefRemainingCount;
								if(ItemRefRemainingCount <= 0)
								{
									break;
								}
							}
						}
					}
				}
			}

			if(ItemRef.Count <= 0)
			{
				continue;
			}

			//2. If any stack count is left, try to move the remaining to a new tile
			//inside the same container.
			bool bSpotFound = false;
			int32 AvailableTile = -1;
			TEnumAsByte<ERotation> bNeededRotation = ItemRef.Rotation;
			int32 ContainerDestination = -1;
			ParentComponent->GetFirstAvailableTile(ItemRef, ContainerRef, CurrentContainerIndexesToIgnore, bSpotFound, AvailableTile, bNeededRotation);
			
			if(bSpotFound)
			{
				ContainerDestination = ItemRef.ContainerIndex;
			}
			else
			{
				/**3. If no tile was found, try to stack with all stackable items inside
				 * any container (Excluding the original container since we have already
				 * attempted to stack with items in there).*/
				if(ItemRef.ItemAsset->CanItemStack())
				{
					TArray<FS_InventoryItem> StackableItems;
					for(auto& CurrentContainer : ParentComponent->ContainerSettings)
					{
						if(CurrentContainer.ContainerIndex != ItemRef.ContainerIndex)
						{
							int32 TotalAmountFound;
							GetAllItemsWithDataAsset(ItemRef.ItemAsset, CurrentContainer.ContainerIndex, StackableItems, TotalAmountFound);
							{
								if(StackableItems.IsValidIndex(0))
								{
									for(auto& StackingItem : StackableItems)
									{
										if(UFL_InventoryFramework::CanStackItems(ItemRef, StackingItem))
										{
											TArray<FS_InventoryItem> TestArray = StackableItems;
											FS_InventoryItem TestItem = StackingItem;
											Internal_StackTwoItems(ItemRef, StackingItem, ItemRefRemainingCount, Item2NewCount);
											ItemRef.Count = ItemRefRemainingCount;
											StackingItem.Count = Item2NewCount;
											if(ItemRefRemainingCount <= 0)
											{
												break;
											}
										}
									}
									if(ItemRefRemainingCount <= 0)
									{
										break;
									}
								}
							}
						}
					}
					if(ItemRef.Count <= 0)
					{
						continue;
					}
				}

				/**4. If it has been stacked as much as possible and any count is left, find
				 * the first available tile and container.*/
				TArray<int32> ContainersToIgnore;
				ContainersToIgnore.Add(CurrentItem.ContainerIndex); //Already checked this container above
				FS_ContainerSettings AvailableContainer;
				ParentComponent->GetFirstAvailableContainerAndTile(CurrentItem, ContainersToIgnore, bSpotFound, AvailableContainer,AvailableTile, bNeededRotation);
				if(bSpotFound)
				{
					ContainerDestination = AvailableContainer.ContainerIndex;
				}
			}
			
			if(bSpotFound)
			{
				TArray<FS_ContainerSettings> ItemsContainers = GetItemsChildrenContainers(ItemRef);
				Internal_MoveItem(ItemRef, ParentComponent, ParentComponent, ContainerDestination, AvailableTile, ItemRef.Count, true, false, true, bNeededRotation, ItemsContainers, Seed);
			}
			else
			{
				/**5. If no available container and no tile was found, the item is dropped.
				 * If it can't be dropped, it's fully deleted. You realistically never want this
				 * to happen. Ideally, you'd have some kind of mailing system or something to
				 * handle this specific scenario.*/
				if(CanItemBeDropped(ItemRef))
				{
					if(UKismetSystemLibrary::IsServer(this) || UKismetSystemLibrary::IsStandalone(this))
					{
						ParentComponent->Internal_DropItem(ItemRef);
					}
					else
					{
						//Remove the widget instantly so it "appears" as if the item as been removed instantly.
						ItemRef.Widget->RemoveFromParent();
					}
				}
				else
				{
					bool Success;
					ParentComponent->Internal_RemoveItemFromInventory(ItemRef, true, true, true, true, true, Seed, Success);
				}
			}
		}
	}
	
	if(ContainerWidget)
	{
		ContainerWidget->ConstructContainers(ContainerRef, ContainerRef.UniqueID.ParentComponent, true);
	}
	
	ContainerSizeAdjusted.Broadcast(Container, Adjustments);
	return true;
}

bool UAC_Inventory::AddTagToContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container not valid - AC_Inventory -> SetTagValueForContainer"), true, true);
		return false;
	}
	
	const bool Success = !UFL_InventoryFramework::GetContainersTags(Container).HasTagExact(Tag);
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagBeAddedToContainer(Tag, Container))
		{
			Internal_AddTagToContainer(Container, Tag);
			return Success;
		}
		return false;
	}
	
	S_AddTagToContainer(Container.UniqueID, Tag, GetOwner()->GetLocalRole());
	return Success;
}

void UAC_Inventory::S_AddTagToContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!OriginalContainer.IsValid())
	{
		return;
	}

	if(!CanTagBeAddedToContainer(Tag, OriginalContainer))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_AddTagToContainer(OriginalContainer, Tag);
			}
			else
			{
				Internal_AddTagToContainer(OriginalContainer, Tag);
				C_AddTagToContainer(ContainerID, Tag);
			}
		}
		else
		{
			Internal_AddTagToContainer(OriginalContainer, Tag);
			C_AddTagToContainer(ContainerID, Tag);
		}
	}
	else
	{
		Internal_AddTagToContainer(OriginalContainer, Tag);
		C_AddTagToContainer(ContainerID, Tag);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_AddTagToContainer(ContainerID, Tag);
		}
	}
}

bool UAC_Inventory::S_AddTagToContainer_Validate(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::C_AddTagToContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	
	if(OriginalContainer.IsValid())
	{
		Internal_AddTagToContainer(OriginalContainer, Tag);
	}
}

void UAC_Inventory::Internal_AddTagToContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		return;
	}

	if(FTagFragment* TagFragment = FindFragment<FTagFragment>(Container.UniqueID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments, true))
	{
		TagFragment->Tags.AddTag(Tag);
		UFL_ExternalObjects::BroadcastTagsUpdated(Tag, true, FS_InventoryItem(), Container);
	}
}

bool UAC_Inventory::CanTagBeAddedToContainer_Implementation(FGameplayTag Tag, FS_ContainerSettings Container)
{
	return true;
}

bool UAC_Inventory::RemoveTagFromContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container not valid - AC_Inventory -> SetTagValueForContainer"), true, true);
		return false;
	}
	
	const bool Success = UFL_InventoryFramework::GetContainersTags(Container).HasTagExact(Tag);
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagBeRemovedFromContainer(Tag, Container))
		{
			Internal_RemoveTagFromContainer(Container, Tag);
			return Success;
		}
		return false;
	}
	
	S_RemoveTagFromContainer(Container.UniqueID, Tag, GetOwner()->GetLocalRole());
	return Success;
}


void UAC_Inventory::S_RemoveTagFromContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);

	if(!OriginalContainer.IsValid())
	{
		return;
	}

	if(!CanTagBeRemovedFromContainer(Tag, OriginalContainer))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveTagFromContainer(OriginalContainer, Tag);
			}
			else
			{
				Internal_RemoveTagFromContainer(OriginalContainer, Tag);
				C_RemoveTagFromContainer(ContainerID, Tag);
			}
		}
		else
		{
			Internal_RemoveTagFromContainer(OriginalContainer, Tag);
			C_RemoveTagFromContainer(ContainerID, Tag);
		}
	}
	else
	{
		Internal_RemoveTagFromContainer(OriginalContainer, Tag);
		C_RemoveTagFromContainer(ContainerID, Tag);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveTagFromContainer(ContainerID, Tag);
		}
	}
}

bool UAC_Inventory::S_RemoveTagFromContainer_Validate(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::C_RemoveTagFromContainer_Implementation(FS_UniqueID ItemID, FGameplayTag Tag)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings Container = ItemID.ParentComponent->GetContainerByUniqueID(ItemID);
	if(Container.IsValid())
	{
		Internal_RemoveTagFromContainer(Container, Tag);
	}
}

void UAC_Inventory::Internal_RemoveTagFromContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		return;
	}

	if(FTagFragment* TagFragment = FindFragment<FTagFragment>(Container.UniqueID.ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments, false))
	{
		TagFragment->Tags.RemoveTag(Tag);
		UFL_ExternalObjects::BroadcastTagsUpdated(Tag, false, FS_InventoryItem(), Container);
	}
}

bool UAC_Inventory::CanTagBeRemovedFromContainer_Implementation(FGameplayTag Tag, FS_ContainerSettings Container)
{
	return true;
}

bool UAC_Inventory::SetTagValueForContainer(FS_ContainerSettings Container, FGameplayTag Tag, float Value, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool AddIfNotFound)
{
	if(!UFL_InventoryFramework::IsContainerValid(Container))
	{
		UFL_InventoryFramework::LogIFPMessage(this, TEXT("Container not valid - AC_Inventory -> SetTagValueForContainer"), true, true);
		return false;
	}

	FS_TagValue TagValue;
	TagValue.Tag = Tag;
	TagValue.Value = Value;
	
	bool Success = false;
	if(!AddIfNotFound)
	{
		FS_TagValue FoundTagValues;
		int32 TagIndex;
		if(!UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetContainersTagValues(Container), Tag, FoundTagValues, TagIndex))
		{
			//We don't want to add the tag, and the tag value can not be found. Abort.
			return false;
		}
		else
		{
			if(!CanTagValueBeSetForContainer(TagValue, Container))
			{
				//Container has tag, but we aren't allowed to set it.
				return false;
			}
		}
	}
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagValueBeSetForContainer(TagValue, Container))
		{
			Internal_SetTagValueForContainer(Container, Tag, Value, AddIfNotFound, CalculationClass, Success);
			return Success;
		}
		return false;
	}
	
	S_SetTagValueForContainer(Container.UniqueID, Tag, Value, GetOwner()->GetLocalRole(), CalculationClass, AddIfNotFound);
	return Success;
}

void UAC_Inventory::S_SetTagValueForContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool AddIfNotFound)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!OriginalContainer.IsValid())
	{
		return;
	}

	FS_TagValue TagValue;
	TagValue.Tag = Tag;
	TagValue.Value = Value;
	
	if(!AddIfNotFound)
	{
		FS_TagValue FoundTagValues;
		int32 TagIndex;
		if(!UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetContainersTagValues(OriginalContainer), Tag, FoundTagValues, TagIndex))
		{
			//We don't want to add the tag, and the tag value can not be found. Abort.
			return;
		}
	}

	if(!CanTagValueBeSetForContainer(TagValue, OriginalContainer))
	{
		return;
	}
	
	//Run through calculation class
	if(IsValid(CalculationClass))
	{
		Value = PreContainerTagValueCalculation(TagValue, OriginalContainer, CalculationClass);
		UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass.GetDefaultObject());
		if(IsValid(Calculator))
		{
			//Find out if the new value has hit some sort of threshold or limit.
			if(Calculator->ShouldValueBeRemovedFromContainer(TagValue, OriginalContainer))
			{
				//New value has hit some sort of defined limit and should be removed,
				//cancel this function and start removing the item.
				S_RemoveTagValueFromContainer(ContainerID, Tag, CallerLocalRole);
				return;
			}
		}
	}	

	bool Success;

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_SetTagValueForContainer(OriginalContainer, Tag, Value, AddIfNotFound, CalculationClass, Success);
			}
			else
			{
				Internal_SetTagValueForContainer(OriginalContainer, Tag, Value, AddIfNotFound, CalculationClass, Success);
				C_SetTagValueForContainer(ContainerID, Tag, Value, AddIfNotFound);
			}
		}
		else
		{
			Internal_SetTagValueForContainer(OriginalContainer, Tag, Value, AddIfNotFound, CalculationClass, Success);
			C_SetTagValueForContainer(ContainerID, Tag, Value, AddIfNotFound);
		}
	}
	else
	{
		Internal_SetTagValueForContainer(OriginalContainer, Tag, Value, AddIfNotFound, CalculationClass, Success);
		C_SetTagValueForContainer(ContainerID, Tag, Value, AddIfNotFound);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_SetTagValueForContainer(ContainerID, Tag, Value, AddIfNotFound);
		}
	}
}

bool UAC_Inventory::S_SetTagValueForContainer_Validate(FS_UniqueID ContainerID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool AddIfNotFound)
{
	return true;
}

void UAC_Inventory::C_SetTagValueForContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag, float Value, bool AddIfNotFound)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(OriginalContainer.IsValid())
	{
		bool Success;
		//Server has already done the calculation, pass in nullptr as it is not needed for clients.
		Internal_SetTagValueForContainer(OriginalContainer, Tag, Value, AddIfNotFound, nullptr, Success);
	}
}

void UAC_Inventory::Internal_SetTagValueForContainer(FS_ContainerSettings Container, FGameplayTag Tag, float Value, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool& Success)
{
	UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
	Success = false;
	
	if(!IsValid(ParentComponent))
	{
		return;
	}

	FS_TagValue NewTagValue;
	NewTagValue.Tag = Tag;
	NewTagValue.Value = Value;

	//Only run through the calculation class if we are playing standalone.
	//If this is multiplayer, the server has already calculated the value.
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(IsValid(CalculationClass))
		{
			Value = PreContainerTagValueCalculation(NewTagValue, Container, CalculationClass);
			NewTagValue.Value = Value;
			if(UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass.GetDefaultObject()); IsValid(Calculator))
			{
				//Find out if the new value has hit some sort of threshold or limit.
				if(Calculator->ShouldValueBeRemovedFromContainer(NewTagValue, Container))
				{
					//New value has hit some sort of defined limit and should be removed,
					//cancel this function and start removing the item.
					Internal_RemoveTagValueFromContainer(Container, Tag);
					return;
				}
			}
		}
	}

	FS_TagValue FoundTagValue;
	int32 TagIndex;
	if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments, AddIfNotFound))
	{
		if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagFragment->TagValues, Tag, FoundTagValue, TagIndex))
		{
			TagFragment->TagValues[TagIndex].Value = Value;
			ParentComponent->ContainerTagValueUpdated.Broadcast(Container, NewTagValue, NewTagValue.Value - FoundTagValue.Value);
			Success = true;
		}
		else
		{
			if(AddIfNotFound)
			{
				TagFragment->TagValues.AddUnique(NewTagValue);
				ParentComponent->ContainerTagValueUpdated.Broadcast(Container, NewTagValue, NewTagValue.Value);
				Success = true;
			}
		}

		UFL_ExternalObjects::BroadcastTagValueUpdated(NewTagValue, true, NewTagValue.Value - FoundTagValue.Value, FS_InventoryItem(), Container);
	}
}

float UAC_Inventory::PreContainerTagValueCalculation(FS_TagValue TagValue, FS_ContainerSettings Container,
	TSubclassOf<UO_TagValueCalculation> CalculationClass)
{
	if(!IsValid(CalculationClass))
	{
		return TagValue.Value;
	}
	
	UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass->GetDefaultObject());
	return Calculator->CalculateContainerTagValue(TagValue, Container);
}

bool UAC_Inventory::CanTagValueBeSetForContainer_Implementation(FS_TagValue TagValue, FS_ContainerSettings Container)
{
	return true;
}

bool UAC_Inventory::RemoveTagValueFromContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	FS_TagValue FoundTagValue;
	int32 TagIndex;
	const bool Success = UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetContainersTagValues(Container), Tag, FoundTagValue, TagIndex);
	
	if(UKismetSystemLibrary::IsStandalone(this))
	{
		if(CanTagValueBeRemovedFromContainer(Tag, Container))
		{
			Internal_RemoveTagValueFromContainer(Container, Tag);
			return Success;
		}
		return false;
	}
	
	S_RemoveTagValueFromContainer(Container.UniqueID, Tag, GetOwner()->GetLocalRole());
	return Success;
}


void UAC_Inventory::S_RemoveTagValueFromContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(!OriginalContainer.IsValid())
	{
		return;
	}

	if(!CanTagValueBeRemovedFromContainer(Tag, OriginalContainer))
	{
		return;
	}

	if(CallerLocalRole == ROLE_Authority)
	{
		if(IsValid(GetOwner()->GetInstigatorController()))
		{
			if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
			{
				Internal_RemoveTagValueFromContainer(OriginalContainer, Tag);
			}
			else
			{
				Internal_RemoveTagValueFromContainer(OriginalContainer, Tag);
				C_RemoveTagValueFromContainer(ContainerID, Tag);
			}
		}
		else
		{
			Internal_RemoveTagValueFromContainer(OriginalContainer, Tag);
			C_RemoveTagValueFromContainer(ContainerID, Tag);
		}
	}
	else
	{
		Internal_RemoveTagValueFromContainer(OriginalContainer, Tag);
		C_RemoveTagValueFromContainer(ContainerID, Tag);
	}
	
	for(const auto& CurrentListener : ContainerID.ParentComponent->Listeners)
	{
		//Update all clients that are currently listening to this component's replication calls.
		if(CurrentListener->GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy && CurrentListener != this)
		{
			CurrentListener->C_RemoveTagValueFromContainer(ContainerID, Tag);
		}
	}
}

bool UAC_Inventory::S_RemoveTagValueFromContainer_Validate(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole)
{
	return true;
}

void UAC_Inventory::C_RemoveTagValueFromContainer_Implementation(FS_UniqueID ContainerID, FGameplayTag Tag)
{
	if(UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	FS_ContainerSettings OriginalContainer = ContainerID.ParentComponent->GetContainerByUniqueID(ContainerID);
	if(OriginalContainer.IsValid())
	{
		Internal_RemoveTagValueFromContainer(OriginalContainer, Tag);
	}
}

void UAC_Inventory::Internal_RemoveTagValueFromContainer(FS_ContainerSettings Container, FGameplayTag Tag)
{
	UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
	
	if(!IsValid(ParentComponent))
	{
		return;
	}

	FS_TagValue FoundTagValue;
	int32 TagIndex;
	if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ParentComponent->ContainerSettings[Container.ContainerIndex].ContainerFragments))
	{
		if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagFragment->TagValues, Tag, FoundTagValue, TagIndex))
		{
			TagFragment->TagValues.RemoveAt(TagIndex);
			ParentComponent->ContainerTagValueUpdated.Broadcast(Container, FoundTagValue, FoundTagValue.Value * -1);
			UFL_ExternalObjects::BroadcastTagValueUpdated(FoundTagValue, true, FoundTagValue.Value * -1, FS_InventoryItem(), Container);
		}
	}
}

bool UAC_Inventory::CanTagValueBeRemovedFromContainer_Implementation(FGameplayTag Tag,
	FS_ContainerSettings Container)
{
	return true;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainersByTag(FGameplayTag Tag)
{
	TArray<FS_ContainerSettings> FoundContainers;
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(UFL_InventoryFramework::GetContainersTags(CurrentContainer).HasTagExact(Tag))
		{
			FoundContainers.Add(CurrentContainer);
		}
	}

	return FoundContainers;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainersByTagValue(FGameplayTag Tag)
{
	TArray<FS_ContainerSettings> FoundContainers;
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		FS_TagValue FoundTagValue;
		int32 TagIndex;
		if(UFL_InventoryFramework::DoesTagValuesHaveTag(UFL_InventoryFramework::GetContainersTagValues(CurrentContainer), Tag, FoundTagValue, TagIndex))
		{
			FoundContainers.Add(CurrentContainer);
		}
	}

	return FoundContainers;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainersByTagQuery(FGameplayTagQuery TagQuery)
{
	TArray<FS_ContainerSettings> FoundContainers;
	if(TagQuery.IsEmpty())
	{
		return FoundContainers;
	}

	for(auto& CurrentContainer : ContainerSettings)
	{
		if(TagQuery.Matches(UFL_InventoryFramework::GetContainersTags(CurrentContainer)))
		{
			FoundContainers.Add(CurrentContainer);
		}
	}

	return FoundContainers;
}

TArray<FS_ContainerSettings> UAC_Inventory::GetContainersByType(TEnumAsByte<EContainerType> ContainerType)
{
	TArray<FS_ContainerSettings> FoundContainers;

	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.ContainerType == ContainerType)
		{
			FoundContainers.Add(CurrentContainer);
		}
	}
	
	return FoundContainers;
}

FS_UniqueID UAC_Inventory::GenerateUniqueID()
{
	TRACE_CPUPROFILER_EVENT_SCOPE("GenerateUniqueID")
	FS_UniqueID GeneratedUniqueID;
	
	if(!GetOwner()->HasAuthority())
	{
		return GeneratedUniqueID;
	}
	
	const int32 RandomInt = UKismetMathLibrary::RandomIntegerInRange(1, 2147483647);
	
	GeneratedUniqueID.IdentityNumber = RandomInt;
	GeneratedUniqueID.ParentComponent = this;

	if(IsUniqueIDInUse(GeneratedUniqueID))
	{
		GeneratedUniqueID = GenerateUniqueID();
	}

	return GeneratedUniqueID;
}

FS_UniqueID UAC_Inventory::GenerateUniqueIDWithSeed(FRandomStream Seed)
{
	FS_UniqueID GeneratedUniqueID;
	
	const int32 RandomInt = UKismetMathLibrary::RandomIntegerInRangeFromStream(Seed, 1, 2147483647);
	
	GeneratedUniqueID.IdentityNumber = RandomInt;
	GeneratedUniqueID.ParentComponent = this;

	if(IsUniqueIDInUse(GeneratedUniqueID))
	{
		FRandomStream NewSeed;
		NewSeed.Initialize(Seed.GetInitialSeed() + 1);
		GeneratedUniqueID = GenerateUniqueIDWithSeed(NewSeed);
	}
	
	return GeneratedUniqueID;
}

bool UAC_Inventory::IsUniqueIDInUse(FS_UniqueID UniqueID)
{
	if(!UFL_InventoryFramework::IsUniqueIDValid(UniqueID))
	{
		return false;
	}
	
	for(auto& CurrentContainer : ContainerSettings)
	{
		if(CurrentContainer.UniqueID == UniqueID)
		{
			return true;
		}

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			if(CurrentItem.UniqueID == UniqueID)
			{
				return true;
			}
		}
	}

	return false;
}

void UAC_Inventory::AddTagsToComponent(const FGameplayTagContainer Tags, bool Broadcast)
{
	S_AddTagsToComponent(Tags);
}

void UAC_Inventory::S_AddTagsToComponent_Implementation(FGameplayTagContainer Tags, bool Broadcast)
{
	for(auto& CurrentTag : Tags)
	{
		if(!TagsContainer.HasTagExact(CurrentTag))
		{
			TagsContainer.AddTag(CurrentTag);
			if(Broadcast)
			{
				ComponentTagAdded.Broadcast(CurrentTag);
			}
		}
	}

	if(Broadcast)
	{
		MC_TagsAddedToComponent(Tags);
	}
}

void UAC_Inventory::MC_TagsAddedToComponent_Implementation(FGameplayTagContainer Tags)
{
	if(!GetOwner()->HasAuthority())
	{
		for(auto& CurrentTag : Tags)
		{
			ComponentTagAdded.Broadcast(CurrentTag);
		}
	}
}

void UAC_Inventory::RemoveTagsFromComponent(const FGameplayTagContainer Tags, bool Broadcast)
{
	S_RemoveTagsFromComponent(Tags);
}

void UAC_Inventory::SetTagValueForComponent(const FS_TagValue TagValue, bool AddIfNotFound,
                                            const TSubclassOf<UO_TagValueCalculation> CalculationClass, const bool Broadcast)
{
	S_SetTagValueForComponent(TagValue, AddIfNotFound, CalculationClass, Broadcast);
}

float UAC_Inventory::PreComponentTagValueCalculation(FS_TagValue TagValue, UAC_Inventory* Component,
	TSubclassOf<UO_TagValueCalculation> CalculationClass)
{
	if(!IsValid(CalculationClass))
	{
		return TagValue.Value;
	}
	
	UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass->GetDefaultObject());
	return Calculator->CalculateComponentTagValue(TagValue, Component);
}

void UAC_Inventory::S_SetTagValueForComponent_Implementation(FS_TagValue TagValue, bool AddIfNotFound,
                                                             TSubclassOf<UO_TagValueCalculation> CalculationClass, bool Broadcast)
{
	float OldValue = 0;
	FS_TagValue FoundTagValue;
	int32 TagValueIndex;
	if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagValuesContainer, TagValue.Tag, FoundTagValue, TagValueIndex))
	{
		OldValue = FoundTagValue.Value;
		if(IsValid(CalculationClass))
		{
			TagValue.Value = PreComponentTagValueCalculation(TagValue, this, CalculationClass);
			UO_TagValueCalculation* Calculator = Cast<UO_TagValueCalculation>(CalculationClass.GetDefaultObject());
			if(IsValid(Calculator))
			{
				//Find out if the new value has hit some sort of threshold or limit.
				if(Calculator->ShouldValueBeRemovedFromComponent(TagValue, this))
				{
					//New value has hit some sort of defined limit and should be removed,
					//cancel this function and start removing the tag.
					TagValuesContainer.RemoveAt(TagValueIndex);
					MC_TagValueRemovedFromComponent(TagValue);
					return;
				}
			}
		}
	}
	else
	{
		if(AddIfNotFound)
		{
			TagValuesContainer.Add(TagValue);
		}
		else
		{
			return;
		}
	}

	if(Broadcast)
	{
		MC_TagValueUpdatedForComponent(TagValue, OldValue);
	}
}

void UAC_Inventory::MC_TagValueUpdatedForComponent_Implementation(FS_TagValue TagValue, float OldValue)
{
	ComponentTagValueUpdated.Broadcast(TagValue, OldValue);
}

void UAC_Inventory::RemoveTagValueFromComponent(const FGameplayTag TagValue, bool Broadcast)
{
	S_RemoveTagValueFromComponent(TagValue, Broadcast);
}

void UAC_Inventory::S_RemoveTagValueFromComponent_Implementation(FGameplayTag TagValue, bool Broadcast)
{
	FS_TagValue FoundTagValue;
	int32 TagValueIndex;
	if(UFL_InventoryFramework::DoesTagValuesHaveTag(TagValuesContainer, TagValue, FoundTagValue, TagValueIndex))
	{
		TagValuesContainer.RemoveAt(TagValueIndex);
		if(Broadcast)
		{
			MC_TagValueRemovedFromComponent(FoundTagValue);
		}
	}
}

void UAC_Inventory::MC_TagValueRemovedFromComponent_Implementation(FS_TagValue TagValue)
{
	ComponentTagValueRemoved.Broadcast(TagValue);
}

void UAC_Inventory::C_SetClientReceivedContainerData_Implementation(bool HasReceived)
{
	ClientReceivedContainerData = HasReceived;
}

void UAC_Inventory::S_RemoveTagsFromComponent_Implementation(FGameplayTagContainer Tags, bool Broadcast)
{
	//Append the two tag containers and call the TagsModified delegate on the way. - V
	for(auto& CurrentTag : Tags)
	{
		if(TagsContainer.HasTagExact(CurrentTag))
		{
			TagsContainer.RemoveTag(CurrentTag);
			if(Broadcast)
			{
				ComponentTagRemoved.Broadcast(CurrentTag);
			}
		}
	}

	if(Broadcast)
	{
		MC_TagsRemovedFromComponent(Tags);
	}
}

void UAC_Inventory::MC_TagsRemovedFromComponent_Implementation(FGameplayTagContainer Tags)
{
	if(!GetOwner()->HasAuthority())
	{
		for(auto& CurrentTag : Tags)
		{
			ComponentTagRemoved.Broadcast(CurrentTag);
		}
	}
}

void UAC_Inventory::S_AddListener_Implementation(UAC_Inventory* Component)
{
	Listeners.AddUnique(Component);
}

void UAC_Inventory::S_RemoveListener_Implementation(UAC_Inventory* Component)
{
	Listeners.Remove(Component);

	//Client is no longer listening to this component,
	//it's best to assume some data has changed and the
	//player will have to request the container data again.
	C_SetClientReceivedContainerData(false);
}

void UAC_Inventory::C_RequestServerDataFromOtherComponent_Implementation(UAC_Inventory* OtherComponent, bool CallDataReceived)
{
	if(!IsValid(OtherComponent))
	{
		return;
	}
	
	if(IsValid(GetOwner()->GetInstigatorController()))
	{
		if(GetOwner()->GetInstigatorController()->IsLocalPlayerController())
		{
			if(!OtherComponent->ClientReceivedContainerData)
			{
				S_SendDataFromOtherComponent(OtherComponent, CallDataReceived);

				//Both Send and Receive RPC's are reliable, it's safe
				//to assume we'll receive the container data "eventually".
				//This prevents the server RPC from being spammed.
				OtherComponent->ClientReceivedContainerData = true;
			}
			else
			{
				if(CallDataReceived)
				{
					OnDataReceivedFromOtherComponent(OtherComponent);
				}
			}
		}
	}
}

void UAC_Inventory::S_SendDataFromOtherComponent_Implementation(UAC_Inventory* OtherComponent, bool CallDataReceived)
{
	if(!OtherComponent)
	{
		return;
	}
	
	const bool CallComponentStarted = !Initialized;
	
	if(OtherComponent->Initialized)
	{
		OtherComponent->C_RequestServerContainerData(false);
	}
	else
	{
		OtherComponent->StartComponent();
	}
	
	OtherComponent->S_AddListener(this);

	//Wipe the tile map before sending it to the clients.
	//This results in much smaller RPC's (around 20% on average)
	//and generating it takes very little CPU time.
	TArray<FS_ContainerSettings> TempContainers = OtherComponent->ContainerSettings;
	for(auto& CurrentContainer : TempContainers)
	{
		CurrentContainer.TileMap.Empty();
	}
	
	C_ReceiveDataFromOtherComponent(OtherComponent, TempContainers, CallDataReceived, CallComponentStarted);
}

void UAC_Inventory::C_ReceiveDataFromOtherComponent_Implementation(UAC_Inventory* OtherComponent, const TArray<FS_ContainerSettings> &Containers, bool CallDataReceived, bool CallComponentStarted)
{
	OtherComponent->ContainerSettings = Containers;
	
	//Clients receive container settings with no tile map, rebuild them.
	for(auto& CurrentContainer : OtherComponent->ContainerSettings)
	{
		OtherComponent->RebuildTileMap(CurrentContainer);
	}

	OtherComponent->RefreshIDMap();
	
	OtherComponent->Initialized = true;
	
	if(CallComponentStarted)
	{
		OtherComponent->ComponentStarted.Broadcast();
	}
	
	if(CallDataReceived)
	{
		OnDataReceivedFromOtherComponent(OtherComponent);
	}
}

void UAC_Inventory::C_AddItemToNetworkQueue_Implementation(FS_UniqueID ItemID)
{
	if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	
	if(IsValid(ItemID.ParentComponent))
	{
		UAC_Inventory* ParentComponent = ItemID.ParentComponent;
		if(!ParentComponent->NetworkQueue.Contains(ItemID))
		{
			ParentComponent->NetworkQueue.AddUnique(ItemID);
			FS_InventoryItem FoundItem = ParentComponent->GetItemByUniqueID(ItemID);
			if(FoundItem.IsValid())
			{
				UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(FoundItem);
				ParentComponent->ItemAddedToNetworkQueue(FoundItem, ItemWidget);
				if(ItemWidget)
				{
					ItemWidget->ParentItemAddedToNetworkQueue();
				}
			}
		}
	}
}

void UAC_Inventory::C_RemoveItemFromNetworkQueue_Implementation(FS_UniqueID ItemID)
{
	if(UKismetSystemLibrary::IsStandalone(this) || UKismetSystemLibrary::IsServer(this))
	{
		return;
	}
	if(IsValid(ItemID.ParentComponent))
	{
		UAC_Inventory* ParentComponent = ItemID.ParentComponent;
		if(ParentComponent->NetworkQueue.Contains(ItemID))
		{
			ParentComponent->NetworkQueue.Remove(ItemID);
			FS_InventoryItem FoundItem = ParentComponent->GetItemByUniqueID(ItemID);
			if(FoundItem.IsValid())
			{
				UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(FoundItem);
				ParentComponent->ItemRemovedFromNetworkQueue(FoundItem, ItemWidget);
				if(ItemWidget)
				{
					ItemWidget->ParentItemRemovedFromNetworkQueue();
				}
			}
		}
	}
}


void UAC_Inventory::C_AddAllContainerItemsToNetworkQueue_Implementation(FS_UniqueID ContainerID)
{
	FS_ContainerSettings FoundContainer = GetContainerByUniqueID(ContainerID);
	if(!FoundContainer.IsValid() || FoundContainer.Items.IsEmpty())
	{
		return;
	}

	//Add the container to the network queue.
	//The container should not be interactable in any way while
	//it is in the queue.
	ContainerID.ParentComponent->NetworkQueue.AddUnique(ContainerID);
	for(auto& CurrentItem : FoundContainer.Items)
	{
		C_AddItemToNetworkQueue(CurrentItem.UniqueID);
	}
}

void UAC_Inventory::C_RemoveAllContainerItemsFromNetworkQueue_Implementation(FS_UniqueID ContainerID)
{
	FS_ContainerSettings FoundContainer = GetContainerByUniqueID(ContainerID);

	if(!FoundContainer.IsValid() || FoundContainer.Items.IsEmpty())
	{
		return;
	}

	for(auto& CurrentItem : FoundContainer.Items)
	{
		C_RemoveItemFromNetworkQueue(CurrentItem.UniqueID);
	}
}

#if WITH_EDITOR

void UAC_Inventory::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(!PropertyChangedEvent.Property->IsValidLowLevel())
	{
		return;
	}
	
	FString StructName = PropertyChangedEvent.Property->GetOwnerStruct()->GetName();

	UAC_Inventory* TargetComponent = this;
	const FString PropertyName = PropertyChangedEvent.GetPropertyName().ToString();
	if(PropertyName == GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemAssetSoftReference) ||
		StructName == "ItemOverrideSettings" ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FS_ContainerSettings, Style) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemInstance) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FS_ContainerSettings, ContainerType))
	{
		/**Some actors, when in editor, will label themselves as trash.
		* This causes the normal "this" to not be the correct component
		* we want to update. But the outer is the correct actor that is
		* not labeled as trash. We get that actor, go through the components
		* and update that one instead of this one.*/
		if(!IsTemplate())
		{
			AActor* InEditorActor = Cast<AActor>(GetOuter());
			for(auto& CurrentComponent : InEditorActor->GetComponents().Array())
			{
				if(IsValid(CurrentComponent))
				{
					if(CurrentComponent->GetClass() == GetClass())
					{
						TargetComponent = Cast<UAC_Inventory>(CurrentComponent);
						break;
					}
				}
			}
		}
		//Since updating the items is extremely fast, we don't really need to bother with much
		//optimizations here. Just go through each item and update the ItemEditorName
		UFL_InventoryFramework::ProcessContainerAndItemCustomizations(TargetComponent->ContainerSettings);
	}

	if(GetFragmentManager())
	{
		//Forward the event to the fragment manager
		GetFragmentManager()->InventoryPostPropertyChange(PropertyChangedEvent, TargetComponent);
	}
	else
	{
		//No component was found. Forward the event to the CDO.
		Cast<UAC_FragmentManager>(UAC_FragmentManager::StaticClass()->GetDefaultObject())->InventoryPostPropertyChange(PropertyChangedEvent, TargetComponent);
	}
}

void UAC_Inventory::PostLoad()
{
	Super::PostLoad();

	// 3.0 Transition code

	UFL_InventoryFramework::UpdateLegacyContainerSettingsCode(ContainerSettings, this);
	
	// end of transition code
}

void UAC_Inventory::OnComponentCreated()
{
	Super::OnComponentCreated();

	TArray<FString> Messages;
	Messages.Add("Please override the GetInventoryComponent function");
	Messages.Add("You might want to implement BPI_IFP_Helpers if this actor will have a preview made of it or dropped by the user through the DropItem function.");
	UFL_InventoryFramework::ImplementInterface(this, GetOuter(), UI_Inventory::StaticClass(), Messages);
}

#endif

// Called when the game starts
void UAC_Inventory::BeginPlay()
{
	Super::BeginPlay();

	ItemFailedSpawn.AddDynamic(this, &UAC_Inventory::LogItemFailedToSpawn);

	/**Fun fact: If you send a client RPC while the client is still being constructed,
	 * the RPC will execute before BeginPlay.
	 * Because of this, we check if the inventory has been initialized and also ensure
	 * that we don't get this error on clients. Since it's irrelevant for them anyways,
	 * since the server will replace everything for them anyways. */
	if(Initialized && !GetOwner()->HasAuthority())
	{
		return;
	}

	TEnumAsByte<EComponentState> CurrentComponentState = GetComponentState();

	if(CurrentComponentState != Raw)
	{
		if(DebugMessages)
		{
			UFL_InventoryFramework::LogIFPMessage(this, FString::Printf(TEXT("Component state was not set to Raw state - Actor: %s"), *GetOwner()->GetName()), true, true);
		}
		
		ConvertToRawState();
	}
}

void UAC_Inventory::LogItemFailedToSpawn(FS_InventoryItem Item, FString Reason)
{
	UFL_InventoryFramework::LogIFPMessage(this, Reason, true, DebugMessages);
}
