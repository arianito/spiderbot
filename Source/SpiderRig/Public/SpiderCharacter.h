#pragma once

#include "GameFramework/Character.h"
#include "SpiderCharacter.generated.h"

class USpiderEffectsComponent;
class UControlRigComponent;

UCLASS()
class SPIDERRIG_API ASpiderCharacter: public ACharacter
{
	GENERATED_BODY()
	FRotator ActorMovementDirection{0};

protected:
	ASpiderCharacter();
	
	bool bOrientRotationToMovement = false;
	friend class ASpiderCamera;

public:
	void ApplyCharacterMovement(const FVector2d& Movement);
	void ApplyCameraMovement(const FVector2d& Movement);
	void ApplyJump();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Effects)
	TObjectPtr<USpiderEffectsComponent> SpiderEffectsComp;

};