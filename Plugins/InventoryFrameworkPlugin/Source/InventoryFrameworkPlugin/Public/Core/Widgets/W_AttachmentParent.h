// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "W_Container.h"
#include "Blueprint/UserWidget.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Interfaces/I_InventoryWidgets.h"
#include "W_AttachmentParent.generated.h"

/**Parent widget meant to host a set of container widgets.
 * This is used by items that can be opened, such as a backpack
 * to display its containers.*/
UCLASS(Abstract)
class INVENTORYFRAMEWORKPLUGIN_API UW_AttachmentParent : public UUserWidget, public II_InventoryWidgets
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Containers")
	TArray<UW_Container*> WidgetContainers;

	UPROPERTY(BlueprintReadWrite, Category = "Item")
	FS_UniqueID ParentItemID;
	
	virtual void GetContainers_Implementation(TArray<UW_Container*>& Containers) override;

	//Get the data of the item this widget is owned by.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "ItemData"))
	void GetOwningItemData(FS_InventoryItem& ItemData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Inventory"))
	void GetInventory(UAC_Inventory*& Component);
};
