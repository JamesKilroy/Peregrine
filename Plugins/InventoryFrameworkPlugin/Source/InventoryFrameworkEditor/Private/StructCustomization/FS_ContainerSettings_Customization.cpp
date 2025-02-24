// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "StructCustomization/FS_ContainerSettings_Customization.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DS_InventoryProjectSettings.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Kismet/KismetStringLibrary.h"

TSharedRef<IPropertyTypeCustomization> FS_ContainerSettings_Customization::MakeInstance()
{
	return MakeShareable(new FS_ContainerSettings_Customization());
}

void FS_ContainerSettings_Customization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	FString GameplayTagString;
	FString ArrayIndexString;

	// Get the struct
	TSharedPtr<IPropertyHandle> InnerStructPropertyHandle = StructPropertyHandle->GetChildHandle("ContainerIdentifier");

	if (InnerStructPropertyHandle.IsValid())
	{
		InnerStructPropertyHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &FS_ContainerSettings_Customization::OnTypeChanged,
							  InnerStructPropertyHandle,
							  StructCustomizationUtils.GetPropertyUtilities()));
		// Access the FName property of the inner struct
		TSharedPtr<IPropertyHandle> FNamePropertyHandle = InnerStructPropertyHandle->GetChildHandle("TagName");

		if (FNamePropertyHandle.IsValid())
		{
			// Get the value of the FName property
			FName FNameValue;
			FNamePropertyHandle->GetValue(/*out*/ FNameValue);
			GameplayTagString = FNameValue.ToString();
		}
	}

	if (GameplayTagString.IsEmpty())
	{
		GameplayTagString = "No tag found";
	}
	else
	{
		if(StructPropertyHandle->GetParentHandle().IsValid() && StructPropertyHandle->GetParentHandle()->IsValidHandle())
		{
			int32 ArrayIndex = StructPropertyHandle->GetIndexInArray();
			ArrayIndexString = FString::Printf(TEXT("%d"), ArrayIndex);

			if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
			{
				if(IFPSettings->TagHierarchySplitAmount > 0)
				{
					FString ShortenedString;
					FString RemainderString = GameplayTagString;
					FString RightString;
					//Start to loop from the end of the string, slowly removing compiling the far right text.
					for(int32 CurrentSplit = 0; CurrentSplit < IFPSettings->TagHierarchySplitAmount; CurrentSplit++)
					{
						//Prevents the string from going too far
						if(!RemainderString.Contains("."))
						{
							break;
						}
						UKismetStringLibrary::Split(RemainderString, ".", RemainderString, RightString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
						if(CurrentSplit == 0)
						{
							//Don't include the tag split text for the first item
							ShortenedString = RightString + ShortenedString;
						}
						else
						{
							ShortenedString = RightString + IFPSettings->TagSplitText + ShortenedString;
						}
					}

					GameplayTagString = ShortenedString;
				}
			}
			
			
			GameplayTagString = "[" + ArrayIndexString + "] " + GameplayTagString;
		}
	}

	FExecuteAction InsertAction = FExecuteAction::CreateLambda([StructPropertyHandle]
		{
			const TSharedPtr<IPropertyHandleArray> ParentPropertyHandleArray = StructPropertyHandle->GetParentHandle()->AsArray();
			const int32 ArrayIndex = StructPropertyHandle->IsValidHandle() ? StructPropertyHandle->GetIndexInArray() : INDEX_NONE;
			if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
			{
				ParentPropertyHandleArray->Insert(ArrayIndex);
			}
		});
		
	const FExecuteAction DeleteAction = FExecuteAction::CreateLambda([StructPropertyHandle]
	{
		const TSharedPtr<IPropertyHandleArray> ParentPropertyHandleArray = StructPropertyHandle->GetParentHandle()->AsArray();
		const TSharedPtr<IPropertyHandleSet> ParentPropertyHandleSet = StructPropertyHandle->GetParentHandle()->AsSet();
		const int32 ArrayIndex = StructPropertyHandle->IsValidHandle() ? StructPropertyHandle->GetIndexInArray() : INDEX_NONE;
		if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
		{
			ParentPropertyHandleArray->DeleteItem(ArrayIndex);
		}
		else if(ParentPropertyHandleSet.IsValid() && ArrayIndex >= 0)
		{
			ParentPropertyHandleSet->DeleteItem(ArrayIndex);
		}
	});

	FExecuteAction DuplicateAction = FExecuteAction::CreateLambda([StructPropertyHandle]
	{
		const TSharedPtr<IPropertyHandleArray> ParentPropertyHandleArray = StructPropertyHandle->GetParentHandle()->AsArray();
		const int32 ArrayIndex = StructPropertyHandle->IsValidHandle() ? StructPropertyHandle->GetIndexInArray() : INDEX_NONE;
		if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
		{
			ParentPropertyHandleArray->DuplicateItem(ArrayIndex);
		}
	});

	HeaderRow
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(GameplayTagString))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[//The Insert/Delete/Duplicate button lives in the header. Since we are removing the header, we have to add it again.
			PropertyCustomizationHelpers::MakeInsertDeleteDuplicateButton(InsertAction, DeleteAction, DuplicateAction)
		]
	];
}

void FS_ContainerSettings_Customization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		auto ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);

		// StructBuilder.AddProperty(ChildHandle.ToSharedRef());
		if(ChildHandle->GetProperty()->GetName() == "HiddenSlots" || ChildHandle->GetProperty()->GetName() == "LockedSlots")
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

void FS_ContainerSettings_Customization::OnTypeChanged(TSharedPtr<IPropertyHandle> TypePropertyHandle,
									 TSharedPtr<IPropertyUtilities> Utils)
{
	if (Utils)
	{
		Utils->ForceRefresh();
	}
}