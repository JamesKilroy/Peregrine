#pragma once
#include "AssetTypeActions_Base.h"
#include "LootTableSystem/Objects/O_LootPool.h"

class FATA_LootPoolObject : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_LootPoolObject(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Purple)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UO_LootPool::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> SubMenus
		{
			FText::FromString("Other")
		};
		return SubMenus;
	}
};
