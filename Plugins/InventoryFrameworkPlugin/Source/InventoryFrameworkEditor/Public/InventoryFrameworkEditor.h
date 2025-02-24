// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetToolsModule.h"
#include "AssetTypeCategories.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"

class FInventoryFrameworkEditor : public IModuleInterface
{
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
	EAssetTypeCategories::Type InventoryFrameworkCategory = EAssetTypeCategories::None;
	
public:
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	template<class T>
	void Internal_RegisterTypeActions(const FString& Name)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		const auto Action = MakeShared<T>(InventoryFrameworkCategory, FText::FromString(Name));
		RegisteredAssetTypeActions.Emplace(Action);
		AssetTools.RegisterAssetTypeActions(Action);
	}

	TSharedPtr<FSlateStyleSet> StyleSetInstance;

	void RegisterStyleSet();

	void CreateWelcomePopup();
};
