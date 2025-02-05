#pragma once

#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "SpiderPlayerController.generated.h"

class UInputMappingContext;
class ASpiderCharacter;

UCLASS()
class SPIDERRIG_API ASpiderPlayerController : public APlayerController
{
	GENERATED_BODY()

private:

	TArray<uint32> BindingHandles;
	TObjectPtr<ASpiderCharacter> PossessedCharacter = nullptr;
	
	UFUNCTION()
	void MoveInputAction(const FInputActionValue& Value) const;
	UFUNCTION()
	void LookInputAction(const FInputActionValue& Value);
	UFUNCTION()
	void JumpInputAction(const FInputActionValue& Value) const;
	

protected:
	virtual void OnPossess(APawn* NewPawn) override;
	virtual void OnUnPossess() override;

	virtual void SetupInputComponent() override;

	void SetupInputSystem() const;
	void BindInputs();
	void UnBindInputs();
	void NotifyCameraCharacterPossession(APawn* NewPawn) const;

public:
	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	TSoftObjectPtr<UInputMappingContext> InputMappingContext;

	
	FVector2D CameraMovement{0};
};
