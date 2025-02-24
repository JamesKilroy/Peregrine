// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "UObject/Object.h"
#include "O_TagValueCalculation.generated.h"


/**Optional class to pass into any tag value setter functions,
 * allowing you to manage various calculations revolving the tag
 * value system, such as putting a clamp on a tag depending on
 * the value of another tag.
 *
 * Docs:
 * https://inventoryframework.github.io/workinginthesystem/tagsystem/#tag-value-calculation */
UCLASS(Abstract, Blueprintable)
class INVENTORYFRAMEWORKPLUGIN_API UO_TagValueCalculation : public UObject
{
	GENERATED_BODY()

public:

	/**In some cases, you have a tag value relationship that you want your game to
	 * understand and not have to constantly perform manual calculations.
	 * For example: Your items might have a durability, but a durability is usually
	 * associated with a max durability to clamp the normal durability value.
	 *
	 * Calculation classes will give you one spot to perform that calculation.
	 * When a value is added or increased, it'll ask the calculation class for
	 * the adjusted value.*/
	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	float CalculateItemTagValue(FS_TagValue TagValue, FS_InventoryItem Item);
	
	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	bool ShouldValueBeRemovedFromItem(FS_TagValue TagValue, FS_InventoryItem Item);

	/**Adjust the requested value of the tag through code.
	 * This is meant for things such as weight, where you
	 * might want to clamp a tag such as Container.Weight to
	 * the value of Container.MaxWeight*/
	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	float CalculateContainerTagValue(FS_TagValue TagValue, FS_ContainerSettings Container);

	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	bool ShouldValueBeRemovedFromContainer(FS_TagValue TagValue, FS_ContainerSettings Container);

	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	float CalculateComponentTagValue(FS_TagValue TagValue, UAC_Inventory* Component);

	UFUNCTION(Category = "Calculation", BlueprintNativeEvent)
	bool ShouldValueBeRemovedFromComponent(FS_TagValue TagValue, UAC_Inventory* Component);
};
