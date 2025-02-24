#pragma once
#include "AssetTypeActions_Base.h"
#include "Recipes/CraftEvents/O_CoreCraftEvent.h"

class FATA_CraftEvent : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_CraftEvent(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Yellow)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UO_CoreCraftEvent::StaticClass(); }
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
