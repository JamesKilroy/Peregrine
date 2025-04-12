// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GSCModularPlayerStateCharacter.generated.h"


class UGSCAbilitySystemComponent;

/**
 * Minimal class that supports extension by game feature plugins.
 *
 * Intended to be used for ACharacters using AbilitySystemComponent living on PlayerState.
 */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularPlayerStateCharacter : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AGSCModularPlayerStateCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Cached AbilitySystemComponent. Real owner is PlayerState, but pointer gets updated to use PlayerState's here in PossessedBy / OnRep_PlayerState
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin AActor Interface
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin APawn Interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	//~ End APawn Interface

	//~ Begin IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag InTagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const override;
	//~ End IGameplayTagAssetInterface

	virtual void SetBase(UPrimitiveComponent* NewBase, const FName BoneName, const bool bNotifyActor) override
	{
		if (NewBase)
		{
			// LoadClass to not depend on the voxel module
			static UClass* const VoxelWorldClass = LoadClass<UObject>(nullptr, TEXT("/Script/Voxel.VoxelWorld"));

			const AActor* BaseOwner = NewBase->GetOwner();
			if (ensure(VoxelWorldClass) &&
				BaseOwner &&
				BaseOwner->IsA(VoxelWorldClass))
			{
				NewBase = Cast<UPrimitiveComponent>(BaseOwner->GetRootComponent());
				ensure(NewBase);
			}
		}

		Super::SetBase(NewBase, BoneName, bNotifyActor);
	}
};
