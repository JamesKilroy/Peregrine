// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetTransform.h"
#include "Framework/Text/TextLayout.h"
#include "Sound/SoundBase.h"
#include "WidgetDataTypes.generated.h"


UENUM(BlueprintType)
enum E_ButtonState
{
	/**Ready to be pressed.*/
	Active,
	
	/**Disabled, not allowed to press or hover.*/
	Deactive,
	
	/**Optional state, usually used for tabs
	 * where the player has multiple selections,
	 * this can be used to show which of those buttons
	 * is selected.*/
	Selected,

	/**In the process of being pressed down.*/
	Pressed,

	/**Mouse or gamepad navigation is currently hovering
	 * or focusing this button.*/
	Hovered
};

USTRUCT(BlueprintType)
struct FS_TextAppearance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	TEnumAsByte<ETextJustify::Type> Alignment = ETextJustify::Center;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	FText Text;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	FLinearColor Color = FLinearColor(0,0,0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	FSlateFontInfo Font;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	FMargin Padding;
};

USTRUCT(BlueprintType)
struct FS_ButtonAppearance
{

	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	FSlateBrush ButtonStyle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Text")
	FS_TextAppearance TextSettings;

	//Sound to play when this state is activated.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound")
	USoundBase* Sound = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	FMargin InnerPadding;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	FVector2D Size = FVector2D(0, 0);

	//Sometimes size will not work in certain design scenarios, for example
	//horizontal alignment set to fill, but we want to scale the widget
	//when a specific state is set. - V
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	FWidgetTransform Transform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Frame")
	FLinearColor FrameColor = FLinearColor(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Frame")
	FMargin BorderSize;

	//Should the frame blink from one color to another during this state?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Frame")
	bool PlayFrameBlinkAnimation = false;

	//What color the frame should interp to during it's animation.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Frame", meta = (EditCondition = "PlayFrameBlinkAnimation"))
	FLinearColor SecondFrameColor = FLinearColor(0, 0, 0);
};

USTRUCT(BlueprintType)
struct FS_FrameSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Size")
	FVector2D Size = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Size")
	FMargin BorderSizes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Colors")
	FLinearColor FrameColor = FLinearColor(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Colors")
	FLinearColor BackgroundColor = FLinearColor(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	bool EnableBlinkAnimation = false;
};

USTRUCT(BlueprintType)
struct FS_SliderSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float MinSliderRange = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float MaxSliderRange = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float DefaultValue = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	int32 FractionalDigits = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FLinearColor SliderColor  = FLinearColor(0, 0, 0);
};

USTRUCT(BlueprintType)
struct FS_TitleSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	TEnumAsByte<ETextJustify::Type> Alignment = ETextJustify::Center;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FSlateFontInfo TitleText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Title")
	FLinearColor TitleColor = FLinearColor(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Title")
	FSlateFontInfo DescriptiveText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Description")
	FLinearColor DescriptionColor = FLinearColor(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	float TitleBorderHeight = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Appearance")
	FLinearColor BorderColor  = FLinearColor(0, 0, 0);
};
	
