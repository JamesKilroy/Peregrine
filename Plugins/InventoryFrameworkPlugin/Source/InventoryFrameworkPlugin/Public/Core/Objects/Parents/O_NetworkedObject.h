// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "O_TickableObject.h"
#include "UObject/Object.h"
#include "O_NetworkedObject.generated.h"

/**
 * Extension of O_TickableObject. This does not just have support for async tasks, but also networking
 * and ownership.
 * You must handle the ownership of this object for networking to work properly.
 */

UCLASS(Abstract)
class INVENTORYFRAMEWORKPLUGIN_API UO_NetworkedObject : public UO_TickableObject
{

	
	GENERATED_BODY()

public:
	
	//--------------------
	// Networking
	
	//Label for network support.
	virtual bool IsSupportedForNetworking () const override { return true; }

	//Allows this object to communicate with the world, important for replication.
	virtual UWorld* GetWorld() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	
	// Call "Remote" (aka, RPC) functions through the owning actors NetDriver
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	//--------------------
	// Functions

	//Get the owner of this object.
	UFUNCTION(BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Owner")) //I can not find a way to get the "Self" pin to be hidden or removed. Someone send help.
	virtual AActor* GetOwningActor() const;
	
	/**Set the owner of this object.*/
	UFUNCTION(BlueprintCallable, Category = "Setters")
	virtual void SetOwner(AActor* NewOwner);

	UPROPERTY()
	AActor* Owner = nullptr;
};
