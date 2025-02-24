// Copyright 2023, Jan Kozlowski, All rights reserved


#include "TimelineManager.h"
#include "FPMovementComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

ATimelineManager::ATimelineManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CrouchTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Crouch Timeline"));
	DashingMovementTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Dashing Movement Timeline"));
	DashStaminaRecoverTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Dash Stamina Timeline"));
	SlidingInfluenceTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Sliding Influence Timeline"));
	RunningStaminaDepletionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Running Stamina Depletion Timeline"));
	RunningStaminaRecoverTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Running Stamina Recover Timeline"));
	WallRunGravityTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Wall Run Gravity Timeline"));
	MantleUpTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Mantle Up Timeline"));
	MantleForwardTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Mantle Forward Timeline"));
	CameraBobbingTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Camera Bobbing Timeline"));
}

void ATimelineManager::BindTimelines()
{
	if (!FPMovement)
		return;

	//Mantle Up Timeline
	{
		if (MantleUpCurve)
		{
			FOnTimelineFloat MantleUpUpdate;
			MantleUpUpdate.BindUFunction(FPMovement, "MantleMovementUpdate");
			MantleUpTimeline->AddInterpFloat(MantleUpCurve, MantleUpUpdate);
		}

		FOnTimelineEvent MantleUpFinished;
		MantleUpFinished.BindUFunction(FPMovement, "MantleUpFinished");
		MantleUpTimeline->SetTimelineFinishedFunc(MantleUpFinished);
	}

	//Mantle Forward Timeline
	{
		if (MantleForwardCurve)
		{
			FOnTimelineFloat MantleForwardUpdate;
			MantleForwardUpdate.BindUFunction(FPMovement, "MantleMovementUpdate");
			MantleForwardTimeline->AddInterpFloat(MantleForwardCurve, MantleForwardUpdate);
		}

		FOnTimelineEvent MantleForwardFinished;
		MantleForwardFinished.BindUFunction(FPMovement, "MantleForwardFinished");
		MantleForwardTimeline->SetTimelineFinishedFunc(MantleForwardFinished);
	}

	//Dashing Movement Timeline
	{
		if (DashingMovementCurve)
		{
			FOnTimelineFloat DashingMovementUpdate;
			DashingMovementUpdate.BindUFunction(FPMovement, "DashingMovementUpdate");
			DashingMovementTimeline->AddInterpFloat(DashingMovementCurve, DashingMovementUpdate);
		}

		FOnTimelineEvent DashingMovementFinished;
		DashingMovementFinished.BindUFunction(FPMovement, "DashingMovementFinished");
		DashingMovementTimeline->SetTimelineFinishedFunc(DashingMovementFinished);
	}

	//Dash Stamina Recover Timeline
	{
		if (DashStaminaRecoverCurve)
		{
			FOnTimelineFloat DashStaminaRecoverUpdate;
			DashStaminaRecoverUpdate.BindUFunction(FPMovement, "DashStaminaRecoverUpdate");
			DashStaminaRecoverTimeline->AddInterpFloat(DashStaminaRecoverCurve, DashStaminaRecoverUpdate);
		}
	}

	//Camera Bobbing Timeline
	{
		if (CameraBobbingCurve)
		{
			FOnTimelineVector CameraBobbingUpdate;
			CameraBobbingUpdate.BindUFunction(FPMovement, "CameraBobbingUpdate");
			CameraBobbingTimeline->AddInterpVector(CameraBobbingCurve, CameraBobbingUpdate);
		}
	}

	//Wallrun Gravity Timeline
	{
		if (WallRunGravityCurve)
		{
			FOnTimelineFloat WallRunGravityUpdate;
			WallRunGravityUpdate.BindUFunction(FPMovement, "WallRunGravityUpdate");
			WallRunGravityTimeline->AddInterpFloat(WallRunGravityCurve, WallRunGravityUpdate);
		}
	}

	//Running Stamina Depletion Timeline
	{
		if (RunningStaminaDepletionCurve)
		{
			FOnTimelineFloat RunningStaminaDepletionUpdate;
			RunningStaminaDepletionUpdate.BindUFunction(FPMovement, "RunningStaminaDepletionUpdate");
			RunningStaminaDepletionTimeline->AddInterpFloat(RunningStaminaDepletionCurve, RunningStaminaDepletionUpdate);
		}
	}

	//Running Stamina Recover Timeline
	{
		if (RunningStaminaRecoverCurve)
		{
			FOnTimelineFloat RunningStaminaRecoverUpdate;
			RunningStaminaRecoverUpdate.BindUFunction(FPMovement, "RunningStaminaRecoverUpdate");
			RunningStaminaRecoverTimeline->AddInterpFloat(RunningStaminaRecoverCurve, RunningStaminaRecoverUpdate);
		}
	}

	//Sliding Influence Timeline
	{
		if (SlidingInfluenceCurve)
		{
			FOnTimelineFloat SlidingInfluenceUpdate;
			SlidingInfluenceUpdate.BindUFunction(FPMovement, "SlidingInfluenceUpdate");
			SlidingInfluenceTimeline->AddInterpFloat(SlidingInfluenceCurve, SlidingInfluenceUpdate);
		}
	}

	//Crouch Timeline
	{
		{
			if (CrouchCurve)
			{
				FOnTimelineFloat CrouchUpdate;
				CrouchUpdate.BindUFunction(FPMovement, "CrouchingTimelineUpdate");
				CrouchTimeline->AddInterpFloat(CrouchCurve, CrouchUpdate);
			}
		}
	}
}
