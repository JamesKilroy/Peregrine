// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Other/ItemAssetActorFactory.h"

#include "Core/Actors/Parents/A_ItemActor.h"
#include "Core/Items/DA_CoreItem.h"

UItemAssetActorFactory::UItemAssetActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NewActorClass = AA_ItemActor::StaticClass();
}

AActor* UItemAssetActorFactory::GetDefaultActor(const FAssetData& AssetData)
{
	if(UDA_CoreItem* ItemAsset = Cast<UDA_CoreItem>(AssetData.GetAsset()))
	{
		if(!ItemAsset->ItemActor.IsNull())
		{
			NewActorClass = ItemAsset->ItemActor.LoadSynchronous();
			UClass* ActorClass = ItemAsset->ItemActor.Get();
			return ActorClass->GetDefaultObject<AActor>();
		}
	}
	return Super::GetDefaultActor(AssetData);
}

bool UItemAssetActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && AssetData.IsInstanceOf(UDA_CoreItem::StaticClass()))
	{
		if(UDA_CoreItem* ItemAsset = Cast<UDA_CoreItem>(AssetData.GetAsset()))
		{
			//Some items don't have an item actor, return false if so
			if(ItemAsset->ItemActor.IsNull())
			{
				return false;
			}
		}
		
		return true;
	}

	return false;
}

FString UItemAssetActorFactory::GetDefaultActorLabel(UObject* Asset) const
{
	if(UDA_CoreItem* ItemAsset = Cast<UDA_CoreItem>(Asset))
	{
		if(!ItemAsset->ItemActor.IsNull())
		{
			//By default, the item actor will be the name of the asset that
			//is dragged into the viewport. In this case, the data asset.
			//We don't want that, use the name of the actor class.
			UClass* ActorClass = ItemAsset->ItemActor.LoadSynchronous();
			return ActorClass->GetName();
		}
	}
	return Super::GetDefaultActorLabel(Asset);
}
