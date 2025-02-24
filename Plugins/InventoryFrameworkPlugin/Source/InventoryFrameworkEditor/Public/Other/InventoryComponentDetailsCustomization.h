#pragma once
#include "IDetailCustomization.h"

/**Simply adds a button to validate fragments.
 * Can't use CallInEditor specifier because that only adds a
 * button while in the level editor, not while in the class editor.*/
class UInventoryComponentDetailsCustomization : public IDetailCustomization
{
public:

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance();

	FReply OnButtonClicked();

	TArray<TWeakObjectPtr<UObject>> Inventories;
};
