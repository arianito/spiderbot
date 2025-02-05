// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderCamera.h"

#include "CameraConfigVolume.h"
#include "SpiderCharacter.h"
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

	const auto Character = Cast<ASpiderCharacter>(OutVT.Target.Get());
	const FVector CameraTargetSocket = Character->GetActorLocation();
	const auto PlayerController = GetOwningPlayerController();

	FVector FinalLocation;
	FRotator FinalRotation;

	if (CurrentVolume)
	{
		const FVector CameraLock = CurrentVolume->CameraPath->FindLocationClosestToWorldLocation(
			CameraTargetSocket, ESplineCoordinateSpace::World);
		FinalLocation = CameraLock;
		FinalRotation = FRotationMatrix::MakeFromX(Character->GetActorLocation() - FinalLocation).Rotator();
	}
	else
	{
		constexpr float CameraToTargetDistance = 100.0f;
		const FVector ForwardVector = PlayerController->GetControlRotation().Vector();
		FinalLocation = CameraTargetSocket - ForwardVector * CameraToTargetDistance;
		FinalRotation = PlayerController->GetControlRotation();
	}

	double RotSpeed = 10.0f;
	FVector LagSpeeds{10, 6, 0.2};
	const auto CharacterMovement = Character->GetCharacterMovement();
	LagSpeeds.Z = FMath::IsNearlyZero(CharacterMovement->Velocity.Z) ? 6.0f : 1.0f;

	if (CurrentVolume)
	{
		LagSpeeds *= 0.75f;
		RotSpeed *= 0.1f;
	}

	TargetRotator = FMath::RInterpTo(TargetRotator, FinalRotation, DeltaTime, RotSpeed);
	TargetLocation = CalculateAxisIndependentLag(TargetLocation, FinalLocation, TargetRotator, LagSpeeds, DeltaTime);

	if (CurrentVolume)
		PlayerController->SetControlRotation(TargetRotator);


	OutVT.POV.Location = TargetLocation;
	OutVT.POV.Rotation = TargetRotator;
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
	const auto Path = CurrentVolume->CameraPath;
	const auto n = Path->GetNumberOfSplinePoints();
	const FVector HeadAndTail[2] = {
		Path->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World),
		Path->GetLocationAtSplinePoint(n - 1, ESplineCoordinateSpace::World)
	};
	float Distance = MAX_FLT;
	int32 Id = 0;
	for (int32 i = 0; i < 2; i++)
	{
		if (const float Dist = FVector::Dist(TargetLocation, HeadAndTail[i]); Dist < Distance)
		{
			Id = i;
			Distance = Dist;
		}
	}

	if (Id == 0)
	{
		Path->SetLocationAtSplinePoint(0, TargetLocation, ESplineCoordinateSpace::World);
	}
	else
	{
		Path->SetLocationAtSplinePoint(n - 1, TargetLocation, ESplineCoordinateSpace::World);
	}
	OverlapMemory.Push(FCameraSplineMemory{Id, HeadAndTail[Id]});

	OverlapLayers++;
}

void ASpiderCamera::OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor->IsA<ACameraConfigVolume>()) return;
	const auto CameraConfig = Cast<ACameraConfigVolume>(OtherActor);
	if (!IsValid(CameraConfig)) return;

	if (!OverlapMemory.IsEmpty())
	{
		const FCameraSplineMemory Value = OverlapMemory.Pop();
		const auto Path = CameraConfig->CameraPath;

		const auto n = Path->GetNumberOfSplinePoints();
		if (Value.Id == 0)
			Path->SetLocationAtSplinePoint(0, Value.PrevPosition, ESplineCoordinateSpace::World);
		else
			Path->SetLocationAtSplinePoint(n - 1, Value.PrevPosition, ESplineCoordinateSpace::World);
	}


	if (!(--OverlapLayers))
		CurrentVolume = nullptr;
}
