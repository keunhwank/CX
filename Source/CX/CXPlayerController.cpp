// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Subsystem/ChatSubsystem.h"
#include "GameState/CXGameStateBase.h"
#include "PlayerState/CXPlayerState.h"
#include "CXGameMode.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

void ACXPlayerController::STCChatLog_Implementation(const TArray<FChatData>& ChatLog)
{
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->SetChatLog(ChatLog);
	}
}

void ACXPlayerController::CTSChatMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

	const FString PlayerName = PlayerState->GetPlayerName();
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->AddChatMessage(PlayerName, Message);
	}

	for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
	{
		if (ACXPlayerController* PC = *It)
		{
			PC->STCChatMessage(PlayerName, Message);
		}
	}
}

void ACXPlayerController::STCChatMessage_Implementation(const FString& Sender, const FString& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Sender, *Message);

	VehicleUI->OnNewChatMessage(Sender, Message);
}


void ACXPlayerController::CTSSetReady_Implementation(bool bNewReady)
{
	ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>();
	ACXGameMode* CXGameMode = GetWorld()->GetAuthGameMode<ACXGameMode>();

	if (!IsValid(CXPlayerState) || !IsValid(CXGameMode))
	{
		return;
	}

	if (!CXGameMode->CanChangeReady())
	{
		return;
	}

	CXPlayerState->bReady = bNewReady;

	UE_LOG(LogTemp, Warning, TEXT("%s Ready: %s"),*CXPlayerState->GetPlayerName(),bNewReady ? TEXT("true") : TEXT("false"));

	//모두 준비 검사
	CXGameMode->TryStartGame();
	
}

void ACXPlayerController::ShowMouseForUI()
{
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
}

void ACXPlayerController::HideMouseForUI()
{
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
		
	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);

	VehicleUI->AddToViewport();

	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		const TArray<FChatData>& ChatLog = ChatSubsystem->GetChatLog();

		for (const FChatData& ChatData : ChatLog)
		{
			VehicleUI->OnNewChatMessage(ChatData.Sender, ChatData.Message);
		}

		ChatSubsystem->ClearChatLog();
		HideMouseForUI();
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	InputComponent->BindKey(
		EKeys::Tab, IE_Pressed,
		this, &ThisClass::ShowMouseForUI);

	InputComponent->BindKey(
		EKeys::Tab, IE_Released,
		this, &ThisClass::HideMouseForUI);
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
	if (IsLocalController() && IsValid(VehicleUI))
	{
		if (const ACXGameStateBase* CXGameState = GetWorld()->GetGameState<ACXGameStateBase>())
		{
			VehicleUI->UpdateReminingTime(CXGameState->GetRemainingTime());
		}
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}
