// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/Widget.h"
#include "Core/Data/IFP_CoreData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FL_CommonHelpers_IFP.generated.h"

class UInputAction;
/**
 * Simple function library to access some C++ only functions
 * to help with IFP integration or utilization.
 */
UCLASS()
class IFP_COMMONASSETS_API UFL_CommonHelpers_IFP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
#pragma region Tags
	
	UFUNCTION(Category = "IFP Common|Tags", BlueprintCallable, BlueprintPure)
	static FGameplayTagContainer GetTagsChildren(FGameplayTag Tag, bool Recursive);

	UFUNCTION(Category = "IFP Common|Tags", BlueprintCallable, BlueprintPure)
	static bool DoesTagHaveChildren(FGameplayTag Tag);

	/**Find out if @Tag is a child of @PotentialParent.
	 * If @DirecChild is true, then @Tag must be a direct child of @PotentialTag
	 * For example; returns true if @Tag is Report.CauseOfDeath.Stabbed and @PotentialParent
	 * is Report.CauseOfDeath.*/
	UFUNCTION(Category = "IFP Common|Tags", BlueprintCallable, BlueprintPure)
	static bool IsTagChildOfTag(FGameplayTag Tag, FGameplayTag PotentialParent, bool DirectChild);

	/** Returns direct parent GameplayTag of this GameplayTag, calling on x.y will return x */
	UFUNCTION(Category = "IFP Common|Tags", BlueprintCallable, BlueprintPure)
	static FGameplayTag GetParentTag(FGameplayTag Tag);

#pragma endregion

	
#pragma region UI
	
	UFUNCTION(Category = "IFP Common|UI", BlueprintCallable, BlueprintPure)
	static UWidget* GetFocusedWidget();

	UFUNCTION(Category = "IFP Common|UI", BlueprintCallable)
	static void SetEnableWorldRendering(bool Enable);

#pragma endregion

};
