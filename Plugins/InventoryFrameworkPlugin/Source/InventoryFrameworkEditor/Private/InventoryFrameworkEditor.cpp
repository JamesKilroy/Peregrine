// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#include "InventoryFrameworkEditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserContextMenu/ATA_AttachmentWidget.h"
#include "ContentBrowserContextMenu/ATA_ContainerWidget.h"
#include "ContentBrowserContextMenu/ATA_ItemTrait.h"
#include "ContentBrowserContextMenu/ATA_ItemComponent.h"
#include "ContentBrowserContextMenu/ATA_ItemComponentTrait.h"
#include "ContentBrowserContextMenu/ATA_ItemAsset.h"
#include "ContentBrowserContextMenu/ATA_ItemActor.h"
#include "ContentBrowserContextMenu/ATA_ItemAssetValidation.h"
#include "ContentBrowserContextMenu/ATA_ItemInstance.h"
#include "ContentBrowserContextMenu/ATA_ItemWidget.h"
#include "ContentBrowserContextMenu/ATA_LootPoolObject.h"
#include "ContentBrowserContextMenu/ATA_TagValueCalculation.h"
#include "ContentBrowserContextMenu/ATA_TileWidget.h"
#include "StructCustomization/FS_CompatibilitySettings_Customization.h"
#include "StructCustomization/FS_ContainerSettings_Customization.h"
#include "StructCustomization/FS_InventoryItem_Customization.h"
#include "StructCustomization/FS_TagValue_Customization.h"
#include "StructCustomization/FS_TileTag_Customization.h"
#include "Core/Data/IFP_CoreData.h"
#include "Interfaces/IPluginManager.h"
#include "Other/InventoryComponentDetailsCustomization.h"
#include "Other/ItemAssetActorFactory.h"
#include "Other/ItemAssetDetailsCustomization.h"
#include "Other/ItemAssetThumbnailRenderer.h"
#include "Styling/SlateStyleRegistry.h"
#include "Subsystems/PlacementSubsystem.h"
#include "Widgets/Text/SRichTextBlock.h"

#if WITH_EDITOR

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

#endif

#define LOCTEXT_NAMESPACE "FInventoryFrameworkPluginModule"
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSetInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleSetInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

void FInventoryFrameworkEditor::StartupModule()
{
	//--------------------
	// Content browser context menu
	
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	InventoryFrameworkCategory = AssetTools.RegisterAdvancedAssetCategory(FName("InventoryFrameworkCategory"), FText::FromString("Inventory Framework"));

	Internal_RegisterTypeActions<FATA_ItemAsset>("Item Asset");
	Internal_RegisterTypeActions<FATA_ItemActor>("Item Actor");
	Internal_RegisterTypeActions<FATA_ItemTrait>("Item Trait");
	Internal_RegisterTypeActions<FATA_ItemComponentTrait>("Item Component Trait");
	Internal_RegisterTypeActions<FATA_ItemComponent>("Item Component");
	Internal_RegisterTypeActions<FATA_TagValueCalculation>("Tag Value Calculator");
	Internal_RegisterTypeActions<FATA_ItemWidget>("Item Widget");
	Internal_RegisterTypeActions<FATA_ContainerWidget>("Container Widget");
	Internal_RegisterTypeActions<FATA_TileWidget>("Tile Widget");
	Internal_RegisterTypeActions<FATA_AttachmentWidget>("Attachment Widget");
	Internal_RegisterTypeActions<FATA_ItemAssetValidation>("Item Asset Validation");
	Internal_RegisterTypeActions<FATA_ItemInstance>("Item Instance");
	Internal_RegisterTypeActions<FATA_LootPoolObject>("Loot Pool");

	//--------------------

	
	//--------------------
	// Struct customization
	
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	
	//Register FS_InventoryItem customization
	PropertyEditorModule.RegisterCustomPropertyTypeLayout( FS_InventoryItem::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FS_InventoryItem_Customization::MakeInstance));

	//Register FS_TagValue customization
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FS_TagValue::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FS_TagValue_Customization::MakeInstance));

	//Register FS_TileTag customization
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FS_TileTag::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FS_TileTag_Customization::MakeInstance));

	//Register FS_ContainerSettings customization
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FS_ContainerSettings::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FS_ContainerSettings_Customization::MakeInstance));

	//Register FS_CompatibilitySettings customization
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FS_CompatibilitySettings::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FS_CompatibilitySettings_Customization::MakeInstance));
	
	PropertyEditorModule.NotifyCustomizationModuleChanged();

	//--------------------


	//--------------------
	// Asset customization

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("DA_CoreItem", FOnGetDetailCustomizationInstance::CreateStatic(&UItemAssetDetailsCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout("AC_Inventory", FOnGetDetailCustomizationInstance::CreateStatic(&UInventoryComponentDetailsCustomization::MakeInstance));
	
	//--------------------


	//--------------------
	// Actor factories

	/**V: There's a small *gotcha* that the editor module has in IFP.
	 * Because the config references blueprints, it means that when the editor module is loaded,
	 * it'll load those blueprints. And since Blueprints will load all hard references, they can
	 * load blueprints that reference other plugins that haven't been loaded in yet, breaking them.
	 *
	 * Since the placement subsystem loads before the IFP editor module, we have to manually register
	 * the actor factories with the placement subsystem.
	 * This isn't super important, but might be important to note for some people who might be
	 * editing the editor module and need to change the loading order.
	 *
	 * The true fix would be to remove the blueprint references, then this module can have any
	 * loading phase that you want. But for a marketplace asset where I want everything to be
	 * "auto-magical" for 99% of people, I'm leaving this "bug" in this module on purpose.*/
	UItemAssetActorFactory* ItemAssetActorFactory = NewObject<UItemAssetActorFactory>();
	GEditor->ActorFactories.Add(ItemAssetActorFactory);
	if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
	{
		PlacementSubsystem->RegisterAssetFactory(ItemAssetActorFactory);
	}

	//--------------------


	//--------------------
	// Thumbnail renderers

	UThumbnailManager::Get().RegisterCustomRenderer(UDA_CoreItem::StaticClass(), UItemAssetThumbnailRenderer::StaticClass());
	
	//--------------------
	
	RegisterStyleSet();

	CreateWelcomePopup();
}

void FInventoryFrameworkEditor::ShutdownModule()
{
	//--------------------
	// Content browser context menu

	const FAssetToolsModule* AssetToolsModulePtr = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
	if(AssetToolsModulePtr)
	{
		IAssetTools& AssetTools = AssetToolsModulePtr->Get();
		for(const auto& CurrentAction : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(CurrentAction);
		}
	}
	
	//--------------------
	
	
	//--------------------
	// Struct customization
	
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		// unregister properties when the module is shutdown
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		
		PropertyModule.UnregisterCustomPropertyTypeLayout("FS_InventoryItem");
		
		PropertyModule.UnregisterCustomPropertyTypeLayout("FS_TagValue");
		
		PropertyModule.UnregisterCustomPropertyTypeLayout("FS_TileTag");

		PropertyModule.UnregisterCustomPropertyTypeLayout("FS_ContainerSettings");

		PropertyModule.UnregisterCustomPropertyTypeLayout("FS_CompatibilitySettings");
	
		PropertyModule.NotifyCustomizationModuleChanged();
	}
	
	//--------------------

	FModuleManager::Get().OnModulesChanged().RemoveAll(this);

	// Unregister the style set and reset the pointer
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSetInstance.Get());
	StyleSetInstance.Reset();
}

void FInventoryFrameworkEditor::RegisterStyleSet()
{
	StyleSetInstance = MakeShareable(new FSlateStyleSet("IFPStyleSet"));
	
	StyleSetInstance->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("InventoryFrameworkPlugin"))->GetBaseDir() / TEXT("Resources"));

	//Inventory component
	StyleSetInstance->Set("ClassIcon.AC_Inventory", new IMAGE_BRUSH(TEXT("InventoryComponent_16x"), FVector2D(16)));
	
	//Item Asset
	StyleSetInstance->Set("ClassIcon.DA_CoreItem", new IMAGE_BRUSH_SVG( "ItemAssetSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.DA_CoreItem", new IMAGE_BRUSH_SVG( "ItemAssetSlate", FVector2D(64)));

	//Item trait
	StyleSetInstance->Set("ClassIcon.ItemTrait", new IMAGE_BRUSH_SVG( "ItemTraitSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.ItemTrait", new IMAGE_BRUSH_SVG( "ItemTraitSlate", FVector2D(64)));

	//Item component
	StyleSetInstance->Set("ClassIcon.ItemComponent", new IMAGE_BRUSH_SVG( "ItemComponentSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.ItemComponent", new IMAGE_BRUSH_SVG( "ItemComponentSlate", FVector2D(64)));
	
	//Item Asset Validation
	StyleSetInstance->Set("ClassIcon.O_ItemAssetValidation", new IMAGE_BRUSH_SVG( "ItemValidatorSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_ItemAssetValidation", new IMAGE_BRUSH_SVG( "ItemValidatorSlate", FVector2D(64)));

	//Item Instance
	StyleSetInstance->Set("ClassIcon.ItemInstance", new IMAGE_BRUSH_SVG( "ItemInstanceSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.ItemInstance", new IMAGE_BRUSH_SVG( "ItemInstanceSlate", FVector2D(64)));

	//Item Actor
	StyleSetInstance->Set("ClassIcon.A_ItemActor", new IMAGE_BRUSH_SVG( "ItemActorSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.A_ItemActor", new IMAGE_BRUSH_SVG( "ItemActorSlate", FVector2D(64)));

	/////////////////
	
	//Recipe Asset
	StyleSetInstance->Set("ClassIcon.DA_CoreCraftingRecipe", new IMAGE_BRUSH_SVG( "CraftingRecipeAssetSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.DA_CoreCraftingRecipe", new IMAGE_BRUSH_SVG( "CraftingRecipeAssetSlate", FVector2D(64)));
		
	//Craft Event
	StyleSetInstance->Set("ClassIcon.O_CoreCraftEvent", new IMAGE_BRUSH_SVG( "CraftEventSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_CoreCraftEvent", new IMAGE_BRUSH_SVG( "CraftEventSlate", FVector2D(64)));
	
	//Craft Data
	StyleSetInstance->Set("ClassIcon.O_CoreRecipeData", new IMAGE_BRUSH_SVG( "CraftDataSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_CoreRecipeData", new IMAGE_BRUSH_SVG( "CraftDataSlate", FVector2D(64)));

	//Craft Requirement
	StyleSetInstance->Set("ClassIcon.O_CoreRecipeRequirement", new IMAGE_BRUSH_SVG( "CraftRequirementSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_CoreRecipeRequirement", new IMAGE_BRUSH_SVG( "CraftRequirementSlate", FVector2D(64)));

	//Craft Visibility Requirement
	StyleSetInstance->Set("ClassIcon.O_CoreRecipeDisplay", new IMAGE_BRUSH_SVG( "CraftVisibilityRequirementSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_CoreRecipeDisplay", new IMAGE_BRUSH_SVG( "CraftVisibilityRequirementSlate", FVector2D(64)));

	///////////////////
	
	//Loot Pool
	StyleSetInstance->Set("ClassIcon.O_LootPool", new IMAGE_BRUSH_SVG( "LootPoolSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_LootPool", new IMAGE_BRUSH_SVG( "LootPoolSlate", FVector2D(64)));

	//Tag Value Calculator
	StyleSetInstance->Set("ClassIcon.O_TagValueCalculation", new IMAGE_BRUSH_SVG( "TagValueCalculatorSlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_TagValueCalculation", new IMAGE_BRUSH_SVG( "TagValueCalculatorSlate", FVector2D(64)));

	//Item Query
	StyleSetInstance->Set("ClassIcon.O_ItemQueryBase", new IMAGE_BRUSH_SVG( "ItemQuerySlate", FVector2D(16)));
	StyleSetInstance->Set("ClassThumbnail.O_ItemQueryBase", new IMAGE_BRUSH_SVG( "ItemQuerySlate", FVector2D(64)));
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
}

void FInventoryFrameworkEditor::CreateWelcomePopup()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("InventoryFrameworkPlugin"));
	
	if(!Plugin.IsValid())
	{
	 return;
	}

	/**Get the current version so the popup appears for each update.
	 * But remove the last number, as that is just for bug fixes.
	 * My versioning is MajorVersion.MinorVersion.BugFixes (e.g., 1.4.2)*/
	FString LastSeenVersion;
	FString CurrentVersion = Plugin->GetDescriptor().VersionName;
	FString PluginName = Plugin->GetName();
	CurrentVersion.Split(".", &CurrentVersion, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	/**Time to check if the user has seen the popup window for this version.
	 * We use the GGameUserSettingsIni file to save these values, because they are per-user.
	 * If another team member installs the plugin, they will now also receive the popup.
	 * If this is saved in other configs that are outside of the "Saved" folder, then
	 * source control will pick it up, meaning that once one person views the popup,
	 * then syncs their config, the popup will not show for other team members.*/
	bool bHasSeenWelcomePopup = false;
	GConfig->GetBool(*PluginName, TEXT("bHasSeenWelcomePopup"), bHasSeenWelcomePopup, GEditorPerProjectIni);
	GConfig->GetString(*PluginName, TEXT("LastSeenVersion"), LastSeenVersion, GEditorPerProjectIni);
	
	if(bHasSeenWelcomePopup && LastSeenVersion == CurrentVersion)
	{
		return;
	}
	
	GConfig->SetBool(*PluginName, TEXT("bHasSeenWelcomePopup"), true, GEditorPerProjectIni);
	GConfig->SetString(*PluginName, TEXT("LastSeenVersion"), *CurrentVersion, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	
	FLinearColor BackgroundColor = FLinearColor(FColor::FromHex(TEXT("191a1cFF")));
	FLinearColor ForegroundColor = FLinearColor(FColor::FromHex(TEXT("232427FF")));
	FLinearColor StripColor = FLinearColor(FColor::FromHex(TEXT("5b5b5bFF")));
	TSharedRef<SWindow> WelcomeWindow = SNew(SWindow)
		.ClientSize(FVector2D(500, 400))
		.SizingRule(ESizingRule::FixedSize)
		.CreateTitleBar(true)
		.SupportsMaximize(false)
		.IsTopmostWindow(true);
	
	WelcomeWindow->SetContent(
	//Background color
	SNew(SBorder)
	.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")) 
	.BorderBackgroundColor(FSlateColor(BackgroundColor))
	.Padding(4, 0, 4, 4)
	[
		//Foreground
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")) 
		.BorderBackgroundColor(FSlateColor(ForegroundColor))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(7)
			[
				//Title
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28))
				.Text(FText::FromString(Plugin->GetFriendlyName()))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(10, 10, 10, 0))
			.HAlign(HAlign_Fill)
			[
				//Separator
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(StripColor)
				.DesiredSizeOverride(FVector2D(5))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Fill)
			.Padding(10, 0, 10, 10)
			[
				//Scroll box in case text is too long
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				.VAlign(VAlign_Top)
				.Padding(10, 10, 10, 10)
				[
					//The rich text that contains hyperlinks to URL's
					SNew(SRichTextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(FString("Hello, and welcome to " + Plugin->GetFriendlyName() + "!\n\n") + R"(
Thank you so much for using my Inventory Framework <3
You might want to review the <a id="browser" link="https://varian.gitbook.io/varian-docs/inventoryframework">documentation</>.
At the top left of the page, you'll find a dropdown for all of my other plugins, some of which are free.

- <a id="browser" link="https://discord.gg/JGcwGpEf">Discord</>
- <a id="browser" link="https://varian.gitbook.io/varian-docs/inventoryframework/inventory-framework/versions">Changelog</>
- <a id="browser" link="https://www.fab.com/sellers/Varian%20Daemon">Marketplace</>
- <a id="browser" link="https://www.youtube.com/@VarianDaemon_">Youtube</>)"))
					
				.TextStyle(FAppStyle::Get(), "NormalText")
				.DecoratorStyleSet(&FAppStyle::Get())
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("link"));
				
					if(URL)
					{
						FPlatformProcess::LaunchURL(**URL, nullptr, nullptr);
					}
				}))
				]
			]
			+ SVerticalBox::Slot()
			.Padding(3, 20, 3, 3)
			.AutoHeight()
			[
				//Version
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("SmallText.Subdued", FCoreStyle::SmallTextSize))
				.Text(FText::FromString("- Version " + Plugin->GetDescriptor().VersionName))
			]
		]
	]);

	FSlateApplication::Get().AddWindow(WelcomeWindow);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventoryFrameworkEditor, InventoryFrameworkEditor)