// Copyright 2023, Jan Kozlowski, All rights reserved

#pragma once

#include "MovementEnums.generated.h"

UENUM(BlueprintType)
enum EDashingType
{
	MovementBased,
	CameraBased
};

UENUM(BlueprintType)
enum EMantlingType
{
	SetVelocity,
	SetLocation
};

UENUM(BlueprintType)
enum EFPMovementMode
{
	Walking,
	Running,
	Crouching,
	Sliding,
	Jumping,
	Wallrunning,
	Mantling,
	Dashing,
	Falling
};

