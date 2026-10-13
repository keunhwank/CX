// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatUI.h"
#include "CXPlayerController.h"

void UChatUI::NativeConstruct()
{
  Super::NativeConstruct();

  if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UChatUI::OnChatInputTextCommitted) == false)
  {
     EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UChatUI::OnChatInputTextCommitted);
  }
}

void UChatUI::NativeDestruct()
{
  if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UChatUI::OnChatInputTextCommitted) == true)
  {
    EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UChatUI::OnChatInputTextCommitted);
  }

  Super::NativeDestruct();
}

void UChatUI::OnNewChatMessage(const FString&, const FString& Message)
{
}

void UChatUI::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
  if (CommitMethod == ETextCommit::OnEnter)
  {
    if (Text.IsEmpty() == false)
    {
      FString ChatMessage = Text.ToString();
      EditableTextBox_ChatInput->SetText(FText::GetEmpty());
      if (ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>())
      {
        PC->CTSChatMessage(ChatMessage);
      }
    }
  }
}
