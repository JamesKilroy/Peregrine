// Copyright 2023, Jan Kozlowski, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraShakeBase.h"
#include "MovementEnums.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "FPMovementComponent.generated.h"

class UInputAction;
class USceneComponent;
class UCameraComponent;

UCLASS(Blueprintable)
class FIRSTPERSONMOVEMENTTEMPLATE_API UFPMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Input

	UFUNCTION(BlueprintCallable, Category = "Input")
		void AddMovementInputContext();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputMappingContext* MovementInputContext;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* JumpAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* DashAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* MantleAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* CrouchAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* RunAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
		UInputAction* MoveAction;
	UPROPERTY(BlueprintReadWrite, Category = "Input")
		class UEnhancedInputComponent* EnhancedInputComponent;
	FVector2D MoveInputVector;

	//Jumping

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Jumping")
		void OnJump(const FInputActionValue& Value);
	void StartJumping();
	bool CanEnterJumping();

	UPROPERTY(BlueprintReadOnly, Category = "FPSMovement|Jumping")
		int CurrentJumpAmount;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Jumping")
		int MaxJumpAmount = 2;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Jumping")
		USoundBase* JumpSound;

	//Dashing

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Dashing")
		void OnDash(const FInputActionValue& Value);
	void StartDashing();
	void StopDashing();
	UFUNCTION()
		void DashingMovementFinished();
	void CalculateDashingDirection();
	UFUNCTION()
		void DashingMovementUpdate(float Alpha);
	UFUNCTION()
		void DashStaminaRecoverUpdate(float Alpha);
	bool CanEnterDashing();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Dashing")
		int MaxDashAmount = 2;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Dashing")
		float DashingSpeed = 10000.f;									//Max speed at which the player moves when dashing
	UPROPERTY(BlueprintReadOnly, Category = "FPSMovement|Dashing")
		float DashingStamina;											//Current dash stamina
	FVector DashingDirection;											//Direction at which the player is dashing
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Dashing")
		TSubclassOf<UCameraShakeBase> DashingShake;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Dashing")
		USoundBase* DashSound;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Dashing")
		TEnumAsByte<EDashingType> DashingType;							//Defines the way the player dashes

	//Mantling

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Mantling")
		void OnMantle(const FInputActionValue& Value);						////Mantle is only triggered when pressing input. Move this to tick event if you want to mantle automatically
	void StartMantling();
	void StopMantling();
	UFUNCTION()
		void MantleUpFinished();
	UFUNCTION()
		void MantleForwardFinished();
	UFUNCTION()
		void MantleMovementUpdate(float Alpha);
	bool MantleCheck();
	bool CanEnterMantling();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		float MantleSpeed = 2000.f;										//Speed at which the player moves if mantling type is SetVelocity
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		float MantleTraceForwardDistance = 50.f;						//Max distance for checking the wall
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		float MantleHeightTraceLength = 100.f;							//Max height while tracing to get mantle surface normal
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		float MantleLocationZOffset = 5.f;								//Used to trace slightly higher than mantle surface
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		float MantleDepth = 75.f;										//Defines how much deep into surface the player is going to mantle on
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		TSubclassOf<UCameraShakeBase> MantlingShake;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		TSubclassOf<AActor> MantleTargetActorClass;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Mantling")
		TEnumAsByte<EMantlingType> MantlingType;						//Defines the way the player mantles
	AActor* MantleUpTargetActor;
	AActor* MantleForwardTargetActor;
	AActor* MantleBeginPointTargetActor;								//Actor that saves the location at which mantle movement started
	AActor* MantleTargetActor;											//Current actor that the player is mantling towards
	USceneComponent* MantleHitComponent;
	FHitResult MantleHitResult;

	//Wallrunning

	void StartWallRun();
	void StopWallRun();
	void WallRunUpdate();
	void WallRunMovement();
	void WallRunJump();
	void WallRunRecover();
	UFUNCTION()
		void WallRunGravityUpdate(float Alpha);
	bool ShouldStopWallrunning();
	bool ShouldSpawnWallParticle();
	bool CheckForWall();
	bool CanEnterWallRun();
	FVector CalculateWallRunVelocity();

	float WallDirection;												//1 is right, -1 is left
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float WallRunSpeed = 600.f;										//Max speed at which the player is moving when wallrunning
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float DistanceToWall = 50.f;									//Max trace distance while searching for wall
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float MaxWallRunViewDotProduct = 0.95f;							//Defines how much away from wall the player can look before he stops wallrunning
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float WallRunJumpOffVelocity = 800.f;							//WallRun jump velocity in XY axis
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float WallRunJumpUpVelocity = 800.f;							//WallRun jump velocity in Z axis
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float WallRunRecoverTime = 0.1f;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		float MaxWallRunMovementDotProduct = 0.95f;						//Defines how much away from wall the player can try to walk off before he falls
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		bool UseGravityForWallRun = true;								//Defines if gravity should apply when wallrunning
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		bool ShouldSpawnWallRunParticle = true;
	bool CanEverWallRun = true;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Wallrunning")
		UNiagaraSystem* WallRunParticle;
	AActor* PreviousWall;
	AActor* CurrentWall;												//Wall that the player is currently running on
	FHitResult WallRunHitResult;

	//Camera movement

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Camera movement")
		void OnLook(const FInputActionValue& Value);
	void CameraTilt(float Angle, float TiltSpeed);
	UFUNCTION()
		void CameraBobbingUpdate(FVector NewLocation);
	void CameraTiltUpdate();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Camera movement")
		float WallRunAngle = 15.f;										//Defines how much camera tilts when wallrunning
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Camera movement")
		float WallRunTiltSpeed = 2.f;									//Defines how fast camera tilts when wallrunning
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Camera movement")
		float StrafingTiltAngle = 5.f;									//Defines how much camera tilts when strafing
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Camera movement")
		float StrafingTiltSpeed = 3.f;									//Defines how fast camera tilts when strafing

	//Crouching

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Crouching")
		void OnCrouch(const FInputActionValue& Value);
	void StartCrouching();
	void StopCrouching();
	void CrouchUpdate();
	UFUNCTION()
		void CrouchingTimelineUpdate(float Alpha);
	bool CheckCrouchRoom();
	bool CanEnterCrouching();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Crouching")
		float CrouchingSpeed = 250.f;									//Max speed at which the player moves when crouching
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Crouching")
		float CrouchingTraceHeightOffset = 3.f;							//Offsets the z start location when tracing for room to stand up
	bool HasRoomToStandUp = true;
	bool ShouldStandUp = true;

	//Running

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Running")
		void OnRunStart(const FInputActionValue& Value);
	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Running")
		void OnRunEnd(const FInputActionValue& Value);
	void StartRunning();
	void StopRunning();
	UFUNCTION()
		void RunningStaminaDepletionUpdate(float Alpha);
	UFUNCTION()
		void RunningStaminaRecoverUpdate(float Alpha);
	bool KeepRunning();
	bool CanEnterRunning();

	UPROPERTY(BlueprintReadOnly, Category = "FPSMovement|Running")
		float CurrentRunningStamina;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Running")
		float RunningSpeed = 1500.f;									//Max speed at which the player moves when running
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPSMovement|Running")
		float MaxRunningStamina = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Running")
		bool UseStaminaForRunning = true;								//Defines whether to use running stamina

	//Sliding

	void StartSliding();
	void StopSliding();
	void SlidingMovement();
	void SlidingUpdate();
	UFUNCTION()
		void SlidingInfluenceUpdate(float NewInfluence);
	bool ShouldUpdateSlidingInfluence();
	bool CanEnterSliding();

	float SlidingInfluence = 1.f;										//Defines how much sliding influences movement. When it reaches 0 sliding stops
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Sliding")
		float MaxSlidingSpeed = 2000.f;									//Max velocity that player can reach when sliding
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Sliding")
		float SlidingForwardForce = 500000.f;							//Forward force that applies to the player when sliding
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Sliding")
		float SlidingSidewaysForce = 350000.f;							//Sideways force that applies to the player when sliding
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Sliding")
		bool SlideCancelWithRun = false;								//Defines if running can stop sliding

	//Walking

	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Walking")
		void OnWalk(const FInputActionValue& Value);
	void StartWalking();
	bool CanEnterWalking();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Walking")
		float WalkingSpeed = 800.f;										//Max speed at which the player moves when walking

	//Falling

	void StartFalling();
	void RestoreLandingEffects();
	void PlayLandingEffects(FVector Location, FRotator Rotation);
	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Falling")
		void OnLandedEvent(const FHitResult& Hit);
	UFUNCTION(BlueprintCallable, Category = "FPSMovement|Falling")
		void OnWalkingOffLedgeEvent(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta);
	bool CanEnterFalling();

	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Falling")
		float AirborneSpeed = 800.f;									//Max speed at which the player moves when in air
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Falling")
		float LandingEffectsRestorationTime = 0.4f;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Falling")
		bool UseLandingEffects = true;									//Defines whether to play special effects when landing on ground
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Falling")
		UNiagaraSystem* OnLandedParticle;
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|Falling")
		TSubclassOf<UCameraShakeBase> LandingShake;

	//Movement State Handling

	void SetMovementValues();
	UFUNCTION(BlueprintCallable, Category = "FPSMovement|MovementStateHandling")
		void Initialize(ACharacter* Character, USceneComponent* CharacterCameraSocket, UCameraComponent* CharacterCamera);
	void EnterMovementMode(TEnumAsByte<EFPMovementMode> NewMode);
	void ExitMovementMode(TEnumAsByte<EFPMovementMode> MovementMode);
	bool TrySwitchingMovementMode(TEnumAsByte<EFPMovementMode> NewMode);

	ACharacter* CharacterOwner;
	USceneComponent* CameraSocket;
	UCameraComponent* Camera;
	class ATimelineManager* TimelineManager;							//components cannot have components attached, so we need an actor to store timelines
	UPROPERTY(EditDefaultsOnly, Category = "FPSMovement|MovementStateHandling")
		TSubclassOf<ATimelineManager> TimelineManagerClass;
	TEnumAsByte<EFPMovementMode> CurrentMovementMode;
	TEnumAsByte<EFPMovementMode> PreviousMovementMode;
	TEnumAsByte<EFPMovementMode> PreviousMovementModeBeforeSwitch;		//This is used only for checking if it is possible to spawn wallrun particles
	FVector PreviousFrameLocation;										//Location of the player on the previous frame

	//Default Movement Values

	float InitialGravityScale;
	float InitialBrakingFrictionFactor;
	float InitialBrakingDecelerationWalking;
	float DefaultCapsuleHalfHeight;
	FVector InitialCameraLocation;
};
