// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/CXItemSpawner.h"
#include "Components/BillboardComponent.h"

// Sets default values
ACXItemSpawner::ACXItemSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillBoard"));
	SetRootComponent(BillboardComponent);
}

// Called when the game starts or when spawned
void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		SpawnItem();
	}
}

void ACXItemSpawner::SpawnItem()
{
	if (!HasAuthority() || IsValid(ItemClass) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	SpawnedItem = World->SpawnActorDeferred<AActor>(ItemClass, GetActorTransform(), this,
		nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	SpawnedItem->OnDestroyed.AddDynamic(this, &ACXItemSpawner::OnItemDestryed);
	SpawnedItem->FinishSpawning(GetActorTransform());


}

void ACXItemSpawner::OnItemDestryed(AActor* DestroyedActor)
{
	if (!HasAuthority() || IsValid(ItemClass) == false)
	{
		return;
	}

	SpawnedItem = nullptr;
	
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACXItemSpawner::SpawnItem, SpawnInterval, false);
}


