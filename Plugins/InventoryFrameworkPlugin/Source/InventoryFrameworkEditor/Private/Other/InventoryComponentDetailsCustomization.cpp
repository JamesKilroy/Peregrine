#include "Other/InventoryComponentDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

TSharedRef<IDetailCustomization> UInventoryComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new UInventoryComponentDetailsCustomization);
}

void UInventoryComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(Inventories);
	IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("Settings");
	SettingsCategory.AddCustomRow(FText::FromString("Settings"))
	.WholeRowWidget
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.Padding(FMargin(0, 5, 0, 10))
			.AutoWidth()
		[
		   SNew(SButton)
		   .Text(FText::FromString("Validate Fragments"))
		   .ToolTipText(FText::FromString("Ensure all fragments on this component are valid."))
		   .OnClicked(this, &UInventoryComponentDetailsCustomization::OnButtonClicked)
		 ]
	];
}

FReply UInventoryComponentDetailsCustomization::OnButtonClicked()
{
	TArray<FString> Errors;
	
	for(auto& CurrentInventory : Inventories)
	{
		if(CurrentInventory.IsValid())
		{
			UAC_Inventory* Inventory = Cast<UAC_Inventory>(CurrentInventory.Get());
			if(Inventory)
			{
				int32 ContainerIndex = -1;
				for(auto& CurrentContainer : Inventory->ContainerSettings)
				{
					ContainerIndex++;
					TArray<int32> DuplicateIndexes = UFL_IFP_FragmentHelpers::FindDuplicateFragments(CurrentContainer.ContainerFragments);
					if(DuplicateIndexes.IsValidIndex(0))
					{
						for(auto& CurrentIndex : DuplicateIndexes)
						{
							FString Error = FString::Printf(TEXT("Fragment %s (index %i) inside container %s is a duplicate. Duplicate fragments are not supported"), 
								*CurrentContainer.ContainerFragments[CurrentIndex].GetScriptStruct()->GetName(),
								CurrentIndex,
								*CurrentContainer.ContainerIdentifier.ToString());
							Errors.Add(Error);
						}
					}
					
					for(auto& ContainerFragment : CurrentContainer.ContainerFragments)
					{
						if(ContainerFragment.GetPtr<>())
						{
							if(!ContainerFragment.GetMutable<>().IsCompatibleWithContainer(Inventory, CurrentContainer))
							{
								FString Error = FString::Printf(TEXT("Fragment %s is incompatible with container %s"),
									*ContainerFragment.GetScriptStruct()->GetName(),
									*CurrentContainer.ContainerIdentifier.ToString());
								Errors.Add(Error);
							}
						}

						if(!ContainerFragment.IsValid())
						{
							FString Error = FString::Printf(TEXT("Fragment is set to null on %s"),
								*CurrentContainer.ContainerIdentifier.ToString());
							Errors.Add(Error);
						}
					}

					for(auto& CurrentItem : CurrentContainer.Items)
					{
						DuplicateIndexes.Empty();
						DuplicateIndexes = UFL_IFP_FragmentHelpers::FindDuplicateFragments(CurrentItem.ItemFragments);
						if(DuplicateIndexes.IsValidIndex(0))
						{
							for(auto& CurrentIndex : DuplicateIndexes)
							{
								FString Error = FString::Printf(TEXT("Fragment %s (index %i) inside item %s is a duplicate. Duplicate fragments are not supported"), 
									*CurrentItem.ItemFragments[CurrentIndex].GetScriptStruct()->GetName(),
									CurrentIndex,
									*CurrentItem.ItemAssetSoftReference.GetAssetName());
								Errors.Add(Error);
							}
						}
						
						for(auto& ItemFragment : CurrentItem.ItemFragments)
						{
							if(ItemFragment.GetPtr<>())
							{
								if(!ItemFragment.GetMutable<>().IsCompatibleWithItem(Inventory, CurrentItem))
								{
									FString Error = FString::Printf(TEXT("Fragment %s is incompatible with item %s"),
										*ItemFragment.GetScriptStruct()->GetName(),
										*CurrentItem.ItemAssetSoftReference.GetAssetName());
									Errors.Add(Error);
								}
							}
							
							if(!ItemFragment.IsValid())
							{
								FString Error = FString::Printf(TEXT("Fragment is set to null on %s"),
									*CurrentItem.ItemAssetSoftReference.GetAssetName());
								Errors.Add(Error);
							}
						}
					}
				}
			}
		}
	}

	if(Errors.IsValidIndex(0))
	{
		for(auto& Error :Errors)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
			FNotificationInfo Info(FText::FromString(Error));
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			Info.bFireAndForget = true;
			Info.bUseSuccessFailIcons = true;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
	else
	{
		FNotificationInfo Info(FText::FromString("Fragments are valid"));
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
	}
	
	return FReply::Handled();
}