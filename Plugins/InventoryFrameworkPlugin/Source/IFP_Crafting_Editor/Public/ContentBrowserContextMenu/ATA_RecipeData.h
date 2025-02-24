#pragma once
#include "AssetTypeActions_Base.h"
#include "Recipes/RecipeData/O_CoreRecipeData.h"

class FATA_RecipeData : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_RecipeData(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Yellow)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UO_CoreRecipeData::StaticClass(); }
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
