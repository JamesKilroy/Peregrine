// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "Core/Fragments/CF_CompatibilitySettings.h"
#include "Core/Data/IFP_CoreData.h"


class FDeferredCleanupSlateBrush;


class UAC_Inventory;
struct FS_CompatibilitySettings;

class FS_InventoryItem_Customization : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	// BEGIN IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	bool OnFilterAsset(const FAssetData& AssetData, TInstancedStruct<FCompatibilitySettingsFragment> CompatibilitySettings, UAC_Inventory* Inventory);

	TSharedPtr<IPropertyUtilities> PropUtils;
	float IconSize = 32;
	TSharedPtr<FDeferredCleanupSlateBrush> IconBrush;
	TWeakObjectPtr<UTexture2D> IconTexturePtr; 
	// END IPropertyTypeCustomization interface
};
