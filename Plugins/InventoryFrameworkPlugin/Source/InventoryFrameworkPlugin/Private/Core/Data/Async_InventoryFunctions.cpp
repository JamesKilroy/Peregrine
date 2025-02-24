// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Data/Async_InventoryFunctions.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

UAsync_SortAndMoveItems* UAsync_SortAndMoveItems::SortAndMoveItems_Async(UAC_Inventory* Component,
                                                                         TEnumAsByte<ESortingType> SortType, FS_ContainerSettings Container, UObject* Context,
                                                                         float StaggerTimer)
{
	UAsync_SortAndMoveItems* NewAsyncObject = NewObject<UAsync_SortAndMoveItems>(Context);
	NewAsyncObject->TargetComponent = Component;
	NewAsyncObject->SortSelection = SortType;
	NewAsyncObject->TargetContainer = Container;
	NewAsyncObject->StaggerTime = StaggerTimer;
	return NewAsyncObject;
}

void UAsync_SortAndMoveItems::Activate()
{
	Super::Activate();

	if(!IsValid(TargetComponent))
	{
		Fail.Broadcast();
		RemoveFromRoot();
		return;
	}

	if(!UFL_InventoryFramework::IsContainerValid(TargetContainer))
	{
		Fail.Broadcast();
		RemoveFromRoot();
		return;
	}
	
	FRandomStream Seed;
	Seed.Initialize(UKismetMathLibrary::RandomIntegerInRange(1, 214748364));
	TargetComponent->SortingFinished.AddDynamic(this, &UAsync_SortAndMoveItems::SortFinished);
	TargetComponent->SortAndMoveItems(SortSelection, TargetContainer, StaggerTime);
}

void UAsync_SortAndMoveItems::SortFinished()
{
	Success.Broadcast();
	TargetComponent->SortingFinished.RemoveDynamic(this, &UAsync_SortAndMoveItems::SortFinished);
	RemoveFromRoot();
}
