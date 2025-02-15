#include "DemoLegRig.h"

#include "Kismet/GameplayStatics.h"


void UDemoLegRig::Initialize(bool bRequestInit)
{
	RigHierarchy = GetHierarchy();
	if (!RigHierarchy) return;
	bIsReady = true;
}


bool UDemoLegRig::Execute(const FName& InEventName)
{
	Super::Execute(InEventName);

	if (!bIsReady)
		return false;
	if (!World)
		World = GetWorld();
	if (!World)
		return false;

	const float Time = World->GetTimeSeconds();
	SetLegLocation(
		FVector(
			FMath::Sin(Time) * 20.0f,
			FMath::Cos(Time) * 20.0f,
			10.0f + FMath::Cos(Time * 4.0f) * 10.0f
		)
	);

	return true;
}

void UDemoLegRig::SetLegLocation(const FVector& LocationGlobal)
{
	const int32& Length = RigHierarchy->Num();

	if (TemporaryChain.Num() < Length)
	{
		TemporaryChain.SetNum(Length, EAllowShrinking::No);
		RotationLimitsPerItem.SetNum(Length, EAllowShrinking::No);
	}

	// Initialize chain with bone transforms
	for (int32 i = 0; i < Length; i++)
	{
		auto Transform = RigHierarchy->GetGlobalTransform(i);
		auto LocalTransform = RigHierarchy->GetLocalTransform(i);
		TemporaryChain[i] = FCCDIKChainLink(Transform, LocalTransform, i);
		RotationLimitsPerItem[i] = 10;
	}

	// Solve IK using CCD algorithm
	const bool IsBoneLocationUpdated = AnimationCore::SolveCCDIK(
		TemporaryChain,
		LocationGlobal,
		0.1f,
		10,
		true,
		false,
		RotationLimitsPerItem
	);

	if (!IsBoneLocationUpdated) return;

	// Update bone transforms if they have to move
	for (int i = 0; i < Length; i++)
	{
		const FCCDIKChainLink& CurrentLink = TemporaryChain[i];
		RigHierarchy->SetGlobalTransform(i, CurrentLink.Transform, true);
	}
}
