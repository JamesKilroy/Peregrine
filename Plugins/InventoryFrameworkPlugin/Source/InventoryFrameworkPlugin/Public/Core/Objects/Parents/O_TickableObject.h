// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "O_TickableObject.generated.h"

/**
 * This object is meant to be used for when you want to send timed or asynchronous tasks to
 * another class/object. This is used by the Equipment to simplify the multiple montages that
 * could be triggered.
 */

UCLASS(Abstract, Blueprintable)

class INVENTORYFRAMEWORKPLUGIN_API UO_TickableObject : public UObject, public FTickableGameObject
{
public:

 GENERATED_BODY()

 //--------------------
 // Variables

 /**Tick should ideally never be used. Tick is really bad for networking and in general isn't
  * the best for gameplay logic either. This is here in case someone REALLY wants it.
  * Also remember, the instanced version will run this tick event, so I REALLY can not recommend
  * using tick event at all inside of this class and any children.*/
 UPROPERTY(BlueprintReadWrite, Category = "Settings")
 bool TickEnabled = false;

 //Objects aren't always destroyed the instant they are labeled for garbage collection.
 //This can be used to find out if this object has been labeled for garbage collection.
 UPROPERTY(BlueprintReadWrite, Category = "Settings")
 bool IsDestroyed = false;

 //--------------------
 // Functions
 
 virtual UWorld* GetWorld() const override;

 //Our begin play.
 //I didn't name it "BeginPlay" because if I wanted to rename it, Rider would modify everything that has BeginPlay in it, including engine code.
 UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NativeFunctions")
 void StartPlay();

 UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NativeFunctions")
 void Destroyed();

 UFUNCTION(BlueprintCallable, Category = "NativeFunctions")
 virtual void RemoveObject();

 //Tick and timer support
 virtual void Tick(float DeltaTime) override;
 virtual bool IsTickable() const override;
 virtual TStatId GetStatId() const override;

protected:
 UFUNCTION(BlueprintImplementableEvent)
 void EventTick(float DeltaTime);
};
