// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

class INVENTORYFRAMEWORKEDITOR_API FS_ContainerSettings_Customization : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	// BEGIN IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// END IPropertyTypeCustomization interface
	
	void OnTypeChanged(TSharedPtr<IPropertyHandle> TypePropertyHandle,
									 TSharedPtr<IPropertyUtilities> Utils);
};
