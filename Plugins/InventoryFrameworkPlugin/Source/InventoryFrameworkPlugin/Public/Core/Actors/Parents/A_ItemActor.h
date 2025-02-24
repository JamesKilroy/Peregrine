// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/a_itemphysicalrepresentation/


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Interfaces/I_Inventory.h"
#include "A_ItemActor.generated.h"

/**Base class for all items that want to have an actor in the world
 * to represent it.
 *
 * Docs:
 * https://inventoryframework.github.io/classes-and-settings/a_itemphysicalrepresentation/ */
UCLASS(Abstract, Blueprintable, BlueprintType)
class INVENTORYFRAMEWORKPLUGIN_API AA_ItemActor : public AActor, public II_Inventory
{
	GENERATED_BODY()
	
public:	

	//Constructors
	AA_ItemActor();
	AA_ItemActor(const FObjectInitializer& ObjectInitializer);
	void Construct();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ExposeOnSpawn = true))
	bool AllowBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ExposeOnSpawn = true))
	bool StartInventoryOnBeginPlay = true;

	//If filled, when the item is spawned we will overwrite the current inventory components ContainerSettings.
	UPROPERTY(BlueprintReadOnly, Category = "Settings", meta = (ExposeOnSpawn = true))
	TArray<FS_ContainerSettings> ContainerSettingsOverwrite;

	/**If this actor is spawned through an equipment system, this UniqueID
	 * will lead you back to the original item struct that lives on the
	 * owning actor's Inventory Component.*/
	UPROPERTY(BlueprintReadWrite, Category = "Inventory", Replicated)
	FS_UniqueID RootItemID;
	
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAC_Inventory> Inventory;

	/**In case this actor is a preview actor, this can be used to get the original
	 * actor this actor was made from.
	 * V: This is the hardest to name variable I have ever faced.*/
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> OriginalActorIfPreview = nullptr;

	/**In some cases, we need to know if the character is a preview actor on BeginPlay.
	 * But because BeginPlay happens instantly when spawning an actor, we can't wait
	 * to execute IsPreviewActor*/
	UPROPERTY(BlueprintReadOnly, Category = "Settings", meta = (ExposeOnSpawn = true))
	bool bIsPreviewActor = false;

	/**When this actor is spawned, what object should it use as a template when being created?
	 * For example, if you are creating a preview actor of a specific item, you could get that
	 * items ItemInstance and pass that in here, and the system will duplicate that ItemInstance.*/
	UPROPERTY(BlueprintReadOnly, Category = "Settings", meta = (ExposeOnSpawn = true))
	UItemInstance* ItemInstanceTemplate = nullptr;

	virtual void PassUniqueID_Implementation(FS_UniqueID UniqueID) override;

	/**In the case of @RootItemID being valid, then the @ParentComponent
	 * from that UniqueID is the one you should be using to retrieve
	 * this items containers or item data.*/
	UFUNCTION(Category = "Inventory", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Inventory"))
	UAC_Inventory* GetInventory();

	/**In the case of @RootItemID being valid, this will retrieve this
	 * item actor's true ItemData straight from the component holding
	 * that data.
	 *
	 * If @RootItemID is invalid, this will return
	 * ContainerSettings[0].Items[0], which should be the ThisActor container.*/
	UFUNCTION(Category = "Inventory", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Get Item Data"))
	FS_InventoryItem GetRootItemData();

	/**Find out if this specific actor instance is an equipment actor or not.*/
	UFUNCTION(Category = "Inventory", BlueprintCallable, BlueprintPure)
	bool IsProxyActor();

	virtual void GetInventoryComponent_Implementation(UAC_Inventory*& Component) override;

	virtual void BeginPlay() override;

	virtual void SetIsPreviewActor_Implementation(bool NewStatus, AActor* OriginalActor) override;

	virtual bool IsPreviewActor_Implementation() override;
};
