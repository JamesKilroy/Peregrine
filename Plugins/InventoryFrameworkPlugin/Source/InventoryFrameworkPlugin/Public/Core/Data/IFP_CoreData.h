// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "IFP_CoreData.generated.h"

class UFL_InventoryFramework;
struct FS_ContainerSettings;
struct FS_InventoryItem;
class UItemInstance;
class UW_Container;
class UW_InventoryItem;
class UItemComponent;
class UIDA_Currency;
class UW_AttachmentParent;
class UAC_Inventory;
class UDA_CoreItem;


#pragma region Enums

UENUM(Blueprintable)
enum EInventoryType
{
	Player,
	//Things such as chests, defeated enemies, pickups, etc.
	Interactable,
	Vendor,
	Pickup,
	Storage
};

UENUM(BlueprintType)
enum EContainerType
{
	//This container holds multiple items.
	Inventory,
	//This container only holds onto 1 item and that item is meant to be an equippable item.
	Equipment,
	/**This is meant for pickup actors, item index 0 should be the item this pickup is supposed to represent.
	 * This should only be index 0 of your container settings and nowhere else.*/
	ThisActor
};

UENUM(BlueprintType)
enum EContainerStyle
{
	//This container will behave like a spacial inventory.
	Grid,
	//This container will behave like a traditional inventory, where every
	Traditional,
	/**This container will not bother with any kind of collision checks,
	 * handling capacity or handling dimensions. This style has very few limitations.
	 * This is primarily used when you want to store items *somewhere* the player
	 * might or might not see it, or if your UI doesn't require any kind of
	 * traditional container. This style has the least default implementation
	 * and is meant to be customized. This style by default will always be
	 * infinite and host as many items as you want, since it doesn't
	 * have any concept of dimensions or constraints.
	 * In many scenarios, these types of containers won't even have a widget.*/
	DataOnly UMETA(DisplayName = "Data-Only") 
};

UENUM(BlueprintType)
enum EItemComparison
{
	Price,
};

UENUM(BlueprintType)
enum EComparisonResult
{
	Upgrade,
	Equal,
	Downgrade
};

//What kind of mesh or blueprint is used to represent the item when it is equipped to the player.
UENUM(BlueprintType)
enum EEquipmentMesh
{
	BlueprintActor,
	SkeletalMesh,
	StaticMesh,
};

UENUM(BlueprintType)
enum EContextMenuOptions
{
	Use,
	Open,
	Loot,
	Equip,
	Unequip,
	Drop,
	Destroy,
	Inspect,
	Sell,
	Buy
};

//To be expanded.
UENUM(BlueprintType)
enum ECurrencyTypes
{
	DefaultCurrency
};

UENUM(BlueprintType)
enum EComponentState
{
	Raw,
	Editor,
	Gameplay
};

UENUM(BlueprintType)
enum ERotation
{
	Zero UMETA(DisplayName = "0"), 
	Ninety UMETA(DisplayName = "90"),
	OneEighty UMETA(DisplayName = "180"),
	TwoSeventy UMETA(DisplayName = "270")
};
ENUM_RANGE_BY_FIRST_AND_LAST(ERotation, ERotation::Zero, ERotation::TwoSeventy);


UENUM(BlueprintType)
enum EObjectNetworkingMethod
{
	Both,
	Server,
	Client
};


#pragma endregion

#pragma region Structs

//Struct for associating a value with a tag.
//This is meant to look and behave similar to FGameplayTag,
//but simply with a value added to the struct.
//This struct is customized inside the
//editor module -> FS_TagValue_Customization.h
USTRUCT(BlueprintType)
struct FS_TagValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag")
	float Value = 0;

	FS_TagValue(){}

	bool IsValid() const
	{
		return Tag.IsValid();
	}

	FORCEINLINE bool operator==(const FS_TagValue& Argument) const
	{
		return Tag == Argument.Tag && Value == Argument.Value;
	}

	FORCEINLINE bool operator!=(const FS_TagValue& Argument) const
	{
		return Tag != Argument.Tag || Value != Argument.Value;
	}

	//Allow the tag value to be directly compared with a FGameplayTag
	FORCEINLINE bool operator==(const FGameplayTag& Argument) const
	{
		return Tag == Argument;
	}
	
	FORCEINLINE bool operator!=(const FGameplayTag& Argument) const
	{
		return Tag != Argument;
	}
};


//Settings that are safe to overwrite.
//It is up to you to handle how these settings interact with stackable items.
USTRUCT(BlueprintType)
struct FS_ItemOverwriteSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	//Soft object reference is important as texture files can easily be the largest file for an item.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> InventoryImage = nullptr;

	/**Data assets can have select default currencies. If any entry is added to this,
	 * that list will be ignored and this one will be used instead.
	 * For optimization, don't have the same currency multiple times in here.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(NoElementDuplicate), Category = "Item")
	TArray<UIDA_Currency*> AcceptedCurrenciesOverwrite;

	/**Only relevant if component is set to "Vendor" or "Storage". If -1, it stacks infinitely.
	 * If 0, use data asset max count.*/
	//TODO: Implement and possibly rework this
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 VendorOrStorageMaxStack = 0;
};

USTRUCT(BlueprintType)
struct FS_UniqueID
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ID")
	int32 IdentityNumber = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "ID")
	UAC_Inventory* ParentComponent = nullptr;
	
	bool operator==(const FS_UniqueID& Argument) const
	{
		return IdentityNumber == Argument.IdentityNumber && ParentComponent == Argument.ParentComponent;
	}

	bool operator!=(const FS_UniqueID& Argument) const
	{
		return IdentityNumber != Argument.IdentityNumber || ParentComponent != Argument.ParentComponent;
	}

	bool IsValid() const
	{
		return ParentComponent && IdentityNumber > 0;
	}

	friend uint32 GetTypeHash( const FS_UniqueID& Key )
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(Key.IdentityNumber));
		Hash = HashCombine(Hash, GetTypeHash(Key.ParentComponent));
		return Hash;
	}

	FS_UniqueID(){}

	FS_UniqueID(int32 InIdentityNumber, UAC_Inventory* InParentComponent)
	{
		IdentityNumber = InIdentityNumber;
		ParentComponent = InParentComponent;
	}
};

USTRUCT(BlueprintType)
struct FS_IDMapEntry
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ID")
	bool IsContainer = false;

	/**Directions to the container or item.
	 * If this is a container ID, then Y is -1.*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ID")
	FIntPoint Directions = FIntPoint();

	FS_IDMapEntry(){}

	FS_IDMapEntry(bool InIsContainer, FIntPoint InDirections)
	{
		IsContainer = InIsContainer;
		Directions = InDirections;
	}
};

/**The core fragment used by IFP. Children fragments of this
 * are meant to be shared by both Containers and Items.*/
USTRUCT(BlueprintType)
struct FCoreFragment
{
	GENERATED_BODY()
	virtual ~FCoreFragment() = default;

	/**Attempt to initialize this fragment.
	 * If this fails, you should prevent it from being
	 * added or possibly remove it.
	 *
	 * This is only executed on the server (or client, depending on
	 * if this fragment is client-only) and it passes by ref,
	 * so you can modify the structs freely */
	virtual bool InitializeFragment(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container, const FS_InventoryItem& Item) { return true; }

	//Is this fragment allowed in a container?
	virtual bool AllowedInContainers() { return true; };
	//Is this fragment compatible with the @Container?
	virtual bool IsCompatibleWithContainer(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container)
	{ return true; }

	//Is this fragment allowed in an item?
	virtual bool AllowedInItems() { return true; };
	//Is this fragment compatible with the @Item?
	virtual bool IsCompatibleWithItem(UObject* ItemOwner, const FS_InventoryItem& Item)
	{
		if(GetNetworkingMethod() == Client)
		{
			if(UKismetSystemLibrary::IsDedicatedServer(ItemOwner))
			{
				return false;
			}
		}
		else if(GetNetworkingMethod() == Server)
		{
			if(!UKismetSystemLibrary::IsServer(ItemOwner))
			{
				return false;
			}
		}

		return true;
	}

	/**Is this fragment allowed in the ItemAsset -> ItemAssetFragments array?
	 * Each fragment that wants to support this will need the code that
	 * uses that array to reflect this possibility.
	 *
	 * For example; The tag fragment is allowed in both and all code revolving
	 * the tag fragment takes this into account.
	 * The ItemOverrideSettings fragment makes no sense to be placed in the
	 * item asset fragments array, so none of the code using that fragment
	 * takes this scenario into account.*/
	virtual bool SupportsItemAsset() { return false; }

	/**After the fragment has been modified, you should ask
	 * the fragment this question. If true, you should attempt
	 * to remove the fragment.
	 * For example; The Tags fragment removes itself if the
	 * item has no tags and no tag values remaining.*/
	virtual bool ShouldRemoveFragment(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container,
	                                  const FS_InventoryItem& Item)
	{ return false;	}

	/**Merge this fragment with another.
	 * NOTE: This is temporarily unused. For some reason, this keeps encountering
	 * errors about double instantiation around TInstancedStruct<FCoreFragment>
	 * when packaging for Mac... It's always Mac's who have odd errors...*/
	virtual void MergeWithFragment(TInstancedStruct<FCoreFragment> OtherFragment) {}

	/**What networking method should this fragment fall under?
	 * NOTE: This will be implemented in 3.1*/
	virtual EObjectNetworkingMethod GetNetworkingMethod()
	{ return EObjectNetworkingMethod::Both;	}

	/**When we press "Validate Fragments", what error messages should
	 * pop up?
	 * Technically, this is supposed to be surrounded with an
	 * "#if WITHEDITOR", but to reduce the amount of people accidentally
	 * forgetting and getting packaging errors, I'm going to leave this
	 * as a normal function.*/
	virtual TArray<FString> GetEditorErrorMessage()
	{ return TArray<FString>{};	}


#if WITH_EDITOR

	virtual void PostEditorPropertyChange(FPropertyChangedEvent& PropertyChangedEvent) {}

#endif

	
};

/**The base fragment for fragments meant to go on only items.*/
USTRUCT(BlueprintType)
struct FItemFragment : public FCoreFragment
{
	GENERATED_BODY()

	virtual bool AllowedInContainers() override { return false; }

	virtual bool
	IsCompatibleWithContainer(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container) override
	{
		return false;
	}
};

/**The base fragment for fragments meant to go on only containers.*/
USTRUCT(BlueprintType)
struct FContainerFragment : public FCoreFragment
{
	GENERATED_BODY()

	virtual bool AllowedInItems() override { return false; }

	virtual bool IsCompatibleWithItem(UObject* ItemOwner, const FS_InventoryItem& Item) override
	{
		return false;
	}
};

//Sub struct for Container settings. Declares what items are in your inventory and where they are and their settings.
USTRUCT(BlueprintType)
struct FS_InventoryItem
{
	GENERATED_BODY()

	/**The item asset to use for this item.
	 *
	 * NOTE: If this struct is inside a ContainerSettings struct that is
	 * inside an inventory component, this will automatically filter all
	 * assets that do NOT fit the containers CompatibilitySettings.
	 * Every time you update those settings, you need to press compile
	 * unless you are editing an actor in the level.
	 *
	 * To disable this, you can go into the
	 * project settings -> InventoryFrameworkPlugin and disable
	 * LimitItemSelectionToCompabilityCheck */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", DisplayName = "Item Asset", meta = (PinHiddenByDefault))
	TSoftObjectPtr<UDA_CoreItem> ItemAssetSoftReference = nullptr;

	/**The hard reference of the item asset for this item.
	 *
	 * This is only populated once the soft reference has been loaded.
	 * Technically, this variable doesn't need to exist, but it exists
	 * for 2 reasons:
	 * 1. Legacy code: Before IFP 3, the soft reference didn't exist.
	 * Removing this would make a lot of developers have to update a lot
	 * of code.
	 * 2. Simpler management and cleaner code. You should rarely ever
	 * have to be thinking "is this asset loaded?" and constantly
	 * trying to load the asset. If this property is null when you
	 * expect it to be filled, then something has gone wrong.
	 * The end code should have the programmer or designer never having
	 * to think about hard or soft references and if they do need to,
	 * then they have access to the soft reference.*/
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	UDA_CoreItem* ItemAsset = nullptr;

	//Technically this isn't needed. There are ways for getting an items container index but it can get expensive
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	int32 ContainerIndex = -1;
	
	//Same story as ContainerIndex comment. This just makes look-ups more efficient.
	UPROPERTY(BlueprintReadWrite, Category = "Item")
    int32 ItemIndex = -1;

	/**The top left tile index of this item.
	 * If set to -1, the component will find the nearest available spot.
	 * If set to -2, the component will try to find a random tile.
	 * If set to 0 or any other valid tile index, the item will try to spawn there.
	 *
	 * IMPORTANT: If you are using custom shaped items, you should NOT use this in
	 * combination with the tilemap when inside a grid container.
	 * The system only hides an items tile to "create" the custom shape. They are
	 * still there under the hood.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "ContainerSupportsTileMap", EditConditionHides))
	int32 TileIndex = -1;
	
	//TODO: Add option for random rotation. Add through a tag?
	/**How is the item rotated? This does nothing for non-spacial containers.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "CanRotate", EditConditionHides, PinHiddenByDefault))
	TEnumAsByte<ERotation> Rotation = Zero;

	/**If this item is stackable, it'll generate a random count using this.
	 * X is min, Y is max. If both are equal, count will be set to that amount.
	 * If both are -1, it'll use the items default stack.
	 * If either are -2, it'll use the normal count, which is set during gameplay.*/
	UPROPERTY(meta = (EditCondition = "IsStackable", EditConditionHides, DeprecatedProperty = "Deprecated, replaced with the RandomItemCount fragment"))
	FIntPoint RandomMinMaxCount_DEPRECATED = FIntPoint(-1, -1);

	/**Set by component using RandomMinMaxCount.
	 * If the item can't stack, this is automatically set to 1.*/
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	int32 Count = 0;

	/**Simple tags to associated with this item.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	FGameplayTagContainer Tags_DEPRECATED;

	/**Tags that can have values associated with them.
	 * For example, you might have a durability tag and want to give it a value.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	TArray<FS_TagValue> TagValues_DEPRECATED;

	/**Not exposed as this is only supposed to be set by the component.
	 * This helps the system identify items.
	 * An entire component can not have 2 identical UniqueID's.
	 * As of 5.1, there is a issue where any UPROPERTY that isn't set to visible
	 * won't get saved on actors when you save a level (if an actor is set to editor
	 * state, then the user swaps levels, this will get reset and the items containers
	 * will be forgotten).
	 * For now, this is being left to being visible on instances.*/
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "Item")
	FS_UniqueID UniqueID;

	/**The component and widgets have been designed to accept these override settings. This lets you create unique items without creating a whole new item asset.
	 * Also allows players to customize items, such as renaming them.
	 * Data from Data Assets can only be read, never written, so if there are any settings in your data assets
	 * you want to modify, you will have to add them to this struct or somewhere else and design your system to accept those overrides.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the OverrideSettings Fragment"))
	FS_ItemOverwriteSettings OverrideSettings_DEPRECATED;

	/**Quick access to all components that are associated with this item.
	 * IMPORTANT: If a components replication is limited to either server or client,
	 * then this array will only include the Components that are relevant.
	 * For @Both, both clients and server will see the Components (Client might be delayed
	 * because of replication).
	 * For @Server, only the server will see the Component.
	 * For @Client, only the client will see the Component.
	 *
	 * This behavior can lead to confusing moments where you are trying to
	 * verify if an item Component is being assigned here. Remember to check
	 * your networking construction settings for the Component.
	 * Keep in mind, this list is maintained through RepNotifies, so clients
	 * aren't always in sync. They will *eventually*, but in general
	 * the client version should not be trusted for Components that aren't
	 * set to @Client.*/
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	TArray<UItemComponent*> ItemComponents;

	/**Quick access to any objects that this item can send updates to.
	 * The goal of this array is for you to add any widgets or objects,
	 * such as a hot bar or quest tracker to this item, then
	 * whenever this item is updated in some way, such as a tag or count being
	 * updated, the I_ExternalObjects interface will call specific functions
	 * on these external objects to alert them.
	 * There is no default logic that populates this array, you must add and remove
	 * objects from this array yourself.
	 * The example project uses the tooltip as an external object to update
	 * in real time for any tag value changes, such as the durability
	 * bar on the gun item.
	 *
	 * It is recommended to use the GetObjectsForItemBroadcast helper function
	 * rather than using this.
	 * This is NOT replicated for 2 reasons:
	 * 1. Minimize RPC size
	 * 2. Most objects in here are widgets, which are client only. So when a client
	 * receives an item struct from the server, it won't include widgets, because
	 * widgets can't exist on a server, so we need to fetch the client version of
	 * this array. Since that is extremely cheap to do, it's simpler to just do
	 * that instead of sending a bigger RPC.*/
	UPROPERTY(BlueprintReadWrite, NotReplicated, Category = "Item")
	TArray<TObjectPtr<UObject>> ExternalObjects;

	/**Direct reference to the item widget.
	 * It is still advised to use the GetWidgetForItem function.*/
	UPROPERTY(BlueprintReadWrite, NotReplicated, Category = "Item", meta = (PinHiddenByDefault))
	UW_InventoryItem* Widget = nullptr;

	/**Fragments are instancable structs, allowing you to add and remove any
	 * data from this struct, even during runtime.
	 *
	 * Docs:
	 * https://inventoryframework.github.io/systems/fragments/ */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item", NoClear,
		meta = (PinHiddenByDefault, ShowTreeView, DisallowedClasses = "/Script/InventoryFrameworkPlugin.ContainerFragment"))
	TArray<TInstancedStruct<FCoreFragment>> ItemFragments;

	/**The object that is representing this item (if any) to perform gameplay
	 * logic, acting as a type of "item actor" without being an actor.
	 * Direct retrieval is discouraged. Use GetItemsInstance to retrieve this.
	 *
	 * This property has 2 rules, which can be modified in the IFP project settings:
	 * 1. "Wild" ItemInstances are not allowed. This refers to items which have no
	 * default ItemInstance assigned in the item asset, but you still want to add
	 * one to the item struct. It is highly advised to keep this rule enabled to
	 * prevent random items having random item instances as keeping track of this
	 * is extremely difficult in medium to large projects.
	 *
	 * 2. You are not allowed to select ItemInstance's that do not match the
	 * same class that was assigned in the item asset. This is to prevent two
	 * identical items having two different ItemInstance classes, leading to
	 * odd behavior.
	 *
	 * I "could" modify the dropdown to filter the available options to just the
	 * one selected in the ItemAsset, but instanced properties are very complex,
	 * and I'd rather work on other features than make this "slightly
	 * more professional".
	 * This is handled through
	 * UFL_InventoryFramework::ProcessContainerAndItemCustomizations
	 */
	UPROPERTY(Category = "Item", BlueprintReadWrite, EditAnywhere, SaveGame, Instanced, meta = (PinHiddenByDefault, EditCondition = "AllowItemInstance", EditConditionHides))
	UItemInstance* ItemInstance = nullptr;

#if WITH_EDITORONLY_DATA

	/**This is only used to give this struct a prettier name in the editor.
	 * This data is not cooked. This is automatically assigned to be the same
	 * as Item->ItemName. This has no impact on gameplay or programming.
	 * This logic is handled in AC_Inventory->PostEditChangeProperty.
	 * If you want, you can go into that function and add any data your
	 * team wants to add to the structs title, for example the count
	 * or if any of the settings had been overriden.
	 * For this to work, this UProperty MUST be visible in some way,
	 * otherwise the editor does not load the property and will fail.
	 * This property is then hidden inside
	 * FS_InventoryItem_Customization.cpp in the editor module.*/
	UPROPERTY(VisibleInstanceOnly, NotReplicated, Category = "Item")
	FString ItemEditorName = FString("No item available");

	//Used to hide @min/maxCount and @OverrideSettings from the editor. Do not use for gameplay
	UPROPERTY(VisibleInstanceOnly, NotReplicated, Category = "Item")
	bool IsStackable = false;

	//Used to hide @Rotation from the editor. Do not use for gameplay
	UPROPERTY(VisibleInstanceOnly, NotReplicated, Category = "Item")
	bool CanRotate = true;

	/**Used to control whether TileIndex is visible*/
	UPROPERTY(VisibleInstanceOnly, NotReplicated, Category = "Item")
	bool ContainerSupportsTileMap = true;

	UPROPERTY(VisibleInstanceOnly, NotReplicated, Category = "Item")
	bool AllowItemInstance = true;

#endif

	//Bool operators so you can compare 2 structs with each other
	
	//Never compare an item that hasn't had a UniqueID assigned to it.
	bool operator==(const FS_InventoryItem& Argument) const
	{
		return UniqueID == Argument.UniqueID && ItemAsset == Argument.ItemAsset && ItemIndex == Argument.ItemIndex && ContainerIndex == Argument.ContainerIndex && TileIndex == Argument.TileIndex && Count == Argument.Count;
	}

	bool operator!=(const FS_InventoryItem& Argument) const
	{
		return UniqueID != Argument.UniqueID || ItemAsset != Argument.ItemAsset || ItemIndex != Argument.ItemIndex || ContainerIndex != Argument.ContainerIndex || TileIndex != Argument.TileIndex || Count != Argument.Count;
	}

	bool IsValid() const
	{
		if(!ItemAsset || ItemIndex < 0 || ContainerIndex < 0 || Count < 0 || !UniqueID.ParentComponent || UniqueID.IdentityNumber == 0)
		{
			return false;
		}
		return true;
	}

	UAC_Inventory* ParentComponent() const
	{
		return UniqueID.ParentComponent;
	}
};

//Settings for what is allowed in a container.
USTRUCT(BlueprintType)
struct FS_CompatibilitySettings
{
	GENERATED_BODY()

	/**What tags must an item have for it to be allowed in this container?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer RequiredTags;

	/**If an item has any of these tags, they are not allowed in this container.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer BlockingTags;

	/**What item types are allowed? These are declared in the item asset.
	 * This will also check children tags, so adding Item.Type.Weapon
	 * will allow any item with the type tag Item.Type.Weapon.Gun and
	 * any other children tags of the tags you add here.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer ItemTagTypes;

	//Only these items are allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	TArray<UDA_CoreItem*> ItemWhitelist;

	//These items are not allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	TArray<UDA_CoreItem*> ItemBlacklist;

	int32 GetMemorySize() const
	{
		return RequiredTags.GetGameplayTagArray().GetAllocatedSize() + BlockingTags.GetGameplayTagArray().GetAllocatedSize()
		 + ItemWhitelist.GetAllocatedSize() + ItemBlacklist.GetAllocatedSize();
	}
};

/**Used for "GetAllItemsAssociatedWithItem" function and quick loot widget
 * to indicate if an item is inside of an item.*/
USTRUCT(BlueprintType)
struct FS_ItemSubLevel
{
	GENERATED_BODY()

	/**This lets you know if Item is attached to something.
	 * For example, if you have a backpack with money, a gun and a helmet inside of it and that gun has a sight,
	 * GetAllItemsAssociatedWithItem will return the list like this:
	 * money level 0, gun level 0, sight level 1, helmet level 0.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sublevel")
	int32 SubLevel = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sublevel")
	FS_InventoryItem Item;
};

UENUM(BlueprintType)
enum EContainerInfinityDirection
{
	Neither,
	X,
	Y
};

USTRUCT(BlueprintType)
struct FS_TileTag
{
	GENERATED_BODY()

	FORCEINLINE FS_TileTag();

	explicit FORCEINLINE FS_TileTag(int32 TileIndex, FGameplayTagContainer Tags);

	//What tile should the tags be applied to?
	UPROPERTY(Category = "Tile", EditAnywhere, BlueprintReadWrite)
	int32 TileIndex = 0;

	//The tags to apply to the tile.
	UPROPERTY(Category = "Tile", EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer Tags;
};

FORCEINLINE FS_TileTag::FS_TileTag()
{
}

FORCEINLINE FS_TileTag::FS_TileTag(const int32 InTileIndex, const FGameplayTagContainer InTags)
{
	TileIndex = InTileIndex;
	Tags = InTags;
} 

//General settings for the container, plus all items in the container.
USTRUCT(BlueprintType)
struct FS_ContainerSettings
{
	GENERATED_BODY()

	//Not exposed because it's set by the component. Used to keep track of what index this container is inside the array.
	UPROPERTY(BlueprintReadWrite, Category = "Container")
	int32 ContainerIndex = -1;

	/**A tag used to identify containers inside the owning actor or item. This is meant to replace the ContainerName from older versions.
	 * Two containers should not use the same identifier, as this identifier is used in several places to get the correct container.
	 * This does NOT mean that two containers from two actors should never match. The system also checks @BelongsToItem directions
	 * to make sure the correct container (even if multiple containers with the same identifier) is found.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ContainerType != EContainerType::ThisActor", EditConditionHides), Category = "Container")
	FGameplayTag ContainerIdentifier;

	/**The type of container this is, Inventory is your standard inventory, Equipment means this only accepts equipment
	 * and the dimensions will always default to 1x1. ThisActor refers to pickups, index 0 of both ContainerSettings
	 * and that indexes Items array is the pickup data for this actor.
	 * So for pickups, you'd add 1 container, set this to ThisActor, then add 1 item to that container and the data 
	 * inside there is your actual pickup data for the actor.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TEnumAsByte<EContainerType> ContainerType = Inventory;

	/** @Grid This container will behave like a spacial inventory.
	 * @Traditional This container will behave like a traditional inventory, where every
	 * item is 1x1, essentially disabling all spatial capabilities.
	 * @DataOnly This container will not bother with any kind of collision checks,
	 * handling capacity or handling dimensions. This style has very few limitations.
	 * This is primarily used when you want to store items *somewhere* the player
	 * might or might not see it, or if your UI doesn't require any kind of
	 * traditional container. This style has the least default implementation
	 * and is meant to be customized. This style by default will always be
	 * infinite and host as many items as you want, since it doesn't
	 * have any concept of dimensions or constraints.
	 * In many scenarios, these types of containers won't even have a widget.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ContainerType != EContainerType::ThisActor", EditConditionHides), Category = "Container")
	TEnumAsByte<EContainerStyle> Style = Grid;

	/**Which direction does the container attempt to expand infinitely?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ContainerType != EContainerType::ThisActor && Style != EContainerStyle::DataOnly", EditConditionHides), Category = "Container")
	TEnumAsByte<EContainerInfinityDirection> InfinityDirection = Neither;

	/**X and Y of your container, this overwrites whatever setting you have made in the container widget.
	 * If this container is infinite, the initial X or Y for this is not important, but during runtime
	 * the appropriate X or Y is dynamically lowered and increased depending on the sizing requirements
	 * of the items inside this container.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ContainerType != EContainerType::ThisActor && Style != EContainerStyle::DataOnly && ContainerType != EContainerType::Equipment", EditConditionHides), Category = "Container")
	FIntPoint Dimensions = FIntPoint(1, 1);

	/**Simple tags to associated with this container.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with TagFragment"))
	FGameplayTagContainer Tags_DEPRECATED;

	/**Tags that can have values associated with them.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with TagFragment"))
	TArray<FS_TagValue> TagValues_DEPRECATED;

	//Your rules for what is allowed in the container.
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with CompatibilitySettingsFragment"))
	FS_CompatibilitySettings CompatibilitySettings_DEPRECATED;

	/**What tags should be applied to specific tiles?
	 * This can be used to apply properties or flags,
	 * such as "Hidden" or "Locked" to specific tiles.
	 *
	 * This is currently a prototype, does not support
	 * runtime changes for multiplayer for now.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with TileTagsFragment"))
	TArray<FS_TileTag> TileTags_DEPRECATED;

	/**X refers to the index of the containerSettings array and Y refers to which item in said ContainerSettings ultimately owns this container (Item must have a attachment widget).
	 * This in essence allows the component to have "infinite" containers.
	 * Once all items have been initialized, X is updated to the containers UniqueID and Y is updated to the items UniqueID, so it's easier to find an items
	 * containers once items have been sorted and moved.
	 *
	 * If either are -1, it means this container belongs to this actor, it does NOT belong to an item.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ContainerType != EContainerType::ThisActor", EditConditionHides), Category = "Container")
	FIntPoint BelongsToItem = FIntPoint(-1, -1);

	/**Fragments are instancable structs, allowing you to add and remove any
	 * data from this struct, even during runtime.
	 *
	 * Docs:
	 * https://inventoryframework.github.io/systems/fragments/ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear,
		meta = (ExcludeBaseStruct, ShowTreeView, DisallowedClasses = "/Script/InventoryFrameworkPlugin.ItemFragment"), Category = "Container")
	TArray<TInstancedStruct<FCoreFragment>> ContainerFragments;

	//To-do Find a way to have TitleProperty read the items ItemName FText variable.
	/**The items in this container. Picking things up and similar automatically adds to this array. Any items added before runtime are simply used as default on-spawn items.
	 * The settings in this struct are pretty self explanatory and have comments if you hover over them.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "ItemEditorName"), Category = "Container")
	TArray<FS_InventoryItem> Items;

	/**Some functions need to find out if a tile is occupied before widgets are valid,
	 * so we keep a list of all tiles and what items are occupying those tiles to optimize.
	 * The way this list works is that each index represents a tile (index 15 is tile 15, index 43 is tile 43, etc)
	 * Then the array element represents the UniqueID of the item inside ContainerSettings.Items that is occupying that tile.
	 * This is the lightest and fastest method to achieve this. The alternative would be to go through
	 * every container and their items, get their tile index and their dimensions, and return a list of all the tiles
	 * they are occupying, but that math is heavier than to just check this list, but that method
	 * might be lighter on memory.*/
	UPROPERTY(BlueprintReadWrite, Category = "Container")
	TArray<int32> TileMap;

	/**Converting an index to a tile isn't expensive, but with how frequently it is used, it can end up being very expensive.
	 * This is generated for free when the tile map is created and allows us to convert an index to a tile virtually
	 * instantly, doing no math at all.*/
	UPROPERTY(BlueprintReadWrite, NotReplicated, Category = "Container", meta = (PinHiddenByDefault))
	TMap<FIntPoint, int32> IndexCoordinates;

	/**While we do try our best to keep the ContainerSettings and ContainerWidgets in parity and same size,
	 * There are moments where you want to wipe out a container while keeping other containers, which
	 * would disrupt this parity. To fix this, we assign containers a uniqueID so containers
	 * can be removed and mixed around with less friction.
	 * As of 5.1, there is a issue where any UPROPERTY that isn't set to visible
	 * won't get saved on actors when you save a level (if an actor is set to editor
	 * state, then the user swaps levels, this will get reset and the items containers
	 * will be forgotten).
	 * For now, this is being left to being visible on instances.*/
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "Container")
	FS_UniqueID UniqueID;
	
	/**Quick access to any objects that this container can send updates to.
	 * The goal of this array is for you to add any widgets or objects,
	 * such as a hot bar or quest tracker to this container, then
	 * whenever this item is updated in some way, such as a tag or equipment being
	 * updated, the I_ExternalObjects interface will call specific functions
	 * on these external objects to alert them.
	 * There is no default logic that populates this array, you must add and remove
	 * objects from this array yourself.
	 *
	 * It is recommended to use the GetObjectsForContainerBroadcast helper function
	 * rather than using this.
	 * This is NOT replicated for 2 reasons:
	 * 1. Minimize RPC size
	 * 2. Most objects in here are widgets, which are client only. So when a client
	 * receives an item struct from the server, it won't include widgets, because
	 * widgets can't exist on a server, so we need to fetch the client version of
	 * this array. Since that is extremely cheap to do, it's simpler to just do
	 * that instead of sending a bigger RPC.*/
	UPROPERTY(BlueprintReadWrite, NotReplicated, Category = "Container")
	TArray<TObjectPtr<UObject>> ExternalObjects;

	/**Direct reference to the container widget.
	 * It is still advised to use GetWidgetForContainer.*/
	UPROPERTY(BlueprintReadWrite, NotReplicated, Category = "Container", meta = (PinHiddenByDefault))
	UW_Container* Widget = nullptr;

	//Can items have a unique shape? If false, items are bound to 1x1
	bool IsSpacialContainer() const
	{
		return (Style == Grid || Style == DataOnly) && ContainerType != Equipment;
	}

	/**Do the style settings support spacial settings? If false, items are bound to 1x1*/
	bool IsSpacialStyle() const
	{
		/**V: This is kind of silly, spacial should just be grids,
		 * the reason why DataOnly is being considered for Spacial is to
		 * is so items inside data only containers retain any dimensions
		 * and rotation they had before going into the data only container.
		 * I don't like this. Me angry. But this makes cleaner code
		 * and much less refactoring, so less angry, but still angry.*/
		return Style == Grid || Style == DataOnly;
	}
	
	bool IsInfinite() const
	{
		if(Dimensions.X < 0 || Dimensions.Y < 0)
		{
			//Either directions are negative, either the container has not been initialized
			//or has been set up improperly.
			return false;
		}

		/**Container isn't set to Neither, but it is set to DataOnly,
		 * which doesn't support capacity, so it is technically infinite.*/ 
		if(InfinityDirection != Neither || Style == DataOnly)
		{
			return true;
		}
		
		return false;
	}

	/**Should this container support the tilemap system?
	 * If not, then this container will not have any type of
	 * collision system.*/
	bool SupportsTileMap() const
	{
		return Style != DataOnly;
	}

	/**yeah nah cba filling out the rest. This should be sufficient, only scenario I can see this
	 * not being enough is if the component has not initialized correctly, which at that point
	 * someone has done something else wrong already.*/
	bool operator==(const FS_ContainerSettings& Argument) const
	{
		if(ContainerIdentifier.IsValid() && Argument.ContainerIdentifier.IsValid())
		{
			return UniqueID == Argument.UniqueID && ContainerIdentifier.MatchesTagExact(Argument.ContainerIdentifier) && ContainerIndex == Argument.ContainerIndex;
		}
		
		return UniqueID == Argument.UniqueID && ContainerIndex == Argument.ContainerIndex;
	}

	bool operator!=(const FS_ContainerSettings& Argument) const
	{
		if(ContainerIdentifier.IsValid() && Argument.ContainerIdentifier.IsValid())
		{
			return UniqueID != Argument.UniqueID || ContainerIdentifier.MatchesTagExact(Argument.ContainerIdentifier) || ContainerIndex != Argument.ContainerIndex;
		}
		
		return UniqueID != Argument.UniqueID || ContainerIndex != Argument.ContainerIndex;
	}

	bool IsValid() const
	{
		if(!UniqueID.ParentComponent)
		{
			return false;
		}

		return true;
	}

	UAC_Inventory* ParentComponent() const
	{
		return UniqueID.ParentComponent;
	}
};

/**Helper struct used by MoveItem*/
USTRUCT(BlueprintType)
struct FS_ItemAndContainers
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	bool bIsRootItem = false;

	//The item itself.
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	FS_InventoryItem Item;

	//The items containers.
	UPROPERTY(BlueprintReadWrite, Category = "Container")
	TArray<FS_ContainerSettings> Containers;
};

/**Used by AC_Inventory -> GetListOfItemsByCount*/
USTRUCT(BlueprintType)
struct FS_ItemCount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
	FS_InventoryItem Item;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
	int32 Count = 0;
};

UENUM(BlueprintType)
enum EEquipmentTagSelection
{
	Notify,
	StartOfComponent,
	EndOfComponent
};

USTRUCT(BlueprintType)
struct FS_EquipmentTagInformation
{
	GENERATED_BODY()

	/**When should the tags below be added to/removed from the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TEnumAsByte<EEquipmentTagSelection> WhenToProcess = EndOfComponent;

	/**When this struct is processed, what tags to we add to/remove from the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "WhenToProcess == EEquipmentTagSelection::Notify", EditConditionHides), Category = "Information")
	FName MontageNotifyName;
};

USTRUCT(BlueprintType)
struct FS_EquipInformation
{
	GENERATED_BODY()

	/**When you are equipping an item, you might want to filter your results to something like "Primary"
	 * or "Secondary". This allows you to filter your results down to a specific result, allowing you
	 * to have a lot of EquipInformation in EquipmentData for a lot of scenarios.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FName> TriggerFilter;
	
	/**If this is NOT empty, all information below will only trigger once a montage notify with this name occurs.
	 * If it is empty, this will trigger instantly.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName WaitForNotify;

	/**Whenever an item is unequipped, it will search all components that belong to the item,
	 * and then the owner until it finds the first scene component with this FName Tag,
	 * then attempts to play the animation with that skeletal mesh.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName PlayAnimationOnMeshWithTag;

	/**The animation to play. If empty, the logic will occur instantly*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TSoftObjectPtr<UAnimMontage> EquipAnimation;

	/**If true, we do not bother to attach the item to anything. This is useful for items
	 * you do not want to have some sort of physical representation when equipped, but
	 * still want an animation to play on a specific mesh. For example when unholstering
	 * a gun, you might want to play an animation on the gun after it has been spawned.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	bool DoNotEquipToMesh = false;

	/**What notify inside the montage are we waiting for to unequip the item?
	 * If montage is empty, this should also be empty.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!DoNotEquipToMesh", EditConditionHides), Category = "Information")
	FName EquipItemOnNotify;
	
	/**Whenever an item is equipped, it will search all components that this item is attached to,
	 * (for example, if this item is a sight and it's attached to a gun, go through the guns components first)
	 * and then the owning actor until it finds the first component with this tag to attach onto.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!DoNotEquipToMesh", EditConditionHides), Category = "Information")
	FName EquipAttachToComponentWithTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!DoNotEquipToMesh", EditConditionHides), Category = "Information")
	FName EquipSocket;
	
	/**Transform to apply to the equipped items local.
	 * This is for prototyping or if you have multiple assets made by
	 * different artists who set the root of the item in slightly different positions
	 * without making a ton of EquipSockets. Ideally you should not need this.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!DoNotEquipToMesh", EditConditionHides), Category = "Information")
	FTransform EquipTransform;

	/**What component with this tag should automatically be assigned as the
	 * leader pose for this equipment? This is only used on skeletal mesh equipment.
	 * If you want it to work on blueprints, you will have to implement that logic
	 * yourself in the equipment manager component.
	 * Remember, leader pose equipment should always be attached to same component
	 * as the skeleton you are assigning as the leader (most of the time, the root
	 * component) and have no animation blueprint playing.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!DoNotEquipToMesh", EditConditionHides), DisplayName = "Leader Pose Tag (Skeleton equipment only)", Category = "Information")
	FName LeaderPoseTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	bool RemovesItemFromNetworkQueue = false;

	/**A custom ID appended to the end of the spawned mesh through this specific
	 * equipment information. This helps with dual mesh setups.
	 * For example, you could have 2 equip information, one for first person
	 * and one for third person. For the Unequip information, it can't tell
	 * which mesh you would want to remove.
	 * You can then fill this out as "FirstPerson" and the other as "ThirdPerson"
	 * to help further filter out what both the equip and unequip information
	 * should target.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName CustomID;

	/**When the Component is finished, what tags to we add to the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FS_EquipmentTagInformation> TagsToAdd;
	
	/**When the Component is finished, what tags to we remove from the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FS_EquipmentTagInformation> TagsToRemove;
};

UENUM(BlueprintType)
enum ERemovalPolicy
{
	Ignore,
	AssignNewAttachLocation,
	Invisible,
	DESTROY UMETA(DisplayName = "Destroy")
};

USTRUCT(BlueprintType)
struct FS_UnequipInformation
{
	GENERATED_BODY()

	/**When you are unequipping an item, you might want to filter your results to something like "Primary"
	 * or "Secondary". This allows you to filter your results down to a specific result, allowing you
	 * to have a lot of UnequipInformation in EquipmentData for a lot of scenarios.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FName> TriggerFilter;

	/**If this is NOT empty, all information below will only trigger once a montage notify
	 * with this name occurs.
	 * If it is empty, this will trigger instantly.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName WaitForNotify;
	
	/**Whenever an item is unequipped, it will search all components that belong to the item,
	 * and then the owner until it finds the first scene component with this FName Tag,
	 * then attempts to play the animation with that skeletal mesh.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName PlayAnimationOnMeshWithTag;

	/**The animation to play. If empty, the logic will occur instantly*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TSoftObjectPtr<UAnimMontage> UnequipAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TEnumAsByte<ERemovalPolicy> RemovalPolicy = Ignore;
	
	/**Whenever an item is equipped, it will search all components that this item is attached to,
	 * (for example, if this item is a sight and it's attached to a gun, go through the guns components first)
	 * and then the owning actor until it finds the first component with this tag to attach onto.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RemovalPolicy == ERemovalPolicy::AssignNewAttachLocation", EditConditionHides), Category = "Information")
	FName UnequipAttachToComponentWithTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RemovalPolicy == ERemovalPolicy::AssignNewAttachLocation", EditConditionHides), Category = "Information")
	FName UnequipSocket;

	/**Transform to apply to the unequipped items local.
	 * This is for prototyping or if you have multiple assets made by
	 * different artists who set the root of the item in slightly different positions
	 * without making a ton of EquipSockets. Ideally you should not need this.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RemovalPolicy == ERemovalPolicy::AssignNewAttachLocation", EditConditionHides), Category = "Information")
	FTransform UnequipTransform;

	/**What notify inside the montage are we waiting for to unequip the item?
	 * If montage is empty, this should also be empty.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName UnequipItemOnNotify;

	/**Items are automatically added to the network queue whenever you equip or unequip an item.
	 * When the montage finishes playing, it'll remove the item from the network queue.
	 * If you have multiple montages playing at the same time, you should only check true
	 * on one of them, and it should always be for the montage that finishes last.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	bool RemovesItemFromNetworkQueue = false;

	/**A custom ID appended to the end of the spawned mesh through this specific
	 * equipment information. This helps with dual mesh setups.
	 * For example, you could have 2 equip information, one for first person
	 * and one for third person. For the Unequip information, it can't tell
	 * which mesh you would want to remove.
	 * You can then fill this out as "FirstPerson" and the other as "ThirdPerson"
	 * to help further filter out what both the equip and unequip information
	 * should target.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	FName CustomID;

	/**When the Component is finished, what tags to we add to the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FS_EquipmentTagInformation> TagsToAdd;

	/**When the Component is finished, what tags to we remove from the item?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Information")
	TArray<FS_EquipmentTagInformation> TagsToRemove;
};

USTRUCT(BlueprintType)
struct FS_EquipmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment Settings")
	TEnumAsByte<EEquipmentMesh> EquipmentMesh = BlueprintActor;

	/*It is normal to have the same blueprint as the PhysicalActor blueprint here, but if the construction cost is too high or any begin play
	logic is getting too complicated, you could make a dummy blueprint that looks the same as the PhysicalActor but with less components so
	it's cheaper to construct.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment Settings", meta=(EditCondition="EquipmentMesh == EEquipmentMesh::BlueprintActor", EditConditionHides))
	TSubclassOf<class AActor> Blueprint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment Settings", meta=(EditCondition="EquipmentMesh == EEquipmentMesh::SkeletalMesh", EditConditionHides))
	USkeletalMeshComponent *Skeleton = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment Settings", meta=(EditCondition="EquipmentMesh == EEquipmentMesh::StaticMesh", EditConditionHides))
	UStaticMeshComponent *Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "TriggerFilter"), Category = "Equipment Settings")
	TArray<FS_EquipInformation> EquipInformation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "TriggerFilter"), Category = "Equipment Settings")
	TArray<FS_UnequipInformation> UnequipInformation;
};

USTRUCT(Blueprintable)
struct FS_VoidItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Void")
	FS_InventoryItem Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Void")
	FVector2D Location = FVector2D(0, 0);
};

//Helper struct for pairing a shape with a rotation.
//Utilized by GetFirstAvailableTile.
USTRUCT(Blueprintable)
struct FRotationAndShape
{
	GENERATED_BODY()

	UPROPERTY(Category = "RotationAndShape", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ERotation> Rotation = Zero;
	
	UPROPERTY(Category = "RotationAndShape", EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> Shape;
	
	FRotationAndShape(){}

	FRotationAndShape(TEnumAsByte<ERotation> InRotation, TArray<FIntPoint> InShape)
	{
		Rotation = InRotation;
		Shape = InShape;
	}
};

#pragma endregion
