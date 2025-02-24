#include "Other/ItemAssetDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void UItemAssetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	//Push the Core Settings category to the top
	IDetailCategoryBuilder& SetupCategory = DetailBuilder.EditCategory("Core Settings");
	SetupCategory.SetSortOrder(0);
}

TSharedRef<IDetailCustomization> UItemAssetDetailsCustomization::MakeInstance()
{
	return MakeShareable(new UItemAssetDetailsCustomization);
}
