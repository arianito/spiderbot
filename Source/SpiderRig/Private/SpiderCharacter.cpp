#include "SpiderCharacter.h"
#include "InputMappingContext.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASpiderCharacter::ASpiderCharacter()
{
	const auto Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 120;
	Movement->FallingLateralFriction = 0.5f;
	Movement->JumpZVelocity = 320.0f;
	Movement->MaxStepHeight = 10.0f;
	Movement->PerchRadiusThreshold = 5.0f;
	Movement->AirControl = 0.25f;
	Movement->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = false;
	JumpMaxCount = 2;
	PrimaryActorTick.bCanEverTick = false;
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
	ActorMovementDirection = UKismetMathLibrary::RLerp(ActorMovementDirection, FRotationMatrix::MakeFromX(DirectionVector).Rotator(), GetWorld()->DeltaTimeSeconds * 10.0f, true);
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
	UE_LOG(LogTemp, Warning, TEXT("Jump"));
}
