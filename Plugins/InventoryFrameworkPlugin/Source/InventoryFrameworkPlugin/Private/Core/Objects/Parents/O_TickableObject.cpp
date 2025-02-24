// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Objects/Parents/O_TickableObject.h"


UWorld* UO_TickableObject::GetWorld() const
{
	if (IsTemplate() || !GetOuter()) // We're the CDO or have no outer (?!).
	{
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

void UO_TickableObject::Destroyed_Implementation()
{
}

void UO_TickableObject::StartPlay_Implementation()
{
}

void UO_TickableObject::RemoveObject()
{
	if(IsDestroyed == false)
	{
		Destroyed();
		MarkAsGarbage();
		IsDestroyed = true;
	}
}

void UO_TickableObject::Tick(float DeltaTime)
{
	if(TickEnabled == true)
	{
		EventTick(DeltaTime);
	}
}

bool  UO_TickableObject::IsTickable() const
{
	// return FTickableGameObject::IsTickable();

	//Enable tick.
	return TickEnabled;
}

TStatId  UO_TickableObject::GetStatId() const
{
	return TStatId();
}

