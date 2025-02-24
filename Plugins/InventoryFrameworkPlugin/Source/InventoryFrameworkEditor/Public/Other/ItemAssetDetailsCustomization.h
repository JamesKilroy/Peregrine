#pragma once
#include "IDetailCustomization.h"

class UItemAssetDetailsCustomization : public IDetailCustomization
{
public:

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance();
};
