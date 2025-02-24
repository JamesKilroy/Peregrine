// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/DA_CoreItem.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Traits/IT_CustomShapeData.h"
#include "Core/Objects/Parents/ItemInstance.h"
#include "Core/Actors/Parents/A_ItemActor.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Fragments/F_Tags.h"
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
#include "EditorAssetLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Editor.h"
#endif


bool UDA_CoreItem::CanItemStack()
{
	return CanItemTypeStack() && MaxStack != 1;
}

bool UDA_CoreItem::CanItemTypeStack()
{
	return true;
}

TSubclassOf<UW_AttachmentParent> UDA_CoreItem::GetAttachmentWidgetClass()
{
	return nullptr;
}

TArray<FS_ContainerSettings> UDA_CoreItem::GetDefaultContainers()
{
	return TArray<FS_ContainerSettings>();
}

TArray<FIntPoint> UDA_CoreItem::GetItemsPureShape(TEnumAsByte<ERotation> Rotation)
{
	TArray<FIntPoint> ReturnShape;

	for(auto& CurrentShape : Shapes)
	{
		if(CurrentShape.Rotation == Rotation)
		{
			return CurrentShape.Shape;
		}
	}

	return ReturnShape;
}

TArray<FIntPoint> UDA_CoreItem::GetDisabledTiles()
{
	TArray<FIntPoint> DisabledTiles;

	//Find out if this asset has the CustomShape object.
	for(UItemTrait*& CurrentObject : TraitsAndComponents)
	{
		if(IsValid(CurrentObject))
		{
			if(CurrentObject->GetClass() == UIT_CustomShapeData::StaticClass())
			{
				//CustomShape object found, get its DisabledTiles.
				return Cast<UIT_CustomShapeData>(CurrentObject)->DisabledTiles;
			}
		}
	}

	return DisabledTiles;
}

FIntPoint UDA_CoreItem::GetAnchorPoint()
{
	FIntPoint AnchorPoint = FIntPoint(0, 0);

	//Find out if this asset has the CustomShape object.
	for(UItemTrait*& CurrentObject : TraitsAndComponents)
	{
		if(IsValid(CurrentObject))
		{
			if(CurrentObject->GetClass() == UIT_CustomShapeData::StaticClass())
			{
				//CustomShape object found, get its DisabledTiles.
				AnchorPoint = Cast<UIT_CustomShapeData>(CurrentObject)->AnchorPoint;
			}
		}
	}

	return AnchorPoint;
}

FText UDA_CoreItem::GetAssetTypeName()
{
	return FText(FText::FromString("Unknown"));
}

TArray<UAC_LootTable*> UDA_CoreItem::GetLootTables()
{
	return TArray<UAC_LootTable*>();
}

int32 UDA_CoreItem::FindObjectIndex(UItemTrait* Object)
{
	for(int32 Index = 0; Index < TraitsAndComponents.Num(); Index++)
	{
		if(TraitsAndComponents[Index] == Object)
		{
			return Index;
		}
	}
	return -1;
}

FPrimaryAssetId UDA_CoreItem::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetRegistryCategory, GetFName());
}

TArray<TSoftClassPtr<UItemComponent>> UDA_CoreItem::GetItemComponentsFromTraits()
{
	TArray<TSoftClassPtr<UItemComponent>> Components;
	
	for(auto& CurrentTrait : TraitsAndComponents)
	{
		if(CurrentTrait->GetClass()->IsChildOf(UIT_ItemComponentTrait::StaticClass()))
		{
			Components.Add(Cast<UIT_ItemComponentTrait>(CurrentTrait)->ItemComponent);
		}
	}

	return Components;
}

#if WITH_EDITOR

void UDA_CoreItem::OpenItemInstanceClass()
{
	if(!ItemInstance.IsNull())
	{
		FString AssetPath = ItemInstance.ToSoftObjectPath().ToString();
		FString RightSplit;
		UKismetStringLibrary::Split(AssetPath, ".", AssetPath, RightSplit);
		UObject* ObjectAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
		if(!ObjectAsset)
		{
			return;
		}
		// Open the asset for editing
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ObjectAsset);
	}
}

void UDA_CoreItem::OpenItemActorClass()
{
	if(!ItemActor.IsNull())
	{
		UClass* ActorClass = ItemActor.LoadSynchronous();
		FString AssetPath = ActorClass->GetClassPathName().ToString();
		FString RightSplit;
		UKismetStringLibrary::Split(AssetPath, ".", AssetPath, RightSplit);
		UObject* ObjectAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
		if(!ObjectAsset)
		{
			return;
		}
		// Open the asset for editing
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ObjectAsset);
	}
}

EDataValidationResult UDA_CoreItem::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if(DeveloperImage.IsNull() && InventoryImage.IsNull())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("No developer image, inventory image or generated item icon is being used. No valid thumbnail for the asset found"));
	}

	if(UseGeneratedItemIcon && ItemActor.IsNull())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("Using generated item icon, but no item actor has been assigned. Item will fall back to a default error texture."));
	}

	if(!UseGeneratedItemIcon && InventoryImage.IsNull())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("Not using generated item icon and no inventory image has been assigned. Item will fall back to a default error texture."));
	}

	if(const_cast<UDA_CoreItem*>(this)->CanItemStack() && const_cast<UDA_CoreItem*>(this)->GetDefaultContainers().IsValidIndex(0))
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("Item can stack and has default containers. IFP does not support this behavior."));
	}
	
	if(IsValid(ValidationClass))
	{
		UO_ItemAssetValidation* ValidationObject = Cast<UO_ItemAssetValidation>(ValidationClass.GetDefaultObject());
		TArray<FString> ErrorMessages;
		if(!ValidationObject->VerifyData(this, ErrorMessages))
		{
			if(ErrorMessages.IsValidIndex(0))
			{
				Result = EDataValidationResult::Invalid;
				for(auto& CurrentError : ErrorMessages)
				{
					Context.AddError(FText::FromString(CurrentError));
				}
			}
		}
	}
	
	return Result;
}

void UDA_CoreItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FString PropertyName = PropertyChangedEvent.GetPropertyName().ToString();

	if(PropertyName == GET_MEMBER_NAME_CHECKED(UDA_CoreItem, TraitsAndComponents))
	{
		if(TraitsAndComponents.IsValidIndex(PropertyChangedEvent.GetArrayIndex(PropertyChangedEvent.Property->GetFName().ToString())))
		{
			UItemTrait* EditingObject = TraitsAndComponents[PropertyChangedEvent.GetArrayIndex(PropertyChangedEvent.Property->GetFName().ToString())];

			if(IsValid(EditingObject))
			{
				bool BroadcastObjectAdded = true;
				if(!EditingObject->AllowMultipleCopiesInDataAsset())
				{
					//Item doesn't allow multiple copies of itself in the array.
					for(int32 OtherObjectIndex = 0; OtherObjectIndex < TraitsAndComponents.Num(); OtherObjectIndex++)
					{
						if(IsValid(TraitsAndComponents[OtherObjectIndex]))
						{
							if(EditingObject != TraitsAndComponents[OtherObjectIndex] && EditingObject->GetClass() == TraitsAndComponents[OtherObjectIndex]->GetClass())
							{
								BroadcastObjectAdded = false;
								//set the object back to null and send a error message.
								TraitsAndComponents[PropertyChangedEvent.GetArrayIndex(PropertyChangedEvent.Property->GetFName().ToString())] = nullptr;
								FText InfoText = FText::FromString("Trait does not allow multiple copies of itself");
								FNotificationInfo Info(InfoText);
								Info.bFireAndForget = true;
								Info.bUseThrobber = false;
								Info.FadeOutDuration = 0.5f;
								Info.ExpireDuration = 5.0f;
								if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
								{
									Notification->SetCompletionState(SNotificationItem::CS_Fail);
								}
							}
						}
					}
				}

				//Notify the object that it has been added to the item asset.
				if(PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
				{
					EditingObject->AddedToItemAsset(this);
					if(BroadcastObjectAdded)
					{
						TraitAdded.Broadcast(EditingObject, this);
					}
				}

				if(PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayRemove)
				{
					TraitRemoved.Broadcast(EditingObject, this);
				}
			}
		}
	}

	BakeShapes();
}

UTexture2D* UDA_CoreItem::GetThumbnailTexture()
{
	if(!InventoryImage.IsNull())
	{
		return InventoryImage.LoadSynchronous();
	}

	if(!DeveloperImage.IsNull())
	{
		return DeveloperImage.LoadSynchronous();
	}

	return nullptr;
}

void UDA_CoreItem::PostLoad()
{
	Super::PostLoad();

	if(Shapes.IsEmpty())
	{
		BakeShapes();
	}

	// 3.0 Transition code

	//Default tags and tag values
	if(DefaultTags.IsValid() || DefaultTagValues.IsValidIndex(0))
	{
		if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ItemStructFragments, true))
		{
			TagFragment->Tags = DefaultTags;
			TagFragment->TagValues = DefaultTagValues;
			DefaultTags = FGameplayTagContainer();
			DefaultTagValues.Empty();
			MarkPackageDirty();
		}
	}

	//Asset tags and tag values
	if(AssetTags.IsValid() || AssetTagValues.IsValidIndex(0))
	{
		if(FTagFragment* TagFragment = FindFragment<FTagFragment>(ItemAssetFragments, true))
		{
			TagFragment->Tags = AssetTags;
			TagFragment->TagValues = AssetTagValues;
			AssetTags = FGameplayTagContainer();
			AssetTagValues.Empty();
			MarkPackageDirty();
		}
	}

	/**Important note here: Since GetDefaultContainers returns a copy, we can't update it for IFP 3.
	 * Due to this, the GetContainerSettingsArray function will find the first property inside this
	 * data asset that is the type of TArray<FS_ContainerSettings> and return that. We could also
	 * get by name, but naming might not be consistent between projects.
	 *
	 * If you have multiple container setting arrays, you might have to convert it manually or
	 * update this code to include that.*/
	if(GetDefaultContainers().IsValidIndex(0))
	{
		if(TArray<FS_ContainerSettings>* ContainerSettings = UFL_InventoryFramework::GetContainerSettingsArray(this))
		{
			UFL_InventoryFramework::UpdateLegacyContainerSettingsCode(*ContainerSettings, this);
		}
	}
	
	// end of transition code
}

void UDA_CoreItem::BakeShapes()
{
	Shapes.Empty();

	TArray<FIntPoint> ReturnShape;
	
	const TArray<FIntPoint> DisabledTiles = GetDisabledTiles();
	
	for(int32 ColumnY = 0; ColumnY < ItemDimensions.Y; ColumnY++)
	{
		for(int32 RowX = 0; RowX < ItemDimensions.X; RowX++)
		{
			if(!DisabledTiles.Contains(FIntPoint(RowX, ColumnY)))
			{
				ReturnShape.Add(FIntPoint(RowX, ColumnY));
			}
		} 
	}
	
	Shapes.Add(FShapeRotation(Zero, ReturnShape));

	for(const ERotation CurrentRotation : TEnumRange<ERotation>())
	{
		if(CurrentRotation != Zero)
		{
			TArray<FIntPoint> TemporaryShape = Shapes[0].Shape;
			TemporaryShape = UFL_InventoryFramework::RotateShape(TemporaryShape, CurrentRotation, FIntPoint(0, 0));

			//Since the anchor is 0,0 we need to adjust the shape
			//so the top left is still 0,0
			FIntPoint ShapeAdjustment = FIntPoint(0, 0);
			switch (CurrentRotation)
			{
			case Ninety:
				ShapeAdjustment.X = ItemDimensions.Y - 1;
				break;
			case OneEighty:
				ShapeAdjustment.X = ItemDimensions.X - 1;
				ShapeAdjustment.Y = ItemDimensions.Y - 1;
				break;
			case TwoSeventy:
				ShapeAdjustment.Y = ItemDimensions.X - 1;
				break;
			default:
				break;
			}

			for(auto& CurrentTile : TemporaryShape)
			{
				CurrentTile.X += ShapeAdjustment.X;
				CurrentTile.Y += ShapeAdjustment.Y;
			}

			Shapes.Add(FShapeRotation(CurrentRotation, TemporaryShape));
		}
	}
}

#endif



