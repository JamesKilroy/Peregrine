// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/IT_WidgetPaddingAndRadius.h"

#include "Core/Items/DA_CoreItem.h"

bool UIT_WidgetPaddingAndRadius::GetSettingsForTile(TEnumAsByte<ERotation> Rotation, FIntPoint Tile, FS_WidgetPaddingAndRadius& Settings)
{
	for(auto& CurrentSettings : PaddingAndRadius)
	{
		if(CurrentSettings.Rotation == Rotation)
		{
			for(auto& CurrentTile : CurrentSettings.TileSettings)
			{
				if(CurrentTile.Tile == Tile)
				{
					Settings = CurrentTile;
					return true;
				}
			}
		}
	}

	return false;
}

void UIT_WidgetPaddingAndRadius::BakeSettings(TEnumAsByte<ERotation> Rotation, UDA_CoreItem* ItemAsset)
{
	TArray<FIntPoint> Shape = ItemAsset->GetItemsPureShape(Rotation);

	//Find the settings we are working with
	int32 SettingsIndex = -1;
	for(int32 CurrentIndex = 0; CurrentIndex < 4; CurrentIndex++)
	{
		if(!PaddingAndRadius.IsValidIndex(CurrentIndex))
		{
			break;
		}
		
		if(PaddingAndRadius[CurrentIndex].Rotation == Rotation)
		{
			SettingsIndex = CurrentIndex;
			PaddingAndRadius[CurrentIndex].TileSettings.Empty();
			break;
		}
	}

	//Settings weren't found, add them
	if(SettingsIndex == -1)
	{
		FS_WidgetPaddingAndRadiusWrapper NewSettings;
		NewSettings.Rotation = Rotation;
		PaddingAndRadius.Add(NewSettings);
		SettingsIndex = PaddingAndRadius.Num() - 1;
	}
	
	for(auto& CurrentIndex : Shape)
	{
		//This is used in case there's no padding,
		//but we still want to apply rounding.
		bool TopModified = false;
		bool RightModified = false;
		bool BottomModified = false;
		bool LeftModified = false;

		FS_WidgetPaddingAndRadius TileSettings;
		FMargin FinalPadding;
		FVector4 FinalRounding;
		FinalRounding.W = 0; //For some reason, every other side is 0 except W
		TileSettings.Tile = CurrentIndex;
		FIntPoint CurrentScanningTile = CurrentIndex;

		//Top side
		CurrentScanningTile.Y = CurrentIndex.Y - 1;
		int32 FoundTile = Shape.Find(FIntPoint(CurrentScanningTile));
		if(FoundTile == -1)
		{
			FinalPadding.Top = Padding.Top;
			TopModified = true;
		}

		//Right side
		CurrentScanningTile.X = CurrentIndex.X + 1;
		CurrentScanningTile.Y = CurrentIndex.Y;
		FoundTile = Shape.Find(FIntPoint(CurrentScanningTile));
		if(FoundTile == -1)
		{
			FinalPadding.Right = Padding.Right;
			RightModified = true;
		}

		//Bottom side
		CurrentScanningTile.X = CurrentIndex.X;
		CurrentScanningTile.Y = CurrentIndex.Y + 1;
		FoundTile = Shape.Find(FIntPoint(CurrentScanningTile));
		if(FoundTile == -1)
		{
			FinalPadding.Bottom = Padding.Bottom;
			BottomModified = true;
		}

		//Left side
		CurrentScanningTile.X = CurrentIndex.X - 1;
		CurrentScanningTile.Y = CurrentIndex.Y;
		FoundTile = Shape.Find(FIntPoint(CurrentScanningTile));
		if(FoundTile == -1)
		{
			FinalPadding.Left = Padding.Left;
			LeftModified = true;
		}

		//Start evaluating the rounding
		if(Rounding.X > 0 || Rounding.Y > 0 || Rounding.Z > 0 || Rounding.W > 0)
		{
			//Top right corner
			if(TopModified && RightModified)
			{
				FinalRounding.Y = Rounding.Y;
			}

			//Bottom right corner
			if(RightModified && BottomModified)
			{
				FinalRounding.Z = Rounding.Z;
			}

			//Bottom left corner
			if(BottomModified && LeftModified)
			{
				FinalRounding.W = Rounding.W;
			}

			//Top left corner
			if(LeftModified && TopModified)
			{
				FinalRounding.X = Rounding.X;
			}
		}

		TileSettings.Padding = FinalPadding;
		TileSettings.Rounding = FinalRounding;

		if(TopModified || RightModified || BottomModified || LeftModified)
		{
			PaddingAndRadius[SettingsIndex].TileSettings.Add(TileSettings);
		}
	}

	
}

bool UIT_WidgetPaddingAndRadius::AddedToItemAsset_Implementation(UDA_CoreItem* ItemAsset)
{
	//Am I crazy or is there no good constructor for FVector4?
	FVector4 NewRounding;
	NewRounding.X = 7;
	NewRounding.Y = 7;
	NewRounding.Z = 7;
	NewRounding.W = 7;
	Rounding = NewRounding;
	
	return Super::AddedToItemAsset_Implementation(ItemAsset);
}

TArray<FString> UIT_WidgetPaddingAndRadius::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	Super::VerifyData_Implementation(ItemAsset);

	TArray<FString> ErrorMessages;

	if(PaddingAndRadius.IsEmpty())
	{
		ErrorMessages.Add("No widget settings found");
	}

	return ErrorMessages;
}
