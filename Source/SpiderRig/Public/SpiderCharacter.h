#pragma once

#include "GameFramework/Character.h"
#include "SpiderCharacter.generated.h"


UCLASS()
class SPIDERRIG_API ASpiderCharacter: public ACharacter
{
	GENERATED_BODY()

protected:
	ASpiderCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


public:
	void ApplyCharacterMovement(const FVector2d& Movement);
	void ApplyCameraMovement(const FVector2d& Movement);
	void ApplyJump();

};