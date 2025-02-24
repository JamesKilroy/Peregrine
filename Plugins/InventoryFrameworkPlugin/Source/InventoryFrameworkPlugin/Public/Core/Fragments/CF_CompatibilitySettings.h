// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CF_CompatibilitySettings.generated.h"

/**Allows the container to filter what is allowed and
 * what is not allowed inside of it.*/
USTRUCT(BlueprintType, DisplayName = "🔒 Compatibility Settings")
struct FCompatibilitySettingsFragment : public FContainerFragment
{
	GENERATED_BODY()

	/**What tags must an item have for it to be allowed in this container?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer RequiredTags;

	/**If an item has any of these tags, they are not allowed in this container.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer BlockingTags;

	/**What item types are allowed? These are declared in the item asset.
	 * This will also check children tags, so adding Item.Type.Weapon
	 * will allow any item with the type tag Item.Type.Weapon.Gun and
	 * any other children tags of the tags you add here.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FGameplayTagContainer ItemTypes;

	//Only these items are allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	TArray<TSoftObjectPtr<UDA_CoreItem>> ItemWhitelist;

	//These items are not allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	TArray<TSoftObjectPtr<UDA_CoreItem>> ItemBlacklist;
};

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UCF_CompatibilitySettings : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "IFP|Fragments|Compatibility Settings", BlueprintCallable)
	static FCompatibilitySettingsFragment GetCompatibilitySettingsFragmentFromContainer(FS_ContainerSettings Container);

	//Attempt to convert @Struct to TagFragment
	UFUNCTION(Category = "IFP|Fragments|Compatibility Settings", BlueprintCallable, BlueprintPure)
	static FCompatibilitySettingsFragment ToCompatibilitySettingsFragment(FInstancedStruct Struct);
};
