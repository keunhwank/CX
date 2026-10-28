// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class ACXPlayerController;

UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();
	
	virtual void BeginPlay() override;

protected:
	void StartWaitngCountdown();
	void HandleWaitingTimerTick();
	void StartPlayingState();
	void HandlePlayningTimerTick();
	void FinishGame();

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Called after a successful login.  This is the first place it is safe to call replicated functions on the PlayerController. */
	virtual void PostLogin(APlayerController* NewPlayer);

	virtual void Logout(AController* Exiting) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

private:
	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

	FTimerHandle GameTimerHandle;

	bool bWaingCountdownStarted = false;

public:
	bool CanChangeReady() const;

	void TryStartGame();

	//서버에서 투표를 저장 하고 전원 동의 여부 확인
	void RequestReplay(ACXPlayerController* Requester);
};



