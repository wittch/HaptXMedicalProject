// Fill out your copyright notice in the Description page of Project Settings.


#include "Syring_Second.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Math/TransformNonVectorized.h"
#include "Medicine_Bottle_Second.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"

// Sets default values
ASyring_Second::ASyring_Second():
	Amount(0),
	IsOverlapped(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cylinder"));
	Cylinder->SetupAttachment(RootComponent);
	Cylinder->OnComponentBeginOverlap.AddDynamic(this, &ASyring_Second::OnOverlapBegin);
	Cylinder->OnComponentEndOverlap.AddDynamic(this, &ASyring_Second::OnOverlapEnd);


	Piston = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Piston"));
	Piston->SetupAttachment(Cylinder);

	Medicine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Medicine"));
	Medicine->SetupAttachment(Cylinder);

}

// Called when the game starts or when spawned
void ASyring_Second::BeginPlay()
{
	Super::BeginPlay();

}

void ASyring_Second::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
	AMedicine_Bottle_Second* castedActor{};
	if (OtherActor ) 
	{
		castedActor = Cast<AMedicine_Bottle_Second>(OtherActor);
	}
		

	if (castedActor)
	{
		IsOverlapped = true;
	}
}

void ASyring_Second::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMedicine_Bottle_Second* castedActor{};
	if ((OtherActor && (OtherActor != this) && OtherComp))
	{
		castedActor = Cast<AMedicine_Bottle_Second>(OtherActor);
	}

	if (castedActor)
	{
		IsOverlapped = false;
	}
}

// Called every frame
void ASyring_Second::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsOverlapped)
	{
		CalcMedicineAmount();
	}
}

void ASyring_Second::CalcMedicineAmount()
{
	Amount = GetCylinderToPistonDistance();
	Amount = Amount * 0.01f;
	Amount = FMath::Clamp(Amount, 0.0f, 1.5f);

	GEngine->AddOnScreenDebugMessage(-1, 30, FColor::Cyan, FString::Printf(TEXT("%f"), Amount));
	FVector CylinderLocation = { 0.0f, -5.0f, 30.0f };
	FVector CylinderScale = { 0.5f, 0.5f, Amount };
	FTransform CylinderTransform;
	CylinderTransform.SetLocation(CylinderLocation);
	CylinderTransform.SetRotation(Medicine->GetRelativeTransform().GetRotation());
	CylinderTransform.SetScale3D(CylinderScale);
	Medicine->SetRelativeScale3D(CylinderScale);
}

float ASyring_Second::GetCylinderToPistonDistance()
{

	FVector CylinderLocation = Cylinder->GetComponentLocation();
	FVector PistonLocation = Piston->GetComponentLocation();
	FVector Coord = { CylinderLocation.X - PistonLocation.X, CylinderLocation.Y - PistonLocation.Y , CylinderLocation.Z - PistonLocation.Z };
	float Distance = pow(Coord.X, 2) + pow(Coord.Y, 2) + pow(Coord.Z, 2);
	Distance = sqrt(Distance);

	return Distance;
}

