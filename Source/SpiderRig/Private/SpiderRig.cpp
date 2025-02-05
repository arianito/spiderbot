// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderRig.h"

#include "Math/Transform.h"
#include "Math/Vector.h"


void USpiderRig::Initialize(bool bRequestInit)
{
	UE_LOG(LogTemp, Display, TEXT("USpiderRig::InitializeBones -> Loading..."));

	mSpineSpring.SetDefaultSpringConstants(0.9, 0.25);
	if (!InitializeSpine()) return;
	if (!InitializeBones()) return;
	bIsReady = true;
}

bool USpiderRig::InitializeSpine()
{
	int32 BoneIndex;
	const auto RigHierarchy = GetHierarchy();

	if (!mSpine.IK.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid spine IK"));
		return false;
	}
	if ((BoneIndex = RigHierarchy->GetIndex(mSpine.IK)) != INDEX_NONE)
	{
		mSpineIndices[0] = BoneIndex;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid spine IK: %s"),
		       *mSpine.IK.Name.ToString());
		return false;
	}


	if (!mSpine.Bone.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid spine Bone"));
		return false;
	}
	if ((BoneIndex = RigHierarchy->GetIndex(mSpine.Bone)) != INDEX_NONE)
	{
		mSpineIndices[1] = BoneIndex;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid spine Bone: %s"),
		       *mSpine.Bone.Name.ToString());
		return false;
	}

	const auto IKOffset = RigHierarchy->GetGlobalControlOffsetTransformByIndex(mSpineIndices[0]);
	const auto BoneOffset = RigHierarchy->GetGlobalControlOffsetTransformByIndex(mSpineIndices[1]);
	mSpineIKOffset = BoneOffset.GetLocation() - IKOffset.GetLocation();
	return true;
}

bool USpiderRig::InitializeBones()
{
	int32 BoneIndex;
	const auto RigHierarchy = GetHierarchy();

	if (!mLegs.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> No leg defined"));
		return false;
	}
	mLegLength = mLegs.Num();
	for (int32 i = 0; i < mLegs.Num(); i++)
	{
		int j = 0;
		const FSpiderLegDef& Leg = mLegs[i];
		if (!Leg.IK.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid control IK at index: %d"), i);
			return false;
		}

		if ((BoneIndex = RigHierarchy->GetIndex(Leg.IK)) != INDEX_NONE)
		{
			mLegIndices[i][j++] = BoneIndex;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid control IK: %s"),
			       *Leg.IK.Name.ToString());
			return false;
		}
		if (!Leg.Bones.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid bone length for: %s"),
			       *Leg.IK.Name.ToString());
			return false;
		}

		mLegIndices[i][j++] = Leg.Bones.Num();
		for (int32 k = 0; k < Leg.Bones.Num(); k++)
		{
			const auto& BoneKey = Leg.Bones[k];
			if (!BoneKey.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid bone IK at index: Legs[%d][%d]"), i,
				       k);
				return false;
			}
			if ((BoneIndex = RigHierarchy->GetIndex(BoneKey)) != INDEX_NONE)
			{
				mLegIndices[i][j++] = BoneIndex;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("USpiderRig::InitializeBones -> Invalid bone \"%s\" for IK %s"),
				       *BoneKey.Name.ToString(), *Leg.IK.Name.ToString());
				return false;
			}
		}
	}
	return true;
}


bool USpiderRig::Execute(const FName& InEventName)
{
	if (!bIsReady) return false;
	SolveSpineLocation(mSpineIndices[0], mSpineIndices[1]);
	for (int i = 0; i < mLegLength; i++)
		SolveCCDForLeg(mLegIndices[i][0], &mLegIndices[i][2], mLegIndices[i][1]);
	return true;
}

void USpiderRig::SolveCCDForLeg(const int32& IKIndex, const int32* BoneIndices, const int32& Length)
{
	if (mChain.Num() != Length)
	{
		mChain.SetNum(Length, EAllowShrinking::No);
		mRotationLimitsPerItem.SetNum(Length, EAllowShrinking::No);
	}

	URigHierarchy* RigHierarchy = GetHierarchy();
	for (int32 i = 0; i < Length; i++)
	{
		const int& BoneIndex = BoneIndices[i];
		auto Transform = RigHierarchy->GetGlobalTransform(BoneIndex);
		auto LocalTransform = RigHierarchy->GetLocalTransform(BoneIndex);
		mChain[i] = FCCDIKChainLink(Transform, LocalTransform, i);
		mRotationLimitsPerItem[i] = 30;
	}
	const auto TargetTransform = RigHierarchy->GetGlobalTransform(IKIndex);
	const bool IsBoneLocationUpdated = AnimationCore::SolveCCDIK(
		mChain,
		TargetTransform.GetLocation(),
		1.0f,
		10.0f,
		true,
		false,
		mRotationLimitsPerItem
	);

	if (IsBoneLocationUpdated)
	{
		for (int i = 0; i < Length; i++)
		{
			const int& BoneIndex = BoneIndices[i];
			const FCCDIKChainLink& CurrentLink = mChain[i];
			RigHierarchy->SetGlobalTransform(BoneIndex, CurrentLink.Transform, true);
		}
	}
}

void USpiderRig::SolveSpineLocation(const int32& IKIndex, const int32& BoneIndex)
{
	URigHierarchy* RigHierarchy = GetHierarchy();
	auto IKTransform = RigHierarchy->GetGlobalTransform(IKIndex);
	const auto TargetLocation = IKTransform.GetLocation() + mSpineIKOffset;
	const auto NewLocation = mSpineSpring.Update(TargetLocation, 0.1);
	if (mSpineSpring.IsInMotion())
	{
		IKTransform.SetLocation(NewLocation);
		RigHierarchy->SetGlobalTransform(BoneIndex, IKTransform, true);
	}
}
