// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/w_highlight/


#pragma once

#include "CoreMinimal.h"
#include "W_Drag.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "W_HighlightWidget.generated.h"

class UW_Tile;
class UW_Container;


UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UW_HighlightWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	//Don't start ticking until we've manually enabled it, as some references need to be set for the tick animation to work.
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool TickEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	float PositionInterpSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	FVector2D ClampedMousePosition = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool InsideContainer = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	float ColorInterpSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	float RotationInterpSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	float SizeInterpSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Color")
	FLinearColor ToColor = FLinearColor(0.35, 0.35, 0.35, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag", meta = (ExposeOnSpawn = true))
	UW_Drag* DragWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag", meta = (ExposeOnSpawn = true))
	UW_InventoryItem* ItemWidget = nullptr;

	//This is the tile that the clamped mouse position is hovering over.
	UPROPERTY(BlueprintReadWrite, Category = "Container and Tiles")
	UW_Tile* ClampedTile = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Container and Tiles")
	UW_Container* Container = nullptr;

	//Container settings of the current tile.
	UPROPERTY(BlueprintReadWrite, Category = "Container and Tiles")
	FS_ContainerSettings TilesContainerSettings;

	UPROPERTY(BlueprintReadWrite, Category = "Container and Tiles")
	FVector2D ContainerClamp;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidget))
	class UImage* Highlight = nullptr;
	
	FTimerHandle ScrollTimer;
};
