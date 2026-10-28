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
	const FString PlayerName = PlayerState->GetPlayerName();

	UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *PlayerName, *Message);

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

	//닉네임 지정하기 전에는 Ready 요청 거절
	if (!CXPlayerState->bNicknameConfirmed)
	{
		return;
	}

	CXPlayerState->bReady = bNewReady;

	UE_LOG(LogTemp, Warning, TEXT("%s Ready: %s"),*CXPlayerState->GetPlayerName(),bNewReady ? TEXT("true") : TEXT("false"));

	//모두 준비 검사
	CXGameMode->TryStartGame();
	
}

void ACXPlayerController::CTSSubmitNickname_Implementation(const FString& Nickname)
{
	ACXPlayerState* PS = GetPlayerState<ACXPlayerState>();

	if (!IsValid(PS))
	{
		return;
	}

	//한번 정하면 다시 변경 안함
	if (PS->bNicknameConfirmed)
	{
		return;
	}

	//이름 앞뒤 공백 제거
	const FString TrimmedNickname = Nickname.TrimStartAndEnd();

	//공백만 입력했거나 12자 이상 초과하면 거절
	if (TrimmedNickname.IsEmpty() || TrimmedNickname.Len() > 12)
	{
		return;
	}

	//서버에서 기본 PlayerName에 닉네임 저장
	PS->SetPlayerName(TrimmedNickname);

	//서버가 정상적으로 저장하면 입력 완료처리
	PS->bNicknameConfirmed = true;

	UE_LOG(LogTemp, Warning, TEXT("Nickname confirmed: %s"), *PS->GetPlayerName());
}

void ACXPlayerController::CTSRequestReplay_Implementation()
{
	ACXGameMode* GM =	GetWorld()->GetAuthGameMode<ACXGameMode>();

	if (IsValid(GM))
	{
		// 서버에서 이 Controller 자신의 투표만 처리
		GM->RequestReplay(this);
	}
}

void ACXPlayerController::ToggleMouseForUI()
{
	if (bShowMouseCursor)
	{
		HideMouseForUI();
	}
	else
	{
		ShowMouseForUI();
	}
}

void ACXPlayerController::ShowMouseForUI()
{
	if (!IsLocalController())
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("ShowMouseForUI"));

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
	UE_LOG(LogTemp, Warning, TEXT("HideMouseForUI"));

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

	bShowMouseCursor = true;

	//닉네임 입력 중에는 UI만 조작
	FInputModeUIOnly InputMode;
	
	InputMode.SetLockMouseToViewportBehavior(
		EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);

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
	
	InputComponent->BindKey(
		EKeys::Tab, IE_Pressed,
		this, &ThisClass::ToggleMouseForUI);
	
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
			//결과 표시
			VehicleUI->UpdateGameResult(CXGameState);

			//결과 화면 버튼과 투표 상태 갱신
			VehicleUI->UpdateReplayUI(CXGameState);
		}

		if (IsLocalController() && IsValid(VehicleUI))
		{
			//복제된 PlayerState의 닉네임 확정 상태 확인
			VehicleUI->UpdateNicknameState();
		}
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}
