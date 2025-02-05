#include "SpiderPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "SpiderCamera.h"
#include "SpiderCharacter.h"

void ASpiderPlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);
	PossessedCharacter = Cast<ASpiderCharacter>(NewPawn);
	NotifyCameraCharacterPossession(NewPawn);
	SetupInputSystem();
}

void ASpiderPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	NotifyCameraCharacterPossession(nullptr);
	UnBindInputs();
	PossessedCharacter = nullptr;
}

void ASpiderPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BindInputs();
}


void ASpiderPlayerController::NotifyCameraCharacterPossession(APawn* NewPawn) const
{
	if (!PlayerCameraManager->IsA<ASpiderCamera>()) return;
	const auto& CameraComponent = Cast<ASpiderCamera>(PlayerCameraManager);
	CameraComponent->OnPossess(NewPawn);
}

void ASpiderPlayerController::BindInputs()
{
	if (InputMappingContext.IsNull()) return;
	if (!InputComponent.IsA<UEnhancedInputComponent>()) return;

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	const auto& Mappings = InputMappingContext.LoadSynchronous()->GetMappings();

	TSet<const UInputAction*> UniqueActions;
	for (const FEnhancedActionKeyMapping& Keymapping : Mappings)
		UniqueActions.Add(Keymapping.Action);

	for (const UInputAction* UniqueAction : UniqueActions)
	{
		const auto& Handle = EnhancedInputComponent->BindAction(
			UniqueAction,
			ETriggerEvent::Triggered,
			Cast<UObject>(this),
			UniqueAction->GetFName()
		);
		BindingHandles.Add(Handle.GetHandle());
	}
}

void ASpiderPlayerController::UnBindInputs()
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	for (const uint32& BindingHandle : BindingHandles)
		EnhancedInputComponent->RemoveBindingByHandle(BindingHandle);
	BindingHandles.Empty();
}

void ASpiderPlayerController::SetupInputSystem() const
{
	if (!PossessedCharacter) return;
	if (InputMappingContext.IsNull()) return;

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSystem) return;

	InputSystem->AddMappingContext(InputMappingContext.LoadSynchronous(), 0);
}

void ASpiderPlayerController::MoveInputAction(const FInputActionValue& Value) const
{
	if (!PossessedCharacter) return;
	PossessedCharacter->ApplyCharacterMovement(Value.Get<FVector2D>());
}

void ASpiderPlayerController::LookInputAction(const FInputActionValue& Value) const
{
	if (!PossessedCharacter) return;
	PossessedCharacter->ApplyCameraMovement(Value.Get<FVector2D>());
}

void ASpiderPlayerController::JumpInputAction(const FInputActionValue& Value) const
{
	if (!PossessedCharacter) return;
	PossessedCharacter->ApplyJump();
}
