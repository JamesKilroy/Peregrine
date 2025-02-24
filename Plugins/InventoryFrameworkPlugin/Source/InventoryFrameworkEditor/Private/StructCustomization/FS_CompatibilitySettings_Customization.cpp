// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "StructCustomization/FS_CompatibilitySettings_Customization.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FS_CompatibilitySettings_Customization::MakeInstance()
{
	return MakeShareable(new FS_CompatibilitySettings_Customization());
}

void FS_CompatibilitySettings_Customization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		StructPropertyHandle->CreatePropertyValueWidget()
	];
}

void FS_CompatibilitySettings_Customization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		auto ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		
		if(ChildHandle->GetProperty()->GetName() == "EquipmentSlots" || ChildHandle->GetProperty()->GetName() == "EquipmentTypes"
			|| ChildHandle->GetProperty()->GetName() == "WeaponTypes" || ChildHandle->GetProperty()->GetName() == "ItemAttachments")
		{
			uint32 NumElements;
			ChildHandle->AsArray()->GetNumElements(NumElements);
			if(NumElements != 0)
			{
				StructBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
		else
		{
			StructBuilder.AddProperty(ChildHandle.ToSharedRef());
		}
	}
}
