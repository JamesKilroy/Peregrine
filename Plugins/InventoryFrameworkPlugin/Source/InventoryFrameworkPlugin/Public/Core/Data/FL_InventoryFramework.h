// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Animation/AnimationAsset.h"
#include "Core/Traits/ItemTrait.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FL_InventoryFramework.generated.h"


class UDragDropOperation;
struct FEventReply;
class UInputAction;
class UInputMappingContext;
class FProperty;

/**Used by the preview actor to get a default animation for skeletal meshes.*/
USTRUCT(BlueprintType)
struct FPreviewAnimationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Animation")
	FString ComponentObjectName;

	UPROPERTY(BlueprintReadWrite, Category="Animation")
	UAnimationAsset* Animation = nullptr;
};


UCLASS(meta = (BlueprintThreadSafe))
class INVENTORYFRAMEWORKPLUGIN_API UFL_InventoryFramework : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:

#pragma region Sorting
	
	
	//Sort the items in a container by their tile index.
	UFUNCTION(BlueprintCallable, Category = "IFP|Sorting Functions")
	static void SortItemsByIndex(UPARAM(ref) TArray <FS_InventoryItem> &Array_To_Sort, TArray <FS_InventoryItem> &Sorted_Array);

	/**Sort the items in a container alphabetically. This does not update any
	 * item positions.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Sorting Functions")
	static void SortItemsAlphabetically(UPARAM(ref) TArray <UDA_CoreItem*> &Array_To_Sort, TArray <UDA_CoreItem*> &Sorted_Array);

	UFUNCTION(BlueprintCallable, Category = "IFP|Sorting Functions")
	static TArray <FS_InventoryItem> SortItemStructsAlphabetically(TArray <FS_InventoryItem> ArrayToSort);

	/**Sorting by type is generally not recommended as it is an enum that is not
	 * localized and changing the order can cause issues with Blueprints.
	 * What I do recommend is using GameplayTags and having some way of associating
	 * a FText, which can be translated, and then sorting by that text.
	 * The other solution would be to have some way of associating a FText with the
	 * enum and skipping the GameplayTags.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Sorting Functions")
	static TArray <FS_InventoryItem> SortItemStructsByType(TArray <FS_InventoryItem> ArrayToSort);

	UFUNCTION(BlueprintCallable, Category = "IFP|Sorting Functions")
	static TArray <FS_InventoryItem> SortItemsByContainerAndIndex(TArray <FS_InventoryItem> ArrayToSort);

	//Sorts containers by ContainerIndex, then BelongsToItem.X, then BelongsToItem.Y
	//Remember to call RefreshIndexes from AC_Inventory so all the containers keep the proper ContainerIndex.
	UFUNCTION(BlueprintCallable, Category = "InventoryFunctions|Sorting Functions")
	static void SortContainers(UPARAM(ref) TArray<FS_ContainerSettings> &Array_To_Sort, TArray<FS_ContainerSettings> &Sorted_Array);

	
#pragma endregion

	
#pragma region Duplication

	
	/**WARNING: If you duplicate a component and attach it to an actor, if they have the same name, the original
	 * component will be destroyed.
	 * This automatically detaches the object from the actor if it's a component.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Duplication", meta = (DeterminesOutputType = "InputObject"))
	static void CloneObject(UObject* InputObject, FName ObjectName, UObject* ObjectOwner, UObject* &OutputObject);

	/**Renames a component. Always try to give it a name that is not identical to another components as
	 * that can lead to odd behavior and maybe even crashes.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Duplication")
	static void RenameComponent(UActorComponent* Component, const FString NewName);

	/**The preview actor will break if any components have the same name, this will generate a name
	 * that is safe to be used whenever you call @RenameComponent.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Duplication")
	static FString GenerateSafeNameForComponent(AActor* Actor, UActorComponent* Component, FRandomStream Seed);

	
#pragma endregion
	
	
#pragma region Helpers

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|HelpersFunctions", meta = (CompactNodeTitle = "==", DisplayName = "UniqueID Equals UniqueID"))
	static bool UniqueIDequalsUniqueID(FS_UniqueID UniqueIDA, FS_UniqueID UniqueIDB);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items", meta = (DisplayName = "Is UniqueID Valid"))
	static bool IsUniqueIDValid(FS_UniqueID UniqueID);

	/**Adds an external widget to an item and/or container.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Items", meta = (DefaultToSelf = "Object"))
	static void AddExternalObjectToItem(UObject* Object, FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Items", meta = (DefaultToSelf = "Object"))
	static void RemoveExternalObjectFromItem(UObject* Object, FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Items")
	static TArray<UObject*> GetExternalObjectsFromItem(FS_InventoryItem Item);

	/**Adds an external widget to an item and/or container.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Containers", meta = (DefaultToSelf = "Object"))
	static void AddExternalObjectToContainer(UObject* Object, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Containers", meta = (DefaultToSelf = "Object"))
	static void RemoveExternalObjectFromContainer(UObject* Object, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions|External Objects|Containers")
	static TArray<UObject*> GetExternalObjectsFromContainer(FS_ContainerSettings Container);

	/**Helper function for those who want to change where the inventory component lives.
	 * For example: The example project has the component on the player character, so
	 * to retrieve it, you would have GetPlayerPawn all around your project, then calling
	 * the GetInventoryComponent interface on it.
	 *
	 * This function lets you change that behavior in one spot. If you want the component
	 * living on the player controller, then modify this function to get the component
	 * on the player controller.
	 *
	 * This is used to fetch what might be class defaults, but might also be data that
	 * changes during runtime.
	 * For example: The Color settings can be fetched from the class defaults, but
	 * the player might want to change their color settings.
	 * Same goes for keybinds and generated item icons.
	 * Ideally, you should replace most of these things with your own system, such
	 * as the color and keybind things should be their own systems in your game.
	 *
	 * For multiplayer games, this should never be used in dedicated server code since there
	 * is no local inventory component, only client code. Retrieving the local inventory
	 * is also essential for RPC's to function correctly since the player can only
	 * send server RPC's through a component it has authority over.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP", meta = (WorldContext = "WorldContext", DefaultToSelf = "WorldContext"))
	static UAC_Inventory* GetLocalInventoryComponent(UObject* WorldContext);

	/**Get the first component with the specified tag, or the first attached actor
	 * with the specified tag.
	 * @ClimbDownHierarchy If true, instead of going down the hierarchy, asking if the
	 * children actors have the component, we will get parent actors instead.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|HelperFunctions", meta = (DefaultToSelf = "Actor"))
	static bool GetComponentByTag(AActor* Actor, FName Tag, USceneComponent*& Component, AActor*& AttachedActor, bool ClimbUpHierarchy = false);

	
#pragma endregion
	
	
#pragma region Items

	
	/**Checks if any of the settings in the ItemOverride aren't set to their default.
	 * This does not load the inventory image soft object reference, but does check
	 * if the soft object path is valid.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Checkers")
	static bool IsItemOverridden(FS_InventoryItem Item);

	/**Check if Item1 can stack with Item2*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Checkers", meta = (ReturnDisplayName = "CanStack"))
	static bool CanStackItems(FS_InventoryItem Item1, FS_InventoryItem Item2);

	/**Find out if an item is out of the bounds of a grid or list container.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Checkers", meta = (ReturnDisplayName = "Out of Bounds"))
	static bool IsItemOutOfBounds(FS_InventoryItem Item, FS_ContainerSettings Container);

	/**Resolve whether the BuyerComponent meets the requirements to buy an item.
	 * If the item belongs to the BuyerComponent, this will always return true.
	 * @param ItemToPurchase The item to check the price of.
	 * @param BuyerComponent The component we will check for the items accepted currencies.
	 * @param Currency The currency BuyerComponent can use to exchange for the item. Returns null
	 * if player can not afford the item, unless ItemToPurchase already belongs to BuyerComponent.*/
	// UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InventoryFunctions|Items|Checkers", meta = (ReturnDisplayName = "Can Afford"))
	// static bool CanAffordItem(FS_InventoryItem ItemToPurchase, UAC_Inventory* BuyerComponent, UIDA_Currency*& Currency);

	/**Performs the same checks as CanAffordItem, but with a specified currency.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Checkers", meta = (ReturnDisplayName = "Can Afford"))
	static bool CanAffordItemWithSpecificCurrency(FS_InventoryItem ItemToPurchase, UAC_Inventory* BuyerComponent, UIDA_Currency* Currency);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Checkers")
	static bool IsItemEquipped(FS_InventoryItem Item);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items", meta = (CompactNodeTitle = "=="))
	static bool ItemEqualsItem(FS_InventoryItem ItemA, FS_InventoryItem ItemB);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items", meta = (CompactNodeTitle = "!="))
	static	bool ItemDoesNotEqualsItem(FS_InventoryItem ItemA, FS_InventoryItem ItemB);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items")
	static bool IsItemValid(FS_InventoryItem Item);

	/**Get the dimensions of an item. This takes into account if the item is rotated or not.
	 * This will automatically adapt to the style of the container the item is currently inside of.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters")
	static FIntPoint GetItemDimensions(FS_InventoryItem Item, bool IgnoreContainerStyle = false, bool IgnoreRotation = false);

	/**Get the dimensions of an item. This takes into account if the item is rotated or not.
	 * This will adjust the dimensions of the item depending on the container style.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters")
	static void GetItemDimensionsWithContext(FS_InventoryItem Item, FS_ContainerSettings Container, int32& X, int32& Y);

	/**Get the traits associated with an item by its tag.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Traits")
	static void GetTraitsByTagForItem(UDA_CoreItem* Item, FGameplayTag Tag, TArray<UItemTrait*> &FoundTraits);

	/**Get the trait associated with an item by its tag.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Traits", meta = (ReturnDisplayName = "Trait"))
	static UItemTrait* GetTraitByTagForItem(UDA_CoreItem* Item, FGameplayTag Tag);

	/**Get the traits associated with an item by its class.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Traits", meta=(DeterminesOutputType="Class", DynamicOutputParam="FoundTraits"))
	static void GetTraitsByClassForItem(UDA_CoreItem* Item, TSubclassOf<UItemTrait> Class, TArray<UItemTrait*> &FoundTraits, bool AllowChildren = false);

	/**Get the trait associated with an item by its class.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Traits", meta=(DeterminesOutputType="Class", ReturnDisplayName = "Trait"))
	static UItemTrait* GetTraitByClassForItem(UDA_CoreItem* Item, TSubclassOf<UItemTrait> Class, bool AllowChildren = false);

	/**The system is very strict on not allowing items to overlap one another, but the editor
	 * utility widget can temporarily disable collision checks. This means that items can
	 * start overlapping one another.
	 * This will return all tiles where two or more items are currently overlapping.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters")
	static TArray<int32> GetOverlappingTiles(FS_ContainerSettings Container);

	/**Get the accepted currencies for this item. This checks the overwrite settings.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Accepted Currencies"))
	static TArray<UIDA_Currency*> GetAcceptedCurrencies(FS_InventoryItem Item);

	/**Get the name of the item. This checks the overwrite settings.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Item Name"))
	static FText GetItemName(FS_InventoryItem Item);

	/**Get the item description. This checks the overwrite settings
	 * @RichTextWrapper The description will automatically replace any text that
	 * includes a tag wrapped with a < and > value with its value.
	 * For example: An item with a tag value IFP.Stat.Durability - 20 with this
	 * tooltip text "Current Durability: <Items.Stat.Durability>" will be turned
	 * into "Current Durability 20". The rich text wrapper will be placed before
	 * that value text and place a </> at the end to style your rich text.
	 * This way, you can highlight the values and have them update in game.
	 * In general, you should be putting the rich text wrapper into the tooltip
	 * text itself, instead of this. This is just for projects where you know
	 * every tag value will be styled the same.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Item Description"))
	static FText GetItemDescription(FS_InventoryItem Item, FString RichTextWrapper);

	/**Get the max count for the item. This can be infinite if it's a vendor or storage.
	 * This checks the overwrite settings. If it's infinite stacking, it'll return 214,748,3646*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Max Count"))
	static int32 GetItemMaxStack(FS_InventoryItem Item);

	/**Get the item price. This can optionally include the price of items this item contains.
	 * This checks the overwrite settings.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Price"))
	static float GetItemPrice(FS_InventoryItem Item, bool IncludeChildItems = true);

	/**Get the items shape inside of its container. This takes its rotation into account.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Shape"))
	static TArray<FIntPoint> GetItemsShape(FS_InventoryItem Item, bool& InvalidTileFound);

	/**Get the items shape inside of a specific container. This takes its rotation into account.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Shape"))
	static TArray<FIntPoint> GetItemsShapeWithContext(FS_InventoryItem Item, FS_ContainerSettings Container, bool& InvalidTileFound);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Attachment Widget"))
	static UW_AttachmentParent* GetItemsAttachmentWidget(FS_InventoryItem Item, bool CreateIfMissing, bool DoNotBind = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Getters", meta = (ReturnDisplayName = "Price"))
	static float GetItemsSpawnChance(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Items|Networking", meta = (ReturnDisplayName = "In Queue"))
	static bool IsItemInNetworkQueue(FS_InventoryItem Item);

	/**Go through the default tags from the items data asset and apply them to the item struct.*/
	UFUNCTION(Category = "IFP|Items|Tags", BlueprintCallable)
	static void AddDefaultTagsToItem(UPARAM(ref) FS_InventoryItem& Item, bool CallDelegates);

	/**Go through the default tag values from the data asset and apply them if they are missing.
	 * @param OverrideValues If a tag value is already on the item, should this function override
	 * the existing value?
	 * @param CallDelegates Call the items owning component delegates and the widget updates
	 * to the items external objects.*/
	UFUNCTION(Category = "IFP|Items|Tags", BlueprintCallable)
	static void AddDefaultTagValuesToItem(UPARAM(ref) FS_InventoryItem& Item, bool OverrideValues, bool CallDelegates);

	/**Return the tags on the item. Optionally append the items asset tags
	 * to retrieve all the tags this item has.*/
	UFUNCTION(Category = "IFP|Items|Tags", BlueprintCallable, BlueprintPure)
	static FGameplayTagContainer GetItemsTags(FS_InventoryItem Item, const bool IncludeAssetTags = true);

	/**Get the tag values on the item. Optionally append the items asset tag values
	 * to retrieve all the tag values this item has.*/
	UFUNCTION(Category = "IFP|Items|Tags", BlueprintCallable, BlueprintPure)
	static TArray<FS_TagValue> GetItemsTagValues(FS_InventoryItem Item, const bool IncludeAssetTagValues = true);

	/**Takes in a @Container and returns the item that owns it, if any.
	 * Remember to check if returned item is valid.*/
	UFUNCTION(Category = "IFP|Items|Tags", BlueprintCallable)
	static FS_InventoryItem GetOwningItemForContainer(FS_ContainerSettings Container);

	/**Gets the items instance, if valid. It might get constructed here as item instances
	 * can be created when requested.
	 * @DoNotConstruct if true, this will simply ignore the @ConstructOnRequest boolean in the
	 * item instances class defaults and never create an object, simply fetching an object
	 * if one exists.*/
	UFUNCTION(Category = "IFP|Items", BlueprintCallable, BlueprintPure)
	static UItemInstance* GetItemsInstance(FS_InventoryItem Item, bool DoNotConstruct);

	/**Takes in a possible out of date item struct and attempts
	 * to fetch the latest copy of it, then updating the passed in struct.
	 * For single player and servers, this should be completely safe to use,
	 * but for multiplayer games, this shouldn't be trusted on clients because
	 * of the latency delay.*/
	UFUNCTION(Category = "IFP|Items", BlueprintCallable)
	static void UpdateItemStruct(UPARAM(ref) FS_InventoryItem& Item);

	/**Get a list of all objects that might be relevant to any interface
	 * events related to the item, such as the item instance, item widget
	 * and external objects.*/
	UFUNCTION(Category = "IFP|Items", BlueprintCallable, BlueprintPure)
	static TArray<UObject*> GetObjectsForItemBroadcast(FS_InventoryItem Item, bool DoNotConstructItemInstance = true);

	UFUNCTION(Category = "IFP|Items", BlueprintCallable, BlueprintPure)
	static TArray<UObject*> GetObjectsForItemBroadcastWithInterface(FS_InventoryItem Item, TSubclassOf<UInterface> Interface,
		bool DoNotConstructItemInstance = true);

	/**Helper function for retrieving the items components.
	 * Recommended to use this over the raw access in the item struct,
	 * because that raw access might be removed in the future and moved
	 * into the external objects array.*/
	UFUNCTION(Category = "IFP|Items", BlueprintCallable, BlueprintPure)
	static TArray<UItemComponent*> GetItemsComponents(FS_InventoryItem Item);

	/**Gets the widget representation of an item.
	 * You can also use this to find out if a widget has been created
	 * for an item by checking if "Widget" is valid.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Items", meta = (ReturnDisplayName = "Widget"))
	static UW_InventoryItem* GetWidgetForItem(FS_InventoryItem Item);

	/**Attempt to get the visual representation of an item.
	 * Depending on how you created the item as a static/skeletal mesh or blueprint,
	 * either Component or Actor might be invalid, but if either are valid, this will
	 * return true. If neither are valid, this returns false.
	 *
	 * @CustomAppendedID This is only used to find specific items that have had special
	 * ID's appended to the end of their assigned tag.
	 * The only case this happens in default IFP is through the Equipment ItemComponent,
	 * where you fill in a CustomID. In that case, this function won't work because
	 * the tag that was given is no longer just the Item's UniqueID, it has extra
	 * information added to it.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Items|Getters")
	static bool GetItemsVisual(FS_InventoryItem Item, FName CustomAppendedID, USceneComponent*& Component, AActor*& Actor);

	/**Get the container this item resides in.*/
	UFUNCTION(Category = "IFP|Items|Getters", BlueprintCallable, BlueprintPure)
	static FS_ContainerSettings GetItemsParentContainer(FS_InventoryItem Item);

	/**Removes the items attachment widget from its external objects array and
	 * from the viewport.
	 * @MarkAsGarbage Completely destroy the widget and all references to
	 * it, allowing it to be garbage collected. The widget will have to
	 * be reconstructed from scratch if you want to re-use it. Putting this
	 * to true will always remove the widget from the viewport.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Items")
	static bool RemoveItemAttachmentWidget(FS_InventoryItem Item, bool MarkAsGarbage);
	
	
#pragma endregion

	
#pragma region Containers

	
	/**Resolve if a container is infinite. @Direction can return Neither, while also returning true in the
	 * case that this is a Data-Only container.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDisplayName = "Is Infinite"))
	static bool IsContainerInfinite(FS_ContainerSettings Container, TEnumAsByte<EContainerInfinityDirection>& Direction);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDisplayName = "Is Valid"))
	static bool IsContainerValid(FS_ContainerSettings Container);

	/**Get X and Y dimensions of a container.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Getters")
	static void GetContainerDimensions(FS_ContainerSettings Container, int32& X, int32& Y);

	/**Isn't this exactly the same as GetEmptyTilesAmount?*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Getters")
	static int32 GetNumberOfFreeTilesInContainer(FS_ContainerSettings Container);

	/**Isn't this exactly the same as GetNumberOfFreeTilesInContainer?*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Getters")
	static int32 GetEmptyTilesAmount(FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Getters")
	static bool IsContainerFull(FS_ContainerSettings Container);

	/**Find out if a container is currently waiting for a network event to finish.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Networking", meta = (ReturnDisplayName = "In Queue"))
	static bool IsContainerInNetworkQueue(FS_ContainerSettings Container);

	/**Check if an items container index, item index and tile index are correct.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDusplayName = "Is Valid"))
	static bool AreItemDirectionsValid(FS_UniqueID ItemID, int32 ContainerIndex, int32 ItemIndex);

	/**Can items have a unique shape? If false, items are bound to 1x1*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDusplayName = "Is Spacial"))
	static bool IsSpacialContainer(FS_ContainerSettings Container);

	/**Do the style settings support spacial settings? If false, items are bound to 1x1*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDusplayName = "Is Spacial"))
	static bool IsSpacialStyle(FS_ContainerSettings Container);

	/**Should this container support the tilemap system?
	 * If not, then this container will not have any type of
	 * collision system.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Containers|Checkers", meta = (ReturnDusplayName = "Is Spacial"))
	static bool DoesContainerSupportTileMap(FS_ContainerSettings Container);

	/**Gets the widget version of a container.
	 * You can also use this to find out if a widget has been created
	 * for a container by checking if @Widget is valid.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Containers|Getters", meta = (ReturnDisplayName = "Widget"))
	static UW_Container* GetWidgetForContainer(FS_ContainerSettings Container);

	/**Get a list of all objects that might be relevant to any interface
	 * events related to the item, such as the item instance, item widget
	 * and external objects.*/
	UFUNCTION(Category = "IFP|Containers", BlueprintCallable, BlueprintPure)
	static TArray<UObject*> GetObjectsForContainerBroadcast(FS_ContainerSettings Container);

	UFUNCTION(Category = "IFP|Containers", BlueprintCallable, BlueprintPure)
	static TArray<UObject*> GetObjectsForContainerBroadcastWithInterface(FS_ContainerSettings Container, TSubclassOf<UInterface> Interface);

	/**Takes in an array of containers, then removes any data you wouldn't
	 * want a preview actor to have. For example, the ExternalObjects array
	 * should be emptied for preview actors to not retrigger any logic unnecessarily */
	UFUNCTION(Category = "IFP|Containers", BlueprintCallable)
	static TArray<FS_ContainerSettings> SanitizeContainersForPreviewActor(TArray<FS_ContainerSettings> Containers);

	/**Removes any unnecessary data from the @Containers that clients do not need,
	 * like Server-only fragments, or data that is cheap to generate but expensive
	 * to replicate, like the TileMap.*/
	UFUNCTION(Category = "IFP|Containers", BlueprintCallable)
	static TArray<FS_ContainerSettings> SanitizeContainersForClientRPC(TArray<FS_ContainerSettings> Containers);

	/**Get a copy of the items tags. Modifying this container does NOT modify
	 * the items tags. Use the appropriate functions to modify an items tags.*/
	UFUNCTION(Category = "IFP|Containers|Tags", BlueprintCallable, BlueprintPure)
	static FGameplayTagContainer GetContainersTags(FS_ContainerSettings Container);

	/**Get a copy of the items tag values. Modifying this container does NOT modify
	 * the items tags. Use the appropriate functions to modify an items tags.*/
	UFUNCTION(Category = "IFP|Containers|Tags", BlueprintCallable, BlueprintPure)
	static TArray<FS_TagValue> GetContainersTagValues(FS_ContainerSettings Container);


#pragma endregion

	
#pragma region Tiles

	
	/**Resolve whether a tile is valid.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Checkers")
	static bool IsTileValid(int32 X, int32 Y, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Checkers")
	static bool IsTileMapIndexValid(int32 Index, FS_ContainerSettings Container);

	/**Convert an index to a X and Y location.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles")
	static void IndexToTile(int32 TileIndex, FS_ContainerSettings Container, int32& X, int32& Y);

	/**Convert a X and Y tile to index.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles", meta = (ReturnDisplayName = "Index"))
	static int32 TileToIndex(int32 X, int32 Y, FS_ContainerSettings Container);

	/**Get the padding to give to an item widget inside a container widget.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles")
	static void GetPaddingForTile(FVector2D TileDimensions, FS_ContainerSettings Container, int32 TileIndex, float& Left, float& Top);
	
	/**Returns all tiles within the dimension of a given point inside a container.
	 * @param Range The X and Y of your range. Remember, this counts 0's.
	 * @param Container Which container to go through
	 * @param StartingIndex The starting point of where to start the range check.
	 * This is the top left index.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Getters")
	static TArray<int32> GetTilesWithinDimension(FIntPoint Range, FS_ContainerSettings Container, int32 StartingIndex);

	/**Basic offset for a tile index, but ensures the tile is clamped inside the
	 * containers X and Y.
	 * @Remainder the remainder of @Offset after the offset.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Getters")
	static int32 ApplyTileOffset(int32 TileIndex, FS_ContainerSettings Container, FIntPoint Offset, FIntPoint& Remainder);


	
#pragma endregion

	
#pragma region Tags

	
	/**Check if the @Tag can be found in the @TagValues array.
	 * @ArrayIndex returns -1 if not found.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tags|Checkers")
	static bool DoesTagValuesHaveTag(TArray<FS_TagValue> TagValues, FGameplayTag Tag, FS_TagValue& FoundTagValue, int32& TagIndex);

	/**Get the value associated with a specified @Tag inside of @TagValues*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tags|Checkers")
	static FS_TagValue GetValueForTag(TArray<FS_TagValue> TagValues, FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tags|Checkers")
	static bool IsTagValueValid(FS_TagValue TagValue);

	/**Strips out the values and wraps all the tags into a tag container.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tags")
	static FGameplayTagContainer ConvertTagValuesToTagContainer(TArray<FS_TagValue> TagValues);

	/**Get the children of a specified tag.
	 * I don't know why this is not a base Unreal Engine function
	 * and why I have to expose it.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tags")
	static FGameplayTagContainer GetTagsChildren(FGameplayTag Tag);


#pragma endregion


#pragma region Other

	
	//--------------------
	// Math (Headaches guaranteed)

	/**Rotate @Shape by the @RotateAmount. If it's left to 0, this will do nothing.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Math")
	static TArray<FIntPoint> RotateShape(TArray<FIntPoint> Shape, TEnumAsByte<ERotation> RotateAmount, FIntPoint AnchorPoint);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Math")
	static TEnumAsByte<ERotation> CombineRotations(TEnumAsByte<ERotation> Rotation1, TEnumAsByte<ERotation> Rotation2);

	//--------------------
	

	//--------------------
	// UI

	/**Calculate whether or not a specific background tile should
	 * have padding or rounding applied to it. This is only meant
	 * for custom shaped items widgets.
	 * Returns false if the tile should not have any padding
	 * or rounding applied.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|UI")
	static bool GetPaddingAndRoundingForCustomShapeItemWidget(FS_InventoryItem Item, FMargin Padding, FVector4 Rounding,
		int32 TileIndex, FMargin& FinalPadding, FVector4& FinalRounding);

	UFUNCTION(BlueprintCallable, Category = "IFP|UI", meta=(DeterminesOutputType="Class", ReturnDisplayName = "Widgets"))
	static TArray<UUserWidget*> GetWidgetsOfClassInHierarchy(UUserWidget* Widget, TSubclassOf<UUserWidget> Class);

	//--------------------


	/**Find out if the @InputAction has been assigned the @Key in the @MappingContext.
	 * This does NOT check if the mapping context is currently active.*/
	UFUNCTION(Category = "IFP|Other", BlueprintCallable, BlueprintPure)
	static bool QueryKeyToMappedInputAction(FKey Key, UInputAction* InputAction, UInputMappingContext* MappingContext);

	UFUNCTION(Category = "IFP|Other", BlueprintCallable, BlueprintPure)
	static TArray<FKey> GetKeysMappedToContext(UInputAction* InputAction, UInputMappingContext* MappingContext);

	UFUNCTION(Category = "IFP|Other", BlueprintCallable, BlueprintPure)
	static bool	AreAnyKeysDown(APlayerController* Controller, TArray<FKey> Keys);

	/**Destroy and mark the object as garbage.
	 * Keep in mind, this is primarily used just for widgets, as they
	 * do not have an equivalent to an Actor's DestroyActor or a components
	 * DestroyComponent. Be careful with this function.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|Other")
	static void MarkObjectAsGarbage(UObject* Object);

#pragma endregion


#pragma region Editor

	
	//--------------------
	// Editor

	/**I'm not sure why MarkPackageDirty is not exposed to blueprint.
	 * This is only meant for editor utility widgets. As of 5.1, modifying a
	 * data asset does not mark it as dirty and if you forget to save it,
	 * you'll lose your changes. This function forcibly labels it as dirty so
	 * you'll get prompted to save it and not lose your changes.*/
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly), Category = "IFP|Editor")
	static void MarkAssetAsDirty(UObject* Object);

	/**Logs a message to IFP's custom category and optionally prints the message to screen.
	 * This is primarily for exposing blueprint to log in the custom category.*/
	UFUNCTION(Category = "IFP|Editor", BlueprintCallable, meta = (DevelopmentOnly, DefaultToSelf = "WorldContext", WorldContext = "WorldContext"), DisplayName = "Log IFP Message")
	static void LogIFPMessage(UObject* WorldContext, FString Message, bool PrintToLog = true, bool PrintToScreen = true, float Duration = 5);

#if WITH_EDITOR

	/**Helper function for setting ItemEditorName for all items inside @Containers*/
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly), Category = "IFP|Editor")
	static void ProcessContainerAndItemCustomizations(UPARAM(ref) TArray<FS_ContainerSettings>& Containers);

	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly), Category = "IFP|Editor")
	static void SetActorEditorOnly(AActor* Actor, bool NewStatus);

	/**Used by the AssetIcon widget to allow people to drag and drop item assets
	 * into the viewport to place the item actor.*/
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly), Category = "IFP|Editor")
	static FEventReply CreateContentBrowserDragDrop(UObject* Asset);

	static void UpdateLegacyContainerSettingsCode(UPARAM(ref) TArray<FS_ContainerSettings>& ContainerSettings, UObject* ContainerOwner);

	static TArray<FS_ContainerSettings>* GetContainerSettingsArray(UObject* Object);

	static bool ImplementInterface(UObject* InstigatingObject, UObject* TargetObject, UClass* Interface, TArray<FString> CustomMessages = TArray<FString>{});


#endif

	
#pragma endregion
};