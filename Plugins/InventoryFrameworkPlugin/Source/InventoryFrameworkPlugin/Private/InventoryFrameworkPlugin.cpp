// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#include "InventoryFrameworkPlugin.h"

#define LOCTEXT_NAMESPACE "FInventoryFrameworkPluginModule"

DEFINE_LOG_CATEGORY(LogInventoryFramework);

void FInventoryFrameworkPluginModule::StartupModule()
{
	UE_LOG(LogInventoryFramework, Log, TEXT("Inventory Framework module initialized"))
}

void FInventoryFrameworkPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventoryFrameworkPluginModule, InventoryFrameworkPlugin)