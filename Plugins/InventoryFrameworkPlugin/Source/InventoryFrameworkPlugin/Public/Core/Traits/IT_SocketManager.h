// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "ItemTrait.h"
#include "IT_SocketManager.generated.h"

USTRUCT(BlueprintType)
struct FItemSocket
{
	GENERATED_BODY()

	/**The rotation of this socket*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TEnumAsByte<ERotation> Rotation = Zero;

	/**The location of this socket in the items pure shape.
	 * If the item is rotated, the location is also rotated.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FIntPoint Location = FIntPoint();

	/**Optional tags to associate with this socket.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FGameplayTagContainer SocketTags;
};

/**This object allows you to place sockets on an item, similar to
 * sockets on static/skeletal meshes.
 * These sockets live on a specific tile and can have a rotation.
 * This rotation is combined with the items current rotation.
 */
UCLASS(DisplayName = "Socket Manager")
class INVENTORYFRAMEWORKPLUGIN_API UIT_SocketManager : public UItemTrait
{
	GENERATED_BODY()

public:

	/**Sockets to add to the item. This can be used to mark a location
	 * on an item, for example the muzzle of a gun. Then, when a silencer
	 * is moved in front of that socket, you could run gameplay logic.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TMap<FName, FItemSocket> Sockets;

	UFUNCTION(Category = "Socket Manager", BlueprintCallable, BlueprintPure)
	int32 GetTileForSocket(FName Socket, FS_InventoryItem ItemData);

	/**Takes a socket and its rotation and applies an offset in that direction.
	 * Remember, the offset math starts at top left and goes towards bottom right.
	 * So positive X is right and negative Y is up.
	 * This is always thinking on Rotation 0, so if you want to go forward, you'd put
	 * in X0/Y-1, then in all rotations, it'll rotate those coordinates for you to
	 * always go forward.*/
	UFUNCTION(Category = "Socket Manager", BlueprintCallable, BlueprintPure)
	int32 GetTileForSocketWithOffset(FName Socket, FS_InventoryItem ItemData, FIntPoint Offset);

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;
};
