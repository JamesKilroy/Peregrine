// Copyright 2023, Jan Kozlowski, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FFirstPersonMovementTemplateModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
