// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "AC_GASHelper.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "IO_AttributeSetReferences.h"
#include "IO_GameplayAbilityReferences.h"
#include "IO_GameplayEffectReferences.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Kismet/KismetSystemLibrary.h"

UE_DEFINE_GAMEPLAY_TAG(GASHELPER_Event_StartComponent, "IFP.Event.GASHelperStarted");
UE_DEFINE_GAMEPLAY_TAG(GASHELPER_Event_ItemEquipped, "IFP.Event.ItemEquipped");
UE_DEFINE_GAMEPLAY_TAG(GASHELPER_Event_ItemUnequipped, "IFP.Event.ItemUnequipped");


UAC_GASHelper::UAC_GASHelper()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAC_GASHelper::StartHelperComponent()
{
	if(!Inventory)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Inventory component not found");
	}

	/**If the inventory is not started, we delay the broadcast until it is started.
	 * This way it does not matter which order you call StartComponent, whether
	 * on this component or the inventory, reducing human error.*/
	if(!Inventory->Initialized)
	{
		Inventory->ComponentStarted.AddDynamic(this, &UAC_GASHelper::OnInventoryStarted);
	}
	else
	{
		TagEventBroadcasted.Broadcast(GASHELPER_Event_StartComponent);
	}
}

UAbilitySystemComponent* UAC_GASHelper::GetAbilityComponent_Implementation()
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void UAC_GASHelper::BindDefaultDelegates_Implementation()
{
	Inventory->ItemEquipped.AddDynamic(this, &UAC_GASHelper::OnItemEquipped);
	Inventory->ItemUnequipped.AddDynamic(this, &UAC_GASHelper::OnItemUnequipped);
}

void UAC_GASHelper::ProcessItem(UDA_CoreItem* Item, FGameplayTagContainer Events)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProcessItem)
	
	if(!Item)
	{
		return;
	}
	
	UIO_GameplayAbilityReferences* Abilities = Cast<UIO_GameplayAbilityReferences>(
			UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayAbilityReferences::StaticClass()));
	if(Abilities)
	{
		GrantItemAbilitiesInternal(Abilities, Events);
		RemoveItemAbilitiesInternal(Abilities, Events);
	}

	UIO_AttributeSetReferences* Attributes = Cast<UIO_AttributeSetReferences>(
		UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_AttributeSetReferences::StaticClass()));
	if(Attributes)
	{
		GrantItemAttributesInternal(Attributes, Events);
		RemoveItemAttributesInternal(Attributes, Events);
	}
		
	UIO_GameplayEffectReferences* Effects = Cast<UIO_GameplayEffectReferences>(
				UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayEffectReferences::StaticClass()));
	if(Effects)
	{
		ApplyItemEffectsInternal(Item, Effects, Events);
		RemoveItemEffectsInternal(Item, Effects, Events);
	}
}

void UAC_GASHelper::GrantItemAbilities(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter)
{
	UIO_GameplayAbilityReferences* Abilities = Cast<UIO_GameplayAbilityReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayAbilityReferences::StaticClass()));
	if(!Abilities)
	{
		return;
	}

	GrantItemAbilitiesInternal(Abilities, GrantEventsFilter);
}

void UAC_GASHelper::GrantItemAbilitiesInternal(UIO_GameplayAbilityReferences* Abilities,
	FGameplayTagContainer GrantEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GrantItemAbilities)
	
	if(!Abilities->HasAnyTagEvents(GrantEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	TArray<FGameplayAbilitySpec> ActiveAbilities = AbilityComponent->GetActivatableAbilities();
	for(auto& CurrentAbility : Abilities->Abilities)
	{
		if(!CurrentAbility.Value.GrantEvents.HasAnyExact(GrantEventsFilter))
		{
			continue;
		}

		/**Find out if we already have the ability and make sure we don't load the ability uneccessarily*/
		bool HasAbility = false;
		for(auto& CurrentActiveAbility : ActiveAbilities)
		{
			TSoftClassPtr<UGameplayAbility> SoftClass = TSoftClassPtr<UGameplayAbility>(FSoftClassPath(CurrentActiveAbility.Ability.GetClass()));
			if(SoftClass == CurrentAbility.Key)
			{
				HasAbility = true;
				break;
			}
		}
		
		if(HasAbility)
		{
			continue;
		}

		//Player does not have ability. Load the class and grant it
		TSubclassOf<UGameplayAbility> Ability = CurrentAbility.Key.LoadSynchronous();
		FGameplayAbilitySpec AbilitySpec(Ability);
		FGameplayAbilitySpecHandle Handle = AbilityComponent->GiveAbility(AbilitySpec);
		if(Handle.IsValid())
		{
			AbilityGranted.Broadcast(Ability, Handle);
		}
	}
}

void UAC_GASHelper::RemoveItemAbilities(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter)
{
	UIO_GameplayAbilityReferences* Abilities = Cast<UIO_GameplayAbilityReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayAbilityReferences::StaticClass()));
	if(!Abilities)
	{
		return;
	}

	RemoveItemAbilitiesInternal(Abilities, RemoveEventsFilter);
}

void UAC_GASHelper::RemoveItemAbilitiesInternal(UIO_GameplayAbilityReferences* Abilities,
	FGameplayTagContainer RemoveEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RemoveItemAbilities)
	
	if(!Abilities->HasAnyTagEvents(RemoveEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	for(auto& CurrentAbility : Abilities->Abilities)
	{
		if(!CurrentAbility.Value.RemoveEvents.HasAnyExact(RemoveEventsFilter))
		{
			continue;
		}
		
		for(auto& CurrentActiveAbility : AbilityComponent->GetActivatableAbilities())
		{
			TSoftClassPtr<UGameplayAbility> SoftClass = TSoftClassPtr<UGameplayAbility>(FSoftClassPath(CurrentActiveAbility.Ability.GetClass()));
			if(SoftClass == CurrentAbility.Key)
			{
				AbilityComponent->ClearAbility(CurrentActiveAbility.Handle);
				AbilityRemoved.Broadcast(CurrentAbility.Key.LoadSynchronous());
				break;
			}
		}
	}
}

void UAC_GASHelper::GrantItemAttributes(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter)
{
	UIO_AttributeSetReferences* Attributes = Cast<UIO_AttributeSetReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_AttributeSetReferences::StaticClass()));
	if(!Attributes)
	{
		return;
	}

	GrantItemAttributesInternal(Attributes, GrantEventsFilter);
}

void UAC_GASHelper::GrantItemAttributesInternal(UIO_AttributeSetReferences* Attributes,
	FGameplayTagContainer GrantEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GrantItemAttributes)
	
	if(!Attributes->HasAnyTagEvents(GrantEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	TArray<UAttributeSet*> ActiveAttributes = AbilityComponent->GetSpawnedAttributes();
	for(auto& CurrentAttribute : Attributes->AttributeSets)
	{
		if(!CurrentAttribute.Value.GrantEvents.HasAnyExact(GrantEventsFilter))
		{
			continue;
		}

		//Check if we already have the attribute
		bool HasAttribute = false;
		for(auto& ActiveAttribute : ActiveAttributes)
		{
			/**Find out if we already have the attribute and make sure we don't load the attribute uneccessarily*/
			TSoftClassPtr<UAttributeSet> SoftClass = TSoftClassPtr<UAttributeSet>(FSoftClassPath(ActiveAttribute->GetClass()));
			if(SoftClass == CurrentAttribute.Key)
			{
				HasAttribute = true;
				break;
			}
		}

		if(HasAttribute)
		{
			continue;
		}

		//Spawn and add the attribute
		UAttributeSet* SpawnedAttribute = NewObject<UAttributeSet>(AbilityComponent->GetOwner(), CurrentAttribute.Key.LoadSynchronous());
		AbilityComponent->AddSpawnedAttribute(SpawnedAttribute);
		AttributeSetGranted.Broadcast(SpawnedAttribute);
	}
}

void UAC_GASHelper::RemoveItemAttributes(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter)
{
	UIO_AttributeSetReferences* Attributes = Cast<UIO_AttributeSetReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_AttributeSetReferences::StaticClass()));
	if(!Attributes)
	{
		return;
	}

	RemoveItemAttributesInternal(Attributes, RemoveEventsFilter);
}

void UAC_GASHelper::RemoveItemAttributesInternal(UIO_AttributeSetReferences* Attributes,
	FGameplayTagContainer RemoveEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RemoveItemAttributes)
	
	if(!Attributes->HasAnyTagEvents(RemoveEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	TArray<UAttributeSet*> ActiveAttributeSets = AbilityComponent->GetSpawnedAttributes();
	for(auto& CurrentAttribute : Attributes->AttributeSets)
	{
		if(!CurrentAttribute.Value.RemoveEvents.HasAnyExact(RemoveEventsFilter))
		{
			continue;
		}

		for(auto& ActiveAttribute : ActiveAttributeSets)
		{
			if(ActiveAttribute->GetClass() == CurrentAttribute.Key.LoadSynchronous())
			{
				AbilityComponent->RemoveSpawnedAttribute(ActiveAttribute);
				AttributeSetRemoved.Broadcast(CurrentAttribute.Key.LoadSynchronous()->GetClass());
			}
		}
	}
}

void UAC_GASHelper::GrantItemEffects(UDA_CoreItem* Item, FGameplayTagContainer GrantEventsFilter)
{
	UIO_GameplayEffectReferences* Effects = Cast<UIO_GameplayEffectReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayEffectReferences::StaticClass()));
	if(!Effects)
	{
		return;
	}

	ApplyItemEffectsInternal(Item, Effects, GrantEventsFilter);
}

void UAC_GASHelper::ApplyItemEffectsInternal(UDA_CoreItem* Item, UIO_GameplayEffectReferences* Effects,
	FGameplayTagContainer GrantEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GrantItemEffects)
	
	if(!Effects->HasAnyTagEvents(GrantEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilityComponent->MakeEffectContext();
	EffectContext.AddSourceObject(AbilityComponent);

	for(auto& CurrentEffect : Effects->Effects)
	{
		if(!GrantEventsFilter.IsEmpty())
		{
			if(!CurrentEffect.Value.GrantEvents.HasAnyExact(GrantEventsFilter))
			{
				continue;
			}
		}

		TSubclassOf<UGameplayEffect> EffectClass = CurrentEffect.Key.LoadSynchronous();
		FGameplayEffectSpecHandle NewHandle = AbilityComponent->MakeOutgoingSpec(EffectClass, 1, EffectContext);
		if (NewHandle.IsValid())
		{
			/**Apply the gameplay effect and then store it in the EffectHandle's array
			 * so it is much simpler to remove, since you need the handle to remove
			 * a gameplay effect.*/
			FActiveGameplayEffectHandle Handle = AbilityComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilityComponent);
			//Store the handle if applied. If the effect is instant, there's no need to track it since we can't remove it
			if(Handle.WasSuccessfullyApplied() && EffectClass.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::Instant)
			{
				EffectHandles.Add(FEffectTracker(Item, CurrentEffect.Value.RemoveEvents, Handle));
			}
		}
	}
}

void UAC_GASHelper::RemoveItemEffects(UDA_CoreItem* Item, FGameplayTagContainer RemoveEventsFilter)
{
	UIO_GameplayEffectReferences* Effects = Cast<UIO_GameplayEffectReferences>(UFL_InventoryFramework::GetTraitByClassForItem(Item, UIO_GameplayEffectReferences::StaticClass()));
	if(!Effects)
	{
		return;
	}

	RemoveItemEffectsInternal(Item, Effects, RemoveEventsFilter);
}

void UAC_GASHelper::RemoveItemEffectsInternal(UDA_CoreItem* Item, UIO_GameplayEffectReferences* Effects,
	FGameplayTagContainer RemoveEventsFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RemoveItemEffects)
	
	if(!Effects->HasAnyTagEvents(RemoveEventsFilter))
	{
		return;
	}

	UAbilitySystemComponent* AbilityComponent = GetAbilityComponent();
	if(!AbilityComponent)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find ability component");
		return;
	}
	
	if(!AbilityComponent->IsOwnerActorAuthoritative())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner of ASC is not authoritative");
		return;
	}

	for(auto& CurrentHandle : EffectHandles)
	{
		if(CurrentHandle.ItemAsset == Item && CurrentHandle.RemoveEvents.HasAnyExact(RemoveEventsFilter))
		{
			AbilityComponent->RemoveActiveGameplayEffect(CurrentHandle.EffectHandle);
		}
	}
}

void UAC_GASHelper::OnTagEventBroadcasted(FGameplayTag Event)
{
	for(auto& CurrentItem : GetItemsForEvent(Event))
	{
		ProcessItem(CurrentItem, FGameplayTagContainer(Event));
	}
}

void UAC_GASHelper::OnInventoryStarted()
{
	TagEventBroadcasted.Broadcast(GASHELPER_Event_StartComponent);
	Inventory->ComponentStarted.RemoveDynamic(this, &UAC_GASHelper::OnInventoryStarted);
}

TArray<UDA_CoreItem*> UAC_GASHelper::GetItemsForEvent_Implementation(FGameplayTag Event)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetItemsForEvent)
	
	TArray<UDA_CoreItem*> Items;

	if(!Inventory)
	{
		return Items;
	}

	for(auto& CurrentContainer : Inventory->ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			//Already processed this asset, continue to the next item
			if(Items.Contains(CurrentItem.ItemAsset))
			{
				continue;
			}
			
			UIO_GameplayAbilityReferences* Abilities = Cast<UIO_GameplayAbilityReferences>(UFL_InventoryFramework::GetTraitByClassForItem(CurrentItem.ItemAsset, UIO_GameplayAbilityReferences::StaticClass()));
			if(Abilities)
			{
				if(Abilities->HasAnyTagEvents(FGameplayTagContainer(Event)))
				{
					Items.Add(CurrentItem.ItemAsset);
					continue;
				}
			}

			UIO_AttributeSetReferences* Attributes = Cast<UIO_AttributeSetReferences>(UFL_InventoryFramework::GetTraitByClassForItem(CurrentItem.ItemAsset, UIO_AttributeSetReferences::StaticClass()));
			if(Attributes)
			{
				if(Attributes->HasAnyTagEvents(FGameplayTagContainer(Event)))
				{
					Items.Add(CurrentItem.ItemAsset);
					continue;
				}
			}

			UIO_GameplayEffectReferences* Effects = Cast<UIO_GameplayEffectReferences>(UFL_InventoryFramework::GetTraitByClassForItem(CurrentItem.ItemAsset, UIO_GameplayEffectReferences::StaticClass()));
			if(Effects)
			{
				if(Effects->HasAnyTagEvents(FGameplayTagContainer(Event)))
				{
					Items.Add(CurrentItem.ItemAsset);
				}
			}
		}
	}
	
	return Items;
}


void UAC_GASHelper::OnStartComponentEvent(FGameplayTag Event)
{
	if(Event != GASHELPER_Event_StartComponent)
	{
		return;
	}

	for(auto& CurrentItem : GetItemsForEvent(Event))
	{
		ProcessItem(CurrentItem, FGameplayTagContainer(Event));
	}
}

void UAC_GASHelper::OnItemEquipped(FS_InventoryItem Item, const TArray<FName>& CustomTriggerFilters)
{
	ProcessItem(Item.ItemAsset, FGameplayTagContainer(GASHELPER_Event_ItemEquipped));
	//Process children items
	if(UAC_Inventory* InventoryComponent = Item.UniqueID.ParentComponent)
	{
		TArray<FS_ItemSubLevel> ChildrenItems;
		InventoryComponent->GetChildrenItems(Item, ChildrenItems);
		for(auto& CurrentItem : ChildrenItems)
		{
			ProcessItem(CurrentItem.Item.ItemAsset, FGameplayTagContainer(GASHELPER_Event_ItemEquipped));
		}
	}
}

void UAC_GASHelper::OnItemUnequipped(FS_InventoryItem ItemData, FS_InventoryItem OldItemData, const TArray<FName>& CustomTriggerFilters)
{
	ProcessItem(ItemData.ItemAsset, FGameplayTagContainer(GASHELPER_Event_ItemUnequipped));
	//Process children items
	if(UAC_Inventory* InventoryComponent = ItemData.UniqueID.ParentComponent)
	{
		TArray<FS_ItemSubLevel> ChildrenItems;
		InventoryComponent->GetChildrenItems(ItemData, ChildrenItems);
		for(auto& CurrentItem : ChildrenItems)
		{
			ProcessItem(CurrentItem.Item.ItemAsset, FGameplayTagContainer(GASHELPER_Event_ItemUnequipped));
		}
	}
}

void UAC_GASHelper::BeginPlay()
{
	Super::BeginPlay();

	TagEventBroadcasted.AddDynamic(this, &UAC_GASHelper::OnStartComponentEvent);
	Inventory = Cast<UAC_Inventory>(GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
	BindDefaultDelegates();
}


