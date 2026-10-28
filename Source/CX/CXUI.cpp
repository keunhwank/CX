// Copyright Epic Games, Inc. All Rights Reserved.


#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "CXPlayerController.h"
#include "GameState/CXGameStateBase.h"
#include "PlayerState/CXPlayerState.h"
#include "UI/CXNicknameUI.h"
#include "Kismet/KismetSystemLibrary.h"


void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	UI_Chat->OnNewChatMessage(InSender, InMessage);
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UCXUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}

void UCXUI::UpdateReminingTime(int32 RemainingTime)
{
	Text_RemainingTime->SetText(
		FText::Format(
			NSLOCTEXT("CXUI", "RemainingTime", "Remaining time: {0} second"),
			FText::AsNumber(RemainingTime)
		)
	);
}

void UCXUI::UpdateGameResult(const ACXGameStateBase* GameState)
{
	if (!IsValid(GameState) || !IsValid(Text_Result))
	{
		return;
	}

	//게임 종료 전에는 결과 숨김
	if (GameState->GetCurrentGameState() != EGameState::Finished)
	{
		Text_Result->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	//화면에 표시하되 마우스 클릭을 가로채지 않음
	Text_Result->SetVisibility(ESlateVisibility::HitTestInvisible);

	const APlayerController* PC = GetOwningPlayer();
	const ACXPlayerState* MyPlayerState = IsValid(PC) ? PC->GetPlayerState<ACXPlayerState>() : nullptr;

	bool bResultReady = IsValid(MyPlayerState) && !GameState->Winners.IsEmpty();

	for (const auto& Winner : GameState->Winners)
	{
		if (!IsValid(Winner.Get()))
		{
			bResultReady = false;
			break;
		}
	}

	//데이터가 준비되기전에 패배로 표시하지 않음
	if (!bResultReady)
	{
		Text_Result->SetText(NSLOCTEXT("CXUI", "ResultPending", "Loding..."));
		return;
	}

	bool bWinner = false;

	// 공동 우승도 있으므로 첫 번째 원소만 비교하지 않음
	for (const auto& Winner : GameState->Winners)
	{
		if (Winner.Get() == MyPlayerState)
		{
			bWinner = true;
			break;
		}
	}

	if (bWinner)
	{
		const bool bJointWinner = GameState->Winners.Num() > 1;

		Text_Result->SetText(
			bJointWinner
			? NSLOCTEXT("CXUI", "JointVictory", "Draw!")
			: NSLOCTEXT("CXUI", "Victory", "Winner!"));

		Text_Result->SetColorAndOpacity(
			FSlateColor(FLinearColor::Yellow));
	}
	else
	{
		Text_Result->SetText(
			NSLOCTEXT("CXUI", "Defeat", "Loser"));

		Text_Result->SetColorAndOpacity(
			FSlateColor(FLinearColor::Red));
	}
}

void UCXUI::UpdateNicknameState()
{
	if (bNicknameUICompleted)
	{
		return;
	}

	ACXPlayerController* PC = Cast<ACXPlayerController>(GetOwningPlayer());

	if (!IsValid(PC))
	{
		return;
	}

	const ACXPlayerState* PS = PC->GetPlayerState<ACXPlayerState>();

	if (!IsValid(PS) || !PS->bNicknameConfirmed)
	{
		return;
	}

	bNicknameUICompleted = true;

	UI_Nickname->SetVisibility(ESlateVisibility::Collapsed);

	//게임입력으로 복귀, 이후에는 기존 Tab 토글 사용
	PC->HideMouseForUI();
}

void UCXUI::UpdateReplayUI(const ACXGameStateBase* GS)
{
	if (!IsValid(GS))
	{
		return;
	}

	ACXPlayerController* PC =
		Cast<ACXPlayerController>(GetOwningPlayer());

	if (!IsValid(PC))
	{
		return;
	}

	const ACXPlayerState* MyPS =
		PC->GetPlayerState<ACXPlayerState>();

	const bool bFinished =
		GS->GetCurrentGameState() == EGameState::Finished;

	//닉네임 확정 후 대기상태에서 Ready 가능
	Button_Ready->SetIsEnabled(
		IsValid(MyPS)
		&& MyPS->bNicknameConfirmed
		&& !MyPS->bReady
		&& GS->GetCurrentGameState() == EGameState::Wating);

	const ESlateVisibility ReplayVisibility = bFinished
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed;

	Button_Replay->SetVisibility(ReplayVisibility);
	Button_Exit->SetVisibility(ReplayVisibility);

	if (bFinished)
	{
		//종료 상태로 바뀐 순간에만 커서 표시
		if (!bWasFinished)
		{
			PC->ShowMouseForUI();
		}

		int32 VoteCount = 0;

		for (APlayerState* PS : GS->PlayerArray)
		{
			const ACXPlayerState* CXPS = Cast<ACXPlayerState>(PS);

			if (IsValid(CXPS) && CXPS->bReplayRequested)
			{
				++VoteCount;
			}
		}

		//투표 완료한 사람은 다시 누르지 못하게 표시
		Button_Replay->SetIsEnabled(
			IsValid(MyPS)
			&& !MyPS->bReplayRequested
			&& GS->PlayerArray.Num() == 2);

		Text_ReplayStatus->SetVisibility(
			ESlateVisibility::HitTestInvisible);

		Text_ReplayStatus->SetText(FText::Format(
			NSLOCTEXT("CXUI", "ReplayVotesV2", "Replay votes: {0}/2"),
			FText::AsNumber(VoteCount)));
	}
	else
	{
		//재경기 시작 또는 상대 이탈로 종료 화면을 벗어남
		if (bWasFinished)
		{
			PC->HideMouseForUI();
		}

		const bool bWaitingForPlayer =
			GS->GetCurrentGameState() == EGameState::Wating
			&& GS->PlayerArray.Num() < 2;

		Text_ReplayStatus->SetVisibility(
			bWaitingForPlayer
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);

		if (bWaitingForPlayer)
		{
			Text_ReplayStatus->SetText(
				NSLOCTEXT("CXUI", "WaitingForOpponentV2",
					"Waiting for another player..."));
		}
	}

	bWasFinished = bFinished;
}

void UCXUI::OnReplayClicked()
{
	if (ACXPlayerController* PC = Cast<ACXPlayerController>(GetOwningPlayer()))
	{
		//투표는 서버에서 결정
		PC->CTSRequestReplay();
	}
}

void UCXUI::OnExitClicked()
{
	APlayerController* PC = GetOwningPlayer();

	if (IsValid(PC) && PC->IsLocalController())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void UCXUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Ready))
	{
		Button_Ready->OnClicked.AddUniqueDynamic(this, &ThisClass::OnReadyClicked);
	}

	//입력창을 띄우고 Ready는 잠금
	UI_Nickname->SetVisibility(ESlateVisibility::Visible);
	Button_Ready->SetIsEnabled(false);

	bNicknameUICompleted = false;

	Button_Replay->OnClicked.AddUniqueDynamic(
		this, &ThisClass::OnReplayClicked);

	Button_Exit->OnClicked.AddUniqueDynamic(
		this, &ThisClass::OnExitClicked);

	//게임종료 전에는 숨김
	Button_Replay->SetVisibility(ESlateVisibility::Collapsed);
	Button_Exit->SetVisibility(ESlateVisibility::Collapsed);
	Text_ReplayStatus->SetVisibility(ESlateVisibility::Collapsed);

	bWasFinished = false;
}

void UCXUI::OnReadyClicked()
{
	ACXPlayerController* PC = Cast<ACXPlayerController>(GetOwningPlayer());

	if (IsValid(PC))
	{
		PC->CTSSetReady(true);
	}
}
