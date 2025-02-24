// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Data/FL_InventoryFramework.h"

#include "GameplayTagsManager.h"
#include "InputMappingContext.h"
#include "InventoryFrameworkPlugin.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Items/DA_CoreItem.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Interfaces/I_Inventory.h"
#include "Core/Interfaces/I_ExternalObjects.h"
#include "Core/Items/IDA_Currency.h"
#include "Core/Traits/IT_Pricing.h"
#include "Core/Objects/Parents/ItemInstance.h"
#include "Core/Widgets/W_AttachmentParent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Core/Widgets/W_InventoryItem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Core/Components/ItemComponent.h"
#include "Core/Fragments/CF_CompatibilitySettings.h"
#include "Core/Fragments/CF_TileTags.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Fragments/F_Tags.h"
#include "Core/Fragments/IF_ItemOverrideSettings.h"
#include "Core/Fragments/IF_RandomItemCount.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR

#include "ContentBrowserDataDragDropOp.h"
#include "ContentBrowserDataSubsystem.h"
#include "IContentBrowserDataModule.h"
#include "Kismet2/BlueprintEditorUtils.h"

class UDS_InventoryProjectSettings;
struct FContentBrowserItem;

#endif

struct FCompatibilitySettingsFragment;


void UFL_InventoryFramework::SortItemsByIndex(UPARAM(ref) TArray <FS_InventoryItem> &Array_To_Sort, TArray <FS_InventoryItem> &Sorted_Array)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SortItemsByIndex)
    Array_To_Sort.StableSort([](const FS_InventoryItem& TempStruct1, const FS_InventoryItem& TempStruct2)
    {
        return TempStruct1.TileIndex < TempStruct2.TileIndex;
    });
    Sorted_Array = Array_To_Sort;
}

void UFL_InventoryFramework::SortItemsAlphabetically(TArray<UDA_CoreItem*>& Array_To_Sort,
    TArray<UDA_CoreItem*>& Sorted_Array)
{
    Array_To_Sort.StableSort();
    Sorted_Array = Array_To_Sort;
}

TArray <FS_InventoryItem> UFL_InventoryFramework::SortItemStructsAlphabetically(TArray<FS_InventoryItem> ArrayToSort)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SortItemStructsAlphabetically)
    ArrayToSort.StableSort([](const FS_InventoryItem& TempStruct1, const FS_InventoryItem& TempStruct2)
    {
        const int32 Value = GetItemName(TempStruct2).ToString().Compare(GetItemName(TempStruct1).ToString());
        //0 means equal, -1 is less than
        if(Value == 0 ||  Value == -1)
        {
            return false;
        }
        //1 means TempStruct2 is higher priority
        else if(Value == 1)
        {
            return true;
        }
    
        return false;
    });
    return ArrayToSort;
}

TArray<FS_InventoryItem> UFL_InventoryFramework::SortItemStructsByType(TArray<FS_InventoryItem> ArrayToSort)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SortItemStructsByType)
    ArrayToSort.StableSort([](const FS_InventoryItem& TempStruct1, const FS_InventoryItem& TempStruct2)
{
    return TempStruct1.ItemAsset->ItemType.GetTagName().ToString() < TempStruct2.ItemAsset->ItemType.GetTagName().ToString();
});
    
    return ArrayToSort;
}

TArray<FS_InventoryItem> UFL_InventoryFramework::SortItemsByContainerAndIndex(TArray<FS_InventoryItem> ArrayToSort)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SortItemsByContainerAndIndex)

    ArrayToSort.StableSort([](const FS_InventoryItem& TempStruct1, const FS_InventoryItem& TempStruct2)
    {
        return TempStruct1.ContainerIndex < TempStruct2.ContainerIndex;
    });
    
    ArrayToSort.StableSort([](const FS_InventoryItem& TempStruct1, const FS_InventoryItem& TempStruct2)
    {
        return TempStruct1.TileIndex < TempStruct2.TileIndex;
    });
    
    return ArrayToSort;
}

void UFL_InventoryFramework::SortContainers(TArray<FS_ContainerSettings>& Array_To_Sort, TArray<FS_ContainerSettings>& Sorted_Array)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SortContainers)
    Array_To_Sort.StableSort([](const FS_ContainerSettings& TempStruct1, const FS_ContainerSettings& TempStruct2)
    {
        return TempStruct1.ContainerIndex < TempStruct2.ContainerIndex;
    });
    
    Sorted_Array = Array_To_Sort;
}

void UFL_InventoryFramework::CloneObject(UObject* InputObject, FName ObjectName, UObject* ObjectOwner, UObject* &OutputObject)
{
    OutputObject = DuplicateObject<UObject>(InputObject, ObjectOwner, ObjectName);
    if(Cast<USceneComponent>(OutputObject))
    {
        Cast<USceneComponent>(OutputObject)->RegisterComponent();
        Cast<USceneComponent>(OutputObject)->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    }
}

void UFL_InventoryFramework::RenameComponent(UActorComponent* Component, const FString NewName)
{
    if(IsValid(Component))
    {
        Component->Rename(*NewName, Component->GetOuter());
    }
}

FString UFL_InventoryFramework::GenerateSafeNameForComponent(AActor* Actor, UActorComponent* Component, FRandomStream Seed)
{
    FString SafeName;
    
    if(!IsValid(Actor) || !IsValid(Component))
    {
        return SafeName;
    }
    TArray<UActorComponent*> SceneComponents;
    Actor->GetComponents(UActorComponent::StaticClass(), SceneComponents);
    
    if(SceneComponents.IsValidIndex(0))
    {
        SafeName = GetNameSafe(Component);
        const int32 RandomInt = UKismetMathLibrary::RandomIntegerInRangeFromStream(Seed, 1, 2147483647);
        FString NumberString = UKismetStringLibrary::Conv_IntToString(RandomInt);

        SafeName.Append("_");
        SafeName.Append(NumberString);

        for(const auto& CurrentComponent : SceneComponents)
        {
            if(IsValid(CurrentComponent))
            {
                if(GetNameSafe(CurrentComponent) == SafeName)
                {
                    FRandomStream NewSeed;
                    NewSeed.Initialize(Seed.GetInitialSeed() + 1);
                    GenerateSafeNameForComponent(Actor, Component, NewSeed);
                }
            }
        }

        return SafeName;
    }

    //There are no other components, component's default name is safe.
    return GetNameSafe(Component);
}

bool UFL_InventoryFramework::ItemEqualsItem(FS_InventoryItem ItemA, FS_InventoryItem ItemB)
{
    return ItemA == ItemB;
}

bool UFL_InventoryFramework::ItemDoesNotEqualsItem(FS_InventoryItem ItemA, FS_InventoryItem ItemB)
{
    return ItemA != ItemB;
}

bool UFL_InventoryFramework::UniqueIDequalsUniqueID(FS_UniqueID UniqueIDA, FS_UniqueID UniqueIDB)
{
    return UniqueIDA == UniqueIDB;
}

bool UFL_InventoryFramework::IsUniqueIDValid(FS_UniqueID UniqueID)
{
    if(IsValid(UniqueID.ParentComponent))
    {
        if(UniqueID.IdentityNumber > 0)
        {
            return true;
        }
    }

    return false;
}

void UFL_InventoryFramework::AddExternalObjectToItem(UObject* Object, FS_InventoryItem Item)
{
    if(!IsValid(Object))
    {
        return;
    }

    if(!Object->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
    {
        UFL_InventoryFramework::LogIFPMessage(Object, FString::Printf(TEXT("%s does not implement the I_ExternalObjects interface"), *Object->GetName()));
        return;
    }

    if(IsItemValid(Item))
    {
        UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
        if(IsValid(ParentComponent))
        {
            if(ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
            {
                if(ParentComponent->ContainerSettings[Item.ContainerIndex].Items.IsValidIndex(Item.ItemIndex))
                {
                    ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ExternalObjects.AddUnique(Object);
                }
            }
        }
    }
}

void UFL_InventoryFramework::RemoveExternalObjectFromItem(UObject* Object, FS_InventoryItem Item)
{
    if(!IsValid(Object))
    {
        return;
    }

    if(IsItemValid(Item))
    {
        UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
        if(IsValid(ParentComponent))
        {
            if(ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
            {
                if(ParentComponent->ContainerSettings[Item.ContainerIndex].Items.IsValidIndex(Item.ItemIndex))
                {
                    ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ExternalObjects.Remove(Object);
                }
            }
        }
    }
}

void UFL_InventoryFramework::AddExternalObjectToContainer(UObject* Object, FS_ContainerSettings Container)
{
    if(!IsValid(Object))
    {
        return;
    }

    if(!Object->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
    {
        UFL_InventoryFramework::LogIFPMessage(Object, FString::Printf(TEXT("%s does not implement the I_ExternalObjects interface"), *Object->GetName()));
        return;
    }
    
    if(IsContainerValid(Container))
    {
        UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
        if(IsValid(ParentComponent))
        {
            if(ParentComponent->ContainerSettings.IsValidIndex(Container.ContainerIndex))
            {
                ParentComponent->ContainerSettings[Container.ContainerIndex].ExternalObjects.AddUnique(Object);
            }
        }
    }
}

void UFL_InventoryFramework::RemoveExternalObjectFromContainer(UObject* Object, FS_ContainerSettings Container)
{
    if(!IsValid(Object))
    {
        return;
    }
    
    if(IsContainerValid(Container))
    {
        UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
        if(IsValid(ParentComponent))
        {
            if(ParentComponent->ContainerSettings.IsValidIndex(Container.ContainerIndex))
            {
                ParentComponent->ContainerSettings[Container.ContainerIndex].ExternalObjects.Remove(Object);
            }
        }
    }
}

TArray<UObject*> UFL_InventoryFramework::GetExternalObjectsFromItem(FS_InventoryItem Item)
{
    if(IsItemValid(Item))
    {
        UpdateItemStruct(Item);
        TArray<UObject*> Objects;
        for(auto& CurrentObject : Item.ExternalObjects)
        {
            if(IsValid(CurrentObject))
            {
                Objects.Add(CurrentObject);
            }
        }

        return Objects;
    }
    
    TArray<UObject*> InvalidObjects;
    return InvalidObjects;
}

TArray<UObject*> UFL_InventoryFramework::GetExternalObjectsFromContainer(FS_ContainerSettings Container)
{
    if(IsContainerValid(Container))
    {
        TArray<UObject*> Objects;
        Container = Container.UniqueID.ParentComponent->GetContainerByUniqueID(Container.UniqueID);
        for(auto& CurrentObject : Container.ExternalObjects)
        {
            if(IsValid(CurrentObject))
            {
                Objects.Add(CurrentObject);
            }
        }

        return Objects;
    }
    
    TArray<UObject*> InvalidObjects;
    return InvalidObjects;
}

UAC_Inventory* UFL_InventoryFramework::GetLocalInventoryComponent(UObject* WorldContext)
{
    TRACE_CPUPROFILER_EVENT_SCOPE("GetLocalInventoryComponent - IFP")
    
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContext, 0);

    if(PlayerPawn)
    {
        if(UKismetSystemLibrary::DoesImplementInterface(PlayerPawn, UI_Inventory::StaticClass()))
        {
            UAC_Inventory* Inventory = nullptr;
            II_Inventory::Execute_GetInventoryComponent(PlayerPawn, Inventory);

            if(IsValid(Inventory))
            {
                return Inventory;
            }
        }

        //Pawn doesn't implement interface, try checking all components.
        UAC_Inventory* Inventory = Cast<UAC_Inventory>(PlayerPawn->GetComponentByClass(UAC_Inventory::StaticClass()));
        if(IsValid(Inventory))
        {
            UFL_InventoryFramework::LogIFPMessage(WorldContext, "Found local inventory, but owner didn't implement I_Inventory or did not pass component through GetInventoryComponent");
            return Inventory;
        }
    }

    return nullptr;
}

bool UFL_InventoryFramework::GetComponentByTag(AActor* Actor, FName Tag, USceneComponent*& Component,
    AActor*& AttachedActor, bool ClimbUpHierarchy)
{
    TRACE_CPUPROFILER_EVENT_SCOPE("GetComponentByTag - IFP")
    
    Component = nullptr;
    AttachedActor = nullptr;
    
    if(!IsValid(Actor))
    {
        return false;
    }
    
    for(auto& CurrentComponent : Actor->GetComponents())
    {
        if(USceneComponent* SceneComponent = Cast<USceneComponent>(CurrentComponent))
        {
            if(SceneComponent->ComponentTags.Contains(Tag))
            {
                Component = SceneComponent;
                return true;
            }
        }
    }

    TArray<AActor*> OtherActors;
    if(ClimbUpHierarchy)
    {
        for(AActor* Parent = Actor->GetAttachParentActor(); Parent != nullptr; Parent = Parent->GetAttachParentActor())
        {
            OtherActors.Add(Parent);
        }
    }
    else
    {
        Actor->GetAttachedActors(OtherActors, true, true);
    }
    
    for(auto& CurrentActor : OtherActors)
    {
        if(CurrentActor->Tags.Contains(Tag))
        {
            Component = CurrentActor->GetRootComponent();
            AttachedActor = CurrentActor;
            return true;
        }
        else
        {
            for(auto& CurrentComponent : CurrentActor->GetComponents())
            {
                if(USceneComponent* SceneComponent = Cast<USceneComponent>(CurrentComponent))
                {
                    if(SceneComponent->ComponentTags.Contains(Tag))
                    {
                        Component = SceneComponent;
                        AttachedActor = CurrentActor;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool UFL_InventoryFramework::IsItemValid(FS_InventoryItem Item)
{
    return Item.IsValid();
}

FIntPoint UFL_InventoryFramework::GetItemDimensions(FS_InventoryItem Item, bool IgnoreContainerStyle, bool IgnoreRotation)
{
    FIntPoint Dimensions = FIntPoint(1);

    //Validate start
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(!IsValid(Item.ItemAsset)) { return Dimensions; }

    if(!IsValid(ParentComponent)) { return Item.ItemAsset->ItemDimensions; }
    
    if(!ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))  { return Item.ItemAsset->ItemDimensions; }
    //Validate end

    if(ParentComponent->ContainerSettings[Item.ContainerIndex].IsSpacialContainer() || IgnoreContainerStyle)
    {
        bool InvertDimensions = IgnoreRotation ? false : Item.Rotation == Ninety || Item.Rotation == TwoSeventy;
        Dimensions.X = UKismetMathLibrary::SelectInt(Item.ItemAsset->ItemDimensions.Y, Item.ItemAsset->ItemDimensions.X, InvertDimensions);
        Dimensions.Y = UKismetMathLibrary::SelectInt(Item.ItemAsset->ItemDimensions.X, Item.ItemAsset->ItemDimensions.Y, InvertDimensions);
        return Dimensions;
    }
    else
    {
        return Dimensions;
    }
}

void UFL_InventoryFramework::GetItemDimensionsWithContext(FS_InventoryItem Item, FS_ContainerSettings Container, int32& X, int32& Y)
{
    if(IsValid(Item.ItemAsset))
    {
        if(Container.IsSpacialContainer())
        {
            bool InvertDimensions = Item.Rotation == Ninety || Item.Rotation == TwoSeventy;
            X = UKismetMathLibrary::SelectInt(Item.ItemAsset->ItemDimensions.Y, Item.ItemAsset->ItemDimensions.X, InvertDimensions);
            Y = UKismetMathLibrary::SelectInt(Item.ItemAsset->ItemDimensions.X, Item.ItemAsset->ItemDimensions.Y, InvertDimensions);
            return;
        }
    }

    X = 1;
    Y = 1;
}

void UFL_InventoryFramework::GetTraitsByTagForItem(UDA_CoreItem* Item, FGameplayTag Tag,
    TArray<UItemTrait*>& FoundObjects)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectsByTagForItem)
    
    FoundObjects.Empty();
    
    if(!Tag.IsValid() || !IsValid(Item))
    {
        return;
    }
    for(auto& CurrentObject : Item->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->TraitTag.MatchesTagExact(Tag))
            {
                FoundObjects.Add(CurrentObject);
            }
        }
    }
}

UItemTrait* UFL_InventoryFramework::GetTraitByTagForItem(UDA_CoreItem* Item, FGameplayTag Tag)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectByTagForItem)
    
    if(!Tag.IsValid() || !IsValid(Item))
    {
        return nullptr;
    }
    
    for(auto& CurrentObject : Item->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->TraitTag == Tag)
            {
                return CurrentObject;
            }
        }
    }

    return nullptr;
}

void UFL_InventoryFramework::GetTraitsByClassForItem(UDA_CoreItem* Item, TSubclassOf<UItemTrait> Class,
    TArray<UItemTrait*>& FoundTraits, bool AllowChildren)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectsByClassForItem)
    
    FoundTraits.Empty();
    
    if(!IsValid(Class) || !IsValid(Item))
    {
        return;
    }
    
    for(auto& CurrentObject : Item->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(AllowChildren)
            {
                if(CurrentObject->GetClass()->IsChildOf(Class))
                {
                    FoundTraits.Add(CurrentObject);
                }
            }
            else
            {
                if(CurrentObject->GetClass() == Class)
                {
                    FoundTraits.Add(CurrentObject);
                }
            }
        }
    }
}

UItemTrait* UFL_InventoryFramework::GetTraitByClassForItem(UDA_CoreItem* Item,
    TSubclassOf<UItemTrait> Class, bool AllowChildren)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectByClassForItem)
    
    if(!IsValid(Class) || !IsValid(Item))
    {
        return nullptr;
    }
    
    for(auto& CurrentObject : Item->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(AllowChildren)
            {
                if(CurrentObject->GetClass()->IsChildOf(Class) || CurrentObject->GetClass() == Class)
                {
                    return CurrentObject;
                }
            }
            else
            {
                if(CurrentObject->GetClass() == Class)
                {
                    return CurrentObject;
                }
            }
        }
    }

    return nullptr;
}

TArray<int32> UFL_InventoryFramework::GetOverlappingTiles(FS_ContainerSettings Container)
{
    TArray<int32> ProcessedTiles;
    TArray<int32> OverlappingTiles;
    if(!Container.Items.IsValidIndex(0) || !IsValid(Container.UniqueID.ParentComponent))
    {
        return OverlappingTiles;
    }

    UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
    for(auto& CurrentItem : Container.Items)
    {
        if(!IsItemOutOfBounds(CurrentItem, Container) && ParentComponent->CheckCompatibility(CurrentItem, Container))
        {
            TArray<int32> CurrentItemsTiles;
            bool InvalidTileFound;
            ParentComponent->GetItemsTileIndexes(CurrentItem, CurrentItemsTiles, InvalidTileFound);
            if(!InvalidTileFound)
            {
                for(int32& CurrentTile : CurrentItemsTiles)
                {
                    if(ProcessedTiles.Contains(CurrentTile))
                    {
                        OverlappingTiles.AddUnique(CurrentTile);
                    }
                    ProcessedTiles.Add(CurrentTile);
                }
            }
        }
    }

    return OverlappingTiles;
}

TArray<UIDA_Currency*> UFL_InventoryFramework::GetAcceptedCurrencies(FS_InventoryItem Item)
{
    TArray<UIDA_Currency*> AcceptedCurrencies;
    if(FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.ItemFragments))
    {
        if(OverrideFragment->AcceptedCurrenciesOverwrite.IsValidIndex(0))
        {
            AcceptedCurrencies.Append(OverrideFragment->AcceptedCurrenciesOverwrite);
        }
    }

    //Find out if we have a Pricing object attached to the item.
    for(auto& CurrentObject : Item.ItemAsset->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->GetClass() == UIT_Pricing::StaticClass())
            {
                AcceptedCurrencies.Append(Cast<UIT_Pricing>(CurrentObject)->DefaultAcceptedCurrencies);
            }
        }
    }

    return AcceptedCurrencies;
}

FText UFL_InventoryFramework::GetItemName(FS_InventoryItem Item)
{
    if(FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.ItemFragments))
    {
         if(!OverrideFragment->ItemName.IsEmpty())
            {
                return OverrideFragment->ItemName;
            }
    }
   

    if(IsValid(Item.ItemAsset))
    {
        return Item.ItemAsset->ItemName;
    }

    return FText::FromString("No name found");
}

FText UFL_InventoryFramework::GetItemDescription(FS_InventoryItem Item, FString RichTextWrapper)
{
    FText Description;

    if(!IsValid(Item.ItemAsset))
    {
        return Description;
    }

    if(FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.ItemFragments))
    {
        if(!OverrideFragment->Description.IsEmpty())
        {
            Description = OverrideFragment->Description;
        }
        else
        {
            Description = Item.ItemAsset->Description;
        }
    }
    else
    {
        Description = Item.ItemAsset->Description;
    }

    FString DescriptionString = Description.ToString();

    //Start converting all tag values in the text into their float values
    for(auto& CurrentTagValue : GetItemsTagValues(Item))
    {
        FString TagString = "<" + CurrentTagValue.Tag.ToString() + ">"; //If you want to change the syntax, change this
        if(DescriptionString.Contains(CurrentTagValue.Tag.ToString()))
        {
            //Text was found, replace it
            FString ReplacementText;
            if(!RichTextWrapper.IsEmpty())
            {
                //Wrap the text in the desired rich text decorator text.
                ReplacementText = RichTextWrapper + FString::SanitizeFloat(CurrentTagValue.Value, 0) + "</>";
            }
            else
            {
                ReplacementText = FString::SanitizeFloat(CurrentTagValue.Value, 0);
            }
            
            DescriptionString.ReplaceInline(*TagString, *ReplacementText);
        }
    }
    
    return FText::AsCultureInvariant(DescriptionString);
}

int32 UFL_InventoryFramework::GetItemMaxStack(FS_InventoryItem Item)
{
    if(!IsUniqueIDValid(Item.UniqueID))
    {
        if(IsValid(Item.ItemAsset))
        {
            return Item.ItemAsset->MaxStack;
        }
        return 0;
    }

    /**Update the item, in case the ref is stale,
     * then fetch the MaxStackOverride tag value
     * and if its found, return its value. */
    UpdateItemStruct(Item);
    if(FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.ItemFragments))
    {
        FS_TagValue TagValue = GetValueForTag(TagFragment->TagValues, IFP_MaxStackOverride);
        if(TagValue.IsValid())
        {
            return TagValue.Value;
        }
    }
    
    return Item.ItemAsset->MaxStack;
}

float UFL_InventoryFramework::GetItemPrice(FS_InventoryItem Item, bool IncludeChildItems)
{
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(!IsValid(ParentComponent))
    {
        return false;
    }
    
    float ChildItemsPrice = 0;
    float ItemPrice = 0;

    if(IncludeChildItems)
    {
        TArray<FS_ItemSubLevel> ChildrenItems;
        ParentComponent->GetChildrenItems(Item, ChildrenItems);
        if(ChildrenItems.IsValidIndex(0))
        {
            for(auto& CurrentItem : ChildrenItems)
            {
                ChildItemsPrice += GetItemPrice(CurrentItem.Item);
            }
        } 
    }

    //Find out if this asset has the Pricing object.
    for(UItemTrait*& CurrentObject : Item.ItemAsset->TraitsAndComponents)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->GetClass() == UIT_Pricing::StaticClass())
            {
                ItemPrice = Cast<UIT_Pricing>(CurrentObject)->Price;
                break;
            }
        }
    }

    return ItemPrice * Item.Count + ChildItemsPrice;
}

TArray<FIntPoint> UFL_InventoryFramework::GetItemsShape(FS_InventoryItem Item, bool& InvalidTileFound)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(GetItemsShape)
    TArray<FIntPoint> ItemsShape;
    InvalidTileFound = false;

    if(!IsItemValid(Item))
    {
        return ItemsShape;
    }

    if(!Item.UniqueID.ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
    {
        return ItemsShape;
    }

    FIntPoint ItemRelativeSpace;

    if(Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].ContainerType != Inventory)
    {
        //Only grid supports complex shapes.
        IndexToTile(Item.TileIndex, Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex], ItemRelativeSpace.X, ItemRelativeSpace.Y);
        ItemsShape.Add(ItemRelativeSpace);
        return ItemsShape;
    }

    if(Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].IsSpacialStyle())
    {
        //Get the pure shape of the item.
        ItemsShape = Item.ItemAsset->GetItemsPureShape(Item.Rotation);

        //Since the pure shape of the item is in local space,
        //we have to apply the @Item's relative space, and thus
        //we can get the shape in the correct place.
        IndexToTile(Item.TileIndex, Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex], ItemRelativeSpace.X, ItemRelativeSpace.Y);
        //Apply offset
        for(auto& CurrentTile : ItemsShape)
        {
            CurrentTile.X += ItemRelativeSpace.X;
            CurrentTile.Y += ItemRelativeSpace.Y;

            if(!IsTileValid(CurrentTile.X, CurrentTile.Y, Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex]))
            {
                InvalidTileFound = true;
            }
        }

        return ItemsShape;
    }
    else
    {
        //Only grid supports complex shapes.
        IndexToTile(Item.TileIndex, Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex], ItemRelativeSpace.X, ItemRelativeSpace.Y);
        ItemsShape.Add(ItemRelativeSpace);
        return ItemsShape;
    }
}

TArray<FIntPoint> UFL_InventoryFramework::GetItemsShapeWithContext(FS_InventoryItem Item,
    FS_ContainerSettings Container, bool& InvalidTileFound)
{
    TArray<FIntPoint> ItemsShape;
    InvalidTileFound = false;
    
    if(!IsContainerValid(Container))
    {
        InvalidTileFound = true;
        return ItemsShape;
    }
    
    Item.ContainerIndex = Container.ContainerIndex;
    Item.UniqueID.ParentComponent = Container.UniqueID.ParentComponent;
    ItemsShape = GetItemsShape(Item, InvalidTileFound);

    return ItemsShape;
}

UW_AttachmentParent* UFL_InventoryFramework::GetItemsAttachmentWidget(FS_InventoryItem Item, bool CreateIfMissing, bool DoNotBind)
{
    //@Item might have stale data or invalid directions, fetch a direct copy
    Item = Item.UniqueID.ParentComponent->GetItemByUniqueID(Item.UniqueID);
    if(Item.IsValid())
    {
        UW_AttachmentParent* AttachmentWidget = nullptr;
        for(auto& CurrentObject : GetExternalObjectsFromItem(Item))
        {
            AttachmentWidget = Cast<UW_AttachmentParent>(CurrentObject);
            if(AttachmentWidget)
            {
                return AttachmentWidget;
            }
        }
        
        if(CreateIfMissing && !IsValid(AttachmentWidget))
        {
            //Direct copy does not have a valid widget, create a new one.
            TArray<FS_ContainerSettings> ItemsContainers;
            if(UW_AttachmentParent* NewWidget = Item.UniqueID.ParentComponent->CreateAttachmentWidgetForItem(Item, false, DoNotBind, ItemsContainers))
            {
                return NewWidget;
            }
        }
    }
    
    return nullptr;
}

float UFL_InventoryFramework::GetItemsSpawnChance(FS_InventoryItem Item)
{
    for(auto& CurrentTagValue : Item.TagValues_DEPRECATED)
    {
        if(CurrentTagValue.Tag == IFP_SpawnChanceValue)
        {
            return CurrentTagValue.Value;
        }
    }

    if(IsValid(Item.ItemAsset))
    {
        return Item.ItemAsset->DefaultSpawnChance;
    }

    //ItemAsset is invalid, can't evaluate spawn chance
    return 0;
}

bool UFL_InventoryFramework::IsItemOverridden(FS_InventoryItem Item)
{
    if(FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.ItemFragments))
    {
        if(!OverrideFragment->ItemName.IsEmptyOrWhitespace()) { return true; }

        if(!OverrideFragment->Description.IsEmptyOrWhitespace()) { return true; }

        if(OverrideFragment->InventoryImage.ToSoftObjectPath().IsValid()) { return true; }

        if(OverrideFragment->AcceptedCurrenciesOverwrite.IsValidIndex(0)) { return true; }
    }
    
    return false;
}

bool UFL_InventoryFramework::CanStackItems(FS_InventoryItem Item1, FS_InventoryItem Item2)
{
    if(IsValid(Item1.ItemAsset) && IsValid(Item2.ItemAsset))
    {
        //Check if item 2 isn't at max stack, check both items are the same data asset, check both items aren't the exact same item by comparing UniqueID, then check if either item can stack at all.
        return GetItemMaxStack(Item2) != Item2.Count && Item1.ItemAsset == Item2.ItemAsset && Item1.UniqueID.IdentityNumber != Item2.UniqueID.IdentityNumber && Item1.ItemAsset->CanItemStack() && Item2.ItemAsset->CanItemStack();
        //Item1.Item->MaxStack != Item1.Count && 
    }
    return false;
}

bool UFL_InventoryFramework::IsItemOutOfBounds(FS_InventoryItem Item, FS_ContainerSettings Container)
{
    if(!Container.SupportsTileMap())
    {
        //Container has no concept of boundaries
        return false;
    }
    
    if(!Item.UniqueID.ParentComponent)
    {
        return true;
    }

    if(!Container.TileMap.IsValidIndex(Item.TileIndex))
    {
        return true;
    }
    
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    TArray<int32> TileIndexes;
    Item.ContainerIndex = Container.ContainerIndex;
    bool InvalidTileFound = false;
    ParentComponent->GetItemsTileIndexes(Item, TileIndexes, InvalidTileFound);

    if(InvalidTileFound)
    {
        return true;
    }
    
    for(int32& CurrentTile : TileIndexes)
    {
        if(!UKismetMathLibrary::InRange_IntInt(CurrentTile, 0, Container.Dimensions.X * Container.Dimensions.Y))
        {
            return true;
        }

        int32 X = 0;
        int32 Y = 0;
        TileToIndex(X, Y, Container);
        if(!IsTileValid(X, Y, Container))
        {
            return true;
        }
    }
    
    return false;
}

bool UFL_InventoryFramework::CanAffordItemWithSpecificCurrency(FS_InventoryItem ItemToPurchase,
                                                               UAC_Inventory* BuyerComponent, UIDA_Currency* Currency)
{
    TArray<UIDA_Currency*> AcceptedCurrencies = GetAcceptedCurrencies(ItemToPurchase);
    if(!AcceptedCurrencies.IsValidIndex(0))
    {
        UFL_InventoryFramework::LogIFPMessage(BuyerComponent, FString::Printf(TEXT("%p has no accepted currencies set up."), ItemToPurchase.ItemAsset), true, true);
        // GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%p has no accepted currencies set up."), ItemToPurchase.Item));
        return false;
    }

    if(!IsValid(BuyerComponent))
    {
        UFL_InventoryFramework::LogIFPMessage(BuyerComponent, TEXT("Buyer component invalid"), true, true);
        // GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Buyer component invalid"));
        
        return false;
    }

    if(!AcceptedCurrencies.Contains(Currency))
    {
        return false;
    }
    
    TArray<FS_InventoryItem> FoundItems;
    int32 TotalAmountFound;
    BuyerComponent->GetAllItemsWithDataAsset(Currency, -1, FoundItems, TotalAmountFound);
    if(FoundItems.IsValidIndex(0))
    {
        for(auto& CurrentItem : FoundItems)
        {
            if(!IsItemValid(CurrentItem))
            {
                continue;
            }
                
            if(GetItemPrice(CurrentItem) >= GetItemPrice(ItemToPurchase))
            {
                return true;
            }
        }
    }

    //No valid items were found.
    return false;
}

bool UFL_InventoryFramework::IsItemEquipped(FS_InventoryItem Item)
{
    UAC_Inventory* Inventory = Item.UniqueID.ParentComponent;

    if(!IsValid(Inventory))
    {
        return false;
    }

    if(Inventory->EquipTag.IsValid())
    {
        if(GetItemsTags(Item, false).HasTag(Inventory->EquipTag))
        {
            return true;
        }
    }

    if(UFL_InventoryFramework::GetItemsParentContainer(Item).ContainerType == Equipment)
    {
        return true;
    }

    return false;
}

bool UFL_InventoryFramework::IsItemInNetworkQueue(FS_InventoryItem Item)
{
    if(IsItemValid(Item))
    {
        return Item.UniqueID.ParentComponent->NetworkQueue.Contains(Item.UniqueID);
    }
    return false;
}

void UFL_InventoryFramework::AddDefaultTagsToItem(FS_InventoryItem& Item, bool CallDelegates)
{
    if(!IsValid(Item.ItemAsset))
    {
        return;
    }
    
    TArray<FGameplayTag> DefaultTags;
    
    FTagFragment* DefaultTagsFragment = FindFragment<FTagFragment>(Item.ItemAsset->ItemStructFragments);
    if(!DefaultTagsFragment)
    {
        //Item has no default tag fragment
        return;
    }
    
    //Get the fragment on the item. Add it if it's missing
    FTagFragment* StructTagsFragment =  FindFragment<FTagFragment>(Item.ItemFragments, true);
    if(!StructTagsFragment)
    {
        return;
    }
    
    DefaultTags = DefaultTagsFragment->Tags.GetGameplayTagArray();
    for(auto& CurrentTag : DefaultTags)
    {
        if(!StructTagsFragment->Tags.HasTagExact(CurrentTag))
        {
            StructTagsFragment->Tags.AddTagFast(CurrentTag);
    
            if(CallDelegates)
            {
                if(!IsItemValid(Item))
                {
                    return;
                }
    
                UFL_ExternalObjects::BroadcastTagsUpdated(CurrentTag, true, Item, FS_ContainerSettings());
            }
        }
    }
}

void UFL_InventoryFramework::AddDefaultTagValuesToItem(FS_InventoryItem& Item, bool OverrideValues, bool CallDelegates)
{
    if(!IsValid(Item.ItemAsset))
    {
        return;
    }

    FTagFragment* DefaultTagsFragment =  FindFragment<FTagFragment>(Item.ItemAsset->ItemStructFragments);
    if(!DefaultTagsFragment)
    {
        //Item has no default tag fragment
        return;
    }
    
    //Get the fragment on the item. Add it if it's missing
    FTagFragment* StructTagsFragment =  FindFragment<FTagFragment>(Item.ItemFragments, true);
    if(!StructTagsFragment)
    {
        return;
    }

    TArray<FS_TagValue> DefaultTagValues = DefaultTagsFragment->TagValues;
    for(auto& CurrentTagValue : DefaultTagValues)
    {
        //Keep track if addition was successful
        bool TagAdded = false;
        //Store delta value for delegates.
        float Delta = CurrentTagValue.Value;
        
        FS_TagValue FoundTagValue;
        int32 TagIndex;
        if(!DoesTagValuesHaveTag(StructTagsFragment->TagValues, CurrentTagValue.Tag, FoundTagValue, TagIndex))
        {
            StructTagsFragment->TagValues.AddUnique(CurrentTagValue);
            TagAdded = true;
        }
        else
        {
            if(OverrideValues)
            {
                Delta = StructTagsFragment->TagValues[TagIndex].Value - CurrentTagValue.Value;
                StructTagsFragment->TagValues[TagIndex].Value = CurrentTagValue.Value;
                TagAdded = true;
            }
        }

        if(CallDelegates && TagAdded)
        {
            if(!IsItemValid(Item))
            {
                return;
            }
            
            Item.UniqueID.ParentComponent->ItemTagValueUpdated.Broadcast(Item, CurrentTagValue, Delta);
            
            UFL_ExternalObjects::BroadcastTagValueUpdated(CurrentTagValue, false, Delta, Item, FS_ContainerSettings());
        }
    }
}

FGameplayTagContainer UFL_InventoryFramework::GetItemsTags(FS_InventoryItem Item, const bool IncludeAssetTags)
{
    if(!IncludeAssetTags)
    {
        return UF_Tags::GetTagFragmentFromItem(Item).Tags;
    }
    else
    {
        if(!IsValid(Item.ItemAsset))
        {
            return UF_Tags::GetTagFragmentFromItem(Item).Tags;
        }
        
        FGameplayTagContainer CombinedContainer;
        CombinedContainer.AppendTags(UF_Tags::GetTagFragmentFromItem(Item).Tags);
        
        //Append will leave duplicate tags, manually check
        //and add any tags that aren't in the container.
        for(auto& CurrentTag : UF_Tags::GetTagFragmentFromItemAsset(Item.ItemAsset).Tags)
        {
            if(!CombinedContainer.HasTagExact(CurrentTag))
            {
                CombinedContainer.AddTag(CurrentTag);
            }
        }
        
        return CombinedContainer;
    }
}

TArray<FS_TagValue> UFL_InventoryFramework::GetItemsTagValues(FS_InventoryItem Item, const bool IncludeAssetTagValues)
{
    TArray<FS_TagValue> CombinedValues;
    
    if(!IsValid(Item.ItemAsset))
    {
        return CombinedValues;
    }
    
    if(!IncludeAssetTagValues)
    {
        return UF_Tags::GetTagFragmentFromItem(Item).TagValues;
    }
    else
    {
        CombinedValues.Append(UF_Tags::GetTagFragmentFromItem(Item).TagValues);

        for(auto& CurrentTagValue : UF_Tags::GetTagFragmentFromItemAsset(Item.ItemAsset).TagValues)
        {
            if(!CombinedValues.Contains(CurrentTagValue.Tag))
            {
                CombinedValues.Add(CurrentTagValue);
            }
        }

        return CombinedValues;
    }
}

FS_InventoryItem UFL_InventoryFramework::GetOwningItemForContainer(FS_ContainerSettings Container)
{
    UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
    if(!IsValid(ParentComponent))
    {
        return FS_InventoryItem();
    }
    
    if(ParentComponent->Initialized)
    {
        FS_ContainerSettings ItemsContainer = ParentComponent->GetContainerByUniqueID(FS_UniqueID(Container.BelongsToItem.X, ParentComponent));
        if(!ItemsContainer.IsValid())
        {
            return FS_InventoryItem();
        }
        
         FS_InventoryItem FoundItem = ParentComponent->GetItemByUniqueID(FS_UniqueID(Container.BelongsToItem.Y, ParentComponent));
        if(FoundItem.IsValid())
        {
            return FoundItem;
        }
    }
    //If the component hasn't started yet, use the BelongsToItem to find
    //the direct reference.
    else
    {
        if(ParentComponent->ContainerSettings.IsValidIndex(Container.BelongsToItem.X))
        {
            if(ParentComponent->ContainerSettings[Container.BelongsToItem.X].Items.IsValidIndex(Container.BelongsToItem.Y))
            {
                return ParentComponent->ContainerSettings[Container.BelongsToItem.X].Items[Container.BelongsToItem.Y];
            }
        }
    }

    return FS_InventoryItem();
}

UItemInstance* UFL_InventoryFramework::GetItemsInstance(FS_InventoryItem Item, bool DoNotConstruct)
{
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(IsValid(ParentComponent))
    {
        Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

        if(DoNotConstruct)
        {
            return Item.ItemInstance;
        }
        
        if(!Item.ItemInstance || !Item.ItemInstance->ItemID.IsValid())
        {
            if(!Item.ItemAsset->ItemInstance.IsNull() && Cast<UItemInstance>(Item.ItemAsset->ItemInstance.LoadSynchronous()->GetDefaultObject())->ConstructOnRequest)
            {
                //Object isn't valid but is being requested,
                //and it wants to be created on request.
                UItemInstance* ItemInstance = ParentComponent->CreateItemInstanceForItem(Item);
                return ItemInstance;
            }
        }
        
        return Item.ItemInstance;
    }

    return Item.ItemInstance;
}

void UFL_InventoryFramework::UpdateItemStruct(FS_InventoryItem& Item)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UpdateItemStruct)
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(!IsValid(ParentComponent))
    {
        return;
    }

    if(AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
    {
        Item = ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex];
        return;
    }
    
    FS_InventoryItem FoundItem = ParentComponent->GetItemByUniqueID(Item.UniqueID);
    if(FoundItem.IsValid())
    {
        Item = FoundItem;
        return;
    }
}

TArray<UObject*> UFL_InventoryFramework::GetObjectsForItemBroadcast(FS_InventoryItem Item, bool DoNotConstructItemInstance)
{
    TArray<UObject*> Objects;
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(!ParentComponent)
    {
        return Objects;
    }

    //Some data may be stale, fetch a fresh copy
    Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

    if(UItemInstance* ItemInstance = UFL_InventoryFramework::GetItemsInstance(Item, DoNotConstructItemInstance))
    {
        Objects.Add(ItemInstance);
    }


    if(UW_InventoryItem* ItemWidget = GetWidgetForItem(Item))
    {
        Objects.Add(ItemWidget);
    }
    
    Objects.Append(GetItemsComponents(Item));

    TArray<UObject*> ExternalObjects = UFL_InventoryFramework::GetExternalObjectsFromItem(Item);
    Objects.Append(ExternalObjects);

    //Remove all the invalid objects from the array
    for(int32 i = 0; i < Objects.Num(); ++i)
    {
        if(!IsValid(Objects[i]))
        {
            Objects.RemoveAt(i);
            i--;
        }
    }

    return Objects;
}

TArray<UObject*> UFL_InventoryFramework::GetObjectsForItemBroadcastWithInterface(FS_InventoryItem Item,
    TSubclassOf<UInterface> Interface, bool DoNotConstructItemInstance)
{
    TArray<UObject*> Objects;
    UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
    if(!ParentComponent)
    {
        return Objects;
    }

    TArray<UObject*> ValidatedObjects;
    Objects = GetObjectsForItemBroadcast(Item, DoNotConstructItemInstance);
    for(auto& CurrentObject : Objects)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->GetClass()->ImplementsInterface(Interface))
            {
                ValidatedObjects.Add(CurrentObject);
            }
        }
    }

    return ValidatedObjects;
}


TArray<UObject*> UFL_InventoryFramework::GetObjectsForContainerBroadcast(FS_ContainerSettings Container)
{
    TArray<UObject*> Objects;
    UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
    if(!ParentComponent)
    {
        return Objects;
    }

    //Some data may be stale, fetch a fresh copy
    Container = ParentComponent->GetContainerByUniqueID(Container.UniqueID);

    if(UW_Container* ContainerWidget = GetWidgetForContainer(Container))
    {
        Objects.Add(ContainerWidget);
    }

    TArray<UObject*> ExternalObjects = UFL_InventoryFramework::GetExternalObjectsFromContainer(Container);
    Objects.Append(ExternalObjects);

    return Objects;
}

TArray<UObject*> UFL_InventoryFramework::GetObjectsForContainerBroadcastWithInterface(FS_ContainerSettings Container,
    TSubclassOf<UInterface> Interface)
{
    TArray<UObject*> Objects;
    UAC_Inventory* ParentComponent = Container.UniqueID.ParentComponent;
    if(!ParentComponent)
    {
        return Objects;
    }

    TArray<UObject*> ValidatedObjects;
    Objects = GetObjectsForContainerBroadcast(Container);
    for(auto& CurrentObject : Objects)
    {
        if(IsValid(CurrentObject))
        {
            if(CurrentObject->GetClass()->ImplementsInterface(Interface))
            {
                ValidatedObjects.Add(CurrentObject);
            }
        }
    }

    return ValidatedObjects;
}

TArray<FS_ContainerSettings> UFL_InventoryFramework::SanitizeContainersForPreviewActor(
    TArray<FS_ContainerSettings> Containers)
{
    for(auto& CurrentContainer : Containers)
    {
        CurrentContainer.ExternalObjects.Empty();
        CurrentContainer.Widget = nullptr;
        for(auto& CurrentItem : CurrentContainer.Items)
        {
            CurrentItem.ExternalObjects.Empty();
            CurrentItem.ItemComponents.Empty();
            CurrentItem.ItemInstance = nullptr;
            CurrentItem.Widget = nullptr;
        }
    }
	
    return Containers;
}

TArray<FS_ContainerSettings> UFL_InventoryFramework::SanitizeContainersForClientRPC(
    TArray<FS_ContainerSettings> Containers)
{
    for(auto& CurrentContainer : Containers)
    {
        /**The tile map is relatively expensive for networking bandwidth,
         * but it's extremely cheap to generate.
         * So we empty it before the RPC, then have the client regenerate it
         * with the data the server gave them.*/
        CurrentContainer.TileMap.Empty();
        /**Start removing any server-only fragments on containers*/
        for(int32 FragmentIndex = 0; FragmentIndex < CurrentContainer.ContainerFragments.Num(); FragmentIndex++)
        {
            if(CurrentContainer.ContainerFragments[FragmentIndex].IsValid())
            {
                if(CurrentContainer.ContainerFragments[FragmentIndex].GetMutablePtr<>()->GetNetworkingMethod() == Server)
                {
                    CurrentContainer.ContainerFragments.RemoveAt(FragmentIndex);
                    FragmentIndex--;
                }
            }
        }

        /**Start removing any server-only fragments on items*/
        for(auto& CurrentItem : CurrentContainer.Items)
        {
            for(int32 FragmentIndex = 0; FragmentIndex < CurrentItem.ItemFragments.Num(); FragmentIndex++)
            {
                if(CurrentItem.ItemFragments[FragmentIndex].IsValid())
                {
                    if(CurrentItem.ItemFragments[FragmentIndex].GetMutablePtr<>()->GetNetworkingMethod() == Server)
                    {
                        CurrentItem.ItemFragments.RemoveAt(FragmentIndex);
                        FragmentIndex--;
                    }
                }
            }
        }
    }

    return Containers;
}

FGameplayTagContainer UFL_InventoryFramework::GetContainersTags(FS_ContainerSettings Container)
{
    FTagFragment TagFragment = UF_Tags::GetTagFragmentFromContainer(Container);
    return TagFragment.Tags;
}

TArray<FS_TagValue> UFL_InventoryFramework::GetContainersTagValues(FS_ContainerSettings Container)
{
    FTagFragment TagFragment = UF_Tags::GetTagFragmentFromContainer(Container);
    return TagFragment.TagValues;
}

TArray<UItemComponent*> UFL_InventoryFramework::GetItemsComponents(FS_InventoryItem Item)
{
    return Item.ItemComponents;
}

UW_InventoryItem* UFL_InventoryFramework::GetWidgetForItem(FS_InventoryItem Item)
{
    UpdateItemStruct(Item);

    return Item.Widget;
}

bool UFL_InventoryFramework::GetItemsVisual(FS_InventoryItem Item, FName CustomAppendedID, USceneComponent*& Component, AActor*& Actor)
{
    Component = nullptr;
    Actor = nullptr;
    if(!Item.IsValid())
    {
        return false;
    }

    
    FName Tag = FName(*FString::Printf(TEXT("%d"), Item.UniqueID.IdentityNumber) + CustomAppendedID.ToString());
    return GetComponentByTag(Item.UniqueID.ParentComponent->GetOwner(), Tag, Component, Actor);
}

FS_ContainerSettings UFL_InventoryFramework::GetItemsParentContainer(FS_InventoryItem Item)
{
    if(!Item.IsValid())
    {
        return FS_ContainerSettings();
    }

    if(UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
    {
        return Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex];
    }

    return FS_ContainerSettings();
}

bool UFL_InventoryFramework::RemoveItemAttachmentWidget(FS_InventoryItem Item, bool MarkAsGarbage)
{
    if(AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
    {
        FS_InventoryItem& ItemRef = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex];

        UW_AttachmentParent* AttachmentWidget = nullptr;
        for(auto& CurrentObject : ItemRef.ExternalObjects)
        {
            AttachmentWidget = Cast<UW_AttachmentParent>(CurrentObject);
            if(AttachmentWidget)
            {
                ItemRef.ExternalObjects.RemoveSingle(AttachmentWidget);
                AttachmentWidget->RemoveFromParent();
                if(MarkAsGarbage)
                {
                    AttachmentWidget->MarkAsGarbage();
                }

                return true;
            }
        }
    }

    return false;
}

bool UFL_InventoryFramework::IsContainerInfinite(FS_ContainerSettings Container,
                                                 TEnumAsByte<EContainerInfinityDirection>& Direction)
{
    if(Container.IsInfinite())
    {
        Direction = Container.InfinityDirection;
        return true;
    }
    else
    {
        Direction = Neither;
        return false;
    }
}

bool UFL_InventoryFramework::IsContainerValid(FS_ContainerSettings Container)
{
    return Container.IsValid();
}

void UFL_InventoryFramework::GetContainerDimensions(FS_ContainerSettings Container, int32& X, int32& Y)
{
    if(Container.ContainerType.operator==(Equipment))
    {
        X = 1;
        Y = 1;
    }
    else
    {
        X = Container.Dimensions.X;
        Y = Container.Dimensions.Y;
    }
}

int32 UFL_InventoryFramework::GetNumberOfFreeTilesInContainer(FS_ContainerSettings Container)
{
    int32 Amount = 0;
    if(!Container.TileMap.IsValidIndex(0))
    {
        return 0;
    }
    
    for(const auto& CurrentTile : Container.TileMap)
    {
        if(CurrentTile == -1)
        {
            Amount++;
        }
    }

    return Amount;
}

int32 UFL_InventoryFramework::GetEmptyTilesAmount(FS_ContainerSettings Container)
{
    int32 Amount = 0;
    
    for(const auto& CurrentTile : Container.TileMap)
    {
        if(CurrentTile == -1)
        {
            Amount++;
        }
    }

    return Amount;
}

bool UFL_InventoryFramework::IsContainerFull(FS_ContainerSettings Container)
{
    return GetNumberOfFreeTilesInContainer(Container) <= 0;
}

bool UFL_InventoryFramework::IsContainerInNetworkQueue(FS_ContainerSettings Container)
{
    if(IsValid(Container.UniqueID.ParentComponent))
    {
        return Container.UniqueID.ParentComponent->NetworkQueue.Contains(Container.UniqueID);
    }
    return false;
}

bool UFL_InventoryFramework::AreItemDirectionsValid(FS_UniqueID ItemID, int32 ContainerIndex, int32 ItemIndex)
{
    if(!IsValid(ItemID.ParentComponent))
    {
        return false;
    }

    if(!ItemID.ParentComponent->ContainerSettings.IsValidIndex(ContainerIndex))
    {
        return false;
    }

    if(!ItemID.ParentComponent->ContainerSettings[ContainerIndex].Items.IsValidIndex(ItemIndex))
    {
        return false;
    }

    if(ItemID.ParentComponent->ContainerSettings[ContainerIndex].Items[ItemIndex].UniqueID != ItemID)
    {
        return false;
    }
    
    return true;
}

bool UFL_InventoryFramework::IsSpacialContainer(FS_ContainerSettings Container)
{
    return Container.IsSpacialContainer();
}

bool UFL_InventoryFramework::IsSpacialStyle(FS_ContainerSettings Container)
{
    return Container.IsSpacialStyle();
}

bool UFL_InventoryFramework::DoesContainerSupportTileMap(FS_ContainerSettings Container)
{
    return Container.SupportsTileMap();
}

UW_Container* UFL_InventoryFramework::GetWidgetForContainer(FS_ContainerSettings Container)
{
    if(!IsValid(Container.UniqueID.ParentComponent))
    {
        return Container.Widget;
    }

    Container = Container.UniqueID.ParentComponent->GetContainerByUniqueID(Container.UniqueID);
    return Container.Widget;
}

bool UFL_InventoryFramework::IsTileValid(int32 X, int32 Y, FS_ContainerSettings Container)
{
    return X >= 0 && Y >= 0 && X < Container.Dimensions.X && Y < Container.Dimensions.Y;
}

bool UFL_InventoryFramework::IsTileMapIndexValid(int32 Index, FS_ContainerSettings Container)
{
    TEnumAsByte<EContainerInfinityDirection> InfinityDirection;
    if(IsContainerInfinite(Container, InfinityDirection))
    {
        return true;
    }
    return Container.TileMap.IsValidIndex(Index);
}

void UFL_InventoryFramework::IndexToTile(int32 TileIndex, FS_ContainerSettings Container, int32& X, int32& Y)
{
    if(TileIndex < 0)
    {
        X = 0;
        Y = 0;
        return;
    }
    
    int32 LocalX = 0;
    int32 LocalY = 0;
    GetContainerDimensions(Container, LocalX, LocalY);
    X = TileIndex % LocalX;
    Y = TileIndex / LocalX;
}

int32 UFL_InventoryFramework::TileToIndex(int32 X, int32 Y, FS_ContainerSettings Container)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(TileToIndex)

    if(!Container.SupportsTileMap())
    {
        return 0;
    }

    if(int32* Index = Container.IndexCoordinates.Find(FIntPoint(X, Y)))
    {
        return *Index;
    }
    else
    {
        //Coordinates either don't exist or are invalid. Resort to old method
        //V: Shouldn't this be deleted? This might just give incorrect results
        //since this seems like it'll only return out-of-bounds results.
        int32 LocalX = 0;
        int32 LocalY = 0;
        GetContainerDimensions(Container, LocalX, LocalY);
        
        return LocalX * (Y + 1) - (LocalX - X);
    }
}

void UFL_InventoryFramework::GetPaddingForTile(FVector2D TileDimensions, FS_ContainerSettings Container,
    int32 TileIndex, float& Left, float& Top)
{
    int32 ContainerX;
    int32 ContainerY;
    GetContainerDimensions(Container, ContainerX, ContainerY);
    int32 TileX;
    int32 TileY;
    IndexToTile(TileIndex, Container, TileX, TileY);
	
    Left = TileDimensions.X * (TileIndex % ContainerX);
    Top = TileDimensions.Y * ((TileIndex + (ContainerX - TileX)) / ContainerX - 1);
}

TArray<int32> UFL_InventoryFramework::GetTilesWithinDimension(FIntPoint Range, FS_ContainerSettings Container, int32 StartingIndex)
{
    TArray<int32> Tiles;
    
    if(!Container.TileMap.IsValidIndex(StartingIndex))
    {
        return Tiles;
    }
    
    int32 XLoop = 0;
    int32 YLoop = 0;
    IndexToTile(StartingIndex, Container, XLoop, YLoop);
    
    for(int32 ColumnY = YLoop; ColumnY < YLoop + Range.Y; ColumnY++)
    {
        if(ColumnY < Container.Dimensions.Y)
        {
            for(int32 RowX = XLoop; RowX < XLoop + Range.X; RowX++)
            {
                if(RowX < Container.Dimensions.X)
                {
                    int32 CurrentIndex = TileToIndex(RowX, ColumnY, Container);;
                    if(Container.TileMap.IsValidIndex(CurrentIndex) && IsTileValid(RowX, ColumnY, Container))
                    {
                        Tiles.Add(CurrentIndex);
                    }
                }
            } 
        }
    }
    return Tiles;
}

int32 UFL_InventoryFramework::ApplyTileOffset(int32 TileIndex, FS_ContainerSettings Container, FIntPoint Offset, FIntPoint& Remainder)
{
    if(!IsContainerValid(Container))
    {
        return 0;
    }

    if(!Container.TileMap.IsValidIndex(TileIndex))
    {
        return 0;
    }

    FIntPoint Tile;
    IndexToTile(TileIndex, Container, Tile.X, Tile.Y);
    FIntPoint ContainerDimensions;
    GetContainerDimensions(Container, ContainerDimensions.X, ContainerDimensions.Y);

    const int32 NewX = UKismetMathLibrary::Clamp(Tile.X + Offset.X, 0, ContainerDimensions.X - 1);
    const int32 NewY = UKismetMathLibrary::Clamp(Tile.Y + Offset.Y, 0, ContainerDimensions.Y - 1);
    Remainder.X = Tile.X + Offset.X - NewX;
    Remainder.Y = Tile.Y + Offset.Y - NewY;
    
    return TileToIndex(NewX, NewY, Container);
}

TArray<FIntPoint> UFL_InventoryFramework::RotateShape(TArray<FIntPoint> Shape, TEnumAsByte<ERotation> RotateAmount, FIntPoint AnchorPoint)
{
    if(RotateAmount == Zero)
    {
        // Shape.Empty();
        return Shape;
    }
    
    //Figure out how often we are going to rotate.
    int32 numRotations = 0;
    switch (RotateAmount)
    {
    case Ninety:
        numRotations = 1;
        break;
    case OneEighty:
        numRotations = 2;
        break;
    case TwoSeventy:
        numRotations = 3;
        break;
    default:
        break;
    }

    for (int32 i = 0; i < numRotations; ++i)
    {
        for (FIntPoint& tile : Shape)
        {
            //Translate the tile to be relative to the anchor point
            int32 relativeX = tile.X - AnchorPoint.X;
            int32 relativeY = tile.Y - AnchorPoint.Y;

            //Rotate the tile
            int32 temp = relativeX;
            relativeX = -relativeY;
            relativeY = temp;

            //Translate the tile back to the absolute position
            tile.X = AnchorPoint.X + relativeX;
            tile.Y = AnchorPoint.Y + relativeY;
        }
    }

    return Shape;
}

TEnumAsByte<ERotation> UFL_InventoryFramework::CombineRotations(TEnumAsByte<ERotation> Rotation1,
    TEnumAsByte<ERotation> Rotation2)
{
    int32 Rotation1Value = static_cast<int32>(Rotation1);
    int32 Rotation2Value = static_cast<int32>(Rotation2);
    
    int32 CombinedValue = Rotation1Value + Rotation2Value;
    if (CombinedValue > 3)
    {
        //Combined value exceeded the enum.
        //Offset it back down into the range of the enum
        CombinedValue -= 4;
    }
    
    return static_cast<ERotation>(CombinedValue);
}

bool UFL_InventoryFramework::GetPaddingAndRoundingForCustomShapeItemWidget(FS_InventoryItem Item, FMargin Padding,
                                                                           FVector4 Rounding, int32 TileIndex, FMargin& FinalPadding, FVector4& FinalRounding)
{
    FinalPadding = FMargin();
    FinalRounding = FVector4();
    
    if(!IsItemValid(Item))
    {
        return false;
    }
    
    FS_ContainerSettings Container = Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex];

    if(Container.ContainerType == Equipment)
    {
        FinalPadding = Padding;
        FinalRounding = Rounding;
        return true;
    }

    FIntPoint Offset = FIntPoint(0);
    int32 CurrentScanningTile = 0;

    //This is used in case there's no padding,
    //but we still want to apply rounding.
    bool TopModified = false;
    bool RightModified = false;
    bool BottomModified = false;
    bool LeftModified = false;

    //Top side
    Offset.Y = -1;
    FIntPoint Remainder;
    if(Container.TileMap.IsValidIndex(CurrentScanningTile))
    {
        CurrentScanningTile = ApplyTileOffset(TileIndex, Container, Offset, Remainder);
        if(Remainder.X < 0 || Remainder.Y < 0 || Container.TileMap[CurrentScanningTile] == -1 || Item.UniqueID.IdentityNumber != Container.TileMap[CurrentScanningTile])
        {
            FinalPadding.Top = Padding.Top;
            TopModified = true;
        }
    }
    else
    {
        FinalPadding.Top = Padding.Top;
        TopModified = true;
    }

    //Right side
    Offset.X = 1;
    Offset.Y = 0;
    CurrentScanningTile = ApplyTileOffset(TileIndex, Container, Offset, Remainder);
    if(Container.TileMap.IsValidIndex(CurrentScanningTile))
    {
        if(Remainder.X < 0 || Remainder.Y < 0 || Container.TileMap[CurrentScanningTile] == -1 || Item.UniqueID.IdentityNumber != Container.TileMap[CurrentScanningTile])
        {
            FinalPadding.Right = Padding.Right;
            RightModified = true;
        }
    }
    else
    {
        FinalPadding.Right = Padding.Right;
        RightModified = true;
    }
    
    //Bottom side
    Offset.X = 0;
    Offset.Y = 1;
    CurrentScanningTile = ApplyTileOffset(TileIndex, Container, Offset, Remainder);
    if(Container.TileMap.IsValidIndex(CurrentScanningTile))
    {
        if(Remainder.X < 0 || Remainder.Y < 0 || Container.TileMap[CurrentScanningTile] == -1 || Item.UniqueID.IdentityNumber != Container.TileMap[CurrentScanningTile])
        {
            FinalPadding.Bottom = Padding.Bottom;
            BottomModified = true;
        }  
    }
    else
    {
        FinalPadding.Bottom = Padding.Bottom;
        BottomModified = true;
    }

    //Left side
    Offset.X = -1;
    Offset.Y = 0;
    CurrentScanningTile = ApplyTileOffset(TileIndex, Container, Offset, Remainder);
    if(Container.TileMap.IsValidIndex(CurrentScanningTile))
    {
        if(Remainder.X < 0 || Remainder.Y < 0 || Container.TileMap[CurrentScanningTile] == -1 || Item.UniqueID.IdentityNumber != Container.TileMap[CurrentScanningTile])
        {
            FinalPadding.Left = Padding.Left;
            LeftModified = true;
        }
    }
    else
    {
        FinalPadding.Left = Padding.Left;
        LeftModified = true;
    }
    
    //Start evaluating the rounding
    if(Rounding.X > 0 || Rounding.Y > 0 || Rounding.Z > 0 || Rounding.W > 0)
    {
        //Top right corner
        if(TopModified && RightModified)
        {
            FinalRounding.Y = Rounding.Y;
        }

        //Bottom right corner
        if(RightModified && BottomModified)
        {
            FinalRounding.Z = Rounding.Z;
        }

        //Bottom left corner
        if(BottomModified && LeftModified)
        {
            FinalRounding.W = Rounding.W;
        }

        //Top left corner
        if(LeftModified && TopModified)
        {
            FinalRounding.X = Rounding.X;
        }
    }
    
    return TopModified || RightModified || BottomModified || LeftModified;
}

TArray<UUserWidget*> UFL_InventoryFramework::GetWidgetsOfClassInHierarchy(UUserWidget* Widget, TSubclassOf<UUserWidget> Class)
{
    TArray<UUserWidget*> FoundWidgets;
    if(!Widget)
    {
        return FoundWidgets;
    }

    // Get the root widget
    UPanelWidget* RootWidget = Cast<UPanelWidget>(Widget->GetRootWidget());
    if(!RootWidget)
    {
        return FoundWidgets;
    }

    // Lambda function to recursively search for widgets of the specified class
    auto RecursiveFindWidgets = [&](UWidget* CurrentWidget, auto& RecursiveFindWidgetsRef) -> void
    {
        if(UUserWidget* UserWidget = Cast<UUserWidget>(CurrentWidget))
        {
            if(UserWidget->GetClass()->IsChildOf(Class) || UserWidget->GetClass() == Class)
            {
                FoundWidgets.Add(UserWidget);
            }
        }

        // If the widget is a panel widget, search its children
        if(UPanelWidget* PanelWidget = Cast<UPanelWidget>(CurrentWidget))
        {
            for(int32 CurrentChild = 0; CurrentChild < PanelWidget->GetChildrenCount(); CurrentChild++)
            {
                UWidget* ChildWidget = PanelWidget->GetChildAt(CurrentChild);
                RecursiveFindWidgetsRef(ChildWidget, RecursiveFindWidgetsRef);
            }
        }

        // Recursively search through user widgets if it's a nested user widget
        if(UUserWidget* NestedUserWidget = Cast<UUserWidget>(CurrentWidget))
        {
            UWidget* NestedRoot = NestedUserWidget->GetRootWidget();
            if(NestedRoot)
            {
                RecursiveFindWidgetsRef(NestedRoot, RecursiveFindWidgetsRef);
            }
        }
    };

    //Start the recursive search
    RecursiveFindWidgets(RootWidget, RecursiveFindWidgets);

    return FoundWidgets;
}

bool UFL_InventoryFramework::QueryKeyToMappedInputAction(FKey Key, UInputAction* InputAction,
                                                         UInputMappingContext* MappingContext)
{
    if(!InputAction || !MappingContext)
    {
        return false;
    }

    for(auto& CurrentMapping : MappingContext->GetMappings())
    {
        if(CurrentMapping.Action == InputAction && CurrentMapping.Key == Key)
        {
            return true;
        }
    }

    return false;
}

TArray<FKey> UFL_InventoryFramework::GetKeysMappedToContext(UInputAction* InputAction,
    UInputMappingContext* MappingContext)
{
    TArray<FKey> Keys;
    
    if(!InputAction || !MappingContext)
    {
        return Keys;
    }

    for(auto& CurrentMapping : MappingContext->GetMappings())
    {
        if(CurrentMapping.Action == InputAction)
        {
            Keys.AddUnique(CurrentMapping.Key);
        }
    }

    return Keys;
}

bool UFL_InventoryFramework::AreAnyKeysDown(APlayerController* Controller, TArray<FKey> Keys)
{
    if(!Controller)
    {
        return false;
    }

    for(auto& CurrentKey : Keys)
    {
        if(Controller->IsInputKeyDown(CurrentKey))
        {
            return true;
        }
    }

    return false;
}

void UFL_InventoryFramework::MarkObjectAsGarbage(UObject* Object)
{
    if(UWidget* Widget = Cast<UWidget>(Object))
    {
        Widget->RemoveFromParent();
    }

    Object->MarkAsGarbage();
}

bool UFL_InventoryFramework::DoesTagValuesHaveTag(TArray<FS_TagValue> TagValues, FGameplayTag Tag,
                                                  FS_TagValue& FoundTagValue, int32& TagIndex)
{
    FS_TagValue NullTag;
    
    for(int32 CurrentIndex = 0; CurrentIndex < TagValues.Num(); CurrentIndex++)
    {
        if(Tag.MatchesTagExact(TagValues[CurrentIndex].Tag))
        {
            TagIndex = CurrentIndex;
            FoundTagValue = TagValues[CurrentIndex];
            return true;
        }
    }
    
    TagIndex = -1;
    FoundTagValue = NullTag;
    return false;
}

FS_TagValue UFL_InventoryFramework::GetValueForTag(TArray<FS_TagValue> TagValues, FGameplayTag Tag)
{
    for(auto& CurrentTagValue : TagValues)
    {
        if(Tag.MatchesTagExact(CurrentTagValue.Tag))
        {
            return CurrentTagValue;
        }
    }
    
    return FS_TagValue();
}

bool UFL_InventoryFramework::IsTagValueValid(FS_TagValue TagValue)
{
    return TagValue.IsValid();
}

FGameplayTagContainer UFL_InventoryFramework::ConvertTagValuesToTagContainer(TArray<FS_TagValue> TagValues)
{
    FGameplayTagContainer TagContainer;
    if(TagValues.IsEmpty())
    {
        return TagContainer;
    }

    for(auto& CurrentTagValue : TagValues)
    {
        TagContainer.AddTag(CurrentTagValue.Tag);
    }

    return TagContainer;
}

FGameplayTagContainer UFL_InventoryFramework::GetTagsChildren(FGameplayTag Tag)
{
    return UGameplayTagsManager::Get().RequestGameplayTagChildren(Tag);
}

void UFL_InventoryFramework::MarkAssetAsDirty(UObject* Object)
{
    if(IsValid(Object))
    {
        Object->MarkPackageDirty();
    }
}

void UFL_InventoryFramework::LogIFPMessage(UObject* WorldContext, FString Message, bool PrintToLog, bool PrintToScreen, float Duration)
{
    if(PrintToLog)
    {
        UE_LOG(LogInventoryFramework, Log, TEXT("%s"), *Message);   
    }
    
    if(PrintToScreen)
    {
        UKismetSystemLibrary::PrintString(WorldContext, Message, true, PrintToLog, FLinearColor(0, 66, 1), Duration);
    }
}

#if WITH_EDITOR

void UFL_InventoryFramework::ProcessContainerAndItemCustomizations(UPARAM(ref) TArray<FS_ContainerSettings>& Containers)
{
    for(auto& CurrentContainer : Containers)
    {
        for(auto& CurrentItem : CurrentContainer.Items)
        {
            if(!CurrentItem.ItemAssetSoftReference.IsNull())
            {
                FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(CurrentItem.ItemFragments);
                if(OverrideFragment && !OverrideFragment->ItemName.IsEmptyOrWhitespace())
                {
                    CurrentItem.ItemEditorName = OverrideFragment->ItemName.ToString();
                }
                else
                {
                    if(CurrentItem.ItemAssetSoftReference.LoadSynchronous()->ItemName.IsEmptyOrWhitespace())
                    {
                        CurrentItem.ItemEditorName = CurrentItem.ItemAssetSoftReference.LoadSynchronous()->GetClass()->GetName() + " (No ItemName has been assigned!)";
                    }
                    else
                    {
                        CurrentItem.ItemEditorName = CurrentItem.ItemAssetSoftReference.LoadSynchronous()->ItemName.ToString();
                    }
                }
                if(IsItemOverridden(CurrentItem))
                {
                    CurrentItem.ItemEditorName = CurrentItem.ItemEditorName + " (Overriden)";
                }

                CurrentItem.IsStackable = CurrentItem.ItemAssetSoftReference.LoadSynchronous()->CanItemStack();
                CurrentItem.CanRotate = CurrentContainer.Style == Grid;

                //Prevent "wild" ItemInstances
                if(CurrentItem.ItemAssetSoftReference.LoadSynchronous()->ItemInstance.IsNull())
                {
                    CurrentItem.ItemInstance = nullptr;
                    CurrentItem.AllowItemInstance = false;
                }
                else
                {
                    CurrentItem.AllowItemInstance = true;
                }

                //Prevent ItemInstance class mixing
                if(CurrentItem.ItemInstance && !CurrentItem.ItemAssetSoftReference.LoadSynchronous()->ItemInstance.IsNull())
                {
                    TSoftClassPtr<UItemInstance> SoftClass = TSoftClassPtr<UItemInstance>(FSoftClassPath(CurrentItem.ItemInstance->GetClass()));
                    if(SoftClass != CurrentItem.ItemAssetSoftReference.LoadSynchronous()->ItemInstance)
                    {
                        CurrentItem.ItemInstance = nullptr;
                    }
                }
            }
            else
            {
                //No item data asset found
                CurrentItem.ItemEditorName = "No item available";
            }

            /**Only show the TileIndex if the container supports a tile map and it's not a equipment container,
             * since equipment containers are just 1 tile it doesn't need to be shown.*/
            CurrentItem.ContainerSupportsTileMap = CurrentContainer.SupportsTileMap() && CurrentContainer.ContainerType != Equipment;
        }
    }
}

void UFL_InventoryFramework::SetActorEditorOnly(AActor* Actor, bool NewStatus)
{
    Actor->bIsEditorOnlyActor = NewStatus;
}

FEventReply UFL_InventoryFramework::CreateContentBrowserDragDrop(UObject* Asset)
{
    /**V: Reverse engineering how the DragDrop actually works for asset items drove me insane.
     * All of the below is an evil hack to recreate the same logic that
     * happens in SAssetView::OnDraggingAssetItem
     * This means that we recreate the same drag and drop logic that is created when dragging
     * an asset in the content browser.*/

    UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
    
    //Get the asset
    FString AssetPath = Asset->GetPathName();
    FContentBrowserItem AssetItem = FContentBrowserItem(ContentBrowserData->GetItemAtPath(FName(*FString("/All" + AssetPath)), EContentBrowserItemTypeFilter::IncludeFiles));
    if(!AssetItem.IsValid())
    {
        //Asset was invalid. Attempt again, but check if it's in a plugin.
        AssetItem = FContentBrowserItem(ContentBrowserData->GetItemAtPath(FName(*FString("/All/Plugins" + AssetPath)), EContentBrowserItemTypeFilter::IncludeFiles));
    }

    //I'm not gonna lie, I have no idea what this is doing and why its needed
    TMap<UContentBrowserDataSource*, TArray<FContentBrowserItemData>> SourcesAndItems;
    FContentBrowserItem::FItemDataArrayView ItemDataArray = AssetItem.GetInternalItems();
    for (const FContentBrowserItemData& ItemData : ItemDataArray)
    {
        if (UContentBrowserDataSource* ItemDataSource = ItemData.GetOwnerDataSource())
        {
            TArray<FContentBrowserItemData>& ItemsForSource = SourcesAndItems.FindOrAdd(ItemDataSource);
            ItemsForSource.Add(ItemData);
        }
    }

    //The DragDropOperation only takes in an array, even though we are working with just one item
    TArray<FContentBrowserItem> SelectedItems;
    SelectedItems.Add(AssetItem);
    TSharedPtr<FDragDropOperation> DragDropOperation = FContentBrowserDataDragDropOp::New(SelectedItems);
    
    FEventReply Reply;
    Reply.NativeReply = FReply::Handled();
    
    Reply.NativeReply.BeginDragDrop(DragDropOperation.ToSharedRef());
    return Reply;
}

void UFL_InventoryFramework::UpdateLegacyContainerSettingsCode(TArray<FS_ContainerSettings>& ContainerSettings, UObject* ContainerOwner)
{
    for(auto& CurrentContainer : ContainerSettings)
	{
		//tags and tag values
		if(CurrentContainer.Tags_DEPRECATED.IsValid() || CurrentContainer.TagValues_DEPRECATED.IsValidIndex(0))
		{
			if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentContainer.ContainerFragments, true))
			{
				TagFragment->Tags = CurrentContainer.Tags_DEPRECATED;
				TagFragment->TagValues = CurrentContainer.TagValues_DEPRECATED;
				CurrentContainer.Tags_DEPRECATED = FGameplayTagContainer();
				CurrentContainer.TagValues_DEPRECATED.Empty();
				ContainerOwner->MarkPackageDirty();
			}
		}

		//Compatibility settings
		if(CurrentContainer.CompatibilitySettings_DEPRECATED.BlockingTags.IsValid() ||
			CurrentContainer.CompatibilitySettings_DEPRECATED.ItemBlacklist.IsValidIndex(0) ||
			CurrentContainer.CompatibilitySettings_DEPRECATED.ItemWhitelist.IsValidIndex(0) ||
			CurrentContainer.CompatibilitySettings_DEPRECATED.RequiredTags.IsValid() ||
			CurrentContainer.CompatibilitySettings_DEPRECATED.ItemTagTypes.IsValid())
		{
			if(FCompatibilitySettingsFragment* CompatibilitySettingsFragment =
				FindFragment<FCompatibilitySettingsFragment>(CurrentContainer.ContainerFragments, true))
			{
				CompatibilitySettingsFragment->BlockingTags = CurrentContainer.CompatibilitySettings_DEPRECATED.BlockingTags;
			    for(auto& CurrentWhitelist : CurrentContainer.CompatibilitySettings_DEPRECATED.ItemWhitelist)
			    {
			        CompatibilitySettingsFragment->ItemWhitelist.Add(CurrentWhitelist);
			    }
			    for(auto& CurrentWhitelist : CurrentContainer.CompatibilitySettings_DEPRECATED.ItemBlacklist)
			    {
			        CompatibilitySettingsFragment->ItemBlacklist.Add(CurrentWhitelist);
			    }
				CompatibilitySettingsFragment->RequiredTags = CurrentContainer.CompatibilitySettings_DEPRECATED.RequiredTags;
				CompatibilitySettingsFragment->ItemTypes = CurrentContainer.CompatibilitySettings_DEPRECATED.ItemTagTypes;

				CurrentContainer.CompatibilitySettings_DEPRECATED = FS_CompatibilitySettings();
				ContainerOwner->MarkPackageDirty();
			}
		}

		//Tile tags
		if(CurrentContainer.TileTags_DEPRECATED.IsValidIndex(0))
		{
			if(FTileTagsFragment* TileTagsFragment = FindFragment<FTileTagsFragment>(CurrentContainer.ContainerFragments, true))
			{
				TileTagsFragment->TileTags = CurrentContainer.TileTags_DEPRECATED;
				CurrentContainer.TileTags_DEPRECATED.Empty();
				ContainerOwner->MarkPackageDirty();
			}
		}

		for(auto& CurrentItem : CurrentContainer.Items)
		{
			//tags and tag values
			if(CurrentItem.Tags_DEPRECATED.IsValid() || CurrentItem.TagValues_DEPRECATED.IsValidIndex(0))
			{
				if(FTagFragment* TagFragment = FindFragment<FTagFragment>(CurrentItem.ItemFragments, true))
				{
					TagFragment->Tags = CurrentItem.Tags_DEPRECATED;
					TagFragment->TagValues = CurrentItem.TagValues_DEPRECATED;
					CurrentItem.Tags_DEPRECATED = FGameplayTagContainer();
					CurrentItem.TagValues_DEPRECATED.Empty();
					ContainerOwner->MarkPackageDirty();
				}
			}

			//Override settings
			if(!CurrentItem.OverrideSettings_DEPRECATED.InventoryImage.IsNull() ||
				!CurrentItem.OverrideSettings_DEPRECATED.Description.IsEmpty() ||
				!CurrentItem.OverrideSettings_DEPRECATED.ItemName.IsEmpty() ||
				CurrentItem.OverrideSettings_DEPRECATED.AcceptedCurrenciesOverwrite.IsValidIndex(0))
			{
				if(FItemOverrideSettings* OverrideSettingsFragment = FindFragment<FItemOverrideSettings>(CurrentItem.ItemFragments, true))
				{
					OverrideSettingsFragment->Description = CurrentItem.OverrideSettings_DEPRECATED.Description;
					OverrideSettingsFragment->InventoryImage = CurrentItem.OverrideSettings_DEPRECATED.InventoryImage;
					OverrideSettingsFragment->ItemName = CurrentItem.OverrideSettings_DEPRECATED.ItemName;
					OverrideSettingsFragment->AcceptedCurrenciesOverwrite = CurrentItem.OverrideSettings_DEPRECATED.AcceptedCurrenciesOverwrite;
					CurrentItem.OverrideSettings_DEPRECATED = FS_ItemOverwriteSettings();
					ContainerOwner->MarkPackageDirty();
				}
			}

			//Random item count
			if(CurrentItem.RandomMinMaxCount_DEPRECATED.X != -1 || CurrentItem.RandomMinMaxCount_DEPRECATED.Y != -1)
			{
				if(FRandomItemCountFragment* RandomItemCountFragment = FindFragment<FRandomItemCountFragment>(CurrentItem.ItemFragments, true))
				{
					RandomItemCountFragment->MinMaxCount = CurrentItem.RandomMinMaxCount_DEPRECATED;
					CurrentItem.RandomMinMaxCount_DEPRECATED = FIntPoint(-1, -1);
					ContainerOwner->MarkPackageDirty();
				}
			}

			 //Transfer the hard references to soft references
			 if(IsValid(CurrentItem.ItemAsset) && CurrentItem.ItemAssetSoftReference.IsNull())
			 {
			 	CurrentItem.ItemAssetSoftReference = CurrentItem.ItemAsset;
			 	CurrentItem.ItemAsset = nullptr;
			 	ContainerOwner->MarkPackageDirty();
			 }
		}
	}
}

TArray<FS_ContainerSettings>* UFL_InventoryFramework::GetContainerSettingsArray(UObject* Object)
{
    if (!Object) return nullptr;

    //Iterate over all properties in the class
    for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;

        //Check if the property is an array
        if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
        {
            //Check if the inner property type matches FS_ContainerSettings
            if(FStructProperty* StructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
            {
                if(StructProperty->Struct->GetFName() == FName("S_ContainerSettings"))
                {
                    //The property matches TArray<FS_ContainerSettings>
                    return ArrayProperty->ContainerPtrToValuePtr<TArray<FS_ContainerSettings>>(Object);
                }
            }
        }
    }
    
    return nullptr;
}

bool UFL_InventoryFramework::ImplementInterface(UObject* InstigatingObject, UObject* TargetObject, UClass* Interface,
    TArray<FString> CustomMessages)
{
    if(!TargetObject || !Interface)
    { return false; }

    if(UGameplayStatics::GetGameInstance(TargetObject))
    {
        //Function is not allowed to run during game session
        return false;
    }
	
    if(InstigatingObject->IsTemplate())
    { return false; }

    if(!TargetObject->GetClass())
    { return false; }

    TObjectPtr<UObject> BPGC = Cast<UBlueprintGeneratedClass>(TargetObject->GetClass())->ClassGeneratedBy;
    if(!BPGC)
    { return false; }
	
    UBlueprint* OuterBlueprint = Cast<UBlueprint>(BPGC);
    if(!OuterBlueprint)
    { return false; }

    if(FBlueprintEditorUtils::ImplementsInterface(OuterBlueprint, true, Interface))
    {
        //Already implements interface
        return false;
    }
	
    FString DialogText = FString::Printf(TEXT("%s requires the implementation of the %s interface to this class.\nThis interface will now automatically be added."),
        *InstigatingObject->GetClass()->GetAuthoredName(), *Interface->GetAuthoredName());

    if(CustomMessages.IsValidIndex(0))
    {
        DialogText += "\n\n";
    }
    for(int32 CurrentIndex = 0; CurrentIndex < CustomMessages.Num(); CurrentIndex++)
    {
        DialogText += "	- " + CustomMessages[CurrentIndex];
        if(CurrentIndex != CustomMessages.Num() - 1)
        {
            //If this isn't the last message, add a new line
            DialogText += "\n\n";
        }
    }
	
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(DialogText));
    FBlueprintEditorUtils::ImplementNewInterface(OuterBlueprint, Interface->GetClassPathName());
    return true;
}


#endif

