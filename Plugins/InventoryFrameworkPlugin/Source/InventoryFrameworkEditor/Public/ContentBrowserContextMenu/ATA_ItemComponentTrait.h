#pragma once
#include "AssetTypeActions_Base.h"
#include "DS_InventoryProjectSettings.h"

class FATA_ItemComponentTrait : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemComponentTrait(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Yellow)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UIT_ItemComponentTrait::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
