// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

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
};



