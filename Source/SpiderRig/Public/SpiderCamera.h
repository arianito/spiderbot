// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "SpiderCamera.generated.h"

class ACameraConfigVolume;
class ASpiderCharacter;

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

	void CalculateLagSpeeds(FVector& LagSpeeds, float& RotSpeed) const;
	void CalculateFreeLook(const bool& bIsGrounded);
	void TraceCameraCollision(const ASpiderCharacter* Character, const FVector& TraceOrigin, FVector& Target, const float& TraceRadius = 5.0f) const;

	bool bIsLocked = false;
	
	APawn* PreviouslyPossessedPawn{nullptr};
	ACameraConfigVolume* CurrentLockedVolume{nullptr};
	
	int32 OverlapCounter{0};
	
	FRotator FreeLookRotation{0};
	FRotator TargetFreeLookRotation{0};
	
	float ChangeStateTimestamp{0};
	float InputChangedTimestamp{0};

	FRotator PrevControlRotator{0};
	FVector TargetLocation{0};
	FRotator TargetRotator{0};

protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;
	

public:
	void OnPossess(APawn* NewPawn);

	UPROPERTY(EditAnywhere)
	FVector GroundedLagSpeed{10, 10, 10};
	
	UPROPERTY(EditAnywhere)
	FVector InAirLagSpeed{10, 10, 2.5f};
	
	UPROPERTY(EditAnywhere)
	float LookLagSpeed{10.0f};
};
