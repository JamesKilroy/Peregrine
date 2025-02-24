// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_InventoryWidgets.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UI_InventoryWidgets : public UInterface
{
	GENERATED_BODY()
};


class INVENTORYFRAMEWORKPLUGIN_API II_InventoryWidgets
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void GetContainers(TArray<UW_Container*>& Containers);
	
	/**Used by widgets, such as vendors, that need information about
	 * another component.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void ReceiveExternalComponent(UAC_Inventory* ExternalComponent);
};
