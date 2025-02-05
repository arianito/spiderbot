#pragma once


#include "Animation/AnimInstance.h"
#include "SpiderAnimBlueprint.generated.h"


UCLASS()
class SPIDERRIG_API USpiderAnimBlueprint: public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};