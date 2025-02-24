// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Other/ItemAssetThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Core/Items/DA_CoreItem.h"

void UItemAssetThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height,
                                       FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UDA_CoreItem* DataAsset = Cast<UDA_CoreItem>(Object);
	if(!DataAsset)
	{
		Super::Draw(Object, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
	}
	
	UTexture2D* ThumbnailToUse = DataAsset->GetThumbnailTexture();
	
	if(ThumbnailToUse)
	{
		//Background by default is pitch black. Set it to the normal gray background.
		//V: I don't know why, but these values are supposed to be 26, but I need to
		//use 90 to get RGB values of 26
		Canvas->Clear(FLinearColor(FColor(90, 90, 90)));
		
		/**Not all textures are a 1:1 aspect ratio. If we just used Width|Height to render the final canvas,
		 * it would stretch. This ensures the aspect ratio is correct.*/
		float MaxRange = Width > Height ? Width : Height;
		float MaxImageRange = ThumbnailToUse->GetResource()->GetSizeX() > ThumbnailToUse->GetResource()->GetSizeY() ?
			ThumbnailToUse->GetResource()->GetSizeX() : ThumbnailToUse->GetResource()->GetSizeY();
		float NormalizeWidth = FMath::GetMappedRangeValueClamped(FVector2D(0, MaxImageRange),
			FVector2D(0, MaxRange), ThumbnailToUse->GetResource()->GetSizeX());
		float NormalizedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0, MaxImageRange),
	FVector2D(0, MaxRange), ThumbnailToUse->GetResource()->GetSizeY());
		//Remember to offset the Y position to make sure textures that aren't 1:1 get centered
		FCanvasTileItem CanvasTile(FVector2D((Width - NormalizeWidth) / 2, (Height - NormalizedHeight) / 2),
			ThumbnailToUse->GetResource(), FVector2D(NormalizeWidth, NormalizedHeight), FLinearColor::White);
		CanvasTile.BlendMode = SE_BLEND_Translucent;
		CanvasTile.Draw(Canvas);
		return;
	}

	Super::Draw(Object, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
}

bool UItemAssetThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	if(Super::CanVisualizeAsset(Object))
	{
		return true;
	}
	
	UDA_CoreItem* DataAsset = Cast<UDA_CoreItem>(Object);
	if(DataAsset)
	{
		return true;
	}

	return false;
}
