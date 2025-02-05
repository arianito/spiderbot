// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraConfigVolume.h"

#include "Components/SplineComponent.h"


ACameraConfigVolume::ACameraConfigVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	bDisplayShadedVolume = true;
	CameraPath = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
}
