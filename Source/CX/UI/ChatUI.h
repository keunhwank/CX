// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/MultilineEditableTextBox.h"
#include "ChatUI.generated.h"

/**
 * 
 */
UCLASS()
class CX_API UChatUI : public UUserWidget
{
	GENERATED_BODY()

public:
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox>EditableTextBox_ChatInput;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox>ChatScrollBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMultiLineEditableTextBox>MultilineEditableTextBox_ChatLog;
};
