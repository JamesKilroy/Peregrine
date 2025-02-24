#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FIFP_CraftingModule : public IModuleInterface
{    
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
