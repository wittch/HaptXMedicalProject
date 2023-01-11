// Fill out your copyright notice in the Description page of Project Settings.


#include "Syring_Second.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
// Sets default values
ASyring_Second::ASyring_Second():
	Amount(0)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	

}

// Called when the game starts or when spawned
void ASyring_Second::BeginPlay()
{
	Super::BeginPlay();

	
}

// Called every frame
void ASyring_Second::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASyring_Second::CalcMedicineAmount(UStaticMeshComponent* mesh)
{
	Amount = mesh->RelativeLocation.Y * 0.1f;
	FVector Meshoffset{ 0,Amount,0 };
	mesh->AddLocalOffset(Meshoffset);
	
}

