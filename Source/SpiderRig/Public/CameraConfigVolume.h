// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/TriggerVolume.h"
#include "CameraConfigVolume.generated.h"

class USplineComponent;

UCLASS(Blueprintable)
class SPIDERRIG_API ACameraConfigVolume : public ATriggerVolume
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACameraConfigVolume();

	UPROPERTY(EditAnywhere)
	TObjectPtr<USplineComponent> CameraPath;
};
