// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderCamera.h"

#include "CameraConfigVolume.h"
#include "SpiderCharacter.h"
#include "SpiderPlayerController.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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

	const UWorld* World = GetWorld();
	check(World);

	const auto Character = Cast<ASpiderCharacter>(OutVT.Target.Get());

	const FVector CameraTargetSocket = Character->GetActorLocation();
	const auto PlayerController = Cast<ASpiderPlayerController>(GetOwningPlayerController());
	const auto CharacterMovement = Character->GetCharacterMovement();

	FVector FinalLocation;
	FRotator FinalRotation;

	const bool bIsGrounded = FMath::IsNearlyZero(CharacterMovement->Velocity.Z) && !CharacterMovement->IsFalling();

	if (CurrentLockedVolume != nullptr && bIsGrounded)
		bIsLocked = true;


	if (bIsLocked)
	{
		CalculateFreeLook(bIsGrounded);

		const FVector CameraLock = CurrentLockedVolume->CameraPath->FindLocationClosestToWorldLocation(
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

	FVector LagSpeeds = bIsGrounded ? GroundedLagSpeed : InAirLagSpeed;
	float RotSpeed = LookLagSpeed;

	CalculateLagSpeeds(LagSpeeds, RotSpeed);

	TargetRotator = FMath::RInterpTo(TargetRotator, FinalRotation, DeltaTime, RotSpeed);
	TargetLocation = CalculateAxisIndependentLag(TargetLocation, FinalLocation, TargetRotator, LagSpeeds, DeltaTime);
	TargetFreeLookRotation = FMath::RInterpTo(TargetFreeLookRotation, FreeLookRotation, DeltaTime, 5.0f);

	if (bIsLocked)
		PlayerController->SetControlRotation(TargetRotator + TargetFreeLookRotation);

	TraceCameraCollision(Character, CameraTargetSocket, TargetLocation);

	OutVT.POV.Location = TargetLocation;
	OutVT.POV.Rotation = TargetRotator + TargetFreeLookRotation;
}

void ASpiderCamera::CalculateFreeLook(const bool& bIsGrounded)
{
	constexpr double Duration = 0.5f;
	const auto World = GetWorld();
	const auto PlayerController = Cast<ASpiderPlayerController>(GetOwningPlayerController());
	const auto LookInput = PlayerController->GetLookInput();
	if (bIsGrounded)
	{
		if (FMath::Abs(LookInput.SquaredLength()) > 0.5f)
		{
			constexpr double DelayAfterLastLook = 3.0f;
			FreeLookRotation += FRotator(LookInput.Y, LookInput.X, 0);
			FreeLookRotation.Pitch = FMath::ClampAngle(FreeLookRotation.Pitch, -5, 30);
			FreeLookRotation.Yaw = FMath::ClampAngle(FreeLookRotation.Yaw, -30, 30);
			InputChangedTimestamp = World->GetTimeSeconds() + DelayAfterLastLook;
		}
	}

	const double t = ((InputChangedTimestamp + Duration) - World->GetTimeSeconds()) / Duration;
	const double f = FMath::Clamp(1.0f - t, 0.0f, 1.0f);

	FreeLookRotation = UKismetMathLibrary::RLerp(FreeLookRotation, FRotator(), f, true);
}

void ASpiderCamera::TraceCameraCollision(const ASpiderCharacter* Character, const FVector& TraceOrigin, FVector& Target,
                                         const float& TraceRadius) const
{
	constexpr ECollisionChannel TraceChannel = ECC_Camera;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Character);
	FHitResult HitResult;
	const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(TraceRadius);
	GetWorld()->SweepSingleByChannel(
		HitResult, TraceOrigin,
		TargetLocation, FQuat::Identity,
		TraceChannel, SphereCollisionShape, Params);
	if (HitResult.IsValidBlockingHit())
		Target += HitResult.Location - HitResult.TraceEnd;
}

void ASpiderCamera::CalculateLagSpeeds(FVector& LagSpeeds, float& RotSpeed) const
{
	constexpr double Duration = 1.61f;
	const double Timeframe = ((ChangeStateTimestamp + Duration) - GetWorld()->GetTimeSeconds()) / Duration;
	const double Coefficient = FMath::Clamp(1.0f - Timeframe, 0.1f, 1.0f);

	LagSpeeds *= Coefficient;
	RotSpeed *= Coefficient;

	if (bIsLocked)
	{
		LagSpeeds *= 0.5f;
		RotSpeed *= 0.1f;
	}
}

void ASpiderCamera::OnPossess(APawn* NewPawn)
{
	if (NewPawn)
	{
		NewPawn->OnActorBeginOverlap.AddDynamic(this, &ASpiderCamera::OnBeginOverlap);
		NewPawn->OnActorEndOverlap.AddDynamic(this, &ASpiderCamera::OnEndOverlap);
		PreviouslyPossessedPawn = NewPawn;
		TargetLocation = NewPawn->GetActorLocation();
	}
	else if (PreviouslyPossessedPawn)
	{
		if (IsValid(PreviouslyPossessedPawn))
		{
			PreviouslyPossessedPawn->OnActorBeginOverlap.RemoveDynamic(this, &ASpiderCamera::OnBeginOverlap);
			PreviouslyPossessedPawn->OnActorEndOverlap.RemoveDynamic(this, &ASpiderCamera::OnEndOverlap);
		}
		PreviouslyPossessedPawn = nullptr;
	}
}

void ASpiderCamera::OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor->IsA<ACameraConfigVolume>()) return;
	const auto CameraConfig = Cast<ACameraConfigVolume>(OtherActor);
	if (!IsValid(CameraConfig)) return;
	CurrentLockedVolume = CameraConfig;
	ChangeStateTimestamp = GetWorld()->GetTimeSeconds();


	const auto Character = Cast<ASpiderCharacter>(OverlappedActor);
	Character->GetCharacterMovement()->bOrientRotationToMovement = true;
	OverlapCounter++;
	UE_LOG(LogTemp, Display, TEXT("ASpiderCamera::OnBeginOverlap %d"), OverlapCounter);
}

void ASpiderCamera::OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor->IsA<ACameraConfigVolume>()) return;
	const auto CameraConfig = Cast<ACameraConfigVolume>(OtherActor);
	if (!IsValid(CameraConfig)) return;

	UE_LOG(LogTemp, Display, TEXT("ASpiderCamera::OnEndOverlap %d"), OverlapCounter);

	if (CurrentLockedVolume && !(--OverlapCounter))
	{
		ChangeStateTimestamp = GetWorld()->GetTimeSeconds();

		const auto Character = Cast<ASpiderCharacter>(OverlappedActor);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		CurrentLockedVolume = nullptr;
		bIsLocked = false;
	}
}
