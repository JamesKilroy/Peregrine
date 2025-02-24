// Copyright 2023, Jan Kozlowski, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPMovementTemplateCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFPMovementComponent;

UCLASS()
class FIRSTPERSONMOVEMENTTEMPLATE_API AFPMovementTemplateCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AFPMovementTemplateCharacter();

protected:

	virtual void BeginPlay() override;

public:	

	virtual void Tick(float DeltaTime) override;

	//Input

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//Function overrides

	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta) override;

	//Components

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
		UCameraComponent* Camera;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
		USpringArmComponent* CameraSpringArm;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
		USceneComponent* CameraSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
		UFPMovementComponent* FPMovement;
};
