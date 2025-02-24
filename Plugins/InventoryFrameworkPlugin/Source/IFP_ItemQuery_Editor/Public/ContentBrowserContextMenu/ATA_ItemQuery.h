#pragma once
#include "AssetTypeActions_Base.h"
#include "O_ItemQueryBase.h"

class FATA_ItemQuery : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemQuery(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Magenta)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UO_ItemQueryBase::StaticClass(); }
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
