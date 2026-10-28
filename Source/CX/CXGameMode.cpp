// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Subsystem/ChatSubsystem.h"
#include "PlayerState/CXPlayerState.h"
#include "GameState/CXGameStateBase.h"
#include "CXPawn.h"

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

	if (!IsValid(CXGameState))
	{
		return;
	}

	CXGameState->SetGameState(EGameState::Playing);
	CXGameState->SetRemainingTime(30);

	for (APlayerState* PS : CXGameState->PlayerArray)
	{
		if (!IsValid(PS))
		{
			continue;
		}

		if (ACXPawn* Vehicle = Cast<ACXPawn>(PS->GetPawn()))
		{
			//카운트다운 종료, 운전가능
			Vehicle->SetMovementLocked(false);
		}
	}

	//플레이 시간 타이머 시작
	GetWorldTimerManager().SetTimer
	(GameTimerHandle, 
		this, 
		&ACXGameMode::HandlePlayningTimerTick, 
		1.f, true
	);
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

	// 결과 화면을 보는 동안 차량 조작 차단
	for (APlayerState* PS : CXGameState->PlayerArray)
	{
		if (!IsValid(PS))
		{
			continue;
		}

		if (ACXPawn* Vehicle = Cast<ACXPawn>(PS->GetPawn()))
		{
			Vehicle->SetMovementLocked(true);
		}
	}
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

void ACXGameMode::Logout(AController* Exiting)
{
	ACXGameStateBase* GS = GetGameState<ACXGameStateBase>();

	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	bWaingCountdownStarted = false;

	if (IsValid(GS))
	{
		//진행 중이던 경기와 결과를 취소하고 대기로 전환
		GS->Winners.Empty();
		GS->SetRemainingTime(0);
		GS->SetGameState(EGameState::Wating);

		for (FConstPlayerControllerIterator It =
			GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();

			//나가는 플레이어는 초기화 대상에서 제외
			if (!IsValid(PC) || PC == Exiting)
			{
				continue;
			}

			if (ACXPlayerState* PS =
				PC->GetPlayerState<ACXPlayerState>())
			{
				PS->bReady = false;
				PS->bReplayRequested = false;
				PS->ResetCoins();
				PS->ResetBonusMaxSpeed();

				//닉네임 확정 여부는 유지
			}

			if (ACXPawn* Vehicle = Cast<ACXPawn>(PC->GetPawn()))
			{
				Vehicle->ReturnToInitialPosition();
				Vehicle->SetMovementLocked(true);
			}
		}
	}

	//엔진의 기본 접속 종료 처리
	Super::Logout(Exiting);
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

		if (!IsValid(CXPlayerState) 
			||!CXPlayerState->bNicknameConfirmed 
			|| !CXPlayerState->bReady 
			|| !IsValid(Cast<ACXPawn>(CXPlayerState->GetPawn())))
		{
			return;
		}
	}

	bWaingCountdownStarted = true;

	//두 차량을 각각 자신의 초기 위치로 복귀
	for (APlayerState* PS : CXGameState->PlayerArray)
	{
		ACXPawn* Vehicle = Cast<ACXPawn>(PS->GetPawn());

		if (IsValid(Vehicle))
		{
			Vehicle->ReturnToInitialPosition();

			//초기 위치로 이동한 뒤 차량 조작 잠금
			Vehicle->SetMovementLocked(true);
		}
	}
	//차량 복귀 후 카운트 다운 시작
	StartWaitngCountdown();
}

void ACXGameMode::RequestReplay(ACXPlayerController* Requester)
{
	ACXGameStateBase* GS = GetGameState<ACXGameStateBase>();

	if (!IsValid(GS) || !IsValid(Requester))
	{
		return;
	}

	//게임 종료 후에만 재경기 투표 가능
	if (GS->GetCurrentGameState() != EGameState::Finished)
	{
		return;
	}

	ACXPlayerState* RequesterPS = Requester->GetPlayerState<ACXPlayerState>();

	if (!IsValid(RequesterPS) || !GS->PlayerArray.Contains(RequesterPS) || !RequesterPS->bNicknameConfirmed)
	{
		return;
	}

	//동일한 사람이 여러번 눌러도 투표는 하나만 저장
	RequesterPS->bReplayRequested = true;

	UE_LOG(LogTemp, Warning, TEXT("%s voted REPLAY"), *RequesterPS->GetPlayerName());

	//두명이 있어야 재경기 가능
	if (GS->PlayerArray.Num() != 2)
	{
		return;
	}

	//한명이라도 거절하거나 플레이어가 없으면 대기
	for (APlayerState* PS : GS->PlayerArray)
	{
		const ACXPlayerState* CXPS = Cast<ACXPlayerState>(PS);

		if (!IsValid(CXPS) || !CXPS->bNicknameConfirmed || !CXPS->bReplayRequested
			|| !IsValid(Cast<ACXPawn>(CXPS->GetPawn())))
		{
			return;
		}
	}

	//재경기 동의 
	GetWorldTimerManager().ClearTimer(GameTimerHandle);

	//이전 경기 결과 제거 
	GS->Winners.Empty();
	GS->SetGameState(EGameState::Wating);

	for (APlayerState* PS : GS->PlayerArray)
	{
		ACXPlayerState* CXPS = Cast<ACXPlayerState>(PS);

		//다음 경기에 사용할 투표 Ready 상태 초기화
		CXPS->bReplayRequested = false;
		CXPS->bReady = false;

		//닉네임은 유지하고 경기 점수만 초기화
		CXPS->ResetCoins();
		CXPS->ResetBonusMaxSpeed();

		ACXPawn* Vehicle = Cast<ACXPawn>(CXPS->GetPawn());

		//기존 복귀, 조작 잠금 기능 재사용하기 
		Vehicle->ReturnToInitialPosition();
		Vehicle->SetMovementLocked(true);
	}

	//Ready를 다시 요구 하지 않고 재경기 카운트 다운 시작
	bWaingCountdownStarted = true;
	StartWaitngCountdown();
}
