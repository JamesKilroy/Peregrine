// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_Validation.generated.h"

/**NOTE: This interface is purely used for asset validation.
 * This interface does not need to be cooked in a shipping build.
 * Nothing in here is vital for any gameplay and none of this
 * should be called during gameplay.
 */

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI)
class UI_Validation : public UInterface
{
	GENERATED_BODY()
};


class INVENTORYFRAMEWORKPLUGIN_API II_Validation
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/**Called by the item data asset when the "Verify Data" button is pressed.
	 * If this returns any error messages, the verification will fail
	 * and a popup on the bottom right will display the error messages.
	 *
	 * This is development only, won't work in a shipping build.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Verification", meta = (DevelopmentOnly, ReturnDisplayName = "Error Messages"))
	TArray<FString> VerifyData(UDA_CoreItem* ItemAsset);
};
