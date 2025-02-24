// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "ItemAssetActorFactory.generated.h"

/**
 * Factory for handling when you drag an item data asset into the viewport.
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemAssetActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UActorFactory Interface
	virtual AActor* GetDefaultActor(const FAssetData& AssetData) override;
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual FString GetDefaultActorLabel(UObject* Asset) const override;
	//~ End UActorFactory Interface
};
