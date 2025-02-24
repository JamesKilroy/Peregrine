#pragma once
#include "AssetTypeActions_Base.h"
#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"

class FATA_CraftingAsset : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_CraftingAsset(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Red)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UDA_CoreCraftingRecipe::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> SubMenus
		{
			FText::FromString("Crafting")
		};
		return SubMenus;
	}
};
