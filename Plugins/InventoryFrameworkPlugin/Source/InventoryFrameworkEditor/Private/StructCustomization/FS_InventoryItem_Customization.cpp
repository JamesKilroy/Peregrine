// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#include "StructCustomization/FS_InventoryItem_Customization.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DS_InventoryProjectSettings.h"
#include "IDetailTreeNode.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Items/DA_CoreItem.h"
#include "Slate/DeferredCleanupSlateBrush.h"

#define LOCTEXT_NAMESPACE "InventoryItemCustomization"

class UDS_InventoryProjectSettings;

TSharedRef<IPropertyTypeCustomization> FS_InventoryItem_Customization::MakeInstance()
{
	// Create the instance and returned a SharedRef
	return MakeShareable(new FS_InventoryItem_Customization());
}

void FS_InventoryItem_Customization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();
	//Retrieve the data asset from the struct
	UObject* DataAssetObject = nullptr;
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemAssetSoftReference))->GetValue(DataAssetObject);
	
	UDA_CoreItem* ItemAsset = Cast<UDA_CoreItem>(DataAssetObject);
	UTexture2D* IconTexture = nullptr;
	
	if(ItemAsset)
	{
		//Retrieve the icon texture using your data asset’s function
		IconTexture = ItemAsset->GetThumbnailTexture();
		IconTexturePtr = IconTexture;

		//FDeferredCleanupSlateBrush is so far, the only safe way of adding a Texture2D to SImage that I've found. All other methods cause a crash eventually
		IconBrush = FDeferredCleanupSlateBrush::CreateBrush(IconTexture, FVector2D(IconSize, IconSize));
	}

	FString HeaderTitle;
	if(ItemAsset)
	{
		HeaderTitle = ItemAsset->ItemName.ToString();
	}
	else
	{
		HeaderTitle = "No Item Available";
	}

	if(StructPropertyHandle->GetIndexInArray() != INDEX_NONE)
	{
		HeaderTitle += "   [ " + FString::FromInt(StructPropertyHandle->GetIndexInArray()) + " ]";
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
		.Padding(2.0f)
		[
			//Display the icon if the brush was created
			SNew(SImage)
			.Image(IconBrush.IsValid() ? IconBrush->GetSlateBrush() : nullptr)
			//If the texture is invalid, just hide the icon
			.Visibility(IconBrush.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			// StructPropertyHandle->CreatePropertyValueWidget(false)
			SNew(STextBlock)
			.Text(FText::FromString(HeaderTitle))
			.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
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

void FS_InventoryItem_Customization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);
	const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>();

	//Get the inventory component, container index and item index
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	UAC_Inventory* InventoryComponent = nullptr;
	TSharedPtr<IPropertyHandle> ParentPropertyHandle = StructPropertyHandle->GetParentHandle();
	TSharedPtr<IPropertyHandleArray> ItemsArrayHandle;
	int32 ContainerIndex = -1;
	if(ParentPropertyHandle.IsValid())
	{
		ItemsArrayHandle = ParentPropertyHandle->AsArray();
		if(ItemsArrayHandle.IsValid())
		{
			//The parent handle is the item struct. So we get the parent of that,
			//which is the container struct.
			ContainerIndex = ParentPropertyHandle->GetParentHandle()->GetIndexInArray();
		}
	}

	int32 ItemIndex = StructPropertyHandle->GetIndexInArray();
	FS_ContainerSettings Container = FS_ContainerSettings();
	
	for(auto& CurrentOuter : OuterObjects)
	{
		if(!CurrentOuter) { continue; }
		
		if(Cast<UAC_Inventory>(CurrentOuter))
		{
			InventoryComponent = Cast<UAC_Inventory>(CurrentOuter);
			Container = InventoryComponent->ContainerSettings[ContainerIndex];
			break;
		}
	}

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		auto ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);

		//We don't want certain properties to be visible.
		if(ChildHandle->GetProperty()->GetName() != GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemEditorName) &&
			ChildHandle->GetProperty()->GetName() !=  GET_MEMBER_NAME_CHECKED(FS_InventoryItem, CanRotate) && 
			ChildHandle->GetProperty()->GetName() != GET_MEMBER_NAME_CHECKED(FS_InventoryItem, IsStackable) &&
			ChildHandle->GetProperty()->GetName() != GET_MEMBER_NAME_CHECKED(FS_InventoryItem, AllowItemInstance) &&
			ChildHandle->GetProperty()->GetName() != GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ContainerSupportsTileMap))
		{
			bool UseDefaultBuilder = true;

			//Customize the ItemAsset child
			if(ChildHandle->GetProperty()->GetName() == GET_MEMBER_NAME_CHECKED(FS_InventoryItem, ItemAssetSoftReference) &&
				IFPSettings->LimitItemSelectionToCompabilityCheck &&
				(ParentPropertyHandle.IsValid() && ParentPropertyHandle->GetParentHandle().IsValid() &&
					ParentPropertyHandle->GetParentHandle()->GetProperty()->GetName() == "ContainerSettings"))
				/**Note around the last ContainerSettings name check: This is required for fragments that can have a ItemStruct
				 * inside of it. An example would be the DefaultItems fragment. Without this check, it's going to get the
				 * index of the item its inside of and believe that is the ContainerIndex, retrieving the incorrect
				 * compatibility settings. This check essentially disables this QoL feature for those type of fragments
				 * until a better solution can be found.*/
			{
				/**BLACK MAGIC TIME!
				 * Since we can't convert IPropertyHandle to a struct reference,
				 * and GetValue does not work with most structs, we have to do a little
				 * bit of cheating. This will get the inventory component, then retrieve
				 * the correct Container from the parents parent handle, since that handle
				 * stores the index in the array, allowing us to find the compatibility
				 * settings to properly filter out assets.*/
				
				//Check if this is an Items array
				if(ItemsArrayHandle.IsValid() && InventoryComponent)
				{
					// Now access the ContainerSettings array within AC_Inventory
					if(InventoryComponent->ContainerSettings.IsValidIndex(ContainerIndex))
					{
						// Get the container at the given index
						TInstancedStruct<FCompatibilitySettingsFragment> CompatibilitySettings = FindTemplatedFragment<FCompatibilitySettingsFragment>(Container.ContainerFragments);
										
						//Filter the dropdown using the inventory component and the compatibility settings.
						StructBuilder.AddProperty(ChildHandle.ToSharedRef())
						.CustomWidget()
						.NameContent()
						[
							ChildHandle->CreatePropertyNameWidget()
						]
						.ValueContent()
						.MaxDesiredWidth(500)
						[
							SNew(SObjectPropertyEntryBox)
							.PropertyHandle(ChildHandle)
							.AllowedClass(UDA_CoreItem::StaticClass())
							.ThumbnailPool(StructCustomizationUtils.GetThumbnailPool())
							.OnShouldFilterAsset(FOnShouldFilterAsset::CreateSP(
								this, &FS_InventoryItem_Customization::OnFilterAsset,
								CompatibilitySettings,
								InventoryComponent))
						]
						.ShouldAutoExpand(true);

						//Customization was successful, don't use default
						UseDefaultBuilder = false;
					}
				}
				
				ChildHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
				{
					//Header icon won't update when asset is changed. Force it to refresh.
					PropUtils->ForceRefresh();
				}));
			}

			if(UseDefaultBuilder)
			{
				StructBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}
}

bool FS_InventoryItem_Customization::OnFilterAsset(const FAssetData& AssetData, TInstancedStruct<FCompatibilitySettingsFragment> CompatibilitySettings, UAC_Inventory* Inventory)
{
	FS_InventoryItem ItemData;
	ItemData.ItemAsset = Cast<UDA_CoreItem>(AssetData.GetAsset());
	FS_ContainerSettings ContainerSettings;
	ContainerSettings.ContainerFragments.Add(CompatibilitySettings);
	
	return !Inventory->CheckCompatibility(ItemData, ContainerSettings);;
}


#undef LOCTEXT_NAMESPACE
