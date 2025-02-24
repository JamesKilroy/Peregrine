// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "Recipes/O_RecipeObject.h"
#include "UObject/Object.h"
#include "O_CoreCraftEvent.generated.h"

/**Code that is executed whenever a craft occurs.*/
UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow"))
class IFP_CRAFTING_API UO_CoreCraftEvent : public UO_RecipeObject
{
	GENERATED_BODY()

public:

	/**What networking type should this event be activated on?
	 * This is used for when you might want to only activate an
	 * event on a client, such as playing a UI sound, or granting
	 * experience, which you'd want to do on the server.
	 *
	 * If the game is single player, this will have no effect
	 *
	 * IMPORTANT: RPC's will not work as this object is inside
	 * a data asset
	 * Client objects are also called on dedicated servers.
	 * If you don't want the logic to happen on dedicated servers,
	 * add a branch in your logic blocking dedicated servers
	 * from executing the code.*/
	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere)
	TEnumAsByte<EObjectNetworkingMethod> NetworkingType = Server;
};
