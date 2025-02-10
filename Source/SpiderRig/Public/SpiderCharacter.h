#pragma once

#include "GameFramework/Character.h"
#include "SpiderCharacter.generated.h"

class UControlRigComponent;

UCLASS()
class SPIDERRIG_API ASpiderCharacter: public ACharacter
{
	GENERATED_BODY()
	FRotator ActorMovementDirection{0};

protected:
	ASpiderCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


public:
	void ApplyCharacterMovement(const FVector2d& Movement);
	void ApplyCameraMovement(const FVector2d& Movement);
	void ApplyJump();


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UControlRigComponent> ControlRig;

};