// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpiderEffectsComponent.generated.h"

class UNiagaraSystem;

UCLASS()
class SPIDERRIG_API USpiderEffectsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USpiderEffectsComponent();
	void NotifyFallenAfterJump(const FVector& WorldLocation, const float &JumpImpact, const bool& bIsLeg );

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PuffEffect;

};
