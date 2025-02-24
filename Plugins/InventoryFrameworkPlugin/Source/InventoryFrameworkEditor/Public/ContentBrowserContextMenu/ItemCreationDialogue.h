#pragma once

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Core/Items/DA_CoreItem.h"
#include "Factories/ItemAssetFactory.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "InventoryFrameworkItemCreationDialogue"

class SItemCreationDialogue : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SItemCreationDialogue){}

	SLATE_END_ARGS()

		/** Constructs this widget with InArgs */
		void Construct(const FArguments& InArgs)
	{
		bOkClicked = false;
		ParentClass = UDA_CoreItem::StaticClass();

		ChildSlot
			[
				SNew(SBorder)
				.Visibility(EVisibility::Visible)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SNew(SBox)
					.Visibility(EVisibility::Visible)
					.WidthOverride(500.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							.Content()
							[
								SAssignNew(ParentClassContainer, SVerticalBox)
							]
						]

						// Ok/Cancel buttons
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Bottom)
							.Padding(8)
							[
								SNew(SUniformGridPanel)
								.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
								.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
								.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
								+ SUniformGridPanel::Slot(0, 0)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
									.OnClicked(this, &SItemCreationDialogue::OkClicked)
									.Text(LOCTEXT("CreateGameplayAbilityBlueprintOk", "OK"))
								]
								+ SUniformGridPanel::Slot(1, 0)
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
										.OnClicked(this, &SItemCreationDialogue::CancelClicked)
										.Text(LOCTEXT("CreateGameplayAbilityBlueprintCancel", "Cancel"))
									]
							]
					]
				]
			];

		MakeParentClassPicker();
	}
	
	/** Sets properties for the supplied GameplayAbilitiesBlueprintFactory */
	bool ConfigureProperties(TWeakObjectPtr<UItemFactory> InFactory)
	{
		ItemFactory = InFactory;

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("CreateItemsOptions", "Create Item Data Asset"))
			.ClientSize(FVector2D(400, 700))
			.SupportsMinimize(false).SupportsMaximize(false)
			[
				AsShared()
			];

		PickerWindow = Window;

		GEditor->EditorAddModalWindow(Window);
		ItemFactory.Reset();

		return bOkClicked;
	}

private:
	class FGameplayAbilityBlueprintParentFilter : public IClassViewerFilter
	{
	public:
		/** All children of these classes will be included unless filtered out by another setting. */
		TSet< const UClass* > AllowedChildrenOfClasses;

		FGameplayAbilityBlueprintParentFilter() {}

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
		}
	};

	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker()
	{
		// Load the classviewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		// Fill in options
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;

		// Only allow parenting to base blueprints.
		Options.bIsBlueprintBaseOnly = true;

		TSharedPtr<FGameplayAbilityBlueprintParentFilter> Filter = MakeShareable(new FGameplayAbilityBlueprintParentFilter);

		// All child child classes of UGameplayAbility are valid.
		Filter->AllowedChildrenOfClasses.Add(UDA_CoreItem::StaticClass());
		Options.ClassFilters.Add(Filter.ToSharedRef());

		ParentClassContainer->ClearChildren();
		ParentClassContainer->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ParentClass", "Parent Class:"))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
			];

		ParentClassContainer->AddSlot()
			[
				ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SItemCreationDialogue::OnClassPicked))
			];
	}

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass)
	{
		ParentClass = ChosenClass;
	}

	/** Handler for when ok is clicked */
	FReply OkClicked()
	{
		if (ItemFactory.IsValid())
		{
			ItemFactory->DataAssetClass = ParentClass.Get();
		}

		CloseDialog(true);

		return FReply::Handled();
	}

	void CloseDialog(bool bWasPicked = false)
	{
		bOkClicked = bWasPicked;
		if (PickerWindow.IsValid())
		{
			PickerWindow.Pin()->RequestDestroyWindow();
		}
	}

	/** Handler for when cancel is clicked */
	FReply CancelClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}
	
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<UItemFactory> ItemFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> ParentClass;

	/** True if Ok was clicked */
	bool bOkClicked;
};

#undef LOCTEXT_NAMESPACE