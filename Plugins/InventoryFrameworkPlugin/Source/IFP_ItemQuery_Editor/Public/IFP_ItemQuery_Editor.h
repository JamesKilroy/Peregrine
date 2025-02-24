#pragma once

#include "CoreMinimal.h"
#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"

class FIFP_ItemQuery_EditorModule : public IModuleInterface
{
    TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
    EAssetTypeCategories::Type InventoryFrameworkCategory = EAssetTypeCategories::None;
    
public:
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
};
