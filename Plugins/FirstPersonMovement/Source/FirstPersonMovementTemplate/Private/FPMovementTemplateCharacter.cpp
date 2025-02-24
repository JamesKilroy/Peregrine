// Copyright 2023, Jan Kozlowski, All rights reserved


#include "FPMovementTemplateCharacter.h"
#include "FPMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AFPMovementTemplateCharacter::AFPMovementTemplateCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Camera socket
	CameraSocket = CreateDefaultSubobject<USceneComponent>(TEXT("Camera Socket"));
	CameraSocket->SetupAttachment(RootComponent);
	CameraSocket->SetRelativeLocation({ 0.f, 0.f, 54.f });

	//Spring arm
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Spring Arm"));
	CameraSpringArm->SetupAttachment(CameraSocket);
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->TargetArmLength = 0.f;

	//Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);
	Camera->bUsePawnControlRotation = false;

	//First Person Movement Component
	FPMovement = CreateDefaultSubobject<UFPMovementComponent>(TEXT("FP Movement"));
	FPMovement->Initialize(this, CameraSocket, Camera);
}

void AFPMovementTemplateCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFPMovementTemplateCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

//Input

void AFPMovementTemplateCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if(FPMovement->JumpAction)
			EnhancedInputComponent->BindAction(FPMovement->JumpAction, ETriggerEvent::Started, FPMovement, &UFPMovementComponent::OnJump);

		if(FPMovement->DashAction)
			EnhancedInputComponent->BindAction(FPMovement->DashAction, ETriggerEvent::Started, FPMovement, &UFPMovementComponent::OnDash);
		
		if(FPMovement->MantleAction)
			EnhancedInputComponent->BindAction(FPMovement->MantleAction, ETriggerEvent::Triggered, FPMovement, &UFPMovementComponent::OnMantle);

		if(FPMovement->LookAction)
			EnhancedInputComponent->BindAction(FPMovement->LookAction, ETriggerEvent::Triggered, FPMovement, &UFPMovementComponent::OnLook);

		if(FPMovement->CrouchAction)
			EnhancedInputComponent->BindAction(FPMovement->CrouchAction, ETriggerEvent::Started, FPMovement, &UFPMovementComponent::OnCrouch);
		
		if (FPMovement->RunAction)
		{
			EnhancedInputComponent->BindAction(FPMovement->RunAction, ETriggerEvent::Started, FPMovement, &UFPMovementComponent::OnRunStart);
			EnhancedInputComponent->BindAction(FPMovement->RunAction, ETriggerEvent::Completed, FPMovement, &UFPMovementComponent::OnRunEnd);
		}
		
		if (FPMovement->MoveAction)
			EnhancedInputComponent->BindAction(FPMovement->MoveAction, ETriggerEvent::Triggered, FPMovement, &UFPMovementComponent::OnWalk);
	}
}

//Function overrides

void AFPMovementTemplateCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	FPMovement->OnLandedEvent(Hit);
}

void AFPMovementTemplateCharacter::OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta)
{
	FPMovement->OnWalkingOffLedgeEvent(PreviousFloorImpactNormal, PreviousFloorContactNormal, PreviousLocation, TimeDelta);
}