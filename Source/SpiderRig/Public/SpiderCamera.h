// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "SpiderCamera.generated.h"

class ACameraConfigVolume;

struct FCameraSplineMemory
{
	int32 Id;
	FVector PrevPosition;
};

UCLASS()
class SPIDERRIG_API ASpiderCamera : public APlayerCameraManager
{
	GENERATED_BODY()
private:
	ASpiderCamera();
	
	UFUNCTION()
	void OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	APawn* PrevPawn{nullptr};
	ACameraConfigVolume* CurrentVolume{nullptr};
	uint32 OverlapLayers{0};
	TArray<FCameraSplineMemory> OverlapMemory{};
	

	FRotator PrevControlRotation{0};
	FVector TargetLocation{0};
	FRotator TargetRotator{0};
	
	
protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;
public:
	void OnPossess(APawn* NewPawn);

};
