#pragma once
#include "AssetTypeActions_Base.h"
#include "DS_InventoryProjectSettings.h"

class FATA_TagValueCalculation : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_TagValueCalculation(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Green)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UO_TagValueCalculation::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
