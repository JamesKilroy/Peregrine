#pragma once
#include "AssetTypeActions_Base.h"

class FATA_ItemActor : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemActor(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor( 63, 126, 255 ))
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return AA_ItemActor::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
