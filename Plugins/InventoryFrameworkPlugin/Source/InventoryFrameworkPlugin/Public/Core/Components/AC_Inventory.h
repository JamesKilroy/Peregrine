// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/ac_inventory/


#pragma once

#include "CoreMinimal.h"
#include "AC_FragmentManager.h"
#include "ItemComponent.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NativeGameplayTags.h"
#include "Components/ActorComponent.h"
#include "Core/Data/Async_InventoryFunctions.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Objects/Parents/O_TagValueCalculation.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h" //Why is this suddenly required in 5.4.3 to package IFP?
#include "Engine/EngineTypes.h"
#include "AC_Inventory.generated.h"

struct FItemOverrideSettings;
class UAC_FragmentManager;
class UIC_ItemAbility;
struct FItemComponentPayload;
class UW_Drag;
class UInputMappingContext;
class UItemInstance;
class UItemComponent;
class UW_InventoryItem;
class UAC_Inventory;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_SkipValidation)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_IncludeLootTables)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_SpawnChanceValue)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_PriceOverrideValue)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(IFP_MaxStackOverride)


#pragma region DelegateDeclerations

//////////////////////////////////////////////////////////////////////////////////////

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComponentStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComponentStopped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComponentPreStop);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FItemAdded, FS_InventoryItem, ItemData, int32, ToIndex, FS_ContainerSettings, ToContainer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemRemoved, FS_InventoryItem, ItemData, FS_ContainerSettings, FromContainer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FItemCountUpdated, FS_InventoryItem, ItemData, int32, OldCount, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemFailedSpawn, FS_InventoryItem, ItemData, FString, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemEquipped, FS_InventoryItem, ItemData, const TArray<FName>&, CustomTriggerFilters);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FItemUnequipped, FS_InventoryItem, ItemData, FS_InventoryItem, OldItemData, const TArray<FName>&, CustomTriggerFilters);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrencyAdditionFailed, UIDA_Currency*, Currency, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FItemMoved, FS_InventoryItem, OriginalItemData, FS_InventoryItem, NewItemData, FS_ContainerSettings, FromContainer, FS_ContainerSettings, ToContainer,
	UAC_Inventory*, FromComponent, UAC_Inventory*, ToComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContainerSizeAdjusted, FS_ContainerSettings, Container, FMargin, Expansion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemDropped, FS_InventoryItem, DroppedItem, AActor*, ItemActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBoughtItem, FS_InventoryItem, Item, UAC_Inventory*, FromComponent, UIDA_Currency*, CurrencyUsed, int32, CurrencyAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSoldItem, FS_InventoryItem, Item, UAC_Inventory*, ToComponent, UIDA_Currency*, CurrencyUsed, int32, CurrencyAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemTagAdded, FS_InventoryItem, ItemData, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemTagRemoved, FS_InventoryItem, ItemData, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FItemTagValueUpdated, FS_InventoryItem, ItemData, FS_TagValue, NewTagValue, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContainerTagAdded, FS_ContainerSettings, Container, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContainerTagRemoved, FS_ContainerSettings, Container, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FContainerTagValueUpdated, FS_ContainerSettings, Container, FS_TagValue, NewTagValue, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTileTagsAdded, FS_ContainerSettings, Container, int32, TileIndex, FGameplayTagContainer, Tags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTileTagsRemoved, FS_ContainerSettings, Container, int32, TileIndex, FGameplayTagContainer, Tags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FItemComponentStarted, FS_UniqueID, OwningItemID, UItemComponent*, ItemComponent, AActor*, OriginalInstigator, FGameplayTag, StartEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FItemComponentStopped, FS_UniqueID, OwningItemID, UItemComponent*, ItemComponent, AActor*, OriginalInstigator, FGameplayTag, StopResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FItemComponentFinished, FS_UniqueID, OwningItemID, UItemComponent*, ItemComponent, AActor*, OriginalInstigator, FGameplayTag, FinishResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentTagAdded, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentTagRemoved, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentTagValueAdded, FS_TagValue, TagValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComponentTagValueUpdated, FS_TagValue, TagValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentTagValueRemoved, FS_TagValue, TagValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSortingFinished); //Used to prevent writing duplicate code inside the Async sorting function
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerInventoryDataReceived, AActor*, InstigatingActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartMultithreadWork);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemAbilityActivated, FS_InventoryItem, Item, UIC_ItemAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemAbilityEnded, FS_InventoryItem, Item, UIC_ItemAbility*, Ability);

DECLARE_DYNAMIC_DELEGATE_FourParams(FItemAssetLoaded, UDA_CoreItem*, ItemAsset, FS_InventoryItem, Item, int32, ContainerIndex, int32, ItemIndex);
DECLARE_DYNAMIC_DELEGATE(FAllItemAssetsLoaded);

//////////////////////////////////////////////////////////////////////////////////////

#pragma endregion


/**The inventory component that hosts everything related to
 * your inventory. Including containers and items and handling
 * replication and management of everything related to them.*/
UCLASS(Blueprintable, ClassGroup=(IFP), meta=(BlueprintSpawnableComponent),
	HideCategories = (ComponentTick, ComponentReplication, Activation, Cooking, Replication, Navigation))
class INVENTORYFRAMEWORKPLUGIN_API UAC_Inventory : public UActorComponent

{
	GENERATED_BODY()
public:

	UAC_Inventory();
	
	//--------------------
	// Networking
	
	//Label for network support.
	virtual bool IsSupportedForNetworking () const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//--------------------
	// Variables

	//If all the data for this component has been generated, this will be set to true.
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool Initialized = false;

	//What kind of inventory is this actor?
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Settings")
	TEnumAsByte<EInventoryType> InventoryType;

	//Your settings for all the containers, items inside those containers, and containers belonging to items.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (TitleProperty = "ContainerIdentifier"), SaveGame)
	TArray<FS_ContainerSettings> ContainerSettings;

	/**A map of all UniqueID's on this component. This table allows us to search
	 * for any item or container in constant time. Whereas searching through a TArray,
	 * you have to iterate over every entry. TMap's use hashing algorithms to achieve
	 * constant search time. So an item that is index 1000 will cost the same to find
	 * as item index 1.*/
	UPROPERTY(Category = "Settings", BlueprintReadOnly)
	TMap<int32, FS_IDMapEntry> ID_Map;

	/**The widget used to present the containers to the player.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Settings")
	TSubclassOf<UUserWidget> WidgetClass;

	/**Reference to the constructed widget class.*/
	UPROPERTY(BlueprintReadWrite,Category = "Settings")
	UUserWidget* WidgetRef;

	/**This is used to keep everyone who is interacting with a specific container
	 * up to date on it's content. If PlayerA updates the container,
	 * we can send RPC events to PlayerB and PlayerC to optimize network
	 * traffic, while also updating the server version so future listeners
	 * can get up to date containers from this component.*/
	UPROPERTY(BlueprintReadWrite, Category = "Networking")
	TArray<UAC_Inventory*> Listeners;

	/**List of items that are currently pending some networking event
	 * This is a system that allows designers to communicate to the player
	 * that an item or container is currently waiting to be processed by the server.
	 * There is currently no way of finding out if an unreliable RPC failed.
	 * So for all functions that want to use this system must be set to Reliable.*/
	UPROPERTY(BlueprintReadOnly, Category = "Networking")
	TArray<FS_UniqueID> NetworkQueue;

	/**Used to keep track if a client has received the container data after
	 * requesting it.*/
	UPROPERTY(BlueprintReadOnly, Category = "Networking")
	bool ClientReceivedContainerData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool DebugMessages = true;

	/**What tags does this component have at the moment?
	 * This is fully replicated, not just to listeners.
	 * This is meant to be interacted with through the Add and Remove
	 * tags from Component functions to maintain delegate support.
	 *
	 * Keep in mind, if the delegates go off, and you are listening as
	 * a client, the delegate might go off before this variable has
	 * replicated.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Replicated)
	FGameplayTagContainer TagsContainer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Replicated)
	TArray<FS_TagValue> TagValuesContainer;

	/**Tag to apply to items when they are equipped.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	FGameplayTag EquipTag;

	/**Component of the actor we are currently interacting with, for example a vendor.*/
	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	UAC_Inventory* InteractingWith = nullptr;

	/**Items have the ability to generate their icons.
	 * To skip generating the same icon over and over,
	 * this will let two items that share the same data asset
	 * to share the same render target, if the setting has
	 * been enabled in the data asset.*/
	UPROPERTY(BlueprintReadWrite, Category = "UI|Icon Generation")
	TMap<FName, UTextureRenderTarget2D*> GeneratedItemIcons;

	/**The active drag widget when dragging an item*/
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UW_Drag* DragWidget = nullptr;

	/**Whether to use UE's default drag and drop behavior or
	 * use IFP's custom behavior.
	 * It is highly recommended to enable this while the
	 * user is using gamepad navigation.
	 *
	 * Docs:
	 * https://inventoryframework.github.io/workinginthesystem/draganddrop/ */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	bool UseDefaultDragDropBehavior = true;

	/**List of all items from the loot table system that have been added
	 * and are now waiting to be initialized.
	 * This is reset after each loot table.*/
	UPROPERTY(Category = "Loot Table System", BlueprintReadOnly)
	TArray<FS_InventoryItem> QueuedLootTableItems;

	/**If the fragment manager is missing from the owner of this inventory component,
	 * what class should we use for the fragment manager? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TSubclassOf<UAC_FragmentManager> FragmentManagerClass = UAC_FragmentManager::StaticClass();

	UPROPERTY()
	TObjectPtr<UAC_FragmentManager> FragmentManager = nullptr;
	UFUNCTION(Category = "Inventory Component|Fragments", BlueprintPure)
	UAC_FragmentManager* GetFragmentManager();
	
	//--------------------
	//Input settings

	/**The mapping context to use for handling navigation
	 * around containers and items.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI", DisplayName = "UI Navigation Context")
	UInputMappingContext* UINavigationContext = Cast<UInputMappingContext>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/MappingContexts/IMC_UI_Navigation_IFP.IMC_UI_Navigation_IFP")));
	
	/**Key that must be pressed to rotate an item while it is being dragged.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* RotateKey = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_RotateKey.IA_UI_RotateKey")));
	
	/**Key that must be held down to perform a split operation on a stacked item*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* SplitKey = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_SplitKey.IA_UI_SplitKey")));
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* SelectItem = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_SelectItem.IA_UI_SelectItem")));
	
	/**Key to navigate up in a container widget.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* NavigateUp = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_NavigateUp.IA_UI_NavigateUp")));
	
	/**Key to navigate right in a container widget.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* NavigateRight = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_NavigateRight.IA_UI_NavigateRight")));
	
	/**Key to navigate down in a container widget.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* NavigateDown = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_NavigateDown.IA_UI_NavigateDown")));
	
	/**Key to navigate left in a container widget.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|UI")
	UInputAction* NavigateLeft = Cast<UInputAction>(StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/EnhancedInput/InputActions/IA_UI_NavigateLeft.IA_UI_NavigateLeft")));

	//--------------------
	
	
	//--------------------
	//Color settings
	/**These are left as BlueprintReadWrite so designers can modify them in the players settings,
	 * but is also EditDefaultsOnly as the default option should always be the same for everyone.*/
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings")
	FLinearColor DefaultHighlight = FLinearColor(0.35, 0.35, 0.35, 0.5); //white

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings")
	FLinearColor StackHighlight = FLinearColor(0.75, 0.75, 0.15, 0.5); //yellow

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings")
	FLinearColor InvalidHighlight = FLinearColor(0.625, 0.0, 0.0, 0.5); //red

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings")
	FLinearColor CombineHighlight = FLinearColor(0.008, 0.75, 0.008, 0.5); //green

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings")
	FLinearColor SwapHighlight = FLinearColor(0.1, 0.21, 0.42, 0.5); //Blue

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Color Settings", meta=(ForceInlineRow))
	TMap<FGameplayTag, FLinearColor> RarityColorSheet; //Filled in blueprint children

private:
	//Used for functions using the FS_ItemSubLevel struct.
	int32 CurrentSubLevel = -1;

#pragma region Delegates

public:

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentStarted ComponentStarted;

	/**The component has been fully stopped and no data is ready to be
	 * used in a gameplay scenario. You should call @StartComponent
	 * to get this actor ready for any gameplay interaction.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentStopped ComponentStopped;

	/**The component is about to be stopped, but all the data is still
	 * valid. This is where you clean up any widgets and so forth.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentStopped ComponentPreStop;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemAdded ItemAdded;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemRemoved ItemRemoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemCountUpdated ItemCountUpdated;

	/**Primarily used by pickups to find out if they succeeded their spawn chance,
	 * so we can find out if we should destroy the actor*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemFailedSpawn ItemFailedSpawn;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemEquipped ItemEquipped;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemUnequipped ItemUnequipped;

	/**The player attempted to sell an item, but the system failed to add the currency
	 * to the player. This is your failsafe, maybe you send the currency through some mail
	 * system or something.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FCurrencyAdditionFailed CurrencyAdditionFailed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemMoved ItemMoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FContainerSizeAdjusted ContainerSizeAdjusted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemDropped ItemDropped;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FSoldItem SoldItem;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FBoughtItem BoughtItem;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemTagAdded ItemTagAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemTagRemoved	ItemTagRemoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemTagValueUpdated ItemTagValueUpdated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FContainerTagAdded ContainerTagAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FContainerTagRemoved ContainerTagRemoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FContainerTagValueUpdated ContainerTagValueUpdated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FTileTagsAdded TileTagsAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FTileTagsRemoved TileTagsRemoved;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemComponentStarted ItemComponentStarted;

	/**Automatically called by item components when they are stopped.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemComponentStopped ItemComponentStopped;

	/**An event that is up to you to call when the corresponding logic inside a component
	 * has finished. It is advised to call this before calling @StopComponent.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemComponentFinished ItemComponentFinished;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentTagAdded ComponentTagAdded;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentTagRemoved ComponentTagRemoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentTagValueAdded ComponentTagValueAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentTagValueUpdated ComponentTagValueUpdated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FComponentTagValueRemoved ComponentTagValueRemoved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FServerInventoryDataReceived ServerInventoryDataReceived;

	/**Delegate used primarily for the ItemQuery system. This is called during StartComponent
	 * BEFORE all items have their ItemEquipped delegate called. Because of how expensive
	 * blueprints and components are to create, this is the ideal moment to start any
	 * foreground thread work. The primary example is O_ItemQueryBase -> OnInventoryStarted
	 * 
	 * I'm exposing this to blueprint because there are more and more plugins popping
	 * up claiming that they provide multithreading to blueprints.*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FStartMultithreadWork StartMultithreadWork;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemAbilityActivated ItemAbilityActivated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemAbilityEnded ItemAbilityEnded;

	FSortingFinished SortingFinished;

#pragma endregion

	
	
	//--------------------
	// Functions

#pragma region Management

	/**This generates all the data needed for functions and widgets, but does not create any widgets.
	 * Only initializes items and generates all the necessary data you would need.
	 * This is where you'd generate things such as UniqueID, random stack count, spawn chance, etc...
	 * This also creates the tile map and gives all items their tile positions, checks for overlaps and so forth.
	 * Once this is called, items should be ready to have their widget version created.
	 * This also calls RefreshIndexes.
	 *
	 * @RemoveSkipValidationTags If set to true, all SkipValidation tags will be ignored and removed, allowing
	 * the system to re-evaluate the container settings. This is most often used for a new game version where
	 * the save might be from an old version.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void StartComponent(bool RemoveSkipValidationTags = false);

	/**This should be called whenever you add or reorganize ContainerSettings.
	 * This will update all containers ContainerIndex's and widgets if valid.
	 * If a container doesn't have a valid UniqueID, this will also generate one.
	 * Also updates Containers BelongsToItem*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void RefreshIndexes();

	/**Sort the items in the container and refresh all the items indexes.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void RefreshItemsIndexes(const FS_ContainerSettings Container);

	/**Clear the tile map and set the whole tile map to -1
	 * This does not scan for items and fill the tile map with the
	 * containers items.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void InitializeTileMap(UPARAM(ref) FS_ContainerSettings& Container);

	/**Refresh the tile map for a container.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void RefreshTileMap(UPARAM(ref) FS_ContainerSettings& Container);

	/**Completely wipe out and remake the tile map from scratch.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void RebuildTileMap(UPARAM(ref) FS_ContainerSettings& Container);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Management")
	void C_RequestServerContainerData(bool CallServerDataReceived);

	UFUNCTION(Server, Reliable, Category = "Management")
	void S_SendContainerDataToClient(bool CallServerDataReceived);

	UFUNCTION(Client, Reliable, Category = "Management")
	void C_ReceiveServerContainerData(const TArray<FS_ContainerSettings> &ServerContainerSettings, bool CallServerDataReceived);

	/**This wipes all references to objects, widgets, and attachment widgets.
	 * This should be called when you are sure you don't want any of the items or containers
	 * to be displayed on the screen until you construct the containers again.
	 * For example the player going a certain distance away from the item.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Management")
	void StopComponent();

	/**You should not be saving the raw container settings, as UniqueID's component
	 * reference can't get saved. This function creates a list of containers identical
	 * to this components ContainerSettings, but some data has been modified so it
	 * is better for save game files.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	TArray<FS_ContainerSettings> GetContainersForSaveState();

	UFUNCTION(BlueprintCallable, Category = "Management", meta = (DisplayName = "Reset All Unique ID's"))
	void ResetAllUniqueIDs();

	/**Returns the components current state.
	 * Docs: inventoryframework.github.io/classes-and-settings/ac_inventory/#states */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Management")
	TEnumAsByte<EComponentState> GetComponentState();

	/**Preps the component to be usable for editor tools. Returns true if the component
	 * was already in raw state and has been successfully converted to editor state.*/
	UFUNCTION(BlueprintCallable, Category = "Management", CallInEditor, meta = (DevelopmentOnly))
	void ConvertFromRawStateToEditorState();

	/**Optimize the data inside this component so it can start doing gameplay
	 * logic as efficiently as possible.
	 * This should never have to be called during gameplay and can lead to crashes
	 * if the player attempts to interact with the component if this is called
	 * after StartComponent.*/
	UFUNCTION(BlueprintCallable, Category = "Management", CallInEditor, meta = (DevelopmentOnly))
	void ConvertToRawState();
	
	/**Reset the ID map and repopulate it.*/
	UFUNCTION(BlueprintCallable, Category = "Management", DisplayName = "Refresh ID Map")
	void RefreshIDMap();

	/**Add a UniqueID to the ID_Map for faster searches.
	 * If the ID is already present, it will simply get updated with the new directions.*/
	UFUNCTION(BlueprintCallable, Category = "Management", DisplayName = "Add UniqueID to ID Map")
	void AddUniqueIDToIDMap(FS_UniqueID UniqueID, FIntPoint Directions, bool IsContainer = false);

	/**Remove a UniqueID from the ID_Map.*/
	UFUNCTION(BlueprintCallable, Category = "Management", DisplayName = "Remove UniqueID from ID Map")
	void RemoveUniqueIDFromIDMap(FS_UniqueID UniqueID);

	/**Check if all UniqueID's in the component are in the ID Map.
	 * This should only be needed for bug-hunting and never in production.
	 * If this ever returns any missing ID's, please report it on the discord,
	 * with details about what your recent actions were.
	 * @MissingContainers what containers were not in the ID map.
	 * @MissingItems what items were not in the ID map.
	 * @UnknownIDs what ID's were in the ID map, but couldn't be found in the component.
	 * @IncorrectDirections ID's that are valid, but the directions are wrong.*
	 * Only returns true if all outputs are empty.*/
	UFUNCTION(BlueprintCallable, Category = "Management", DisplayName = "Validate ID Map")
	bool ValidateIDMap(TArray<FS_ContainerSettings> &MissingContainers, TArray<FS_InventoryItem> &MissingItems,
		TArray<FS_UniqueID> &UnknownIDs, TArray<FS_UniqueID> &IncorrectDirections);

	UFUNCTION(BlueprintCallable, Category = "Management")
	void BroadcastNewAssignedUniqueID(FS_UniqueID OldID, FS_UniqueID NewID);

	/**Start async loading all item assets inside this inventory component.
	 * Only usable if the component isn't initialized.
	 * @AssetBundles What assets to load alongside the item asset.
	 * @OnItemAssetLoaded Callback for when an individual item is loaded.
	 * This allows you to react and perform code when an item is loaded.
	 * Keep in mind, the item is not initialized yet. You can use
	 * the ContainerIndex and ItemIndex to get a ref to the item struct.
	 * @OnAllItemAssetsLoaded Callback when ALL item assets have been loaded.
	 * Ideal for starting the component as nothing will have to be
	 * loaded instantly.*/
	UFUNCTION(BlueprintCallable, Category = "Management")
	void LoadItemAssets(TArray<FName> AssetBundles, FItemAssetLoaded OnItemAssetLoaded, FAllItemAssetsLoaded OnAllItemAssetsLoaded);

	void CleanupNullReferences();

#pragma endregion
	

#pragma region Items
	
	/**Attempt to move the item to a specific container and index.
	 * This does NOT attempt to stack the item with any items that are in the way.
	 * If @SkipCollisionCheck is true, you are telling this function that YOU have already
	 * checked for collision and that this function should blindly trust the data you feed it.
	 * If called from a client, the return's will not be valid because we are sending off
	 * an RPC to let the server handle the move
	 * If either component is a vendor, it'll check if the @FromComponent can afford
	 * the item, but will not perform the currency exchange. You should implement
	 * that logic yourself in Blueprints.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void MoveItem(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent, UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex,
		int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, TEnumAsByte<ERotation> NewRotation);

	/**Process the client request
	 *
	 * For those who are trying to optimize network traffic in IFP, the MoveItem is the second-highest
	 * cost RPC (at least in-regards to frequency it is used), so this is a good spot to try and optimize things.
	 * Here are some notes around this possible optimization, in these notes we are using an example of two players
	 * interacting with a chest, PlayerA and PlayerB:
	 * - PlayerA moves an item out of their inventory and into the chest. Since PlayerB doesn't have the information
	 * to move the item from PlayerA's inventory and into the chest in their world (because they don't know what is
	 * inside PlayerA's inventory), they can't fulfill this request. Hence, why the client receives the full sized
	 * item struct and container struct.
	 * - The proper way to optimize this would be to have a separate function for PlayerA that only takes in the
	 * UniqueID's of the item and containers, since it has the information required to skip this.
	 * - The server has all the information, so it is able to accept just an ID for the item the player wants to move.
	 * Since it's virtually instant to get an item or container with just its ID, thanks to the ID map system, this
	 * trick can be used to minimize plenty of RPC's.*/
	UFUNCTION(Server, Reliable, WithValidation)
	void S_MoveItem(FS_UniqueID ItemToMove, UAC_Inventory* FromComponent, UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex,
		int32 Count, bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, ERotation NewRotation, ENetRole CallerLocalRole);

	/**Finalize the MoveItem request.
	 * TODO: Try to replace @ItemContainers with the UniqueID of the containers to minimize the RPC size.*/
	UFUNCTION(Client, Reliable)
	void C_MoveItem(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent, UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex,int32 Count,
		bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, ERotation NewRotation, const TArray<FS_ContainerSettings> &ItemContainers, FRandomStream Seed);

	/**Non-replicated version of MoveItem*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void Internal_MoveItem(FS_InventoryItem ItemToMove, UAC_Inventory* FromComponent, UAC_Inventory* ToComponent, int32 ToContainer, int32 ToIndex, int32 Count,
		bool CallItemMoved, bool CallItemAdded, bool SkipCollisionCheck, TEnumAsByte<ERotation> NewRotation, TArray<FS_ContainerSettings> ItemContainers, FRandomStream Seed);

	/**Swap the location of two items.
	 * This can get heavy for networking, as this is simply calling MoveItem twice.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void SwapItemLocations(FS_InventoryItem Item1, FS_InventoryItem Item2, bool CallItemMoved);

	UFUNCTION(Server, Reliable, WithValidation)
	void S_SwapItemLocations(FS_InventoryItem Item1, FS_InventoryItem Item2, bool CallItemMoved, ENetRole CallerLocalRole);
	
	/**Adds the item to the tile map of the item's container.
	 * This does NOT check if an item is in the way. You should do that before hand.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void AddItemToTileMap(const FS_InventoryItem Item);

	/**Only use on containers that need their tile map populated, but they haven't been added to the component yet.
	 * This is used by functions such as TryAddNewItem, as it can add containers, but requires collision before
	 * we add it to the component.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void AddItemToUninitializedTileMap(FS_InventoryItem Item, UPARAM(ref) FS_ContainerSettings& Container);
	
	/**Remove an item from the tile map in it's owning container.
	 * This should really only be called when you are removing or moving an item.
	 * This does not remove the item itself from the container, only its collision.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void RemoveItemFromTileMap(FS_InventoryItem Item);
	
	/**Remove an item from the inventory.
	 * Because we want to keep widgets on the blueprint level, this is meant to be overriden.
	 * Designers might need ItemData and/or UniqueID, so it is recommended to call the parent function
	 * at the end of the Blueprint override.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void RemoveItemFromInventory(FS_InventoryItem Item, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, bool& Success);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveItemFromInventory(FS_UniqueID ItemID, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, ENetRole CallerLocalRole);
	
	UFUNCTION(Client, Reliable)
	void C_RemoveItemFromInventory(FS_UniqueID ItemID, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, FRandomStream Seed);

	/**Remove an item from the inventory.
	 * Because we want to keep widgets on the blueprint level, this is meant to be overriden.
	 * Designers might need ItemData and/or UniqueID, so it is recommended to call the parent function
	 * at the end of the Blueprint override.
	 * The @Seed is required if the container will be adjusted, for example infinite containers.*/
	UFUNCTION(BlueprintNativeEvent, Category = "Items", DisplayName = "Remove Item From Inventory (Non-replicated)")
	void Internal_RemoveItemFromInventory(FS_InventoryItem Item, bool CallItemRemoved, bool CallItemUnequipped, bool RemoveItemComponents, bool RemoveItemsContainers, bool RemoveItemInstance, FRandomStream Seed, bool& Success);

	/**Attempt to drop an item. This is being left for the blueprint level since this requires
	 * a lot of context. Some people will want it to be thrown away from the player, spawn
	 * in front of them, a random location near them or simply don't want players able
	 * to drop items. This can then be changed into a "Delete Item" function, where
	 * you can have a popup widget, notify the player they are about to delete an item
	 * and then just remove it from the inventory once they confirm.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(FS_InventoryItem Item);

	UFUNCTION(Server, Reliable)
	void S_DropItem(FS_UniqueID ItemID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Items", DisplayName = "DropItem")
	void Internal_DropItem(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "Items")
	bool CanItemBeDropped(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "Items")
	bool CanItemBeDestroyed(FS_InventoryItem Item);

	/**Gets all containers that belong to "Item" and updates their BelongsToItem.
	@param OldXY refers to the old BelongsToItem directions where "Item" was before this was called.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UpdateItemsContainers(FS_InventoryItem Item, FIntPoint OldXY);

	/**Create and then add the attachment widget for an item if valid.
	 * If you've already constructed the widget, use "AddContainersFromWidget"
	 * This constructs the container widgets.
	 * @DoNotBind Set to true if you are going to manually bind the widgets
	 * with their containers. This will now just create the widget instance
	 * and assign it to the struct.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	UW_AttachmentParent* CreateAttachmentWidgetForItem(FS_InventoryItem Item, bool ResetData, bool DoNotBind, TArray<FS_ContainerSettings>& AddedContainers);
	
	/**Attempt to add an uninitialized item to this component.
	 * This also stacks the item with other items if possible.
	 * This function will kick clients if they attempt to call this.
	 * A function that allows you to add any item from your item database
	 * is simply too dangerous for clients to be trusted with.
	 *
	 * Docs: inventoryframework.github.io/workinginthesystem/adding-new-items-during-runtime/
	 * 
	 * @Item The only setting that must be valid is the item data asset.
	 * If ContainerIndex is -1, it'll find the first available container.
	 * Random count is supported. If TileIndex is -1, it'll find the first available
	 * tile. Random tile is currently not supported.
	 * For infinite containers, specified tiles out of bounds or random tiles
	 * is currently not supported.
	 * @ItemsContainers for containers that belong to Item, BelongsToItem
	 * coordinates should be -1/-1.
	 * The only other data that needs to be valid is the container name, it must match
	 * the widget the owning item's attachment widget.
	 * Everything else can be left for default or filled if you want to.
	 * @NewItem The newly created item (if successful). If this item is
	 * stackable and it was combined into another stack until the item depleted,
	 * it'll return the last stack it stacked with. The return is only valid
	 * for servers and single player sessions.
	 * @StackDelta If the item was stacked onto another item, this will return
	 * the difference between the original stack and the new stack count.*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
	void TryAddNewItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ItemsContainers, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta);

	/**It is safe to call the Internal_TryAddNewItem through this because the item has been added
	 * to the server. If the client finds a way to add the item only for them, the moment they try to
	 * interact with it in any way, the server will notice the data is not synced and will kick them.*/
	UFUNCTION(Client, Reliable)
	void C_TryAddNewItem(FS_InventoryItem Item, const TArray<FS_ContainerSettings> &ItemsContainers, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, FRandomStream Seed);
	
	void Internal_TryAddNewItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ItemsContainers, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, FRandomStream Seed, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta);

	/**Attempt to add an uninitialized item to this component.
	 * This also stacks the item with other items if possible.
	* This function will kick clients if they attempt to call this.
	* A function that allows you to add any item from your item database
	* is simply too dangerous for clients to be trusted with.
	*
	* Docs: inventoryframework.github.io/workinginthesystem/adding-new-items-during-runtime/
	* 
	* @ItemAsset The item asset the new item should be based off of.
	* @NewItem The newly created item (if successful). If this item is
	* stackable and it was combined into another stack until the item depleted,
	* it'll return the last stack it stacked with. The return is only valid
	* for servers and single player sessions.
	* @StackDelta If the item was stacked onto another item, this will return
	* the difference between the original stack and the new stack count.
	* @ContainerIndex Which container to add the item to. If set to -1,
	* it will find the first available container.
	* @TileIndex Which tile to add the item to. If set to -1,
	* it will find the first available tile.*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
	void TryAddNewItemByDataAsset(UDA_CoreItem* ItemAsset, int32 Count, UAC_Inventory* DestinationComponent, bool CallItemAdded, bool SkipStacking, bool& Result, FS_InventoryItem& NewItem, int32& StackDelta, int32 ContainerIndex = -1, int32 TileIndex = -1);

	/**Attempt to stack Item1 with Item2.
	 * If called on server, both Remaining counts will always be accurate.
	 * If called on client, both Remaining Counts will be predicted,
	 * and thus might be inaccurate.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void StackTwoItems(FS_InventoryItem Item1, FS_InventoryItem Item2, int32& Item1RemainingCount, int32& Item2NewStackCount);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_StackTwoItems(FS_UniqueID Item1ID, FS_UniqueID Item2ID, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_StackTwoItems(FS_UniqueID Item1ID, FS_UniqueID Item2ID);

	void Internal_StackTwoItems(FS_InventoryItem Item1, FS_InventoryItem Item2, int32& Item1RemainingCount, int32& Item2NewStackCount);

	/**Attempt to split an item into two separate stacks.
	 * If called on server, both Remaining Counts will always be accurate.
	 * If called on client, both Remaining Counts will be predicted,
	 * and thus might be inaccurate.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex,
		int32& Item1RemainingCount, int32& Item2NewStackCount);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, UDA_CoreItem* ItemDataAsset, int32 NewStackContainerIndex,
		int32 NewStackTileIndex, FS_UniqueID NewStackUniqueID, FRandomStream Seed);

	void Internal_SplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex, FS_UniqueID NewStackUniqueID,
		int32& Item1RemainingCount, int32& Item2NewStackCount, FRandomStream Seed);
	
	/**Increase an items stack count. Clamped to the items max stack.
	 * If called on server, NewCount will always be accurate.
	 * If called on client, NewCount will be predicted, and thus might
	 * be inaccurate.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void IncreaseItemCount(FS_InventoryItem Item, int32 Count, int32& NewCount);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_IncreaseItemCount(FS_UniqueID ItemID, int32 Count, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_IncreaseItemCount(FS_UniqueID ItemID, int32 Count);

	void Internal_IncreaseItemCount(FS_UniqueID ItemID, int32 Count, int32& NewCount);

	/**Reduce an items stack count. Clamped min is 0.
	 * If called on server, NewCount will always be accurate.
	 * If called on client, NewCount will be predicted, and thus might
	 * be inaccurate.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void ReduceItemCount(FS_InventoryItem Item, int32 Count, bool RemoveItemIf0, int32& NewCount);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_ReduceItemCount(FS_UniqueID ItemID, int32 Count, bool RemoveItemIf0, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_ReduceItemCount(FS_UniqueID ItemID, int32 Count, bool RemoveItemIf0, FRandomStream Seed);

	void Internal_ReduceItemCount(FS_InventoryItem Item, int32 Count, bool RemoveItemIf0, FRandomStream Seed);

	/**Go over all items inside a specified container and reduce the item count of all
	 * items that match the same data asset as @Item.
	 * @param Item What data asset must the items match?
	 * @param Count How much to reduce the matching items count.
	 * @param TargetComponent What components containers are we trying to modify.
	 * @param ContainerIndex If -1, search all containers.
	 * @param RemoveItemsIf0 Whether or not to completely remove the items if their
	 * stack ends up being 0.
	 * @param RemainingCount How much of Count was not used. If you reduce by 20,
	 * but the system only managed to remove 13, this will return 7.
	 * This value is not reliable for clients.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, bool RemoveItemsIf0, int32& RemainingCount);

	UFUNCTION(Server, Reliable)
	void S_MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, bool RemoveItemsIf0, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, FRandomStream Seed, bool RemoveItemsIf0);

	void Internal_MassReduceCount(UDA_CoreItem* Item, int32 Count, UAC_Inventory* TargetComponent, int32 ContainerIndex, FRandomStream Seed, bool RemoveItemsIf0);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void NotifyItemSold(FS_InventoryItem Item, UIDA_Currency* Currency, int32 Amount, UAC_Inventory* Buyer, UAC_Inventory* Seller);

	UFUNCTION(Server, Reliable)
	void S_NotifyItemSold(FS_InventoryItem Item, UIDA_Currency* Currency, int32 Amount, UAC_Inventory* Buyer, UAC_Inventory* Seller);

	/**For single player, this can be ignored, you should be able to modify the item
	 * struct directly with no problems.
	 * This is intended for multiplayer and notifying the server and other clients about
	 * updated information.
	 * This will also call the interface update event to the widget and external widgets.
	 *
	 * The only setting you are not allowed to update is the SpawnChance, as that will cause
	 * issues for generated item icons, dropping the item and more. The spawn chance
	 * is only supposed to be rolled once on the server and that is during StartComponent.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UpdateItemsOverrideSettings(FS_InventoryItem Item, FItemOverrideSettings NewSettings, AActor* ActorRequestingChange);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_UpdateItemsOverrideSettings(FS_UniqueID ItemID, FItemOverrideSettings NewSettings, ENetRole CallerLocalRole, AActor* ActorRequestingChange);

	UFUNCTION(Client, Reliable)
	void C_UpdateItemsOverrideSettings(FS_UniqueID ItemID, FItemOverrideSettings NewSettings);

	void Internal_UpdateItemsOverrideSettings(FS_InventoryItem Item, FItemOverrideSettings NewSettings);

	/**Sort and move all items inside this container by the sorting type.
	* If an item does not fit after the move, by default the item will be
	* moved to a new container. If no available container is found, the
	* item is dropped.
	* @StaggerTimer How long of a delay should be between each item being sorted?
	* If this is 0 or less, it'll be instantaneous.
	* @LockingTag Tag applied to the item and will be removed once all items have
	* been sorted.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void SortAndMoveItems(TEnumAsByte<ESortingType> SortType, FS_ContainerSettings Container, float StaggerTimer = 0);

	UFUNCTION(Server, Reliable)
	void S_SortAndMoveItems(ESortingType SortType, FS_UniqueID ContainerID, float StaggerTimer, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_SortAndMoveItems(ESortingType SortType, FS_UniqueID ContainerID, float StaggerTimer, FRandomStream Seed);

	void Internal_SortAndMoveItems(TEnumAsByte<ESortingType> SortType, const FS_ContainerSettings& Container, float StaggerTimer, FRandomStream Seed);

	//Used when sorting is using a stagger, this is the timer function that handles items one by one.
	UFUNCTION()
	void T_SortAndMoveItems(UAC_Inventory* ParentComponent, const FS_ContainerSettings& Container, FS_InventoryItem Item, FRandomStream Seed, bool bLastItem);

	/**Remember to call this before setting the UniqueID for the item this object belongs to.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UpdateItemComponentsUniqueID(FS_UniqueID OldUniqueID, FS_UniqueID NewUniqueID);

	/**Get the traits ItemComponent.
	 *
	* @Trait The instanced trait inside the item data asset.
	* 
	* @CreateComponent If no component exists for this trait, attempt to create it.
	* 
	* @Event What event should the item component attempt to activate the moment it
	* gets created? This is optional. Look at ItemComponent -> ActivateEvent
	* 
	* @Instigator Who is trying to retrieve the item Component
	* 
	* @Payload Optional payload to pass data to the Component*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	UItemComponent* GetItemComponent(FS_InventoryItem Item, UIT_ItemComponentTrait* Trait, bool CreateComponent, AActor* Instigator = nullptr, FGameplayTag Event = FGameplayTag(), FItemComponentPayload Payload = FItemComponentPayload());

	/**Creates and binds the item item instance to this item struct.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	UItemInstance* CreateItemInstanceForItem(UPARAM(ref) FS_InventoryItem& Item);

	/**Get the count of items inside this component.
	 * @OptionalFilter If empty, we search all containers.
	 * If specified, only search those containers.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	int32 GetItemCount(UDA_CoreItem* ItemAsset, TArray<FS_ContainerSettings> OptionalFilter);

	/**Update the equip status of an item. This will replicate.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UpdateItemsEquipStatus(FS_InventoryItem Item, bool IsEquipped, TArray<FName> CustomTriggerFilters);

	UFUNCTION(Server, Reliable)
	void S_UpdateItemsEquipStatus(FS_UniqueID ItemID, bool IsEquipped, const TArray<FName>& CustomTriggerFilters, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_UpdateItemsEquipStatus(FS_UniqueID ItemID, bool IsEquipped, const TArray<FName>& CustomTriggerFilters);

	void Internal_UpdateItemsEquipStatus(FS_InventoryItem Item, bool IsEquipped, TArray<FName> CustomTriggerFilters);

	/* Splits an item into a set amount of stacks (@SplitAmount) with each stack being the size of @StackSize
	 * For example; 10 potions with @StackSize of 2 and @SplitAmount of 3 will create 3 stacks of 2, leaving the
	 * original item at 4.
	 * If the stack runs out, in the above example lets change the potions amount to 5, it will create 2 stacks
	 * of 2 and 1 stack of 1, since it ran out of item count to split.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void MassSplitStack(FS_InventoryItem Item, int32 StackSize, int32 SplitAmount, FS_ContainerSettings DestinationContainer);

	UFUNCTION(Server, Reliable)
	void S_MassSplitStack(FS_UniqueID ItemID, int32 StackSize, int32 SplitAmount, FS_UniqueID ContainerID, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_MassSplitStack(FS_UniqueID ItemID, int32 StackSize, int32 SplitAmount, FS_UniqueID ContainerID,
		FRandomStream Seed, int32 ItemCountReduction);

	/* To simplify the code, we use @AmountReduced on the server to let players who can't
	 * perform the collision checks (for example, the destination being a container they
	 * don't have access to) understand how much to feed into @ItemCountReduction.
	 * If the client can't perform collision checks in the @DestinationContainer
	 * because it's not relevant to them, then they can not perform most of the code
	 * in this function. Meaning they do not know when to stop reducing the item count.
	 * Hence, why they need this information from the server. */
	int32 Internal_MassSplitStack(FS_InventoryItem Item, int32 StackSize, int32 SplitAmount, FS_ContainerSettings DestinationContainer,
	                             FRandomStream Seed, int32 ItemCountReduction);

	/**Helper function for getting all item instance classes
	 * that aren't loaded yet.*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	TArray<TSoftClassPtr<UItemInstance>> GetUnloadedItemInstances();

#pragma endregion


#pragma region ItemsTags
	
	//--------------------
	// Item tags

	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.
	 *
	 * Docs: inventoryframework.github.io/workinginthesystem/tagsystem/*/
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	bool AddTagToItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue = false);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_AddTagToItem(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue = false);

	UFUNCTION(Client, Reliable)
	void C_AddTagToItem(FS_UniqueID ItemID, FGameplayTag Tag, bool IgnoreNetworkQueue = false);

	void Internal_AddTagToItem(FS_InventoryItem Item, FGameplayTag Tag);

	/**Asked by the AddTagToItem function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items|Tags")
	bool CanTagBeAddedToItem(FGameplayTag Tag, FS_InventoryItem Item);

	/**Add a tag to an item. Returns true or false depending if the removal was successful.
	 * This is predictive, if ran on the client, the return might not work.
	 *
	 * Docs: inventoryframework.github.io/workinginthesystem/tagsystem/*/
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	bool RemoveTagFromItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue = false);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveTagFromItem(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue = false);

	UFUNCTION(Client, Reliable)
	void C_RemoveTagFromItem(FS_UniqueID ItemID, FGameplayTag Tag, bool DoNotAddToNetworkQueue = false);

	void Internal_RemoveTagFromItem(FS_InventoryItem Item, FGameplayTag Tag);

	/**Asked by the RemoveTagFromItem function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items|Tags")
	bool CanTagBeRemovedFromItem(FGameplayTag Tag, FS_InventoryItem Item);

	//--------------------
	
	
	//--------------------
	// Item tag values

	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.
	 * @Item The item to modify
	 * @Tag The tag to add
	 * @Value The value to associated with the tag
	 * @AddIfNotFound If the tag value does not exist on the item, do we add it?
	 * @CalculationClass What class should we use to adjust the calculation, if any?
	 * @IgnoreNetworkQueue Should this tag ignore the network queue?
	 *
	 * Docs: inventoryframework.github.io/workinginthesystem/tagsystem/*/
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	bool SetTagValueForItem(FS_InventoryItem Item, FGameplayTag Tag, float Value, bool AddIfNotFound = true, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool IgnoreNetworkQueue = false);
	
	UFUNCTION(Server, WithValidation, Reliable)
	void S_SetTagValueForItem(FS_UniqueID ItemID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, bool AddIfNotFound = true, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool IgnoreNetworkQueue = false);
	
	UFUNCTION(Client, Reliable)
	void C_SetTagValueForItem(FS_UniqueID ItemID, FGameplayTag Tag, float Value, bool AddIfNotFound = true, bool IgnoreNetworkQueue = false);
	
	void Internal_SetTagValueForItem(FS_InventoryItem Item, FGameplayTag Tag, float Value, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool& Success);
	
	float PreItemTagValueCalculation(FS_TagValue TagValue, FS_InventoryItem Item, TSubclassOf<UO_TagValueCalculation> CalculationClass);

	/**Asked by the SetTagValueForItem function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items|Tags")
	bool CanTagValueBeAddedToItem(FS_TagValue TagValue, FS_InventoryItem Item);

	/**Asked by the SetTagValueForItem function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items|Tags")
	bool CanTagValueBeSetForItem(FS_TagValue TagValue, FS_InventoryItem Item);

	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.
	 *
	 * Docs: inventoryframework.github.io/workinginthesystem/tagsystem/*/
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	bool RemoveTagValueFromItem(FS_InventoryItem Item, FGameplayTag Tag, bool IgnoreNetworkQueue = false);
	
	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveTagValueFromItem(FS_UniqueID ItemID, FGameplayTag Tag, ENetRole CallerLocalRole, bool IgnoreNetworkQueue = false);
	
	UFUNCTION(Client, Reliable)
	void C_RemoveTagValueFromItem(FS_UniqueID ItemID, FGameplayTag Tag, bool IgnoreNetworkQueue = false);
	
	void Internal_RemoveTagValueFromItem(FS_InventoryItem Item, FGameplayTag Tag);

	/**Asked by the SetTagValueForItem function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Items|Tags")
	bool CanTagValueBeRemovedFromItem(FGameplayTag TagValue, FS_InventoryItem Item);

	/**Get all items with a specified tag.
	 *
	 * @ContainerIndex If set to -1, search all containers.
	 * Otherwise, this specifies what container to search. */
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	TArray<FS_InventoryItem> GetItemsByTag(FGameplayTag Tag, int32 ContainerIndex = -1);

	/**Get all items with a specified tag value.
	 *
	 * @ContainerIndex If set to -1, search all containers.
	 * Otherwise, this specifies what container to search. */
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	TArray<FS_InventoryItem> GetItemsByTagValue(FGameplayTag Tag, int32 ContainerIndex = -1);

	/**Get all items with a specified tag type.
	 *
	 * @ContainerIndex If set to -1, search all containers.
	 * Otherwise, this specifies what container to search. */
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	TArray<FS_InventoryItem> GetItemsByType(FGameplayTag Tag, int32 ContainerIndex = -1);

	/**Get all items that match a tag query.
	 *
	 * @ContainerIndex If set to -1, search all containers.
	 * Otherwise, this specifies what container to search. */
	UFUNCTION(BlueprintCallable, Category = "Items|Tags")
	TArray<FS_InventoryItem> GetItemsByTagQuery(FGameplayTagQuery TagQuery, int32 ContainerIndex = -1);

	//--------------------

#pragma endregion
	

#pragma region ItemCheckers
	
	/**Resolve whether the BuyerComponent meets the requirements to buy an item.
	* If the item belongs to the BuyerComponent, this will always return true.
	* @param ItemToPurchase The item to check the price of.
	* @param BuyerComponent The component we will check for the items accepted currencies.
	* @param Currency The currency BuyerComponent can use to exchange for the item. Returns null
	* if player can not afford the item, unless ItemToPurchase already belongs to BuyerComponent.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Items|Checkers", meta = (ReturnDisplayName = "Can Afford"))
	bool CanAffordItem(FS_InventoryItem ItemToPurchase, UIDA_Currency*& Currency); //V: TODO: Move this to library?

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Items|Checkers")
	bool CanItemBeRotated(FS_InventoryItem Item);

	/**What actor is the owner for ItemComponent?
	 * This is meant to be overriden by people who have the inventory component living
	 * on the player controller or player state, or somewhere else.
	 * Since ItemComponent might want to live on an actor that is replicated to other clients,
	 * that means they need to be attached to an actor that suits that case.
	 * Controllers are only replicated between the owning client and the server,
	 * so they aren't good candidates. In this case, you'd want to override this function
	 * and get the possessed pawn/character.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Items")
	AActor* GetItemComponentOwner();
	
	/**Constructs item Component with networking policy of Server or Both*/
	UFUNCTION(Server, reliable, WithValidation)
	virtual void S_ConstructServerItemComponent(UDA_CoreItem* CoreItem, FS_UniqueID UniqueID, UIT_ItemComponentTrait* Trait, AActor* Instigator, TSubclassOf<UItemComponent> ItemComponent, FGameplayTag Event = FGameplayTag(), FItemComponentPayload Payload = FItemComponentPayload());

	/**Constructs item Component with networking policy of Client*/
	UFUNCTION(Client, reliable)
	virtual void C_ConstructClientItemComponent(UDA_CoreItem* CoreItem, FS_UniqueID UniqueID, UIT_ItemComponentTrait* Trait, AActor* Instigator, TSubclassOf<UItemComponent> ItemComponent, FGameplayTag Event = FGameplayTag(), FItemComponentPayload Payload = FItemComponentPayload());
	
	/**Check if @Item will fit in the desired tile.
	 * @Optimize: To fill in the @ItemsInTheWay array, this function keeps going
	 * even after it finds out that something is in the way. By checking this to
	 * true, this function will stop instantly when *anything* is in the way.
	 * Though this will mean the @ItemsInTheWay array will always be empty.*/
	UFUNCTION(BlueprintCallable, Category = "Items|Checkers")
	void CheckForSpace(FS_InventoryItem Item, FS_ContainerSettings Container, int32 TopLeftIndex, TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable, int32& AvailableTile,
		TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize = false);

	/**Calls CheckForSpace, but for all rotations.*/
	UFUNCTION(BlueprintCallable, Category = "Items|Checkers")
	void CheckAllRotationsForSpace(FS_InventoryItem Item, const FS_ContainerSettings Container, const int32 TopLeftIndex, TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable,
		TEnumAsByte<ERotation>& NeededRotation, int32& AvailableTile, TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize = false);

	/**Check if @Shape will fit in the desired tile. This should be in local space.
	 * @Optimize: To fill in the @ItemsInTheWay array, this function keeps going
	 * even after it finds out that something is in the way. By checking this to
	 * true, this function will stop instantly when *anything* is in the way.
	 * Though this will mean the @ItemsInTheWay array will always be empty.*/
	UFUNCTION(BlueprintCallable, Category = "Items|Checkers")
	void CheckForSpaceForShape(TArray<FIntPoint> Shape, FS_ContainerSettings Container, int32 TopLeftIndex, TArray<FS_InventoryItem> ItemsToIgnore, TArray<int32> TilesToIgnore, bool& SpotAvailable, int32& AvailableTile,
		TArray<FS_InventoryItem>& ItemsInTheWay, bool Optimize = false);
	
	/**Check if the item can be split*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Items|Checkers", meta = (ReturnDisplayName = "CanSplit"))
	bool CanSplitItem(FS_InventoryItem Item, int32 SplitAmount, UAC_Inventory* DestinationComponent, int32 NewStackContainerIndex, int32 NewStackTileIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Items|Checkers", meta = (ReturnDisplayName = "CanSwap"))
	bool CanSwapItemLocations(FS_InventoryItem Item1, FS_InventoryItem Item2, TEnumAsByte<ERotation>& Item1RequiredRotation, TEnumAsByte<ERotation>& Item2RequiredRotation);

	/**Override settings can be dangerous to expose to any actor
	 * in a multiplayer session. This function is called when
	 * UpdateItemsOverrideSettings is called to validate if an
	 * actor has permission to update the settings.
	 * By default, this always returns true.*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Items|Checkers")
	bool CanActorChangeItemOverrideSettings(AActor* Actor, FS_InventoryItem Item);

	/**Find out if @Item is allowed into @Container.
	 * This can be overriden at a blueprint level and the parent function should be called
	 * at the end of the blueprint function, if any requirements are not met, you should
	 * return false. If any are met, do not return and call the parent function.
	 * Look at the blueprint parent of this component for an example.
	 * This allows you to inject Blueprint level systems your project may contain into
	 * this function.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Items|Checkers", meta = (ReturnDisplayName = "Is Allowed"))
	bool CheckCompatibility(FS_InventoryItem Item, FS_ContainerSettings Container);

	/**Go through all container settings and return true if any
	 * item found is valid.
	 * This does not include items inside ThisActor containers.*/ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Items|Checkers")
	bool HasAnyItems();

#pragma endregion


#pragma region Getters
	
	/**Goes through all containers and returns the first container found with the given name.
	 * This excludes all containers that belong to an item, since you should not use this function for finding those containers.
	 * This behavior can be disabled by checking @AllowItemsContainers to true. You'll only want to disable this when you
	 * are passing an items containers into the @ContainerList */
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetContainerByIdentifier(FGameplayTag ContainerIdentifier, TArray<FS_ContainerSettings> ContainerList, bool AllowItemsContainers, bool& FoundContainer,FS_ContainerSettings& Container, int32& ContainerIndex);

	/*Goes through all containers and finds the first item with a matching UniqueID.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FS_InventoryItem GetItemByUniqueID(FS_UniqueID UniqueID);

	/**A more optimized version of GetItemByUniqueID by just searching a specific container.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (DisplayName = "Get Item By UniqueID In Container"))
	void GetItemByUniqueIDInContainer(FS_UniqueID UniqueID, int32 ContainerIndex, bool& ItemFound, FS_InventoryItem& Item);

	/**Gets all items until the count is met. If you have 3 stacks of ammo and each stack is 30,
	 * and you pass in 60 into @Count, this will return 2 of those stacks of ammo.
	 * This can overflow, for example if you put 31 into @Count, with the above stack example,
	 * it'll return 2 stacks, the count will tell you how much from each stack you should deduct.
	 * This could be ideal for a reload mechanic, or deducting currency from a component when
	 * it buys an item.
	 * If @ContainerIndex is -1, this will check all containers.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_ItemCount> GetListOfItemsByCount(UDA_CoreItem* Item, int32 Count, int32 ContainerIndex, int32& TotalFoundCount);
	
	/**Get an item at a specific container and specific tile index. "Item" is only valid if "ItemFound" is true.
	 * This should not be used to check for collision, use "CheckForSpace" or "CheckBothRotationsForSpace"
	 * This does not require a top left index.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (ReturnDisplayName = "Item"))
	FS_InventoryItem GetItemAtSpecificIndex(FS_ContainerSettings Container, int32 TileIndex);

	/**Find the first available space for an item. Also checks for both rotations if possible.
	 * This does NOT try to stack the item or find any items to stack with. Only finds a free tile.
	 * This does NOT check if the Item is compatible with the container, you should do that before
	 * calling this function.
	 * NOTE: If you are overriding this function at a blueprint level, I suggest you do NOT call
	 * the parent function unless you want the last resort to be the default component behavior.
	 * If you are going to call the parent function, call it at the end of the blueprint version.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetFirstAvailableTile(FS_InventoryItem Item, FS_ContainerSettings Container, const TArray<int32>& IndexesToIgnore, bool& SpotFound, int32& AvailableTile, TEnumAsByte<ERotation>& NeededRotation);

	/**Find the first available space for an item in the first container that is compatible. Checks alls rotations.
	 * This does NOT try to stack the item or find any items to stack with. Only finds a free tile.
	 * If a container is infinite, but not tiles  are available, the @AvailableTile will return as -1, and @SpotFound
	 * will return true. This is an indicator that the container needs to be expanded.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Getters")
	void GetFirstAvailableContainerAndTile(FS_InventoryItem Item, const TArray<int32>& ContainersToIgnore, bool& SpotFound, FS_ContainerSettings& AvailableContainer, int32& AvailableTile, TEnumAsByte<ERotation>& NeededRotation);
	
	/**Get all containers that belong to "Item".
	 * To use this while in editor, you must fill in @Item's ContainerIndex and ItemIndex.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (ReturnDisplayName = "Containers"))
	TArray<FS_ContainerSettings> GetItemsChildrenContainers(FS_InventoryItem Item);

	/**Gets the widget version of a container.
	 * You can also use this to find out if a widget has been created
	 * for a container by checking if @Widget is valid.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (DeprecatedFunction = "Moved to helper library, FL_InventoryFramework -> GetWidgetForContainer"))
	void GetWidgetForContainer(FS_ContainerSettings Container, UW_Container*& Widget);

	/**Gets the widget version of a item. Container widget must also be valid.
	 * You can also use this to find out if a widget has been created
	 * for a item by checking if "Widget" is valid.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (DeprecatedFunction = "Moved to helper library, FL_InventoryFramework -> GetWidgetForItem"))
	void GetWidgetForItem(FS_InventoryItem Item, UW_InventoryItem*& Widget);

	/**Go through all containers and get their widgets.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<UW_Container*> GetAllContainerWidgets();

	/**Go through all containers and remove their widgets.
	 * This should only be called if you are CERTAIN the player
	 * is not interacting with any container widgets.
	 * Primarily used for garbage collection.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void RemoveAllContainerWidgets();
	

	/**Get all items, and the items inside those items and so forth.
	 * Useful for when you're working with infinite containers and you need
	 * all items associated with the master item.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetChildrenItems(FS_InventoryItem Item, TArray<FS_ItemSubLevel>& AssociatedItems);

	/**Get all containers, and the containers belonging to the items inside those containers and so forth.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetAllContainersAssociatedWithItem(FS_InventoryItem Item, TArray<FS_ContainerSettings>& Containers);

	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_ContainerSettings> GetContainerSettingsForSpawningItemActor(FS_InventoryItem Item);

	/**This does the reverse of GetChildrenItems,
	 * this will get all items that are holding this item, down the ownership
	 * tree. So if "Item" is a bullet, which is inside a magazine and that
	 * magazine is attached to a gun, this would return; Magazine Level 0, Gun Level 1.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetParentItems(FS_InventoryItem Item, TArray<FS_ItemSubLevel>& ParentItems);

	/**Get the item that owns this item. For example, if @Item was a sight attached
	 * to a gun, this would return the gun.*/
	UFUNCTION(BlueprintCallable, Category = "Getters", meta = (ReturnDisplayName = "ParentItem"))
	FS_InventoryItem GetParentItem(FS_InventoryItem Item);
	
	/**Get all items inside this component.
	 * @param ContainerIndex if -1, we go through ALL containers.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_InventoryItem> GetAllItems(int32 ContainerIndex = -1);
	
	/**Get all items that share the same data asset.
	 * @param ContainerIndex if set to -1, this will search all containers.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	void GetAllItemsWithDataAsset(UDA_CoreItem* DataAsset, int32 ContainerIndex, TArray<FS_InventoryItem>& Items, int32& TotalCountFound);

	/**Get all the indexes of all the tiles this item is occupying.
	 * This should only be called from a Grid container, since all other
	 * container styles will just return the items current tile index.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters")
	void GetItemsTileIndexes(FS_InventoryItem Item, TArray<int32>& Indexes, bool& InvalidTileFound);
	//TODO: Move this to function library

	/**This simply returns all locked and hidden tiles as those are typically viewed as tiles the functions should ignore*/
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent, Category = "Getters")
	TArray<int32> GetGenericIndexesToIgnore(FS_ContainerSettings Container);

	/**Gets all nearby items for @Item.
	 * @Range dictates how far we scan from each of the items items.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_InventoryItem> GetNearbyItems(FS_InventoryItem Item, int32 Range = 1);

	/**Gets nearby items for @Item, in a specific direction.
	 * This uses Bresenham's line algorithm, this is not perfect for
	 * uneven directions, such as X=2 and Y=1. Ideally, you want to
	 * keep both directions the same, or one of them as 0, since this
	 * can suffer from the same problems as anti aliasing. The only
	 * way around this would be to increase the resolution of all
	 * items and containers.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_InventoryItem> GetNearbyItemsDirectional(FS_InventoryItem Item, FIntPoint Direction, TArray<int32>& Tiles);

	/**Get all equipment containers that currently do not have any
	 * items inside of them.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_ContainerSettings> GetAvailableEquipmentContainers();

	/**Get all equipment containers that are compatible with an item.
	 * @OnlyEmptyContainers if true, will only return a list of containers
	 * that have nothing in them. If false, simply get all equipment
	 * containers that meet the compatibility check.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_ContainerSettings> GetCompatibleEquipContainersForItem(FS_InventoryItem Item, bool OnlyEmptyContainers = true);

	/**Return a list of all containers in this component that
	 * the @Item is compatible with.
	 * @ContainersFilter if filled, check those containers. If empty,
	 * then search all containers.*/
	UFUNCTION(BlueprintCallable, Category = "Getters")
	TArray<FS_ContainerSettings> GetCompatibleContainersForItem(FS_InventoryItem Item, TArray<FS_ContainerSettings> ContainersFilter);

#pragma endregion


#pragma region Tiles
	
	/**Usually used to update the color of the highlight to communicate stacking, item merging, etc.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Tiles|Visuals")
	void UpdateHighlight(UW_HighlightWidget* HighlightWidget);

	/**Adds the @Tags to the specified containers TileTag's to grant a tile unique properties,
	 * such as hiding or locking the tile.*/
	UFUNCTION(BlueprintCallable, Category = "Tiles|Tags")
	void AddTagsToTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags);

	UFUNCTION(Server, Reliable, Category = "Tiles|Tags")
	void S_AddTagsToTile(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable, Category = "Tiles|Tags")
	void C_AddTagsToTile(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags);

	void Internal_AddTagsToTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags);

	/**Adds the @Tags to the specified containers TileTag's to grant a tile unique properties,
	* such as hiding or locking the tile.*/
	UFUNCTION(BlueprintCallable, Category = "Tiles|Tags")
	void RemoveTagsFromTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags);

	UFUNCTION(Server, Reliable, Category = "Tiles|Tags")
	void S_RemoveTagsFromTile(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable, Category = "Tiles|Tags")
	void C_RemoveTagsFromTile(FS_UniqueID ContainerID, int32 TileIndex, FGameplayTagContainer Tags);

	void Internal_RemoveTagsFromTile(FS_ContainerSettings Container, int32 TileIndex, FGameplayTagContainer Tags);

#pragma endregion


#pragma region Containers
	
	/**Prep a container to be displayed on the screen.
	 * This calls "ConstructContainer" for the widget.
	 * This should only be called once per widget.*/
	UFUNCTION(BlueprintCallable, Category = "Containers")
	void BindContainerWithWidget(FS_ContainerSettings Container, UW_Container* Widget, bool& Success);

	/**Get all containers this component owns. This excludes
	 * all containers that belong to items.*/
	UFUNCTION(BlueprintCallable, Category = "Containers")
	TArray<FS_ContainerSettings> GetComponentOwningContainers();

	/**Finds a container and returns if it found.
	 * Index is returned as well in case you are dealing with a container
	 * who's ContainerIndex has not yet been updated.*/
	UFUNCTION(BlueprintCallable, Category = "Containers")
	FS_ContainerSettings GetContainerByUniqueID(FS_UniqueID UniqueID);

	/**Check whether the @Container belongs to the component. As in,
	 * it does not belong to an item.*/
	UFUNCTION(BlueprintCallable, Category = "Containers")
	bool DoesContainerBelongToComponent(FS_ContainerSettings Container);

	/**We use FMargin here since we can't assume the growth direction of the container.
	 * This function will attempt to expand or shrink a specific container.
	 * You shrink by putting minus values into the "Expansion" and expand the container
	 * by putting positive values in "Expansion".
	 * This will call "ContainerSizeAdjusted" delegate on both client and server.
	 * This will not shrink a container smaller than 1x1.
	 * The container must be initialized.
	 * The adjustments are automatically truncated, as the numbers need to be whole numbers.
	 * Use with care, this can get very expensive if items have to be moved.
	 * If shrinking, @ClampToItems will prevent the container from shrinking past items.*/
	UFUNCTION(BlueprintCallable, Category = "Containers")
	void AdjustContainerSize(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems);
	
	UFUNCTION(Server, Reliable, Category = "Containers")
	void S_AdjustContainerSize(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems, ENetRole CallerLocalRole);
	
	UFUNCTION(Client, Reliable, Category = "Containers")
	void C_AdjustContainerSize(FS_UniqueID ContainerID, FMargin Adjustments, bool ClampToItems, FRandomStream Seed);

	UFUNCTION(BlueprintCallable, Category = "Containers", meta = (DisplayName = "Adjust Container Size (Non-replicated)"))
	bool Internal_AdjustContainerSize(FS_ContainerSettings Container, FMargin Adjustments, bool ClampToItems, FRandomStream Seed);
	
	
	//--------------------
	// Container Tags

	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.*/
	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	bool AddTagToContainer(FS_ContainerSettings Container, FGameplayTag Tag);
 
	UFUNCTION(Server, WithValidation, Reliable)
	void S_AddTagToContainer(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole);
 
	UFUNCTION(Client, Reliable)
	void C_AddTagToContainer(FS_UniqueID ContainerID, FGameplayTag Tag);
 
	void Internal_AddTagToContainer(FS_ContainerSettings Container, FGameplayTag Tag);

	/**Asked by the AddTagToContainer function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Containers|Tags")
	bool CanTagBeAddedToContainer(FGameplayTag Tag, FS_ContainerSettings Container);
 
	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.*/
	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	bool RemoveTagFromContainer(FS_ContainerSettings Container, FGameplayTag Tag);
 
	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveTagFromContainer(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole);
 
	UFUNCTION(Client, Reliable)
	void C_RemoveTagFromContainer(FS_UniqueID ContainerID, FGameplayTag Tag);
 
	void Internal_RemoveTagFromContainer(FS_ContainerSettings Container, FGameplayTag Tag);

	/**Asked by the RemoveTagFromContainer function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Containers|Tags")
	bool CanTagBeRemovedFromContainer(FGameplayTag Tag, FS_ContainerSettings Container);

	//--------------------
	
	
	//--------------------
	// Container Tag Values
	
	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.
	 * @Item The item to modify
	 * @Tag The tag to add
	 * @Value The value to associated with the tag
	 * @CalculationClass What class should we use to adjust the calculation, if any?
	 * @AddIfNotFound If the tag value does not exist on the item, do we add it?*/
	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	bool SetTagValueForContainer(FS_ContainerSettings Container, FGameplayTag Tag, float Value, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool AddIfNotFound = true);
	
	UFUNCTION(Server, WithValidation, Reliable)
	void S_SetTagValueForContainer(FS_UniqueID ContainerID, FGameplayTag Tag, float Value, ENetRole CallerLocalRole, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool AddIfNotFound = true);
	
	UFUNCTION(Client, Reliable)
	void C_SetTagValueForContainer(FS_UniqueID ContainerID, FGameplayTag Tag, float Value, bool AddIfNotFound = true);
	
	void Internal_SetTagValueForContainer(FS_ContainerSettings Container, FGameplayTag Tag, float Value, bool AddIfNotFound, TSubclassOf<UO_TagValueCalculation> CalculationClass, bool& Success);

	float PreContainerTagValueCalculation(FS_TagValue TagValue, FS_ContainerSettings Container, TSubclassOf<UO_TagValueCalculation> CalculationClass);
	
	/**Asked by the RemoveTagFromContainer function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Containers|Tags")
	bool CanTagValueBeSetForContainer(FS_TagValue TagValue, FS_ContainerSettings Container);

	/**Add a tag to an item. Returns true or false depending if the addition was successful.
	 * This is predictive, if ran on the client, the return might not work.*/
	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	bool RemoveTagValueFromContainer(FS_ContainerSettings Container, FGameplayTag Tag);
	
	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveTagValueFromContainer(FS_UniqueID ContainerID, FGameplayTag Tag, ENetRole CallerLocalRole);
	
	UFUNCTION(Client, Reliable)
	void C_RemoveTagValueFromContainer(FS_UniqueID ContainerID, FGameplayTag Tag);
	
	void Internal_RemoveTagValueFromContainer(FS_ContainerSettings Container, FGameplayTag Tag);

	/**Asked by the RemoveTagFromContainer function, if this returns false, that function will fail.
	 * If multiplayer, this is only asked on the server.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Containers|Tags")
	bool CanTagValueBeRemovedFromContainer(FGameplayTag Tag, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	TArray<FS_ContainerSettings> GetContainersByTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	TArray<FS_ContainerSettings> GetContainersByTagValue(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Containers|Tags")
	TArray<FS_ContainerSettings> GetContainersByTagQuery(FGameplayTagQuery TagQuery);

	UFUNCTION(BlueprintCallable, Category = "Containers|Helpers")
	TArray<FS_ContainerSettings> GetContainersByType(TEnumAsByte<EContainerType> ContainerType);
	

	//--------------------

#pragma endregion
	

#pragma region Component
	
	/**Generates a new UniqueID. This UniqueID can be used either for containers or items.
	 *  This will only return a valid UniqueID if called from the server.
	 *  If you need to generate a UniqueID for a client, use GenerateUniqueIDWithSeed.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Initializers", meta = (CompactNodeTitle = "GenerateID"))
	FS_UniqueID GenerateUniqueID();

	/**Generates a UniqueID based on a seed. This is ideal for ensuring that a client and
	 * server end up generating the same UniqueID.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Initializers", meta = (CompactNodeTitle = "GenerateID (Seed)", DisplayName = "Gnerate UniqueID With Seed"))
	FS_UniqueID GenerateUniqueIDWithSeed(FRandomStream Seed);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Initializers", meta = (DisplayName = "Is UniqueID In Use"))
	bool IsUniqueIDInUse(FS_UniqueID UniqueID);

	/**Add tags to the components tag container.
	 * If called from a client, this will automatically call the server
	 * function and replicate.
	 *
	 * Disable @Broadcast if you do not want the delegate to go off.*/
	UFUNCTION(BlueprintCallable, Category = "Component|Tags")
	void AddTagsToComponent(const FGameplayTagContainer Tags, bool Broadcast = true);

	UFUNCTION(Server, Unreliable)
	void S_AddTagsToComponent(FGameplayTagContainer Tags, bool Broadcast = true);

	UFUNCTION(NetMulticast, Unreliable)
	void MC_TagsAddedToComponent(FGameplayTagContainer Tags);

	/**Remove tags from the components tag container.
	 * If called from a client, this will automatically call the server
	 * function and replicate.
	 *
	 * Disable @Broadcast if you do not want the delegate to go off.*/
	UFUNCTION(BlueprintCallable, Category = "Component|Tags")
	void RemoveTagsFromComponent(const FGameplayTagContainer Tags, bool Broadcast = true);

	UFUNCTION(Server, Unreliable)
	void S_RemoveTagsFromComponent(FGameplayTagContainer Tags, bool Broadcast = true);

	UFUNCTION(NetMulticast, Unreliable)
	void MC_TagsRemovedFromComponent(FGameplayTagContainer Tags);

	/**Add tags to the components tag value container.
	 * If called from a client, this will automatically call the server
	 * function and replicate
	 *
	 * Disable @Broadcast if you do not want the delegate to go off.*/
	UFUNCTION(BlueprintCallable, Category = "Component|Tags")
	void SetTagValueForComponent(const FS_TagValue TagValue, bool AddIfNotFound = true, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool Broadcast = true);

	UFUNCTION(Server, Unreliable)
	void S_SetTagValueForComponent(FS_TagValue TagValue, bool AddIfNotFound = true, TSubclassOf<UO_TagValueCalculation> CalculationClass = nullptr, bool Broadcast = true);

	UFUNCTION(NetMulticast, Unreliable)
	void MC_TagValueUpdatedForComponent(FS_TagValue TagValue, float OldValue);

	float PreComponentTagValueCalculation(FS_TagValue TagValue, UAC_Inventory* Component, TSubclassOf<UO_TagValueCalculation> CalculationClass);

	/**Remove tags from the components tag container.
	 * If called from a client, this will automatically call the server
	 * function and replicate.
	 *
	 * Disable @Broadcast if you do not want the delegate to go off.*/
	UFUNCTION(BlueprintCallable, Category = "Component|Tags")
	void RemoveTagValueFromComponent(const FGameplayTag TagValue, bool Broadcast = true);

	UFUNCTION(Server, Unreliable)
	void S_RemoveTagValueFromComponent(FGameplayTag TagValue, bool Broadcast = true);

	UFUNCTION(NetMulticast, Unreliable)
	void MC_TagValueRemovedFromComponent(FS_TagValue TagValue);

	/**Returns the total value of the @Tag from all items inside containers
	 * matching the container type set in @ContainersToCheck*/
	UFUNCTION(Category = "Tags", BlueprintCallable)
	float GetTotalValueOfTag(FGameplayTag Tag, TArray<TEnumAsByte<EContainerType>> ContainersToCheck, bool Items = true, bool Containers = false, bool Component = false);

#pragma endregion
	

#pragma region NetworkManagement
	
	//--------------------
	// Networking functions
		
	/**Whenever a inventory is modified, it attempts to send an RPC event to all listeners.
	 * If an actors component isn't added as a listener, networking will not work for that actor
	 * This should always be the component on the players currently possessed actor.*/
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Networking||Management")
	void S_AddListener(UAC_Inventory* Component);

	/**Remove a component as a listener for this component. They will no longer receive RPC
	 * updates whenever a container or item is modified inside this component.
	 * It is important you call this when the player is done interacting with another  component.
	 * Otherwise they will keep receiving irrelevant RPC's.*/
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Networking||Management")
	void S_RemoveListener( UAC_Inventory* Component);
	
	UFUNCTION(Client, Reliable)
	void C_SetClientReceivedContainerData(bool HasReceived);

	/**Start the process of retrieving data from the server and sending it to the client.
	 * Optionally, you can skip calling OnDataReceivedFromOtherComponent in case all
	 * you want is to replicate the container settings and have no events fire.*/
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "Networking||Client")
	void C_RequestServerDataFromOtherComponent(UAC_Inventory* OtherComponent, bool CallDataReceived = true);

	UFUNCTION(Server, Reliable)
	void S_SendDataFromOtherComponent(UAC_Inventory* OtherComponent, bool CallDataReceived = true);

	UFUNCTION(Client, Reliable)
	void C_ReceiveDataFromOtherComponent(UAC_Inventory* OtherComponent, const TArray<FS_ContainerSettings> &Containers, bool CallDataReceived = true, bool CallComponentStarted = true);

	/**The client has requested data from the server. The server has at this point
	 * processed the request. The client has received the data and is ready to proceed.*/
	UFUNCTION(BlueprintImplementableEvent)
	void OnDataReceivedFromOtherComponent(UAC_Inventory* OtherComponent);

	/**Adds an item to the network queue. This calls the corresponding functions
	 * on both the parent component and the item widget (if valid).
	 * This should rarely, if ever, be called on the server.*/
	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Networking||Client")
	void C_AddItemToNetworkQueue(FS_UniqueID ItemID);

	/**Removes an item to the network queue. This calls the corresponding functions
	 * on both the parent component and the item widget (if valid).
	 * Server can call this in case the server function fails for whatever reason,
	 * and because of that does not want to waste a potentially big RPC call just
	 * to remove the item from the queue.*/
	UFUNCTION(BlueprintCallable, CLient, Reliable, Category = "Networking||Client")
	void C_RemoveItemFromNetworkQueue(FS_UniqueID ItemID);

	/**An item has been added to the network queue, this is where you want to hook in any
	 * indicators for the player to knw that the client is waiting for a response from the server.
	 * This is typically represented by making the item gray-scale or start flashing.*/
	UFUNCTION(BlueprintImplementableEvent)
	void ItemAddedToNetworkQueue(FS_InventoryItem Item, UW_InventoryItem* ItemWidget);

	/**An item has been removed from the network queue, the server has processed whatever
	 * request the client attempted to perform and the item should be synced with the
	 * server data. This is where you want to remove any UI indicators you might have for
	 * the player that tells them the item was waiting for a server response.*/
	UFUNCTION(BlueprintImplementableEvent)
	void ItemRemovedFromNetworkQueue(FS_InventoryItem Item, UW_InventoryItem* ItemWidget);

	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Networking||Client")
	void C_AddAllContainerItemsToNetworkQueue(FS_UniqueID ContainerID);

	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Networking||Client")
	void C_RemoveAllContainerItemsFromNetworkQueue(FS_UniqueID ContainerID);

#pragma endregion

#pragma region Editor

	#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostLoad() override;

	virtual void OnComponentCreated() override;

	#endif

#pragma endregion

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void LogItemFailedToSpawn(FS_InventoryItem Item, FString Reason);
};
