// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ItemAssetThumbnailRenderer.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemAssetThumbnailRenderer : public UBlueprintThumbnailRenderer
{
	GENERATED_BODY()

	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily) override;

	virtual bool CanVisualizeAsset(UObject* Object) override;
};
