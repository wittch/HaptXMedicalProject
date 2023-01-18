// Fill out your copyright notice in the Description page of Project Settings.


#include "Medicine_Bottle_Second.h"
#include "Syring_Second.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

// Sets default values
AMedicine_Bottle_Second::AMedicine_Bottle_Second():
	MedicineBottleAmount(0),
	StartValue(0)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	MedicineBottle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MedicineBottle"));
	MedicineBottle->SetupAttachment(MedicineBottle);
	MedicineBottle->OnComponentBeginOverlap.AddDynamic(this, &AMedicine_Bottle_Second::OnOverlapBegin);
	MedicineBottle->OnComponentEndOverlap.AddDynamic(this, &AMedicine_Bottle_Second::OnOverlapEnd);

	MedicineBottleCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MedicineBottleCap"));
	MedicineBottleCap->SetupAttachment(MedicineBottle);

	Medicine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MedicineAmount"));
	Medicine->SetupAttachment(MedicineBottle);

}

// Called when the game starts or when spawned
void AMedicine_Bottle_Second::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMedicine_Bottle_Second::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsOverlapped)
	{
		CalcMedicineAmount();
	}
}

void AMedicine_Bottle_Second::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	

	ASyring_Second* castedActor{};
	if (OtherActor)
	{
		castedActor = Cast<ASyring_Second>(OtherActor);
	}


	if (castedActor)
	{
		IsOverlapped = true;
		SetSyringValue(castedActor);
		StartValue = GetSyringValue()->GetAmount();
		GEngine->AddOnScreenDebugMessage(-1, 30, FColor::Cyan, FString::Printf(TEXT("%f"), StartValue));
	}

}


void AMedicine_Bottle_Second::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASyring_Second* castedActor{};
	if ((OtherActor && (OtherActor != this) && OtherComp))
	{
		castedActor = Cast<ASyring_Second>(OtherActor);
	}

	if (castedActor)
	{
		IsOverlapped = false;
		SetSyringValue(nullptr);
		StartValue = 0;
		GEngine->AddOnScreenDebugMessage(-1, 30, FColor::Cyan, FString::Printf(TEXT("%f"), StartValue));
	}
}

void AMedicine_Bottle_Second::CalcMedicineAmount()
{
	float value = 1 - ((GetSyringValue()->GetAmount() - StartValue) * 0.005f);
	FMath::Clamp(value, 0.0f, 1.0f);

	FVector BottleLocation = { 0.0f, 0.0f, 10.0f };
	FVector BottleScale = { 1.0f, 1.0f, value };
	FTransform BottleTransform;
	BottleTransform.SetLocation(BottleLocation);
	BottleTransform.SetRotation(Medicine->GetRelativeTransform().GetRotation());
	BottleTransform.SetScale3D(BottleScale);
	Medicine->SetRelativeScale3D(BottleScale);
}