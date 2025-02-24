// Copyright (C) Varian Daemon 2023. All Rights Reserved.

//Docs: https://inventoryframework.github.io/classes-and-settings/da_coreitem/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Objects/Parents/O_ItemAssetValidation.h"
#include "Engine/DataAsset.h"
#include "DA_CoreItem.generated.h"

class UAC_LootTable;
class UO_LootPool;
class UItemTrait;
class UW_InventoryItem;
class UW_Container;
class AA_ItemActor;
class UItemInstance;

USTRUCT(Blueprintable)
struct FShapeRotation
{
	GENERATED_BODY()

	UPROPERTY(Category = "Shape", BlueprintReadWrite)
	TEnumAsByte<ERotation> Rotation = Zero;

	UPROPERTY(Category = "Shape", BlueprintReadWrite)
	TArray<FIntPoint> Shape;

	FShapeRotation(){}

	FShapeRotation(TEnumAsByte<ERotation> InRotation, TArray<FIntPoint> InShape)
	{
		Rotation = InRotation;
		Shape = InShape;
	}

	
};

#if WITH_EDITORONLY_DATA
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTraitAdded, UItemTrait*, Trait, UDA_CoreItem*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTraitRemoved, UItemTrait*, Trait, UDA_CoreItem*, Asset);
#endif

/**The core asset for all items. Most items should have a specialized
 * data asset child derived from this class, but this class can still be
 * used for basic items that do not need their own specialized child.
 *
 * Docs: https://inventoryframework.github.io/classes-and-settings/da_coreitem/ */

UCLASS(Blueprintable, BlueprintType)
class INVENTORYFRAMEWORKPLUGIN_API UDA_CoreItem : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/**Name of the item displayed to the player. This can be overwritten inside
	 * an items Override settings.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings", AssetRegistrySearchable)
	FText ItemName;

	/**Item description displayed to the player when they hover over this item.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings", Meta = (MultiLine = "True"), AssetRegistrySearchable)
	FText Description;

	/**The item type that is presented to the player. This is different from the
	 * Type variable (only visible in C++) as that is used to cast to the different
	 * children to keep most functions inside of this class.
	 * For example; You might have a sword item and a spear item. They are both
	 * children of the Weapon data asset, which makes the Type variable be "Weapon"
	 * so it's easy to cast to the weapon data asset. But the "presented" item
	 * type to the player is a sword and a spear.
	 *
	 * Tip: You can add ", meta = (Categories = "IFP.Items.Types")" to the end of this
	 * property to filter the options to your types tags.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings", AssetRegistrySearchable)
	FGameplayTag ItemType;
	
	/*Items with no valid physical actor can not be dropped or inspected and icon generation
	 * will not work.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings", meta = (AssetBundles = "ItemActor"))
	TSoftClassPtr<AA_ItemActor> ItemActor = nullptr;

	/**The object to create when a struct for this item is created.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings")
	TSoftClassPtr<UItemInstance> ItemInstance = nullptr;

	/**Gameplay logic tied to this item.
	 * Traits are where you store data, and components are simple actor components that use the
	 * data from the trait to drive its logic.
	 * Docs: inventoryframework.github.io/classes-and-settings/traitsandcomponents/ */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite, Category = "Core Settings")
	TArray<UItemTrait*> TraitsAndComponents;

	/**Fragments to add to the item struct on @StartComponent,
	 * creating a unique instance. This allows it to be modified
	 * during runtime, but is worse for memory.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Core Settings", NoClear,
	meta = (PinHiddenByDefault, ShowTreeView, DisallowedClasses = "/Script/InventoryFrameworkPlugin.ContainerFragment"))
	TArray<TInstancedStruct<FCoreFragment>> ItemStructFragments;

	/**Fragments that are bound to the asset.
	 * These are NOT modifiable during runtime, but since it
	 * is bound to just this asset, only one instance is created,
	 * meaning it is far better for memory.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Core Settings", NoClear,
	meta = (PinHiddenByDefault, ShowTreeView, DisallowedClasses = "/Script/InventoryFrameworkPlugin.ContainerFragment"))
	TArray<TInstancedStruct<FCoreFragment>> ItemAssetFragments;

	/**The dimensions of the item inside of grid containers.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Core Settings")
	FIntPoint ItemDimensions = FIntPoint(1, 1);

	/**Simple tags to associated with this item.
	 * When the component is started, these tags are automatically applied
	 * if the item does not have any of these tags.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	FGameplayTagContainer DefaultTags;

	/**Tags that can have values associated with them.
	 * For example, you might have a durability tag and want to give it a value.
	 * When the component is started, these tags are automatically applied
	 * if the item does not have any of these tags and set the correct value.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	TArray<FS_TagValue> DefaultTagValues;

	/**Tags that are NOT added to the created items struct.
	 * This can NOT be overriden or modified during runtime,
	 * this is meant for scenarios where you want to have
	 * a tag associated with an item, but never want it removed
	 * or changed.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	FGameplayTagContainer AssetTags;

	/**Tag values that are NOT added to the created items struct.
	 * This can NOT be overriden or modified during runtime,
	 * this is meant for scenarios where you want to have
  	 * a tag value associated with an item, but never want it removed
	 * or changed.*/
	UPROPERTY(meta = (DeprecatedProperty = "Deprecated, replaced with the Tag Fragment"))
	TArray<FS_TagValue> AssetTagValues;

	/**Containers can have specified widgets to create the item widgets.
	 * This allows you to override the widget used for this specific item
	 * when it's time to create the widget representation of this item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TMap<TSubclassOf<UW_Container>, TSubclassOf<UW_InventoryItem>> ItemWidgetOverrides;
	
	/**It is recommended to try and use normal images for as many items as possible, as this could get heavy with hundreds
	 * of items in a players inventory. What this does is it gets the PhysicalActor and creates a copy, then snaps a photo
	 * with the SceneCaptureComponent and uses that for the icon of this item.
	 * Set to true by default so designers can prototype faster.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	bool UseGeneratedItemIcon = true;

	/**Soft object reference is important as texture files can easily be the largest file for an item.
	 * This also allows you to unload this image if you end up using the item overwrite image.
	 * If UseGeneratedItemIcon is true, this texture is not loaded.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI", meta = (AssetBundles = "InventoryImage", EditCondition = "!UseGeneratedItemIcon", EditConditionHides, ThumbnailOverride))
	TSoftObjectPtr<UTexture2D> InventoryImage = nullptr;
	//ThumbnailOverride is for a separate plugin called EditorAssistant, which can be found on my Github:
	//https://github.com/Variann/ModularityPractice/wiki/Editor-Assistant

	/**If false, we essentially do not destroy the preview actor after snapping the texture.
	 * This allows you to have animated icons, but this can get expensive really quickly.
	 * If true, we will destroy the preview actor, but will snap 1 frame before doing so.
	 * That texture will then be released when the widget is destroyed.
	 * It is HIGHLY recommended to keep this enabled for 99% of items.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	bool UseStaticCapture = true;

	/**If true, whenever an item is created, we store the item data asset name in a map with a generated icon, then when another
	 * item is added to the inventory that shares the same data asset, we just use that icon instead of generating an exact duplicate.
	 * Only disable this if the item can have visible attachments or can be customized in any way.
	 * For example, if you have two guns that are the same data asset but you've applied a skin to one, if this is set to true,
	 * either they will share that item icon or it will show the one without the skin, depending which was made first.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	bool OptimizeIconCreation = true;

	/**This, CameraDistance and ActorLocation are only for prototyping.
	 * Ideal scenario is that your mesh has a PreviewCameraSocket shown in the documentation
	 * and you fine tune the location and rotation that way.
	 * Transform we give the icon actor before we snap the texture.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	FRotator IconActorRotation = FRotator(-90, -90, 0);

	/**This, CameraDistance and ActorRotation are only for prototyping.
	 * Ideal scenario is that your mesh has a PreviewCameraSocket shown in the documentation
	 * and you fine tune the location and rotation that way.
	 * Location adjustment for the camera before  we snap the texture.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	FVector IconActorLocation = FVector(0, 0, 0);

	/**This, ActorLocation and ActorRotation are only for prototyping.
	 * Ideal scenario is that your mesh has a PreviewCameraSocket shown in the documentation
	 * and you fine tune the location and rotation that way.
	 * The distance between the camera and the icon actor.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	float GeneratedIconCameraDistance = 0;

	/**How far away is this item from the camera when inspected*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "UseGeneratedItemIcon", EditConditionHides))
	float InspectDistance = 30;

	/**Remember, 1 is a 100%, 0.5 is 50% and so on.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core Settings", meta = (UIMin = 0, UIMax = 1, ClampMin = 0, ClampMax = 1))
	float DefaultSpawnChance = 1.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Settings", meta=(EditCondition="CanItemTypeStack()", EditConditionHides))
	int32 MaxStack = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Settings", meta=(EditCondition="CanItemTypeStack()", EditConditionHides))
	int32 DefaultStack = 1;

	/**Dirty hack to trick the asset manager to associate this asset with a specific type.
	 * For example, you can make a "Equippables" type in your asset manager, then set the
	 * default value of this to "Equippables" and all items will now go in that category.
	 * You do not need to restart the engine for the asset manager to get this.
	 * Docs: inventoryframework.github.io/introduction/firststeps/assetmanager/#adding-custom-asset-types*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Developer Settings", AssetRegistrySearchable)
	FName AssetRegistryCategory = FName("Items");

#if WITH_EDITORONLY_DATA

	/**InventoryImage should be empty if you are using a generated item icon. This is used
	 * for development tools that don't use the generated item icon.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Developer Settings", meta = (AssetBundles = "DeveloperImage"))
	TSoftObjectPtr<UTexture2D> DeveloperImage = nullptr;
	
	/**Arbitrary keywords which you can use for development tools. The inventory helper utility
	 * is a good example of how this is used. This is useful for items that implies some sort
	 * of keyword, but the data in this asset is too difficult to query.
	 * For example, this could be a quest item and you might not have integrated the quest
	 * system into the inventory system. This means it is difficult to resolve if this item
	 * belongs to a quest. While this is not the perfect solution, it is a good solution
	 * for general arbitrary search queries.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Developer Settings")
	TArray<FName> ArbitrarySearchKeywords;

	/**Arbitrary tags to use with development tools.
	 * This is NOT included in the packaged product.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Developer Settings")
	FGameplayTagContainer ArbitrarySearchTags;

	/**Arbitrary tag values to use with development tools.
	 * This is NOT included in the packaged product.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "Tag"), Category = "Developer Settings")
	TArray<FS_TagValue> ArbitrarySearchTagValues;

	/**The class we use to validate this items data when pressing the "Validate Data" button.*/
	UPROPERTY(EditInstanceOnly, Category = "Developer Settings", BlueprintReadWrite)
	TSubclassOf<UO_ItemAssetValidation> ValidationClass = StaticLoadClass(UObject::StaticClass(), nullptr,
		TEXT("/InventoryFrameworkPlugin/Core/Editor/ItemAssetValidation/O_BasicItemValidation.O_BasicItemValidation_C"));

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Developer Settings")
	FTraitAdded TraitAdded;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Developer Settings")
	FTraitRemoved TraitRemoved;

#endif

	UPROPERTY()
	TArray<FShapeRotation> Shapes;
	

	//--------------------
	// Functions
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|ItemAsset|Checkers")
	virtual bool CanItemStack();

	UFUNCTION()
	virtual bool CanItemTypeStack();

	//--------------------
	//Getters
	
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Widget"))
	virtual TSubclassOf<UW_AttachmentParent> GetAttachmentWidgetClass();

	/**If the item has the ability to hold items, the data  asset will contain the default containers.
	 * This retrieves that array and gives you a copy.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Containers"))
	virtual TArray<FS_ContainerSettings> GetDefaultContainers();

	/**Get the items shape with the top left starting at 0,0. This will not take
	 * the items container into consideration at all.
	 * This assumes that you want the shape as if the item was in a grid container.
	 * This is only meant for items that aren't initialized yet,*/
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Shape"))
	TArray<FIntPoint> GetItemsPureShape(TEnumAsByte<ERotation> Rotation);

	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Disabled Tiles"))
	TArray<FIntPoint> GetDisabledTiles();

	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Anchor Point"))
	FIntPoint GetAnchorPoint();
	
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", BlueprintPure)
	virtual FText GetAssetTypeName();

	/**Retrieve any loot tables this item wants to include when it is created.
	 * These loot tables should only focus the containers that this item owns.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Pools"))
	virtual TArray<UAC_LootTable*> GetLootTables();
	
	int32 FindObjectIndex(UItemTrait* Object);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**Get all the soft class item component references from this items traits.
	 * This can be used to async load all item components at some convenient
	 * time before these item components get constructed.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|ItemAsset|Getters", meta = (ReturnDisplayName = "Item Components"))
	TArray<TSoftClassPtr<UItemComponent>> GetItemComponentsFromTraits();


#if WITH_EDITOR

	UFUNCTION(CallInEditor, Category = "Core Settings", DisplayName = "Open Item Instance Class")
	void OpenItemInstanceClass();

	UFUNCTION(CallInEditor, Category = "Core Settings", DisplayName = "Open Item Actor Class")
	void OpenItemActorClass();

	/**Use the CDO of @ValidationClass, which is set in the Developer Settings to validate
	 * the data inside this data asset.
	 * The system comes with a basic validation class as an example, in general this just checks
	 * if data meets your projects requirement for quality control. This can also ask Objects
 	 * or other CDO's through the I_Inventory->VerifyData interface function to find out if the
	 * data is set up correctly.
	 * This is being done through the @ValidationClass, so you can do your validation at a
	 * blueprint level, or swap out validation classes for different type of validation.*/
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UTexture2D* GetThumbnailTexture();

	virtual void PostLoad() override;

	/**Calculate all rotations and store them in the @Shapes
	 * array, so we only ever perform these calculations once
	 * during development time and never during runtime.
	 * This can be called in editor in case you are using a
	 * custom editor tool, such as the custom shape toolbox.*/
	UFUNCTION(BlueprintCallable, Category = "Developer", meta = (DevelopmentOnly))
	void BakeShapes();

#endif
};


