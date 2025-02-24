// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "IO_GameplayEffectReferences.h"

#include "AC_GASHelper.h"
#include "GameplayEffect.h"

TArray<TSoftClassPtr<UGameplayEffect>> UIO_GameplayEffectReferences::GetEffectsForEvent(FGameplayTag Event)
{
	TArray<TSoftClassPtr<UGameplayEffect>> FoundAbilities;

	if(!Event.IsValid())
	{
		return FoundAbilities;
	}

	for(auto& CurrentAttribute : Effects)
	{
		if(CurrentAttribute.Value.HasTagEvent(FGameplayTagContainer(Event)))
		{
			FoundAbilities.Add(CurrentAttribute.Key);
		}
	}

	return FoundAbilities;
}

bool UIO_GameplayEffectReferences::HasAnyTagEvents(FGameplayTagContainer Events)
{
	if(Events.IsEmpty())
	{
		return false;
	}
	
	for(auto& CurrentAbility : Effects)
	{
		if(CurrentAbility.Value.HasTagEvent(Events))
		{
			return true;
		}
	}

	return false;
}

TArray<FString> UIO_GameplayEffectReferences::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	TArray<FString> Errors;
	
	for(auto& CurrentAbility : Effects)
	{
		for(auto& CurrentGrantEvent : CurrentAbility.Value.GrantEvents)
		{
			if(CurrentAbility.Value.RemoveEvents.HasTagExact(CurrentGrantEvent))
			{
				Errors.Add(FString::Printf(TEXT("GrantEvent and RemoveEvent contain the same event %s for ability %s"), *CurrentGrantEvent.ToString(), *CurrentAbility.Key.ToString()));
			}
		}
	}
	
	return Errors;
}