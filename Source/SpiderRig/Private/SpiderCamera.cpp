// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderCamera.h"

#include "CameraConfigVolume.h"
#include "SpiderCharacter.h"
#include "SpiderPlayerController.h"
#include "Components/BrushComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

ASpiderCamera::ASpiderCamera()
{
}

FVector CalculateAxisIndependentLag(const FVector& CurrentLocation, const FVector& TargetLocation,
                                    FRotator CameraRotation, const FVector& LagSpeeds,
                                    const float& DeltaTime)
{
	CameraRotation.Roll = 0.0f;
	CameraRotation.Pitch = 0.0f;
	const FVector UnrotatedCurLoc = CameraRotation.UnrotateVector(CurrentLocation);
	const FVector UnrotatedTargetLoc = CameraRotation.UnrotateVector(TargetLocation);

	const FVector ResultVector(
		FMath::FInterpTo(UnrotatedCurLoc.X, UnrotatedTargetLoc.X, DeltaTime, LagSpeeds.X),
		FMath::FInterpTo(UnrotatedCurLoc.Y, UnrotatedTargetLoc.Y, DeltaTime, LagSpeeds.Y),
		FMath::FInterpTo(UnrotatedCurLoc.Z, UnrotatedTargetLoc.Z, DeltaTime, LagSpeeds.Z));

	return CameraRotation.RotateVector(ResultVector);
}


void ASpiderCamera::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
	if (!OutVT.Target) return;
	if (!OutVT.Target.IsA<ASpiderCharacter>()) return;


	UWorld* World = GetWorld();
	check(World);

	const auto Character = Cast<ASpiderCharacter>(OutVT.Target.Get());
	const FVector CameraTargetSocket = Character->GetActorLocation();
	const auto PlayerController = Cast<ASpiderPlayerController>(GetOwningPlayerController());
	const auto CharacterMovement = Character->GetCharacterMovement();

	FVector FinalLocation;
	FRotator FinalRotation;

	const bool bIsGrounded = FMath::IsNearlyZero(CharacterMovement->Velocity.Z) && !CharacterMovement->IsFalling();

	if (CurrentVolume != nullptr && bIsGrounded)
	{
		bAllowFollow = true;
	}


	if (bAllowFollow)
	{
		if (bIsGrounded)
		{
			if (FMath::Abs(PlayerController->CameraMovement.SquaredLength()) > 0.5f)
			{
				FreeLookRotation += FRotator(PlayerController->CameraMovement.Y, PlayerController->CameraMovement.X, 0);
				FreeLookRotation.Pitch = FMath::ClampAngle(FreeLookRotation.Pitch, -5, 30);
				FreeLookRotation.Yaw = FMath::ClampAngle(FreeLookRotation.Yaw, -30, 30);

				constexpr double Delay = 3.0f;
				LookAtTimestamp = World->GetTimeSeconds() + Delay;
			}
		}


		constexpr double d = 0.5f;
		const double t = ((LookAtTimestamp + d) - World->GetTimeSeconds()) / d;
		const double f = FMath::Clamp(1.0f - t, 0.0f, 1.0f);

		FreeLookRotation = UKismetMathLibrary::RLerp(FreeLookRotation, FRotator(), f, true);


		const FVector CameraLock = CurrentVolume->CameraPath->FindLocationClosestToWorldLocation(
			CameraTargetSocket, ESplineCoordinateSpace::World);
		FinalLocation = CameraLock;
		FinalRotation = FRotationMatrix::MakeFromX(CameraTargetSocket - FinalLocation).Rotator();
	}
	else
	{
		FreeLookRotation.Pitch = 0;
		FreeLookRotation.Yaw = 0;
		constexpr float CameraToTargetDistance = 100.0f;
		const FVector ForwardVector = PlayerController->GetControlRotation().Vector();
		FinalLocation = CameraTargetSocket - ForwardVector * CameraToTargetDistance;
		FinalRotation = PlayerController->GetControlRotation();
	}

	double RotSpeed = 10.0f;
	FVector LagSpeeds{10, 10, 0.2};
	LagSpeeds.Z = bIsGrounded ? 10.0f : 2.5f;

	constexpr double d = 1.61f;
	const double t = ((EnteredVolumeTimestamp + d) - World->GetTimeSeconds()) / d;
	const double f = FMath::Clamp(1.0f - t, 0.1f, 1.0f);

	LagSpeeds *= f;
	RotSpeed *= f;

	if (bAllowFollow)
	{
		LagSpeeds *= 0.5f;
		RotSpeed *= 0.1f;
	}

	TargetRotator = FMath::RInterpTo(TargetRotator, FinalRotation, DeltaTime, RotSpeed);
	TargetLocation = CalculateAxisIndependentLag(TargetLocation, FinalLocation, TargetRotator, LagSpeeds, DeltaTime);
	TargetFreeLookRotation = FMath::RInterpTo(TargetFreeLookRotation, FreeLookRotation, DeltaTime, 5.0f);

	if (bAllowFollow)
	{
		PlayerController->SetControlRotation(TargetRotator + TargetFreeLookRotation);
	}

	FVector TraceOrigin = CameraTargetSocket;
	float TraceRadius = 5.0f;
	ECollisionChannel TraceChannel = ECC_Camera;


	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Character);

	FHitResult HitResult;
	const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(TraceRadius);
	World->SweepSingleByChannel(HitResult, TraceOrigin, TargetLocation, FQuat::Identity,
	                            TraceChannel, SphereCollisionShape, Params);


	if (HitResult.IsValidBlockingHit())
	{
		TargetLocation += HitResult.Location - HitResult.TraceEnd;
	}


	OutVT.POV.Location = TargetLocation;
	OutVT.POV.Rotation = TargetRotator + TargetFreeLookRotation;
}


void ASpiderCamera::OnPossess(APawn* NewPawn)
{
	if (NewPawn)
	{
		NewPawn->OnActorBeginOverlap.AddDynamic(this, &ASpiderCamera::OnBeginOverlap);
		NewPawn->OnActorEndOverlap.AddDynamic(this, &ASpiderCamera::OnEndOverlap);
		PrevPawn = NewPawn;
	}
	else if (PrevPawn)
	{
		if (IsValid(PrevPawn))
		{
			PrevPawn->OnActorBeginOverlap.RemoveDynamic(this, &ASpiderCamera::OnBeginOverlap);
			PrevPawn->OnActorEndOverlap.RemoveDynamic(this, &ASpiderCamera::OnEndOverlap);
		}
		PrevPawn = nullptr;
	}
}

void ASpiderCamera::OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor->IsA<ACameraConfigVolume>()) return;
	const auto CameraConfig = Cast<ACameraConfigVolume>(OtherActor);
	if (!IsValid(CameraConfig)) return;
	CurrentVolume = CameraConfig;
	EnteredVolumeTimestamp = GetWorld()->GetTimeSeconds();

	OverlapLayers++;
	UE_LOG(LogTemp, Display, TEXT("ASpiderCamera::OnBeginOverlap %d"), OverlapLayers);
}

void ASpiderCamera::OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor->IsA<ACameraConfigVolume>()) return;
	const auto CameraConfig = Cast<ACameraConfigVolume>(OtherActor);
	if (!IsValid(CameraConfig)) return;

	UE_LOG(LogTemp, Display, TEXT("ASpiderCamera::OnEndOverlap %d"), OverlapLayers);

	if (CurrentVolume && !(--OverlapLayers))
	{
		EnteredVolumeTimestamp = GetWorld()->GetTimeSeconds();
		CurrentVolume = nullptr;
		bAllowFollow = false;
	}
}
