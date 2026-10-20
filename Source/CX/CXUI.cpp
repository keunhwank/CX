// Copyright Epic Games, Inc. All Rights Reserved.


#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "CXPlayerController.h"


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

void UCXUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Ready))
	{
		Button_Ready->OnClicked.AddUniqueDynamic(this, &ThisClass::OnReadyClicked);
	}
}

void UCXUI::OnReadyClicked()
{
	ACXPlayerController* PC = Cast<ACXPlayerController>(GetOwningPlayer());

	if (IsValid(PC))
	{
		PC->CTSSetReady(true);
	}
}
