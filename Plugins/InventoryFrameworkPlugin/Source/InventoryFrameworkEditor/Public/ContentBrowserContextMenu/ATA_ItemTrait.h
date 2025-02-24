#pragma once
#include "AssetTypeActions_Base.h"
#include "DS_InventoryProjectSettings.h"

class FATA_ItemTrait : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemTrait(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Yellow)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UItemTrait::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
