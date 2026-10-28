// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void CTSAddCoin(const int32& Amount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;

	int32 GetCoins() const { return Coins; }
	float GetBounsMaxSpeed() const { return BonusMaxSpeed; }

	void AddCoins(int32 Amount);
	bool TryRemoveCoins(int32 Amount);

	UPROPERTY(Replicated)
	bool bReady = false;

	//플레이어가 재경기를 요청 결과 기록
	UPROPERTY(Replicated)
	bool bReplayRequested = false;

	void AddBonusMaxSpeed(float Amount);

	//보유한 코인 0으로 초기화
	void ResetCoins();

	//보유한 MaxSpeed 0으로 초기화
	void ResetBonusMaxSpeed();

	//플레이어가 닉네임을 입력 완료했는지 표시
	UPROPERTY(Replicated)
	bool bNicknameConfirmed = false;

	UFUNCTION(BlueprintCallable)
	FString GetPlayerStateInfo() const;
public:
	UPROPERTY(Replicated)
	int32 Coins = 0;

	UPROPERTY(Replicated)
	float BonusMaxSpeed = 0.f;
};
