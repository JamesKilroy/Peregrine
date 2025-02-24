#pragma once
#include "AssetTypeActions_Base.h"
#include "DS_InventoryProjectSettings.h"
#include "Core/Components/ItemComponent.h"

class FATA_ItemComponent : public FAssetTypeActions_Base
{
private:
	const EAssetTypeCategories::Type AssetCategory;
	const FText Name;
	const FColor Color;
	
public:

	explicit FATA_ItemComponent(EAssetTypeCategories::Type AssetCategory, const FText& Name, const FColor& Color = FColor::Cyan)
		: AssetCategory(AssetCategory), Name(Name), Color(Color)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return UItemComponent::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
};
