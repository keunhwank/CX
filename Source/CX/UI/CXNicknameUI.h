// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXNicknameUI.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

UCLASS()
class CX_API UCXNicknameUI : public UUserWidget
{
	GENERATED_BODY()
	

protected:
	virtual void NativeConstruct() override;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UEditableTextBox> EditableTextBox_Nickname;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UButton> Button_ConfirmNickname;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UTextBlock> Text_NicknameError;

  UFUNCTION()
  void OnConfirmNicknameClicked();


};
