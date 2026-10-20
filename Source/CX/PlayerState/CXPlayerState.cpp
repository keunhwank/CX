// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"

// 콘솔 변수 값을 변수 선언 
static int32 GMyTestCoin = 0;

//FAutoConsoleVariableRef 정의 및 람다 콜백 등록
static FAutoConsoleVariableRef CVarAddCoinServer(TEXT("cheat.addcoin.server"), GMyTestCoin, TEXT("add coin request server"),
  FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar) // 람다 콜백
    {
      UE_LOG(LogTemp, Warning, TEXT("콘솔 변수 변경됨! GMyTestCoin값: %d"), GMyTestCoin);

      if (GWorld)
      {
        if (GWorld->GetFirstPlayerController())
        {
          GWorld->GetFirstPlayerController()->GetPlayerState<ACXPlayerState>()->CTSAddCoin(GMyTestCoin);
        }
      }
      GMyTestCoin = 0;
    }),
  ECVF_Cheat  //플래그 (치트 기능 등으로 분류)
);

static FAutoConsoleVariableRef CVarAddCoinClient(TEXT("cheat.addcoin.Client"), GMyTestCoin, TEXT("add coin request client"),
  FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar) // 람다 콜백
    {
      UE_LOG(LogTemp, Warning, TEXT("콘솔 변수 변경됨! GMyTestCoin값: %d"), GMyTestCoin);

      if (GWorld)
      {
        if (GWorld->GetFirstPlayerController())
        {
          GWorld->GetFirstPlayerController()->GetPlayerState<ACXPlayerState>()->Coins += GMyTestCoin;
        }
      }
      GMyTestCoin = 0;
    }),
  ECVF_Cheat  //플래그 (치트 기능 등으로 분류)
);
void ACXPlayerState::CTSAddCoin_Implementation(const int32& Amount)
{
  //서버 시점
  AddCoins(Amount);
}


void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
  DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
  DOREPLIFETIME(ThisClass, bReady);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
  if (HasAuthority() == false || Amount <= 0)
  {
    return;
  }

  FString PlayerName = GetPlayerName();
  Coins += Amount;
  UE_LOG(LogTemp, Warning, TEXT("%s - AddCoins: %d(+%d)"), *PlayerName, Coins, Amount);
}

bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
  if (HasAuthority() == false || Amount <= 0 || Coins < 5)
  {
    return false;
  }

  Coins -= Amount;
  return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
  if (HasAuthority() == false || Amount <= 0)
  {
    return;
  }

  BonusMaxSpeed += Amount;
}

FString ACXPlayerState::GetPlayerStateInfo() const
{
  return FString::Printf(TEXT("Coins: %d, BonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);
}
