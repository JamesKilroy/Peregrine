// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Widgets/W_AttachmentParent.h"
#include "Factories/Factory.h"
#include "AttachmentWidgetFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UAttachmentWidgetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UAttachmentWidgetFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UW_AttachmentParent> AttachmentWidgetClass;
};
