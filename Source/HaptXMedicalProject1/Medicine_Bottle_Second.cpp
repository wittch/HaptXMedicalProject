// Fill out your copyright notice in the Description page of Project Settings.


#include "Medicine_Bottle_Second.h"
#include "Syring_Second.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
AMedicine_Bottle_Second::AMedicine_Bottle_Second()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMedicine_Bottle_Second::BeginPlay()
{
	Super::BeginPlay();
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AMedicine_Bottle_Second::OnSphereOverlap);
	
}

// Called every frame
void AMedicine_Bottle_Second::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMedicine_Bottle_Second::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
	
	if (OtherActor)
	{
		ASyring_Second* Syring = Cast<ASyring_Second>(OtherActor);
		if (Syring)
		{
			FVector MeshOffset{ 0, 1.0f - (Syring->GetAmount() * 0.1f), 0 };
			MedicineAmount->AddLocalOffset(MeshOffset);
		}
	}
}
