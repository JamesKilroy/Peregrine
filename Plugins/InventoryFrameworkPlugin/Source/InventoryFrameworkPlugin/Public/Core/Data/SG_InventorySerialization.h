// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "SG_InventorySerialization.generated.h"

struct FS_InventoryItem;
class UItemInstance;

class INVENTORYFRAMEWORKPLUGIN_API FSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
public:
	FSaveGameArchive(FArchive& InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

USTRUCT(BlueprintType)
struct FItemInstanceRecord
{
	GENERATED_BODY()

public:

	//The container this object was in
	UPROPERTY()
	int32 ContainerIndex = -1;

	//The item this object belonged to
	UPROPERTY()
	int32 ItemIndex = -1;

	UPROPERTY()
	UClass* ObjectClass = nullptr;

	//byte data of all the SaveGame properties
	UPROPERTY()
	TArray<uint8> PropertyData;
};

/**Save game class that helps with serializing the ContainerSettings in an
 * inventory component.
 * This is only needed for serializing item instances.
 * This class needs to be somewhere in your hierarchy for your save game class.*/
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API USG_InventorySerialization : public USaveGame
{
	GENERATED_BODY()

	//The records of all the item instances
	UPROPERTY()
	TArray<FItemInstanceRecord> ItemInstanceRecords;

	UFUNCTION(Category = "IFP Serialization", BlueprintCallable)
	void SaveItemInstance(UItemInstance* ItemInstance);

	UFUNCTION(Category = "IFP Serialization", BlueprintCallable)
	void LoadItemInstance(FS_InventoryItem Item);

	UFUNCTION(Category = "IFP Serialization")
	void SaveContainersForActor(AActor* Actor);

	UFUNCTION(Category = "IFP Serialization")
	void LoadContainersForActor(AActor* Actor);
};
