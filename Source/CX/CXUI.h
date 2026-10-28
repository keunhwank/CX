// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ChatUI.h"
#include "CXUI.generated.h"

class UTextBlock;
class UButton;
class ACXGameStateBase;
class UCXNicknameUI;

UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
	GENERATED_BODY()
	

public:
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UChatUI> UI_Chat;

protected:

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

public:

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

	void UpdateReminingTime(int32 RemainingTime);

	//승리, 패배 표시
	void UpdateGameResult(const ACXGameStateBase* GameState);

	//서버가 이름을 확정했으면 확인하고 화면 갱신
	void UpdateNicknameState();

	//종료버튼과 투표 현황 갱신
	void UpdateReplayUI(const ACXGameStateBase* GameState);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Result;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RemainingTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Ready;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXNicknameUI> UI_Nickname;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Replay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReplayStatus;

	UFUNCTION()
	void OnReplayClicked();

	UFUNCTION()
	void OnExitClicked();

	// 종료화면 진입시 커서를 한 번만 표시하기 위한 값
	bool bWasFinished = false;

	//완료처리를 매프레임 반복하지 않도록 기록
	bool bNicknameUICompleted = false;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnReadyClicked();

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);

};
