#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Mineral.generated.h"

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Mineral : public UDA_CoreItem
{
    GENERATED_BODY()

    virtual FText GetAssetTypeName() override;
};
