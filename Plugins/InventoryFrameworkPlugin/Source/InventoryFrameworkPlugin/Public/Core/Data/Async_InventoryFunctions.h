// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IFP_CoreData.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Async/AsyncWork.h"
#include "Async_InventoryFunctions.generated.h"

class UAC_Inventory;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFail);

//Meant to be extended in the future
//once more sorting methods are added.
UENUM(BlueprintType)
enum ESortingType
{
	Name,
	Type
};

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UAsync_SortAndMoveItems : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FSuccess Success;

	UPROPERTY(BlueprintAssignable)
	FFail Fail;

	UPROPERTY()
	UAC_Inventory* TargetComponent = nullptr;

	UPROPERTY()
	TEnumAsByte<ESortingType> SortSelection;
	
	UPROPERTY()
	FS_ContainerSettings TargetContainer;

	UPROPERTY()
	float StaggerTime = 0;

	/**Sort the items and move them asynchronously. This is NOT replicated.
	 * Use SortAndMoveItems from the target component for replication.*/
	UFUNCTION(Category="IFP|Sorting Functions", BlueprintCallable, DisplayName = "Sort and Move Items (Async)", meta=(BlueprintInternalUseOnly="true", WorldContext="Context"))
	static UAsync_SortAndMoveItems* SortAndMoveItems_Async(UAC_Inventory* Component, TEnumAsByte<ESortingType> SortType, FS_ContainerSettings Container, UObject* Context, float StaggerTimer = 0);

	virtual void Activate() override;

	UFUNCTION()
	void SortFinished();
};