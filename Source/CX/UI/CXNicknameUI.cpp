#include "UI/CXNicknameUI.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "CXPlayerController.h"

void UCXNicknameUI::NativeConstruct()
{
  Super::NativeConstruct();

  //오류 문구 숨기기
  Text_NicknameError->SetVisibility(ESlateVisibility::Collapsed);

  Button_ConfirmNickname->OnClicked.AddUniqueDynamic(this, &ThisClass::OnConfirmNicknameClicked);
}

void UCXNicknameUI::OnConfirmNicknameClicked()
{
  ACXPlayerController* PC = Cast<ACXPlayerController>(GetOwningPlayer());

  if (!IsValid(PC))
  {
    return;
  }

  //입력값을 읽고 앞뒤 공백 제거
  const FString Nickname = EditableTextBox_Nickname->GetText().ToString().TrimStartAndEnd();

  if (Nickname.IsEmpty()
    || Nickname.Len() > 12
    || Nickname.Contains(TEXT("\n"))
    || Nickname.Contains(TEXT("\r"))
    || Nickname.Contains(TEXT("\t")))
  {
    Text_NicknameError->SetText(NSLOCTEXT("NicknameUI", "Invalid","Please enter it again"));

    Text_NicknameError->SetVisibility(ESlateVisibility::HitTestInvisible);

    return;
  }

  Text_NicknameError->SetVisibility(
    ESlateVisibility::Collapsed);

  PC->CTSSubmitNickname(Nickname);
}
