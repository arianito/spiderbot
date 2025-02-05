#pragma once

#include "ControlRig.h"
#include "CCDIK.h"
#include "FSpiderLegDef.h"
#include "FSpiderSpineDef.h"
#include "Engine/SpringInterpolator.h"
#include "SpiderRig.generated.h"

#define MAX_SPIDER_LEG_LENGTH 8
#define MAX_SPIDER_LEG_BONE_LENGTH 8


UCLASS(Blueprintable)
class SPIDERRIG_API USpiderRig : public UControlRig
{
	GENERATED_BODY()

protected:
	virtual bool Execute(const FName& InEventName) override;
	virtual void Initialize(bool bRequestInit) override;

private:
	bool InitializeBones();
	bool InitializeSpine();
	void SolveCCDForLeg(const int32& IKIndex, const int32* BoneIndices, const int32& Length);
	void SolveSpineLocation(const int32& IKIndex, const int32& BoneIndex);

	bool bIsReady = false;
	int32 mLegIndices[MAX_SPIDER_LEG_LENGTH][MAX_SPIDER_LEG_BONE_LENGTH + 1];
	int32 mLegLength = 0;
	int32 mSpineIndices[2];
	FVector mSpineIKOffset;
	FVectorRK4SpringInterpolator mSpineSpring;


	TArray<FCCDIKChainLink> mChain;
	TArray<float> mRotationLimitsPerItem;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Legs"))
	TArray<FSpiderLegDef> mLegs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spine"))
	FSpiderSpineDef mSpine;
};
