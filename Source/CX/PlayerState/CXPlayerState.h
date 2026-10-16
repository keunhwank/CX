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

	void AddBonusMaxSpeed(float Amount);

	UFUNCTION(BlueprintCallable)
	FString GetPlayerStateInfo() const;
public:
	UPROPERTY(Replicated)
	int32 Coins = 0;

	UPROPERTY(Replicated)
	float BonusMaxSpeed = 0.f;
};
