// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/GamePlayStatics.h"
#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"


#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ACXPawn::InputAttack);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

float ACXPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigatorm, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigatorm, DamageCauser);
	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);
	if (ActualDamage <= 0 || DamageCauser == this || HasAuthority() == false || Attacker == nullptr)
	{
		return 0.f;
	}

	//맞은사람
	ACXPlayerState* VictimPlayerState = GetPlayerState<ACXPlayerState>();
	//때린사람
	ACXPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ACXPlayerState>();

	const int32 TransferCoinAmount = (int32)ActualDamage;
	if (VictimPlayerState->TryRemoveCoins((int32)ActualDamage))
	{
		UE_LOG(LogTemplateVehicle, Warning, TEXT("%s transferred %d coins to %s"), 
			*GetNameSafe(VictimPlayerState), TransferCoinAmount, *GetNameSafe(AttackerPlayerState));
		AttackerPlayerState->AddCoins(TransferCoinAmount);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Warning, TEXT("%s Has no coins to transfer %s"),
			*GetNameSafe(VictimPlayerState), *GetNameSafe(AttackerPlayerState));
	}
	
	
	return ActualDamage;
}

void ACXPawn::ReturnToInitialPosition()
{
	//클라이언트가 임의로 위치를 변경하지 않도록 제한
	if (!HasAuthority())
	{
		return;
	}

	//현재 가속, 조항, 브레이크, 입력을 초기화
	if (IsValid(ChaosVehicleMovement))
	{
		ChaosVehicleMovement->SetThrottleInput(0.f);
		ChaosVehicleMovement->SetSteeringInput(0.f);
		ChaosVehicleMovement->SetBrakeInput(0.f);
		ChaosVehicleMovement->SetHandbrakeInput(false);
	}

	ACXPlayerState* PS = GetPlayerState<ACXPlayerState>();
	if (IsValid(PS))
	{
		//코인 또는 보너스 스피드를 보유하고 있으면 0으로 초기화
		if (PS->GetCoins() > 0 || PS->GetBounsMaxSpeed() > 0)
		{
			PS->ResetCoins();

			UE_LOG(LogTemp, Warning, TEXT("%s Coins Reset: %d"),
				*PS->GetPlayerName(),
				PS->GetCoins());

			PS->ResetBonusMaxSpeed();

			UE_LOG(LogTemp, Warning, TEXT("%s BonusMaxSpeed Reset: %.2f"),
				*PS->GetPlayerName(),
				PS->GetBounsMaxSpeed());
		}
	}

	//저장한 위치로 이동
	SetActorTransform(
		InitialTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (IsValid(GetMesh()))
	{
		//이동전 주행 속도 제거
		GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		//이동전 회전 속도 제거
		GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	ForceNetUpdate();

	UE_LOG(LogTemplateVehicle, Warning, TEXT("%s returned to initial position"), *GetNameSafe(this));
}

void ACXPawn::SetMovementLocked(bool bLocked)
{
	//잠금 여부는 서버에서 결정
	if (!HasAuthority())
	{
		return;
	}

	bMovementLocked = bLocked;

	//서버에서는 OnRep이 자동으로 호출되지 않아 직접 적용
	ApplyMovementLock();

	ForceNetUpdate();
}

void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bMovementLocked);
}

void ACXPawn::OnRep_MovementLocked()
{
	//클라이언트 차량에도 동일한 설정 적용
	ApplyMovementLock();
}

void ACXPawn::ApplyMovementLock()
{
	if (!IsValid(ChaosVehicleMovement))
	{
		return;
	}

	//이전에 누르고 있던 가속, 조항 입력 제거
	ChaosVehicleMovement->SetThrottleInput(0.f);
	ChaosVehicleMovement->SetSteeringInput(0.f);

	//잠기면 브레이크 유지, 풀리면 브레이크 해제
	ChaosVehicleMovement->SetBrakeInput(bMovementLocked ? 1.f : 0.f);
	ChaosVehicleMovement->SetHandbrakeInput(bMovementLocked);

	if (bMovementLocked && IsValid(GetMesh()))
	{
		//잠금 직전 남아 있던 이동, 회전 속도 제거
		GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void ACXPawn::BeginPlay()
{
	Super::BeginPlay();

	//실제 게임 위치는 서버가 관리
	if (HasAuthority())
	{
		InitialTransform = GetActorTransform();
	}
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// 카운트다운 중에는 차량 조작 무시
	if (bMovementLocked)
	{
		return;
	}

	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

void ACXPawn::InputAttack(const FInputActionValue& Value)
{
	CTSAttack();
}

void ACXPawn::MulticastHitEffect_Implementation(const FVector& TraceStart, const FVector& TraceEnd, bool bHitVehicle, const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	UWorld* World = GetWorld();
	DrawDebugLine(World, TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 3.0f);

	if (!bHitVehicle)
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(World, HitParticle, ImpactPoint, ImpactNormal.Rotation());
}

void ACXPawn::CTSAttack_Implementation()
{
	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * 700.0f;

	UWorld* World = GetWorld();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHitVehicle = false;
	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params))
	{
		AActor* Actor = HitResult.GetActor();
		UE_LOG(LogTemplateVehicle, Error, TEXT("Attack trace hit: %s"), *GetNameSafe(HitResult.GetActor()));

		if (ACXPawn* CXPawn = Cast<ACXPawn>(Actor))
		{
			bHitVehicle = true;
			UGameplayStatics::ApplyDamage(CXPawn, 5.f, GetController(), this, nullptr);
		}
	}
	MulticastHitEffect(Start, End, bHitVehicle, HitResult.ImpactPoint, HitResult.ImpactNormal);
}

#undef LOCTEXT_NAMESPACE