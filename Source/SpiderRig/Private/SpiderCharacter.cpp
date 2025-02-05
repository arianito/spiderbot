#include "SpiderCharacter.h"
#include "InputMappingContext.h"
#include "GameFramework/CharacterMovementComponent.h"

ASpiderCharacter::ASpiderCharacter()
{
	const auto Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 120;
	Movement->FallingLateralFriction = 0.5f;
	Movement->bOrientRotationToMovement = false;
	Movement->JumpZVelocity = 320.0f;
	Movement->MaxStepHeight = 10.0f;
	Movement->PerchRadiusThreshold = 5.0f;
	Movement->AirControl = 0.25f;
	JumpMaxCount = 2;
}

void ASpiderCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASpiderCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ASpiderCharacter::ApplyCharacterMovement(const FVector2d& Movement)
{
	const auto& ControlRotation = GetControlRotation();

	const FRotator Rotator(0, ControlRotation.Yaw, 0);
	const auto DirectionVector = Rotator.RotateVector(FVector(Movement.Y, Movement.X, 0));
	AddMovementInput(DirectionVector);
	SetActorRotation(FRotationMatrix::MakeFromX(DirectionVector).Rotator());
}

void ASpiderCharacter::ApplyCameraMovement(const FVector2d& Movement)
{
	AddControllerYawInput(Movement.X);
	AddControllerPitchInput(-Movement.Y);
}

void ASpiderCharacter::ApplyJump()
{
	Jump();
	UE_LOG(LogTemp, Warning, TEXT("Jump"));
}
