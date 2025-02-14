// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpiderEffectsComponent.generated.h"

class UNiagaraSystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPIDERRIG_API USpiderEffectsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USpiderEffectsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	void NotifyFallenAfterJump(const FVector& WorldLocation, const float &JumpImpact, const bool& bIsLeg );

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PuffEffect;

};
