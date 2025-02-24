// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/IT_SocketManager.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Items/DA_CoreItem.h"
#include "Core/Traits/IT_CustomShapeData.h"
#include "Kismet/KismetSystemLibrary.h"

int32 UIT_SocketManager::GetTileForSocket(FName Socket, FS_InventoryItem ItemData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetTileForSocket)
	if(!ItemData.IsValid())
	{
		return -1;
	}
	
	FItemSocket* ItemSocket = Sockets.Find(Socket);
	if(!ItemSocket)
	{
		UFL_InventoryFramework::LogIFPMessage(this, FString::Printf(TEXT("Could not find socket %s for item"), *Socket.ToString()));
		return -1;
	}
	
	UAC_Inventory* ParentComponent = ItemData.UniqueID.ParentComponent;
	FS_ContainerSettings ContainerSettings = ParentComponent->ContainerSettings[ItemData.ContainerIndex];

	if(!ContainerSettings.IsSpacialContainer())
	{
		UFL_InventoryFramework::LogIFPMessage(this, FString::Printf(TEXT("Sockets are only supported in spacial containers")));
		return -1;
	}
	
	/**V: Sneaky hack to get the exact tile offset we need for the items rotation.
	 * The baked shape has already calculated all versions of a specific tile being rotated
	 * in the items local space. IE, X0/Y0 is index 0 and all rotations of it are also stored
	 * in index 0 for each rotated shape.
	 * This means that if we get the index of the socket in the default rotation of 0, let's say
	 * it is X5/Y4 and the index is 20. We can now use index 20 of all the rotated shapes to "rotate"
	 * that X5/Y4, without doing any math.*/
	int32 SocketDefaultRotationIndex = ItemData.ItemAsset->Shapes[0].Shape.Find(ItemSocket->Location);
	FIntPoint SocketRotatedLocation;
	if(SocketDefaultRotationIndex == -1)
	{
		return -1;
	}
	
	for(auto& CurrentShape : ItemData.ItemAsset->Shapes)
	{
		if(CurrentShape.Rotation != ItemData.Rotation)
		{
			continue;
		}
		
		SocketRotatedLocation = CurrentShape.Shape[SocketDefaultRotationIndex];
		break;
	}
	
	FIntPoint ItemTile;
	UFL_InventoryFramework::IndexToTile(ItemData.TileIndex, ContainerSettings, ItemTile.X, ItemTile.Y);
	ItemTile += SocketRotatedLocation;
	return UFL_InventoryFramework::TileToIndex(ItemTile.X, ItemTile.Y, ContainerSettings);
}

int32 UIT_SocketManager::GetTileForSocketWithOffset(FName Socket, FS_InventoryItem ItemData, FIntPoint Offset)
{
	if(!ItemData.IsValid())
	{
		return -1;
	}
	
	FItemSocket* ItemSocket = Sockets.Find(Socket);
	if(!ItemSocket)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Could not find socket for item");
		return -1;
	}
	
	UAC_Inventory* ParentComponent = ItemData.UniqueID.ParentComponent;
	FS_ContainerSettings ContainerSettings = ParentComponent->ContainerSettings[ItemData.ContainerIndex];
	
	if(!ContainerSettings.IsSpacialContainer())
	{
		UFL_InventoryFramework::LogIFPMessage(this, FString::Printf(TEXT("Sockets are only supported in spacial containers")));
		return -1;
	}

	//Get the original location of the socket and get the offset
	int32 SocketLocation = GetTileForSocket(Socket, ItemData);
	FIntPoint Remainder;
	int32 OffsetIndex = UFL_InventoryFramework::ApplyTileOffset(SocketLocation, ContainerSettings, Offset, Remainder);

	//Convert the offset index into a XY location
	FIntPoint OffsetTile;
	UFL_InventoryFramework::IndexToTile(OffsetIndex, ContainerSettings, OffsetTile.X, OffsetTile.Y);

	//Create a "shape" so we can use the RotateShape helper function
	TArray<FIntPoint> Shape;
	Shape.Add(OffsetTile);

	//Convert the original socket location into a XY location
	FIntPoint SocketTileLocation;
	UFL_InventoryFramework::IndexToTile(SocketLocation, ContainerSettings, SocketTileLocation.X, SocketTileLocation.Y);

	//Rotate the offset tile around the location of the original socket tile.
	//We combine rotation of the socket and the item to ensure the sockets rotation is correct.
	Shape = UFL_InventoryFramework::RotateShape(Shape,
		UFL_InventoryFramework::CombineRotations(ItemSocket->Rotation, ItemData.Rotation), SocketTileLocation);
	
	return UFL_InventoryFramework::TileToIndex(Shape[0].X, Shape[0].Y, ContainerSettings);
}

TArray<FString> UIT_SocketManager::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	TArray<FString> ErrorMessages = Super::VerifyData_Implementation(ItemAsset);

	if(Sockets.IsEmpty())
	{
		ErrorMessages.Add("No sockets found.");
	}

	if(ItemAsset)
	{
		//Check if any sockets are inside a disabled tile.
		TArray<UItemTrait*> CustomShapeData;
		UFL_InventoryFramework::GetTraitsByClassForItem(ItemAsset, UIT_CustomShapeData::StaticClass(), CustomShapeData);
		if(CustomShapeData.IsValidIndex(0))
		{
			TArray<FIntPoint> DisabledTiles = Cast<UIT_CustomShapeData>(CustomShapeData[0])->DisabledTiles;
			for(auto& CurrentSocket : Sockets)
			{
				if(DisabledTiles.Contains(CurrentSocket.Value.Location))
				{
					ErrorMessages.Add(FString::Printf(TEXT("Socket `%s` is inside a hidden tile"), *CurrentSocket.Key.ToString()));
				}
			}
		}
	}

	return ErrorMessages;
}
