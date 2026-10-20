// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ChatUI.h"
#include "CXUI.generated.h"

class UTextBlock;
class UButton;


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

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RemainingTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Ready;

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
