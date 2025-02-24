#pragma once
#include "AssetTypeActions_Base.h"
#include "Core/Objects/Parents/ItemInstance.h"

class FATA_ItemInstance : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemInstance(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Yellow)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UItemInstance::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
