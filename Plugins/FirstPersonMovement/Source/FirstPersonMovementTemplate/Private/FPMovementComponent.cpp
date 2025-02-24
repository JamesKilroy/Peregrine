// Copyright 2023, Jan Kozlowski, All rights reserved

#include "FPMovementComponent.h"
#include "Engine/World.h"
#include "Components/TimelineComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "TimelineManager.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Vector.h"

UFPMovementComponent::UFPMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UFPMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	SetMovementValues();
	AddMovementInputContext();
}

void UFPMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CrouchUpdate();
	SlidingUpdate();
	WallRunUpdate();
	CameraTiltUpdate();

	PreviousFrameLocation = CharacterOwner->GetActorLocation();

	if (EnhancedInputComponent && MoveAction)
		MoveInputVector = EnhancedInputComponent->GetBoundActionValue(MoveAction).Get<FVector2D>();
}

//Input

void UFPMovementComponent::AddMovementInputContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(CharacterOwner->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MovementInputContext, 0);
		}
	}

	//Bind action value to be able to access it later (we do it here, because SetupPlayerInputComponent function is not accessible in blueprint)
	EnhancedInputComponent = CharacterOwner->FindComponentByClass<UEnhancedInputComponent>();
	if (EnhancedInputComponent && MoveAction)
		EnhancedInputComponent->BindActionValue(MoveAction);

}

//Jumping

void UFPMovementComponent::OnJump(const FInputActionValue& Value)
{
	if (CurrentMovementMode == EFPMovementMode::Crouching || CurrentMovementMode == EFPMovementMode::Sliding)
	{
		//Stop sliding/crouching on jump input

		ExitMovementMode(CurrentMovementMode);

		if (CharacterOwner->GetCharacterMovement()->IsFalling())
			EnterMovementMode(EFPMovementMode::Falling);		//This is only triggered when player falls under obstacle and has to crouch in air
		else
			EnterMovementMode(EFPMovementMode::Walking);
	}
	else if (TrySwitchingMovementMode(EFPMovementMode::Jumping))
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Jumping);
		EnterMovementMode(EFPMovementMode::Falling);		//Immediately start falling

		if (JumpSound)
			UGameplayStatics::PlaySound2D(GetWorld(), JumpSound);
	}
}

void UFPMovementComponent::StartJumping()
{
	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = AirborneSpeed;

	if (PreviousMovementMode == EFPMovementMode::Wallrunning)
		WallRunJump();
	else
	{
		CurrentJumpAmount--;		//Decrease jump amount

		//Jump direction (vector pointing up)
		FVector JumpVector = CharacterOwner->GetActorUpVector() * CharacterOwner->GetCharacterMovement()->JumpZVelocity;
		CharacterOwner->LaunchCharacter(JumpVector, false, true);
	}
}

bool UFPMovementComponent::CanEnterJumping()
{
	return HasRoomToStandUp && CurrentJumpAmount > 0 &&
		CurrentMovementMode != EFPMovementMode::Mantling && CurrentMovementMode != EFPMovementMode::Dashing;	//Mantling and dashing disable jumping
}

//Dashing

void UFPMovementComponent::OnDash(const FInputActionValue& Value)
{
	if (TrySwitchingMovementMode(EFPMovementMode::Dashing))
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Dashing);

		//Effects

		if (DashSound)
			UGameplayStatics::PlaySound2D(GetWorld(), DashSound);

		if (DashingShake)
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraShake(DashingShake, 1.f);
	}
}

void UFPMovementComponent::StartDashing()
{
	CalculateDashingDirection();

	UseLandingEffects = false;		//Prevents from spamming effects
	CanEverWallRun = false;			//Do not wallrun while dashing

	TimelineManager->DashingMovementTimeline->PlayFromStart();		//Start dashing movement

	DashingStamina--;		//Reduce dashing stamina;

	//Set stamina recover timeline position based on current stamina amount

	float NewTime = DashingStamina / float(MaxDashAmount) * TimelineManager->DashStaminaRecoverTimeline->GetTimelineLength();
	TimelineManager->DashStaminaRecoverTimeline->SetNewTime(NewTime);

	TimelineManager->DashStaminaRecoverTimeline->Play();		//Start recovering dashing stamina
}

void UFPMovementComponent::StopDashing()
{
	TimelineManager->DashingMovementTimeline->Stop();

	CanEverWallRun = true;

	CharacterOwner->LaunchCharacter(DashingDirection * AirborneSpeed, true, true);		//Prevents the player from getting launched far away

	//Do not restore effects immediately
	FTimerHandle RestorationHandle;
	CharacterOwner->GetWorldTimerManager().SetTimer(RestorationHandle, this, &UFPMovementComponent::RestoreLandingEffects, LandingEffectsRestorationTime, false);
}

void UFPMovementComponent::DashingMovementFinished()
{
	ExitMovementMode(EFPMovementMode::Dashing);

	if (TrySwitchingMovementMode(EFPMovementMode::Falling))
		EnterMovementMode(EFPMovementMode::Falling);		//Always try to switch to falling after dashing
}

void UFPMovementComponent::CalculateDashingDirection()
{
	switch (DashingType)		//Select dashing direction based on dashing type (feel free to add your own)
	{

	case EDashingType::MovementBased:
	{
		DashingDirection = { MoveInputVector.X, MoveInputVector.Y, 0.f };		//Dash in movement input direction
		DashingDirection = CharacterOwner->GetActorRotation().RotateVector(DashingDirection).GetSafeNormal();		//You can also use GetLastInputVector function, but it won't always be accurate

		if (DashingDirection.Size() == 0.f)
			DashingDirection = CharacterOwner->GetActorForwardVector();		//If the player is not moving (entering input), dash forward anyway

		break;
	}

	case EDashingType::CameraBased:
	{
		DashingDirection = Camera->GetForwardVector();		//Dash in direction the camera if facing

		break;
	}

	default:
		break;

	}
}

void UFPMovementComponent::DashingMovementUpdate(float Alpha)
{
	CharacterOwner->GetCharacterMovement()->Velocity = DashingDirection * DashingSpeed * Alpha;		//Move in dashing direction

	if (!HasRoomToStandUp)		//If the player moves under obstacle that is to low to walk around under, stop dashing and start crouching
	{
		ExitMovementMode(EFPMovementMode::Dashing);
		EnterMovementMode(EFPMovementMode::Crouching);
	}
}

void UFPMovementComponent::DashStaminaRecoverUpdate(float Alpha)
{
	DashingStamina = FMath::Lerp(0.f, float(MaxDashAmount), Alpha);		//Recover dash stamina to max
}

bool UFPMovementComponent::CanEnterDashing()
{
	return DashingStamina >= 1.f && HasRoomToStandUp &&
		CurrentMovementMode != EFPMovementMode::Mantling;		//Mantling disables dashing
}

//Mantling

void UFPMovementComponent::OnMantle(const FInputActionValue& Value)
{
	if (TrySwitchingMovementMode(EFPMovementMode::Mantling))
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Mantling);

		if (MantlingShake)
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraShake(MantlingShake, 1.f);
	}
}

void UFPMovementComponent::StartMantling()
{
	//Attach target actors to the object the player is trying to mantle on 
	//(this is only useful when the object is moving, so we can't just store location as vectors)

	//MantleUpTargetActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	MantleUpTargetActor->SetActorLocation(MantleHitResult.TraceStart);
	MantleUpTargetActor->AttachToComponent(MantleHitComponent, FAttachmentTransformRules::KeepWorldTransform);

	MantleForwardTargetActor->SetActorLocation(MantleHitResult.TraceEnd);
	MantleForwardTargetActor->AttachToComponent(MantleHitComponent, FAttachmentTransformRules::KeepWorldTransform);

	MantleBeginPointTargetActor->SetActorLocation(CharacterOwner->GetActorLocation());
	MantleBeginPointTargetActor->AttachToComponent(MantleHitComponent, FAttachmentTransformRules::KeepWorldTransform);

	MantleTargetActor = MantleUpTargetActor;		//Mantling is divided into two parts - first up then forward

	UseLandingEffects = false;		//Prevents from spamming effects
	CanEverWallRun = false;			//Mantling overrides wallrun

	TimelineManager->MantleUpTimeline->PlayFromStart();		//Start moving up
}

void UFPMovementComponent::StopMantling()
{
	FVector LaunchVector = { 0.f, 0.f, CharacterOwner->GetCharacterMovement()->GetGravityZ() };
	CharacterOwner->LaunchCharacter(LaunchVector, true, true);								//Makes sure the player sticks to the ground

	CanEverWallRun = true;

	//Do not restore effects immediately
	FTimerHandle RestorationHandle;
	CharacterOwner->GetWorldTimerManager().SetTimer(RestorationHandle, this, &UFPMovementComponent::RestoreLandingEffects, LandingEffectsRestorationTime, false);
}

void UFPMovementComponent::MantleUpFinished()
{
	AActor* StoredActorReference = MantleBeginPointTargetActor;		//Store reference to begin point actor

	//Set targets for moving forward
	MantleBeginPointTargetActor = MantleUpTargetActor;
	MantleTargetActor = MantleForwardTargetActor;

	//Makes sure we don't lose the reference to begin point actor, so we can use it next time we mantle
	MantleUpTargetActor = StoredActorReference;

	TimelineManager->MantleForwardTimeline->PlayFromStart();
}

void UFPMovementComponent::MantleForwardFinished()
{
	ExitMovementMode(EFPMovementMode::Mantling);
	EnterMovementMode(EFPMovementMode::Falling);		//Always start falling after mantling
}

void UFPMovementComponent::MantleMovementUpdate(float Alpha)
{
	switch (MantlingType)		//Select mantling movement based on mantling type
	{

	case EMantlingType::SetVelocity:		//Set velocity - less accurate, but doesn't move the player through objects
	{
		//Mantling direction
		FVector MantlingDirection = MantleTargetActor->GetActorLocation() - MantleBeginPointTargetActor->GetActorLocation();
		MantlingDirection = MantlingDirection.GetSafeNormal();

		CharacterOwner->GetCharacterMovement()->Velocity = MantlingDirection * MantleSpeed;

		break;
	}

	case EMantlingType::SetLocation:		//Set location - more accurate, but can move through objects
	{
		FVector NewLocation = FMath::Lerp(MantleBeginPointTargetActor->GetActorLocation(), MantleTargetActor->GetActorLocation(), Alpha);

		CharacterOwner->SetActorLocation(NewLocation);

		break;
	}

	default:
		break;

	}
}

bool UFPMovementComponent::MantleCheck()
{
	FCollisionShape Capsule = FCollisionShape::MakeCapsule
	(CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), DefaultCapsuleHalfHeight);
	FCollisionQueryParams MantleParams;
	MantleParams.AddIgnoredActor(CharacterOwner);

	FVector Start, End;

	//Check for forward wall
	{
		//Check slighly higher than player's current location

		Start = CharacterOwner->GetActorLocation();
		Start.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		//Trace forward

		End = Start + (CharacterOwner->GetActorForwardVector() * MantleTraceForwardDistance);

		GetWorld()->SweepSingleByChannel(MantleHitResult, Start, End, FQuat::Identity, ECC_Visibility, Capsule, MantleParams);
	}

	//Return false if doesn't find a wall
	if (!MantleHitResult.bBlockingHit ||
		//Do not mantle on object simulating physics or tagged as unmountable
		MantleHitResult.GetActor()->ActorHasTag("Unmountable") || MantleHitResult.GetComponent()->IsSimulatingPhysics())
		return false;

	//Get the floor normal of the object the player wants to mantle on
	{
		//Trace from up to down to get the highest possible location

		Start = MantleHitResult.ImpactPoint;
		Start.Z += MantleHeightTraceLength;
		End = MantleHitResult.ImpactPoint;

		GetWorld()->SweepSingleByChannel(MantleHitResult, Start, End, FQuat::Identity, ECC_Visibility, Capsule, MantleParams);
	}

	//If not hit or the object is unwalkable, return false
	if (!CharacterOwner->GetCharacterMovement()->IsWalkable(MantleHitResult) || !MantleHitResult.bBlockingHit)
		return false;

	//Check for available space
	{
		MantleHitComponent = MantleHitResult.GetComponent();		//Save hit component

		//Trace start location based on the player's location and wall height

		Start = CharacterOwner->GetActorLocation();
		Start.Z = MantleHitResult.Location.Z + MantleLocationZOffset;

		End = FVector::CrossProduct(CharacterOwner->GetActorRightVector(), MantleHitResult.Normal) * MantleDepth;
		End = Start + End;		//Vector pointing towards the object's edge

		//If it hits something, there is no available space
		return !GetWorld()->SweepSingleByChannel(MantleHitResult, Start, End, FQuat::Identity, ECC_Visibility, Capsule, MantleParams);
	}
}

bool UFPMovementComponent::CanEnterMantling()
{
	return CharacterOwner->GetCharacterMovement()->IsFalling() &&		//Mantle only when in air
		CurrentMovementMode != EFPMovementMode::Mantling &&				//Mantle only once at a time
		MantleCheck();
}

//Wallrunning

void UFPMovementComponent::StartWallRun()
{
	//Save last wall and set current wall

	PreviousWall = CurrentWall;
	CurrentWall = WallRunHitResult.GetActor();

	TimelineManager->WallRunGravityTimeline->PlayFromStart();		//Disable the gravity for a while so the player doesn't fall off immediately

	CurrentJumpAmount = MaxJumpAmount;								//Reset jump amount

	if (ShouldSpawnWallParticle() && WallRunParticle)				//Spawn particle on the wall if possible
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WallRunParticle,
			WallRunHitResult.ImpactPoint, UKismetMathLibrary::MakeRotFromZ(WallRunHitResult.ImpactNormal), FVector::OneVector, true, true);
}

void UFPMovementComponent::StopWallRun()
{
	//Reset gravity to normal

	TimelineManager->WallRunGravityTimeline->Stop();
	CharacterOwner->GetCharacterMovement()->GravityScale = InitialGravityScale;
}

void UFPMovementComponent::WallRunUpdate()
{
	if (CurrentMovementMode == EFPMovementMode::Wallrunning)		//If is wallrunning, perform wallrun movement, if not try to start wallrun
		WallRunMovement();
	else if (TrySwitchingMovementMode(EFPMovementMode::Wallrunning))
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Wallrunning);
	}
}

void UFPMovementComponent::WallRunMovement()
{
	if (ShouldStopWallrunning())		//Stop wallrun if the conditions are no longer fullfilled
	{
		ExitMovementMode(EFPMovementMode::Wallrunning);
		EnterMovementMode(EFPMovementMode::Falling);
	}
	else
	{
		//Trace towards the wall the get wall normal

		FCollisionQueryParams WallRunParams;
		WallRunParams.AddIgnoredActor(CharacterOwner);

		FVector Start = CharacterOwner->GetActorLocation();
		FVector End = WallRunHitResult.Normal * -1.f * DistanceToWall;		//Vector pointing in the direction of the wall
		End = Start + End;


		if (GetWorld()->LineTraceSingleByChannel(WallRunHitResult, Start, End, ECC_Visibility, WallRunParams))
		{
			CharacterOwner->GetCharacterMovement()->Velocity = CalculateWallRunVelocity();
		}
		else
		{
			//Stop wallrun if the trace doesn't hit the wall

			ExitMovementMode(EFPMovementMode::Wallrunning);
			EnterMovementMode(EFPMovementMode::Falling);
		}
	}
}

void UFPMovementComponent::WallRunJump()
{
	FVector JumpDirection = WallRunHitResult.Normal * WallRunJumpOffVelocity;
	JumpDirection.Z = WallRunJumpUpVelocity;

	CharacterOwner->LaunchCharacter(JumpDirection, false, false);

	CurrentJumpAmount--;		//Decrease jump amount

	CanEverWallRun = false;		//Prevents the player from immediately sticking back to wall

	FTimerHandle RecoverHandle;
	CharacterOwner->GetWorldTimerManager().SetTimer(RecoverHandle, this, &UFPMovementComponent::WallRunRecover, WallRunRecoverTime, false);
}

void UFPMovementComponent::WallRunRecover()
{
	CanEverWallRun = true;
}

void UFPMovementComponent::WallRunGravityUpdate(float Alpha)
{
	//Increase gravity over time

	CharacterOwner->GetCharacterMovement()->GravityScale = FMath::Lerp(0.f, InitialGravityScale, Alpha);
}

bool UFPMovementComponent::ShouldStopWallrunning()
{
	if (!WallRunHitResult.GetActor())
		return false;

	FVector Velocity2D = CharacterOwner->GetVelocity();
	Velocity2D.Z = 0.f;

	
	float Dot_RightVector_Velocity2D = FVector::DotProduct(CharacterOwner->GetActorRightVector(), Velocity2D.GetSafeNormal());
	float Dot_ForwardVector_Velocity = FVector::DotProduct((CharacterOwner->GetActorForwardVector() * MoveInputVector.X).GetSafeNormal(), Velocity2D.GetSafeNormal());

	return abs(Dot_RightVector_Velocity2D) >= MaxWallRunViewDotProduct ||
		(Dot_ForwardVector_Velocity < MaxWallRunMovementDotProduct&& MoveInputVector.X != 0.f) ||
		MoveInputVector.Y == WallDirection * -1.f ||
		(CharacterOwner->GetActorLocation().X == PreviousFrameLocation.X && CharacterOwner->GetActorLocation().Y == PreviousFrameLocation.Y) ||
		!WallRunHitResult.GetActor()->ActorHasTag("WallRunObject");
}

bool UFPMovementComponent::ShouldSpawnWallParticle()
{
	if (CurrentWall == PreviousWall)
		return PreviousMovementModeBeforeSwitch != EFPMovementMode::Wallrunning;

	return true;
}

bool UFPMovementComponent::CheckForWall()
{
	FCollisionQueryParams WallRunParams;
	WallRunParams.AddIgnoredActor(CharacterOwner);

	FVector Start = CharacterOwner->GetActorLocation();
	FVector End = CharacterOwner->GetActorRightVector() * DistanceToWall;

	//Check for wall on the right

	if (GetWorld()->LineTraceSingleByChannel(WallRunHitResult, Start, Start + End, ECC_Visibility, WallRunParams) &&
		WallRunHitResult.GetActor()->ActorHasTag("WallRunObject"))		//Remove this check to be able to wallrun on any object (not advised)
	{
		//Wall found on the right

		WallDirection = 1.f;
		return true;
	}


	//Check for wall on the left

	if (GetWorld()->LineTraceSingleByChannel(WallRunHitResult, Start, Start - End, ECC_Visibility, WallRunParams) &&
		WallRunHitResult.GetActor()->ActorHasTag("WallRunObject"))		//Remove this check to be able to wallrun on any object (not advised)
	{
		//Wall found on the left

		WallDirection = -1.f;
		return true;
	}

	//No valid wall found

	WallDirection = 0.f;
	return false;
}

bool UFPMovementComponent::CanEnterWallRun()
{
	bool VectorComparisonResult = (CharacterOwner->GetActorLocation().X != PreviousFrameLocation.X && CharacterOwner->GetActorLocation().Y != PreviousFrameLocation.Y);

	return CharacterOwner->GetCharacterMovement()->IsFalling() && CheckForWall() &&
		!ShouldStopWallrunning() &&			//Make sure the player is not going to immediately end wallrun
		CanEverWallRun &&
		VectorComparisonResult;				//If the player is being blocked by something, stop wallrun
}

FVector UFPMovementComponent::CalculateWallRunVelocity()
{
	//Vector pointing in the same direction as the wall

	FVector WallRunMovementDirection = FVector::CrossProduct(WallRunHitResult.GetActor()->GetActorUpVector(), WallRunHitResult.Normal);
	WallRunMovementDirection *= WallDirection * WallRunSpeed;

	WallRunMovementDirection.Z += CharacterOwner->GetCharacterMovement()->GetGravityZ() * UseGravityForWallRun;		//If enabled, add z velocity

	return WallRunMovementDirection;
}

//Camera movement

void UFPMovementComponent::OnLook(const FInputActionValue& Value)
{
	//Get input vector

	FVector2D InputVector2D = Value.Get<FVector2D>();
	float Yaw = InputVector2D.X;
	float Pitch = InputVector2D.Y;

	//Right/Left

	CharacterOwner->AddControllerYawInput(Yaw);

	//Up/Down

	CharacterOwner->AddControllerPitchInput(Pitch);
}

void UFPMovementComponent::CameraTilt(float Angle, float TiltSpeed)
{
	FRotator Target = CharacterOwner->GetControlRotation();
	Target.Roll = Angle;

	FRotator NewRotation = UKismetMathLibrary::RInterpTo(CharacterOwner->GetControlRotation(), Target, GetWorld()->GetDeltaSeconds(), TiltSpeed);

	CharacterOwner->GetController()->SetControlRotation(NewRotation);
}

void UFPMovementComponent::CameraBobbingUpdate(FVector NewLocation)
{
	//Gives the effect of smooth landing

	CameraSocket->SetRelativeLocation(InitialCameraLocation + NewLocation);
}

void UFPMovementComponent::CameraTiltUpdate()
{
	if (CurrentMovementMode == EFPMovementMode::Wallrunning)
		CameraTilt(WallRunAngle * WallDirection * -1.f, WallRunTiltSpeed);			//Tilt based on wallrunning
	else
		CameraTilt(StrafingTiltAngle * MoveInputVector.Y, StrafingTiltSpeed);		//Tilt based on strafing
}

//Crouching

void UFPMovementComponent::OnCrouch(const FInputActionValue& Value)
{
	if (CurrentMovementMode == EFPMovementMode::Crouching)		//If crouching, try standing up 
		ShouldStandUp = !ShouldStandUp;			//Triggering this twice makes it so that the players doesn't want to stand up
	else if (TrySwitchingMovementMode(EFPMovementMode::Sliding))		//If not crouching, slide if possible
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Sliding);
	}
	else if (TrySwitchingMovementMode(EFPMovementMode::Crouching))		//If cannot slide, crouch if possible
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Crouching);
	}
}

void UFPMovementComponent::StartCrouching()
{
	ShouldStandUp = false;		//Does not want to stand up

	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = CrouchingSpeed;

	TimelineManager->CrouchTimeline->Play();		//Scale the capsule component
}

void UFPMovementComponent::StopCrouching()
{
	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;

	ShouldStandUp = true;		//Wants to stand up

	TimelineManager->CrouchTimeline->Reverse();		//Resize the capsule component back to normal
}

void UFPMovementComponent::CrouchUpdate()
{
	//If the player wants is crouching and wants to stand up, do so if possible

	HasRoomToStandUp = CheckCrouchRoom();

	if (HasRoomToStandUp && CurrentMovementMode == EFPMovementMode::Crouching && TrySwitchingMovementMode(EFPMovementMode::Walking))
	{
		ExitMovementMode(CurrentMovementMode);

		if (CharacterOwner->GetCharacterMovement()->IsFalling())
			EnterMovementMode(EFPMovementMode::Falling);
		else
			EnterMovementMode(EFPMovementMode::Walking);
	}
}

void UFPMovementComponent::CrouchingTimelineUpdate(float Alpha)
{
	float HalfHeight = FMath::Lerp(DefaultCapsuleHalfHeight, CharacterOwner->GetCharacterMovement()->GetCrouchedHalfHeight(), Alpha);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(HalfHeight, true);

	//If capsule height is getting resized back to normal, but the player moves under an obstacle, start crouching back

	if (!HasRoomToStandUp && CurrentMovementMode != EFPMovementMode::Crouching && CurrentMovementMode != EFPMovementMode::Sliding)
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Crouching);
	}
}

bool UFPMovementComponent::CheckCrouchRoom()
{
	//Check if there is space between standing height location and ground

	//Actor's center location if the player was standing up
	FVector ActorCentre = CharacterOwner->GetActorLocation();
	ActorCentre.Z += DefaultCapsuleHalfHeight - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	//Player standing height with offset
	FVector Start = ActorCentre;
	Start.Z += DefaultCapsuleHalfHeight + CrouchingTraceHeightOffset;

	//Player's feet
	FVector End = ActorCentre;
	End.Z -= DefaultCapsuleHalfHeight;

	FCollisionQueryParams CrouchRoomCheckParams;
	CrouchRoomCheckParams.AddIgnoredActor(CharacterOwner);
	CrouchRoomCheckParams.AddIgnoredActor(CharacterOwner->GetCharacterMovement()->CurrentFloor.HitResult.GetActor());		//Ignore current floor

	//If nothing is hit, there is space

	FHitResult CrouchRoomHitResult;

	return !GetWorld()->LineTraceSingleByChannel(CrouchRoomHitResult, Start, End, ECC_Visibility, CrouchRoomCheckParams);
}

bool UFPMovementComponent::CanEnterCrouching()
{
	return !CharacterOwner->GetCharacterMovement()->IsFalling() &&		//Do not crouch in air
		//Dashing, mantling and wallrunning disable crouching
		CurrentMovementMode != EFPMovementMode::Dashing &&
		CurrentMovementMode != EFPMovementMode::Mantling &&
		CurrentMovementMode != EFPMovementMode::Wallrunning;
}

//Running

void UFPMovementComponent::OnRunStart(const FInputActionValue& Value)
{
	if (TrySwitchingMovementMode(EFPMovementMode::Running))
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Running);
	}
}

void UFPMovementComponent::OnRunEnd(const FInputActionValue& Value)
{
	//Stop running when input stops, but only do so when current movement mode is running

	if (CurrentMovementMode == EFPMovementMode::Running && TrySwitchingMovementMode(EFPMovementMode::Walking))
	{
		ExitMovementMode(EFPMovementMode::Running);
		EnterMovementMode(EFPMovementMode::Walking);
	}
}

void UFPMovementComponent::StartRunning()
{
	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;

	if (UseStaminaForRunning)		//If this is false, the player will be able to run forever
	{
		TimelineManager->RunningStaminaRecoverTimeline->Stop();		//Stop recovering stamina

		//Set stamina depletion timeline position based on current stamina amount

		float NewTime = TimelineManager->RunningStaminaDepletionTimeline->GetTimelineLength() * CurrentRunningStamina / MaxRunningStamina;
		TimelineManager->RunningStaminaDepletionTimeline->SetNewTime(NewTime);

		TimelineManager->RunningStaminaDepletionTimeline->Reverse();		//Start depleting stamina
	}
}

void UFPMovementComponent::StopRunning()
{
	if (UseStaminaForRunning)
	{
		TimelineManager->RunningStaminaDepletionTimeline->Stop();		//Stop depleting stamina

		//Set stamina recover timeline position based on current stamina amount

		float NewTime = TimelineManager->RunningStaminaRecoverTimeline->GetTimelineLength() * CurrentRunningStamina / MaxRunningStamina;
		TimelineManager->RunningStaminaRecoverTimeline->SetNewTime(NewTime);

		TimelineManager->RunningStaminaRecoverTimeline->Play();		//Start recovering stamina
	}
}

void UFPMovementComponent::RunningStaminaDepletionUpdate(float Alpha)
{
	//Deplete stamina

	CurrentRunningStamina = FMath::Lerp(MaxRunningStamina, 0.f, Alpha);

	if (!KeepRunning())
	{
		ExitMovementMode(EFPMovementMode::Running);

		if (HasRoomToStandUp)		//If has space, start walking, if not start crouching
			EnterMovementMode(EFPMovementMode::Walking);
		else
			EnterMovementMode(EFPMovementMode::Crouching);
	}
}

void UFPMovementComponent::RunningStaminaRecoverUpdate(float Alpha)
{
	//Recover stamina

	CurrentRunningStamina = FMath::Lerp(0.f, MaxRunningStamina, Alpha);
}

bool UFPMovementComponent::KeepRunning()
{
	return CurrentRunningStamina > 0.f &&							//Stop running when stamina is depleted
		MoveInputVector.X > 0.f &&
		!CharacterOwner->GetCharacterMovement()->IsFalling();		//Do not run if the player is falling
}

bool UFPMovementComponent::CanEnterRunning()
{
	bool ReturnValue = CurrentRunningStamina > 0.f &&		//Has stamina
		MoveInputVector.X &&								//Is moving forward
		HasRoomToStandUp &&
		!CharacterOwner->GetCharacterMovement()->IsFalling();				//Only run when on the ground

	if (CurrentMovementMode == EFPMovementMode::Sliding)		//If currently sliding also check if the player can slide cancel
		return SlideCancelWithRun && ReturnValue;

	return ReturnValue;
}

//Sliding

void UFPMovementComponent::StartSliding()
{
	ShouldStandUp = false;		//Does not want to stand up

	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = CrouchingSpeed;

	//Makes the player slide
	CharacterOwner->GetCharacterMovement()->BrakingFrictionFactor = 0.f;
	CharacterOwner->GetCharacterMovement()->BrakingDecelerationWalking = 0.f;

	TimelineManager->SlidingInfluenceTimeline->SetNewTime(0.f);		//Reset timeline position
	TimelineManager->CrouchTimeline->Play();						//Scale capsule size
}

void UFPMovementComponent::StopSliding()
{
	//Stops the player from sliding
	CharacterOwner->GetCharacterMovement()->BrakingFrictionFactor = InitialBrakingFrictionFactor;
	CharacterOwner->GetCharacterMovement()->BrakingDecelerationWalking = InitialBrakingDecelerationWalking;

	TimelineManager->SlidingInfluenceTimeline->Stop();
	TimelineManager->CrouchTimeline->Reverse();		//Resize the capsule back to normal
}

void UFPMovementComponent::SlidingMovement()
{
	//If true, start depleting influence, if not then stop
	if (ShouldUpdateSlidingInfluence())
		TimelineManager->SlidingInfluenceTimeline->Play();		//This is true if the player is up the slope or on flat surface		
	else
		TimelineManager->SlidingInfluenceTimeline->Stop();

	//Vector pointing in player's forward direction, based on current floor normal
	FVector ForwardVelocity = FVector::CrossProduct(CharacterOwner->GetActorRightVector(), CharacterOwner->GetCharacterMovement()->CurrentFloor.HitResult.Normal) * SlidingForwardForce;

	FVector SidewaysVelocity = CharacterOwner->GetActorRightVector() * MoveInputVector.Y * SlidingSidewaysForce;

	FVector SlidingForce = (ForwardVelocity + SidewaysVelocity) * SlidingInfluence;

	CharacterOwner->GetCharacterMovement()->AddForce(SlidingForce);

	if (CharacterOwner->GetVelocity().Size() > MaxSlidingSpeed)		//If force applied is too big, limit current speed to max sliding speed
		CharacterOwner->GetCharacterMovement()->Velocity = CharacterOwner->GetVelocity().GetSafeNormal() * MaxSlidingSpeed * SlidingInfluence;
}

void UFPMovementComponent::SlidingUpdate()
{
	if (CurrentMovementMode == EFPMovementMode::Sliding)		//Perform sliding movement only when sliding
		SlidingMovement();
}

void UFPMovementComponent::SlidingInfluenceUpdate(float NewInfluence)
{
	SlidingInfluence = NewInfluence;

	//If influence reaches zero, start crouching
	if (SlidingInfluence == 0.f && CurrentMovementMode == EFPMovementMode::Sliding)
	{
		ExitMovementMode(EFPMovementMode::Sliding);
		EnterMovementMode(EFPMovementMode::Crouching);
	}
}

bool UFPMovementComponent::ShouldUpdateSlidingInfluence()
{
	FHitResult FloorHitResult = CharacterOwner->GetCharacterMovement()->CurrentFloor.HitResult;
	return FVector::UpVector == FloorHitResult.Normal ||	//Check if the current surface is flat
		//Check if velocity vector is facing the opposite direction than vector pointing down the current slope
		FVector::DotProduct(CharacterOwner->GetVelocity().GetSafeNormal(),
			FVector::CrossProduct(FloorHitResult.Normal, FVector::CrossProduct(FloorHitResult.Normal, FVector::UpVector))) < 0.f;
}

bool UFPMovementComponent::CanEnterSliding()
{
	return !CharacterOwner->GetCharacterMovement()->IsFalling() &&
		CurrentMovementMode == EFPMovementMode::Running;		//Can only enter sliding through running
}

//Walking

void UFPMovementComponent::OnWalk(const FInputActionValue& Value)
{
	//Get input vector
	FVector2D InputVector = Value.Get<FVector2D>();

	//Only add movement input when one of these is the current movement mode
	if (CurrentMovementMode == EFPMovementMode::Walking ||
		CurrentMovementMode == EFPMovementMode::Running ||
		CurrentMovementMode == EFPMovementMode::Crouching ||
		CurrentMovementMode == EFPMovementMode::Falling ||
		CurrentMovementMode == EFPMovementMode::Wallrunning)
	{
		//Forward/Backward
		CharacterOwner->AddMovementInput(CharacterOwner->GetActorForwardVector(), InputVector.X);

		//Right/Left
		CharacterOwner->AddMovementInput(CharacterOwner->GetActorRightVector(), InputVector.Y);
	}
}

void UFPMovementComponent::StartWalking()
{
	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
	CurrentJumpAmount = MaxJumpAmount;		//Reset jump amount
}

bool UFPMovementComponent::CanEnterWalking()
{
	return CurrentMovementMode != EFPMovementMode::Sliding &&		//Sliding disables regular walking
		CurrentMovementMode != EFPMovementMode::Mantling &&		//Mantling disables walking
		CurrentMovementMode != EFPMovementMode::Dashing &&		//Dashing disables walking
		HasRoomToStandUp && ShouldStandUp;
}

//Falling

void UFPMovementComponent::StartFalling()
{
	CharacterOwner->GetCharacterMovement()->MaxWalkSpeed = AirborneSpeed;
}

void UFPMovementComponent::RestoreLandingEffects()
{
	UseLandingEffects = true;
}

void UFPMovementComponent::PlayLandingEffects(FVector Location, FRotator Rotation)
{
	if (UseLandingEffects)		//Set this to false if you want the effects disabled
	{
		if (OnLandedParticle)	//Play particle effect on the ground
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), OnLandedParticle, Location, Rotation, FVector::OneVector, true, true);

		TimelineManager->CameraBobbingTimeline->PlayFromStart();

		if (LandingShake)
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraShake(LandingShake, 1.f);
	}
}

void UFPMovementComponent::OnLandedEvent(const FHitResult& Hit)
{
	if (TrySwitchingMovementMode(EFPMovementMode::Walking))		//If possible, start walking after landing
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Walking);

		PlayLandingEffects(Hit.ImpactPoint, UKismetMathLibrary::MakeRotFromZ(Hit.Normal));
	}
	//If not, try crouching
	else if (!ShouldStandUp &&
		CurrentMovementMode != EFPMovementMode::Crouching &&
		CurrentMovementMode != EFPMovementMode::Sliding &&
		CurrentMovementMode != EFPMovementMode::Mantling)
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Crouching);

		PlayLandingEffects(Hit.ImpactPoint, UKismetMathLibrary::MakeRotFromZ(Hit.Normal));
	}
}

void UFPMovementComponent::OnWalkingOffLedgeEvent(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta)
{
	if (TrySwitchingMovementMode(EFPMovementMode::Falling))		//If possible, start falling when walking off ledge
	{
		ExitMovementMode(CurrentMovementMode);
		EnterMovementMode(EFPMovementMode::Falling);
	}
}

bool UFPMovementComponent::CanEnterFalling()
{
	//Makes sure that accidentally stepping of surface does not break mantling movement
	return CurrentMovementMode != EFPMovementMode::Mantling;
}

//Movement State Handling

void UFPMovementComponent::SetMovementValues()
{
	EnterMovementMode(EFPMovementMode::Falling);		//Always start with falling as the player won't always spawn on the ground

	//Spawn actors for mantle targeting and a timeline manager actor
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (MantleTargetActorClass)
		{
			MantleBeginPointTargetActor = GetWorld()->SpawnActor<AActor>(MantleTargetActorClass, CharacterOwner->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			MantleUpTargetActor = GetWorld()->SpawnActor<AActor>(MantleTargetActorClass, CharacterOwner->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			MantleForwardTargetActor = GetWorld()->SpawnActor<AActor>(MantleTargetActorClass, CharacterOwner->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
		}

		if (TimelineManagerClass)
		{
			TimelineManager = GetWorld()->SpawnActor<ATimelineManager>(TimelineManagerClass, CharacterOwner->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			TimelineManager->FPMovement = this;

			//Bind curves to timelines - you can use them to make movement more smooth
			//(make sure that curve assets are set, or else it will break)
			TimelineManager->BindTimelines();
		}
	}

	//Set default values

	InitialCameraLocation = CameraSocket->GetRelativeLocation();
	DefaultCapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	InitialGravityScale = CharacterOwner->GetCharacterMovement()->GravityScale;
	InitialBrakingFrictionFactor = CharacterOwner->GetCharacterMovement()->BrakingFrictionFactor;
	InitialBrakingDecelerationWalking = CharacterOwner->GetCharacterMovement()->BrakingDecelerationWalking;
	CurrentRunningStamina = MaxRunningStamina;
	CurrentJumpAmount = MaxJumpAmount;
	DashingStamina = MaxDashAmount;
}

void UFPMovementComponent::Initialize(ACharacter* Character, USceneComponent* CharacterCameraSocket, UCameraComponent* CharacterCamera)
{
	CharacterOwner = Character;
	CameraSocket = CharacterCameraSocket;
	Camera = CharacterCamera;
}

void UFPMovementComponent::EnterMovementMode(TEnumAsByte<EFPMovementMode> NewMode)
{
	PreviousMovementModeBeforeSwitch = PreviousMovementMode;		//Save previous movement mode as previous mode before switch
	PreviousMovementMode = CurrentMovementMode;			//Save current movement mode as previous
	CurrentMovementMode = NewMode;			//Set new mode

	//Choose appropriate function and enter new movement mode
	switch (NewMode)
	{

	case EFPMovementMode::Walking:
		StartWalking();
		break;

	case EFPMovementMode::Running:
		StartRunning();
		break;

	case EFPMovementMode::Crouching:
		StartCrouching();
		break;

	case EFPMovementMode::Sliding:
		StartSliding();
		break;

	case EFPMovementMode::Jumping:
		StartJumping();
		break;

	case EFPMovementMode::Wallrunning:
		StartWallRun();
		break;

	case EFPMovementMode::Mantling:
		StartMantling();
		break;

	case EFPMovementMode::Dashing:
		StartDashing();
		break;

	case EFPMovementMode::Falling:
		StartFalling();
		break;

	default:
		break;

	}
}

void UFPMovementComponent::ExitMovementMode(TEnumAsByte<EFPMovementMode> MovementMode)
{
	//Choose appropriate function and exit movement mode
	switch (MovementMode)
	{

	case EFPMovementMode::Running:
		StopRunning();
		break;

	case EFPMovementMode::Crouching:
		StopCrouching();
		break;

	case EFPMovementMode::Sliding:
		StopSliding();
		break;

	case EFPMovementMode::Wallrunning:
		StopWallRun();
		break;

	case EFPMovementMode::Mantling:
		StopMantling();
		break;

	case EFPMovementMode::Dashing:
		StopDashing();
		break;

	case EFPMovementMode::Falling:
		ShouldStandUp = true;		//No need to create a function for this one
		break;

	default:
		break;

	}
}

bool UFPMovementComponent::TrySwitchingMovementMode(TEnumAsByte<EFPMovementMode> NewMode)
{
	//Return false if the mode the player is trying to enter is the same as the current
	if (NewMode == CurrentMovementMode)
		return false;

	//Choose appropriate function and return its value
	switch (NewMode)
	{

	case EFPMovementMode::Walking:
		return CanEnterWalking();

	case EFPMovementMode::Running:
		return CanEnterRunning();

	case EFPMovementMode::Crouching:
		return CanEnterCrouching();

	case EFPMovementMode::Sliding:
		return CanEnterSliding();

	case EFPMovementMode::Jumping:
		return CanEnterJumping();

	case EFPMovementMode::Wallrunning:
		return CanEnterWallRun();

	case EFPMovementMode::Mantling:
		return CanEnterMantling();

	case EFPMovementMode::Dashing:
		return CanEnterDashing();

	case EFPMovementMode::Falling:
		return CanEnterFalling();

	default:
		return false;

	}
}