// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "O_CoreRecipeData.h"
#include "RD_TimeToCraftBase.h"
#include "RD_TimeToCraft.generated.h"

/**
 * After the player requests the craft,
 * how long should it take to finish?
 */
UCLASS(DisplayName = "Time to Craft")
class IFP_CRAFTING_API URD_TimeToCraft : public URD_TimeToCraftBase
{
	GENERATED_BODY()

public:
	virtual float GetTimeToCraft_Implementation() override
	{
		return TimeToCraft;
	}

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadOnly)
	float TimeToCraft = 1;
};
