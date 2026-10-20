// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/CXGameStateBase.h"
#include "Net/UnrealNetwork.h"


void ACXGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ThisClass, CurrentGameState);
  DOREPLIFETIME(ThisClass, RemainingTime);
  DOREPLIFETIME(ThisClass, Winners);
}

void ACXGameStateBase::SetGameState(EGameState NewGaemState)
{
  if (!HasAuthority())
  {
    return;
  }

  CurrentGameState = NewGaemState;
}

void ACXGameStateBase::SetRemainingTime(int32 NewRemainingTime)
{
  if (!HasAuthority())
  {
    return;
  }
  
  RemainingTime = FMath::Max(0, NewRemainingTime);
}
