// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Subsystem/ChatSubsystem.h"
#include "PlayerState/CXPlayerState.h"
#include "GameState/CXGameStateBase.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
}

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();

	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	if (IsValid(CXGameState))
	{
		CXGameState->SetGameState(EGameState::Wating);
		CXGameState->SetRemainingTime(5);
	}

}

void ACXGameMode::StartWaitngCountdown()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	CXGameState->SetGameState(EGameState::Wating);
	CXGameState->SetRemainingTime(5);
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandleWaitingTimerTick, 1.f, true);
	UE_LOG(LogTemp, Warning, TEXT("Waitng Start"));
}

void ACXGameMode::HandleWaitingTimerTick()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		StartPlayingState();
	}
}

void ACXGameMode::StartPlayingState()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	CXGameState->SetGameState(EGameState::Playing);
	CXGameState->SetRemainingTime(30);
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandlePlayningTimerTick, 1.f, true);
}

void ACXGameMode::HandlePlayningTimerTick()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		FinishGame();
	}
}

void ACXGameMode::FinishGame()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();

	int32 HighestCoins = -1;
	TArray<ACXPlayerState*>Winners;

	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		if (ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState))
		{
			const int32 PlayerCoins = CXPlayerState->GetCoins();
			if (HighestCoins < PlayerCoins)
			{
				HighestCoins = PlayerCoins;
				Winners.Empty();
				Winners.Add(CXPlayerState);
			}
			else if (HighestCoins == PlayerCoins)
			{
				Winners.Add(CXPlayerState);
			}
		}
	}
	CXGameState->Winners = Winners;
	CXGameState->SetRemainingTime(0);
	CXGameState->SetGameState(EGameState::Finished);
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (IsValid(NewPlayer) == false)
	{
		return;
	}

	if (ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer))
	{
		// 서버 시점에 클라로 채팅 로그를 보내줌
		if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
		{
			const TArray<FChatData> ChatLog = ChatSubsystem->GetChatLog();
			if (!ChatLog.IsEmpty())
			{
				CXPlayerController->STCChatLog(ChatLog);
			}
		}
	}
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}

bool ACXGameMode::CanChangeReady() const
{
	const ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();

	if (IsValid(CXGameState) && CXGameState->GetCurrentGameState() == EGameState::Wating && !bWaingCountdownStarted)
	{
		return true;
	}
	return false;
}

void ACXGameMode::TryStartGame()
{
	if (!CanChangeReady())
	{
		return;
	}

	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();

	if (CXGameState->PlayerArray.Num() != 2)
	{
		return;
	}

	for (APlayerState* PS : CXGameState->PlayerArray)
	{
		const ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PS);
		if (!IsValid(CXPlayerState) || !CXPlayerState->bReady || !IsValid(CXPlayerState->GetPawn()))
		{
			return;
		}
	}
	bWaingCountdownStarted = true;

	// 초기 위치 복귀 연결

	StartWaitngCountdown();
}
