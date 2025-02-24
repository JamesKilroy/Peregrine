#pragma once
#include "AssetTypeActions_Base.h"
#include "Core/Items/DA_CoreItem.h"

class FATA_ItemAsset : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemAsset(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Red)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UDA_CoreItem::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
