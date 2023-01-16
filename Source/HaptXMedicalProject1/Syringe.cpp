// Fill out your copyright notice in the Description page of Project Settings.


#include "Syringe.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include <HaptxPrimitives/Public/Components/hx_1d_translator_component.h>

// Sets default values
ASyringe::ASyringe()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Driver = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Driver"));
	RootComponent = Driver;
	Driver_Bit = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Dirver_Bit"));
	Driver_Bit->SetupAttachment(Driver);
	Liquid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Liquid"));
	Liquid->SetupAttachment(Driver);

	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_Driver(TEXT("StaticMesh'/Game/Third/Body_COPY.Body_COPY'"));
	if (SM_Driver.Succeeded())
	{
		Driver->SetStaticMesh(SM_Driver.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_Driver_Bit(TEXT("StaticMesh'/Game/Zero/Button.Button'"));
	if (SM_Driver_Bit.Succeeded())
	{
		Driver_Bit->SetStaticMesh(SM_Driver_Bit.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh>SM_Liquid(TEXT("StaticMesh'/Game/Third/SM_MERGED_Cylinder_COPY_20.SM_MERGED_Cylinder_COPY_20'"));
	if (SM_Liquid.Succeeded())
	{
		Liquid->SetStaticMesh(SM_Liquid.Object);
		Liquid->SetRelativeRotation(FRotator(0, 0, -90.f));
		Liquid->SetRelativeScale3D(FVector(0.5, 0.5, 0.1));
		Liquid->SetRelativeLocation(FVector(0, 100, 28));
	}

	Driver_Bit->OnComponentBeginOverlap.AddDynamic(this, &ASyringe::OnOverlapBegin);


}

// Called when the game starts or when spawned
void ASyringe::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASyringe::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



}

void ASyringe::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Black, FString::Printf(TEXT("BEGIN OVERLAP")));


	
}



