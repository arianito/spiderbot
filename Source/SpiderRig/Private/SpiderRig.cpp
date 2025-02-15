// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderRig.h"

#include "SpiderEffectsComponent.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


bool USpiderRig::InitializeSpine()
{
	int32 BoneIndex;

	if (!Spine.IK.IsValid() || (BoneIndex = RigHierarchy->GetIndex(Spine.IK)) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeSpine -> Invalid spine IK"));
		return false;
	}

	// Storing the initial spine location
	const auto& SpineIKIndex = BoneIndex;
	const auto SpineGlobal = RigHierarchy->GetInitialGlobalTransform(SpineIKIndex);
	InitialSpineLocationGlobal = SpineGlobal.GetLocation();


	if (!Spine.Bone.IsValid() || (BoneIndex = RigHierarchy->GetIndex(Spine.Bone)) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeSpine -> Invalid spine Bone"));
		return false;
	}

	// Storing the bone index so we can speed up setting the transform
	SpineIndex = BoneIndex;
	// Configure spine spring interpolator
	SpineSpringInterpolator.SetDefaultSpringConstants(SpineSpringStiffness, SpineSpringDampingRatio);

	return true;
}

bool USpiderRig::InitializeLegs()
{
	int32 BoneIndex;

	if (Legs.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeLegs -> at least two legs are required"));
		return false;
	}

	// Initialize legs
	LegLength = Legs.Num();

	// Resize arrays so the can fit legs
	if (LegLocationsWorld.Num() != LegLength)
		LegLocationsWorld.SetNum(LegLength, EAllowShrinking::No);

	if (FinalLegLocationsGlobal.Num() != LegLength)
		FinalLegLocationsGlobal.SetNum(LegLength, EAllowShrinking::No);

	if (LegState.Num() != LegLength)
		LegState.SetNum(LegLength, EAllowShrinking::No);


	for (int32 i = 0; i < LegLength; i++)
	{
		int j = 0;
		LegLocationsWorld[i] = FVector(0, 0, 0);
		FinalLegLocationsGlobal[i] = FVector(0, 0, 0);
		const FSpiderLegDef& Leg = Legs[i];

		if (!Leg.IK.IsValid() || (BoneIndex = RigHierarchy->GetIndex(Leg.IK)) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeLegs -> Invalid IK for Leg[%d]"), i);
			return false;
		}

		// First value of LegIndices is the IK index
		LegIndices[i][j++] = BoneIndex;
		if (!Leg.Bones.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeLegs -> Invalid length for Leg[%d]"), i);
			return false;
		}

		// second value of LegIndices is the chain length
		const int32 BonesLength = Leg.Bones.Num();
		LegIndices[i][j++] = BonesLength;

		for (int32 k = 0; k < BonesLength; k++)
		{
			const auto& BoneKey = Leg.Bones[k];
			if (!BoneKey.IsValid() || (BoneIndex = RigHierarchy->GetIndex(BoneKey)) == INDEX_NONE)
			{
				UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeLegs -> Invalid bone at Leg[%d][%d]"), i, k);
				return false;
			}
			// the rest are bone indices for faster lookup
			LegIndices[i][j++] = BoneIndex;
		}
	}
	return true;
}

bool USpiderRig::InitializeVariables()
{
	ParentActor = GetHostingActor();
	if (!ParentActor) return false;
	if (!ParentActor->IsA<ACharacter>()) return false;
	ParentCharacter = Cast<ACharacter>(ParentActor);

	CharacterMovementComponent = ParentCharacter->GetCharacterMovement();
	if (!CharacterMovementComponent) return false;

	if (!ToeStickGroundTimeline) return false;
	if (!ToeOffsetTimeline) return false;

	RigHierarchy = GetHierarchy();
	if (!RigHierarchy) return false;

	SpiderEffects = ParentActor->GetComponentByClass<USpiderEffectsComponent>();
	if (!SpiderEffects) return false;

	return true;
}

void USpiderRig::Initialize(bool bRequestInit)
{
	if (!InitializeVariables()) return;
	if (!InitializeSpine()) return;
	if (!InitializeLegs()) return;
	bIsReady = true;
}


bool USpiderRig::Execute(const FName& InEventName)
{
	Super::Execute(InEventName);
	// If initialization failed, don't bother running the simulation!
	if (!bIsReady) return false;

	// initialize runtime variables
	if (!bIsInitialized)
	{
		if (!ParentSceneComponent)
		{
			ParentSceneComponent = GetOwningSceneComponent();
			if (!ParentSceneComponent) return false;
		}
		if (!LivingWorld)
		{
			LivingWorld = GetWorld();
			if (!LivingWorld) return false;
		}
		bIsInitialized = true;
	}


	// Calculate the delta time
	const float ElapsedTime = LivingWorld->GetTimeSeconds();
	const float RigDeltaTime = ElapsedTime - PrevFrame;
	PrevFrame = ElapsedTime;


	
	// Calculate local velocity
	FVector LocalVelocity = RotateWorldToGlobal(CharacterMovementComponent->Velocity);
	float HorizontalSpeed = FMath::Clamp(LocalVelocity.Size2D() / CharacterMovementComponent->MaxWalkSpeed, 0, 1);
	float VerticalSpeed = LocalVelocity.Z;

	
	const bool bIsPawnControlled = ParentCharacter->IsPawnControlled();
	if (bIsControlled && !bIsPawnControlled)
		CharacterMovementComponent->Velocity = FVector(0, 0, 0);
	bIsControlled = bIsPawnControlled;

	// If moving, setup movement timestamp, to smoothly transition between different steps
	if (HorizontalSpeed > 0.01)
		LastMovementTimestamp = ElapsedTime;

	// Calculate rotation based on jumping and falling velocity
	LocalVelocity.Normalize();
	LocalVelocity *= VerticalSpeed * FallingRotationZSpeedCoefficient * -1.0f;
	LocalVelocity.Z = 0;
	const float& Pitch = FMath::Clamp(LocalVelocity.X, -FallingRotationLimit, FallingRotationLimit);
	const float& Roll = FMath::Clamp(-LocalVelocity.Y, -FallingRotationLimit, FallingRotationLimit);
	const FRotator Rotator = FRotator(Pitch, 0, Roll);
	FinalSpineRotation = FMath::RInterpTo(FinalSpineRotation, Rotator, RigDeltaTime, SpineRotationLag);

	// Calculate timeline for movement and stall state transition 
	const float MovementTransitionTimeframe =
		(LastMovementTimestamp + MovementTransitionDuration - ElapsedTime) / MovementTransitionDuration;
	float OneOnMovement = FMath::Clamp(MovementTransitionTimeframe, 0.0f, 1.0f);
	float OneOnStall = 1.0f - OneOnMovement;


	// Calculate motor value to feed it into the curves
	MotorValue += HorizontalSpeed * RigDeltaTime * ThrottleMultiplier;

	// Calculate lagged horizontal speed
	LaggedHorizontalSpeed = FMath::FInterpTo(
		LaggedHorizontalSpeed,
		HorizontalSpeed,
		RigDeltaTime,
		LazyLag + (OneOnStall * LazyStallLagMultiplier)
	);


	FVector SpineLocationGlobal = InitialSpineLocationGlobal;
	if (CharacterMovementComponent->IsFalling())
	{
		if (VerticalSpeed < 0.0f)
		{
			if (!bIsFallStarted)
			{
				JumpZStart = ParentCharacter->GetActorLocation().Z;
				bIsFallStarted = true;
			}
		}
		bIsFalling = true;
		SetSpineTransform(SpineLocationGlobal, FinalSpineRotation, RigDeltaTime * SpineSpringLag);
		for (int i = 0; i < LegLength; i++)
		{
			const auto& LegIKIndex = LegIndices[i][0];
			const auto LegGlobalTransform = RigHierarchy->GetInitialGlobalTransform(LegIKIndex);

			// Offset the legs while falling to match the character speed 
			const FVector LegVelocityOffset = LocalVelocity.GetClampedToSize(0.0f, FallingLegOffsetHorizontalLimit);

			// spread the legs while falling to make it look like its jumping!
			const float LegSpreadMultiplier = FMath::Clamp(
				VerticalSpeed * FallingLegSpreadSpeedCoefficient * -1.0f,
				FallingMinLegSpread,
				FallingMaxLegSpread
			);

			FVector CurrentLegLocation = LegGlobalTransform.GetLocation() * LegSpreadMultiplier + LegVelocityOffset;

			// Legs can also go up and down, for more realistic look and feel!
			CurrentLegLocation.Z = FMath::Clamp(VerticalSpeed * FallingLegLocationCoefficient * -1.0f,
			                                    FallingMinLegOffset, FallingMaxLegOffset);

			SetLegLocation(i, CurrentLegLocation, RigDeltaTime * ToeFallingLag);
		}
	}
	else
	{
		const FVector UpVectorWorld = RotateGlobalToWorld(FVector::UpVector);
		const auto SpineLocationWorld = TransformGlobalToWorld(SpineLocationGlobal);

		if (bIsFalling)
		{
			// Reset movement factors on fall
			OneOnMovement = 0;
			OneOnStall = 1;

			JumpImpact = FMath::Abs(JumpZStart - ParentCharacter->GetActorLocation().Z);
			JumpZStart = 0;
			bIsFallStarted = false;
			SpiderEffects->NotifyFallenAfterJump(SpineLocationWorld, JumpImpact * 2.0f, false);
		}

		for (int i = 0; i < LegLength; i++)
		{
			const auto& LegIKIndex = LegIndices[i][0];
			const auto LegTransformGlobal = RigHierarchy->GetInitialGlobalTransform(LegIKIndex);
			FVector LegLocationWorld = TransformGlobalToWorld(LegTransformGlobal.GetLocation());

			// Find the leg location on the ground
			TraceSingleLeg(LegLocationWorld, SpineLocationWorld, UpVectorWorld);

			// Calculate the time value to evaluate curves
			const float EvenlyDistributedCycle = i / static_cast<float>(LegLength);
			const float OffCycleMultiplier = LaggedHorizontalSpeed * AnimationOffCycleCoefficient;
			const double RepeatedTimeValue =
				FMath::Frac(MotorValue + (1 - OffCycleMultiplier) * EvenlyDistributedCycle);


			// Calculate leg offset
			const float StickToGroundFactor = ToeStickGroundTimeline->GetFloatValue(RepeatedTimeValue);
			const float AllowedToRaiseFactor = 1.0f - StickToGroundFactor;
			const float LegOffsetFactor = ToeOffsetTimeline->GetFloatValue(RepeatedTimeValue);
			const float LegOffsetCoefficient = FMath::Clamp(LegOffsetFactor * AllowedToRaiseFactor, -2.0f, 2.0f);
			LegLocationWorld += UpVectorWorld * OneOnMovement * StepHeight * LegOffsetCoefficient;


			// Lerp the leg position between its previously grounded location to its current ground location
			LegLocationsWorld[i] = FMath::Lerp(
				LegLocationsWorld[i],
				LegLocationWorld,
				FMath::Clamp(AllowedToRaiseFactor + OneOnStall, 0.0f, 1.0f)
			);


			// Convert calculated leg location to rig space
			FVector NewLegLocationGlobal = TransformWorldToGlobal(LegLocationsWorld[i]);


			// Calculate mean spine location based on spider legs
			const float FinalSpineLocationZ = FMath::Max(SpineLocationGlobal.Z, NewLegLocationGlobal.Z);

			// interpolate to its initial location on stall
			SpineLocationGlobal.Z = FMath::Lerp(
				InitialSpineLocationGlobal.Z,
				FinalSpineLocationZ,
				1.0f - LaggedHorizontalSpeed
			);

			// If spider has fallen on the ground, add an extra force, make it look natural
			if (bIsFalling)
			{
				SpineLocationGlobal.Z -= FallingImpactOnSpine;
				SpiderEffects->NotifyFallenAfterJump(LegLocationsWorld[i], JumpImpact, true);
			}

			// Setup bone transforms
			SetSpineTransform(SpineLocationGlobal, FinalSpineRotation, RigDeltaTime * SpineSpringLag);
			SetLegLocation(i, NewLegLocationGlobal, RigDeltaTime * LegMovementLag);
		}
		bIsFalling = false;
	}
	return true;
}

void USpiderRig::SetLegLocation(const int32& LegIndex, const FVector& NewLegLocationGlobal, const float& Dt)
{
	const int32& Length = LegIndices[LegIndex][1];
	const int32* BoneIndices = &LegIndices[LegIndex][2];
	FVector& LegLocationGlobal = FinalLegLocationsGlobal[LegIndex];

	// Interpolate to final leg location
	LegLocationGlobal = FMath::VInterpTo(LegLocationGlobal, NewLegLocationGlobal, Dt, ToePlacementLagSpeed);

	// Initialize temporary arrays
	if (TemporaryChain.Num() != Length)
	{
		TemporaryChain.SetNum(Length, EAllowShrinking::No);
		RotationLimitsPerItem.SetNum(Length, EAllowShrinking::No);
	}

	// Initialize chain with bone transforms
	for (int32 i = 0; i < Length; i++)
	{
		const int& BoneIndex = BoneIndices[i];
		auto Transform = RigHierarchy->GetGlobalTransform(BoneIndex);
		auto LocalTransform = RigHierarchy->GetLocalTransform(BoneIndex);
		TemporaryChain[i] = FCCDIKChainLink(Transform, LocalTransform, i);
		RotationLimitsPerItem[i] = 10;
	}

	// Solve IK using CCD algorithm
	const bool IsBoneLocationUpdated = AnimationCore::SolveCCDIK(
		TemporaryChain,
		LegLocationGlobal,
		IKPrecision,
		IKSolveIteration,
		true,
		false,
		RotationLimitsPerItem
	);

	if (!IsBoneLocationUpdated) return;

	// Update bone transforms if they have to move
	for (int i = 0; i < Length; i++)
	{
		const int& BoneIndex = BoneIndices[i];
		const FCCDIKChainLink& CurrentLink = TemporaryChain[i];
		RigHierarchy->SetGlobalTransform(BoneIndex, CurrentLink.Transform, true);
	}
}

void USpiderRig::SetSpineTransform(const FVector& SpineLocationGlobal, const FRotator& RotationGlobal, const float& Dt)
{
	const auto NewLocation = SpineSpringInterpolator.Update(SpineLocationGlobal, Dt);
	RigHierarchy->SetGlobalTransform(SpineIndex, FTransform(RotationGlobal.Quaternion(), NewLocation), true);
}


bool USpiderRig::TraceSingleLeg(FVector& LegLocationWorld, const FVector& RootLocationWorld,
                                const FVector& UpVectorWorld) const
{
	FVector TraceDirection = (LegLocationWorld - (RootLocationWorld + UpVectorWorld * ToeTraceOriginUpward));
	TraceDirection.Normalize();

	// A ray-cast from Head to Toe
	const FVector TraceOriginWorld = LegLocationWorld - TraceDirection * ToeTraceDepthInward;
	const FVector TraceEndWorld = LegLocationWorld + TraceDirection * ToeTraceDepthOutward;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ParentActor);
	const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(ToeTraceRadius);
	FHitResult HitResult;
	if (LivingWorld->SweepSingleByChannel(HitResult, TraceOriginWorld, TraceEndWorld, FQuat::Identity, ECC_Visibility,
	                                      SphereCollisionShape, Params))
	{
		LegLocationWorld = HitResult.ImpactPoint;
		return true;
	}

	// If unsuccessful, try grabbing a ledge
	// A ray-cast from Toe to Spine
	if (LivingWorld->SweepSingleByChannel(
		HitResult,
		HitResult.TraceEnd, RootLocationWorld, FQuat::Identity, ECC_Visibility, SphereCollisionShape,
		Params))
	{
		LegLocationWorld = HitResult.ImpactPoint;
		return true;
	}
	return false;
}
