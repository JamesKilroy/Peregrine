// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "IO_AttributeSetReferences.h"

#include "AC_GASHelper.h"

TArray<TSoftClassPtr<UAttributeSet>> UIO_AttributeSetReferences::GetAttributeSetsForEvent(FGameplayTag Event)
{
	TArray<TSoftClassPtr<UAttributeSet>> Attributes;

	for(auto& CurrentAttribute : AttributeSets)
	{
		if(CurrentAttribute.Value.GrantEvents.HasTagExact(Event) || CurrentAttribute.Value.RemoveEvents.HasTagExact(Event))
		{
			Attributes.Add(CurrentAttribute.Key);
		}
	}

	return Attributes;
}

bool UIO_AttributeSetReferences::HasAnyTagEvents(FGameplayTagContainer Events)
{
	if(Events.IsEmpty())
	{
		return false;
	}
	
	for(auto& CurrentAbility : AttributeSets)
	{
		if(CurrentAbility.Value.HasTagEvent(Events))
		{
			return true;
		}
	}

	return false;
}

TArray<FString> UIO_AttributeSetReferences::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	TArray<FString> Errors;

	for(auto& CurrentAttribute : AttributeSets)
	{
		for(auto& CurrentGrantEvent : CurrentAttribute.Value.GrantEvents)
		{
			if(CurrentAttribute.Value.RemoveEvents.HasTagExact(CurrentGrantEvent))
			{
				Errors.Add(FString::Printf(TEXT("GrantEvent and RemoveEvent contain the same event %s for attribute %s"), *CurrentGrantEvent.ToString(), *CurrentAttribute.Key.ToString()));
			}
		}
	}

	return Errors;
}
