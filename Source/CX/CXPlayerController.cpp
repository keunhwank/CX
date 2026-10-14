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
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
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
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}
