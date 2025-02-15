#include "SpiderCharacter.h"

#include "SpiderEffectsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASpiderCharacter::ASpiderCharacter()
{
	const auto Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 160;
	Movement->FallingLateralFriction = 1.0f;
	Movement->JumpZVelocity = 320.0f;
	Movement->MaxStepHeight = 10.0f;
	Movement->PerchRadiusThreshold = 5.0f;
	Movement->AirControl = 0.25f;
	Movement->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = false;
	JumpMaxCount = 2;
	PrimaryActorTick.bCanEverTick = false;

	SpiderEffectsComp = CreateDefaultSubobject<USpiderEffectsComponent>(TEXT("SpiderEffectsComponent"));
}

void ASpiderCharacter::ApplyCharacterMovement(const FVector2d& Movement)
{
	const auto CharMovement = GetCharacterMovement();
	const auto& ControlRotation = GetControlRotation();
	const FRotator Rotator(0, ControlRotation.Yaw, 0);
	FVector MoveDir = FVector(Movement.Y, Movement.X, 0);
	MoveDir = Rotator.RotateVector(MoveDir);
	const auto DirectionVector = MoveDir;
	AddMovementInput(DirectionVector);
	const auto OrientRot = bOrientRotationToMovement
		                       ? FRotationMatrix::MakeFromX(DirectionVector).Rotator()
		                       : FRotator(0, GetControlRotation().Yaw, 0);
	ActorMovementDirection = UKismetMathLibrary::RLerp(ActorMovementDirection, OrientRot,
	                                                   GetWorld()->GetDeltaSeconds() * 10.0f, true);
	SetActorRotation(ActorMovementDirection);
}

void ASpiderCharacter::ApplyCameraMovement(const FVector2d& Movement)
{
	AddControllerYawInput(Movement.X);
	AddControllerPitchInput(-Movement.Y);
}

void ASpiderCharacter::ApplyJump()
{
	Jump();
}
