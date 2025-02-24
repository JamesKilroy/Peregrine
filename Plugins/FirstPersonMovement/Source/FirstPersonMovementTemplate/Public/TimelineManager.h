// Copyright 2023, Jan Kozlowski, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimelineManager.generated.h"

class UTimelineComponent;
class UCurveFloat;
class UCurveVector;

UCLASS()
class FIRSTPERSONMOVEMENTTEMPLATE_API ATimelineManager : public AActor
{
	GENERATED_BODY()
	
public:	

	ATimelineManager();

	void BindTimelines();

	class UFPMovementComponent* FPMovement;

	//Curves

	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* DashingMovementCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* DashStaminaRecoverCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* MantleUpCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* MantleForwardCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* WallRunGravityCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveVector* CameraBobbingCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* CrouchCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* RunningStaminaDepletionCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* RunningStaminaRecoverCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Curves")
		UCurveFloat* SlidingInfluenceCurve;

	//Timelines

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Crouching")
		UTimelineComponent* CrouchTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Dashing")
		UTimelineComponent* DashingMovementTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Dashing")
		UTimelineComponent* DashStaminaRecoverTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Sliding")
		UTimelineComponent* SlidingInfluenceTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Running")
		UTimelineComponent* RunningStaminaDepletionTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Running")
		UTimelineComponent* RunningStaminaRecoverTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Wallrunning")
		UTimelineComponent* WallRunGravityTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Mantling")
		UTimelineComponent* MantleUpTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Mantling")
		UTimelineComponent* MantleForwardTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPSMovement|Camera movement")
		UTimelineComponent* CameraBobbingTimeline;
};
