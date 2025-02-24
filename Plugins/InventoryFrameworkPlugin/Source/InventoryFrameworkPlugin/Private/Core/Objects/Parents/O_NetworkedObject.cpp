// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Objects/Parents/O_NetworkedObject.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "Engine/NetDriver.h"


AActor* UO_NetworkedObject::GetOwningActor() const
{
	if(Owner)
	{
		return Owner;
	}
	return GetTypedOuter<AActor>();
}

void UO_NetworkedObject::SetOwner(AActor* NewOwner)
{
	if(NewOwner)
	{
		Owner = NewOwner;
		Rename(nullptr, NewOwner);
	}
}

UWorld* UO_NetworkedObject::GetWorld() const
{
	//Try to get the world from the owning actor if we have one
	AActor* Outer = GetOwningActor();
	if(Outer)
	{
		return Outer->GetWorld();
	}
	//Else return null - the latent action will fail to initialize
	return nullptr;
}

void UO_NetworkedObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate any blueprint variables labeled for replication.
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

int32 UO_NetworkedObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	// return UObject::GetFunctionCallspace(Function, Stack);
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);

}

bool UO_NetworkedObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	AActor* TempOwner = GetOwningActor();
	UNetDriver* NetDriver = TempOwner->GetNetDriver();
	if (NetDriver)
	{
		NetDriver->ProcessRemoteFunction(TempOwner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;

}