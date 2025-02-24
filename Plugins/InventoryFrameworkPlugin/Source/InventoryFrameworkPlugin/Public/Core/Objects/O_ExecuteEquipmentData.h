// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "Core/Objects/Parents/O_NetworkedObject.h"
#include "O_ExecuteEquipmentData.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExecutionFinished, UO_ExecuteEquipmentData*, Object);

UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin))
class INVENTORYFRAMEWORKPLUGIN_API UO_ExecuteEquipmentData : public UO_NetworkedObject
{
	GENERATED_BODY()

public:

	//--------------------
	// Variables

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FExecutionFinished ExecutionFinished;
	
	UPROPERTY(BlueprintReadWrite, Category = "Data", Meta = (ExposeOnSpawn = true))
	FS_EquipInformation EquipmentInformation;
	
	UPROPERTY(BlueprintReadWrite, Category = "Data", Meta = (ExposeOnSpawn = true))
	FS_UnequipInformation UnequipInformation;

	UPROPERTY(BlueprintReadWrite, Category = "Data", Meta = (ExposeOnSpawn = true))
	TEnumAsByte<EEquipmentMesh> EquipmentMesh;

	/**List of all the other objects that are executing equipment data
	 * in sync with this one. This way we can communicate with objects
	 * that are waiting for a anim notify.*/
	UPROPERTY(BlueprintReadWrite, Category = "Data")
	TArray<UO_ExecuteEquipmentData*> OtherObjects;

	UPROPERTY(BlueprintReadWrite, Category = "Data", Meta = (ExposeOnSpawn = true))
	UDA_CoreItem* ParentItem;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	void ExecuteEquipmentInformation(bool OverrideWaitForNotify, FName TagToAssign);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	void ExecuteUnequipmentInformation(bool OverrideWaitForNotify, FName TagToFind);
};
