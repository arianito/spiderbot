#pragma once

#include "ControlRig.h"
#include "CCDIK.h"
#include "DemoLegRig.generated.h"

UCLASS(Blueprintable)
class RIGTUTORIAL_API UDemoLegRig : public UControlRig
{
	GENERATED_BODY()

	void SetLegLocation(const FVector& LocationGlobal);
	
	TArray<float> RotationLimitsPerItem;
	TArray<FCCDIKChainLink> TemporaryChain;
	//
	UWorld* World;
	URigHierarchy* RigHierarchy = nullptr;
	bool bIsReady = false;

protected:
	virtual bool Execute(const FName& InEventName) override;
	virtual void Initialize(bool bRequestInit) override;
};
