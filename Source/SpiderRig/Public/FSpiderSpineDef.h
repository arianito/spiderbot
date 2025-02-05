
#pragma once

#include "ControlRig.h"
#include "FSpiderSpineDef.generated.h"

USTRUCT(BlueprintType)
struct SPIDERRIG_API FSpiderSpineDef
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Control IK"))
	FRigElementKey IK;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spine Bone"))
	FRigElementKey Bone;

};