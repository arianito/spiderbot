
#pragma once

#include "ControlRig.h"
#include "FSpiderLegDef.generated.h"

USTRUCT(BlueprintType)
struct SPIDERRIG_API FSpiderLegDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Control IK"))
	FRigElementKey IK;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Bones"))
	TArray<FRigElementKey> Bones;

};