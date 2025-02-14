// Fill out your copyright notice in the Description page of Project Settings.


#include "SpiderEffectsComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"


USpiderEffectsComponent::USpiderEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void USpiderEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
}


void USpiderEffectsComponent::NotifyFallenAfterJump(const FVector& WorldLocation, const float& JumpImpact,
                                                    const bool& bIsLeg)
{
	AsyncTask(ENamedThreads::GameThread, [this, WorldLocation, JumpImpact, bIsLeg]()
	{
		if (PuffEffect)
		{
			UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PuffEffect, this,
				NAME_None, WorldLocation, FRotator(0.f),
				EAttachLocation::Type::KeepWorldPosition, true);
			Effect->SetVariableVec2(FName("ScaleFactor"), FVector2D(FMath::Clamp(JumpImpact / 4.0f, 10.0f, 40.0f)));
		}
	});
}
