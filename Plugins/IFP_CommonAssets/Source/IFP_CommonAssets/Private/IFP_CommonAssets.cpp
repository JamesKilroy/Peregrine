// Copyright Epic Games, Inc. All Rights Reserved.

#include "IFP_CommonAssets.h"

#define LOCTEXT_NAMESPACE "FIFP_CommonAssetsModule"

void FIFP_CommonAssetsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FIFP_CommonAssetsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FIFP_CommonAssetsModule, IFP_CommonAssets)