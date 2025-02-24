// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CF_TileTags.generated.h"

/**Enables tags and tag values on tiles.
 * Usually, this fragment takes up a lot of memory
 * to improve performance. For some projects, this might
 * not be favourable. You will want to change the setting
 * inside of C++ inside this fragment.*/
USTRUCT(BlueprintType, DisplayName = "🔖 Tile Tags")
struct FTileTagsFragment : public FContainerFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TArray<FS_TileTag> TileTags;

	/**Since we can't use TMap's and there are hundreds of tiles
	 * in some cases, the TileTag system can become very expensive.
	 * This is because the TileTags array is not 1:1 with the TileMap.
	 * So if we wanted to get tile 100's tags, we would have to increment
	 * over every element inside TileTags until we find the one that
	 * belongs to tile 100.
	 *
	 * If this is set to true, the TileTags will create one element for
	 * each index inside the TileMap, so it will be 1:1, and you can now
	 * simply get index 100. Even though the other 99 tiles don't
	 * have any tags associated with them. Hence, why this improves
	 * the game thread performance, but decreases the memory
	 * performance of this fragment. */
	bool OptimizeForGameThread = true;

	virtual bool InitializeFragment(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container, const FS_InventoryItem& Item) override;
	
	virtual bool IsCompatibleWithContainer(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container) override;
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UCF_TileTags : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "IFP|Fragments|Getters", BlueprintCallable)
	static FTileTagsFragment GetTileTagsFragmentFromContainer(FS_ContainerSettings Container);

	//Attempt to convert @Struct to TagFragment
	UFUNCTION(Category = "IFP|Fragments|Conversion", BlueprintCallable, BlueprintPure)
	static FTileTagsFragment ToTileTagsFragment(FInstancedStruct Struct);

	/**The tile tag array is not 1:1 to the tilemap array, some tiles don't have a tag
	 * so they aren't added to the tiletag array.
	 * This will give you the index of @TileIndex inside the TileTag array.
	 * Will return -1 if not found.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Getters")
	static int32 GetTileTagIndexForTile(FS_ContainerSettings Container, int32 TileIndex);

	/**Get any tags the desired tile owns.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IFP|Tiles|Getters")
	static FGameplayTagContainer GetTilesTags(FS_ContainerSettings Container, int32 TileIndex);
};
