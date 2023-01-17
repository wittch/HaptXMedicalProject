// Fill out your copyright notice in the Description page of Project Settings.


#include <HaptxPrimitives/Public/Components/hx_1d_translator_component.h>
#include "Syring_Second.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Math/TransformNonVectorized.h"
// Sets default values
ASyring_Second::ASyring_Second():
	Amount(0)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	


	TranslatorComponent = CreateDefaultSubobject<UHx1DTranslatorComponent>(TEXT("TranslatorComponent"));
	TranslatorComponent->SetupAttachment(Cylinder);

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

<<<<<<< HEAD
void ASyring_Second::CalcMedicineAmount()
{
	Amount = GetCylinderToPistonDistance();
	Amount = Amount * 0.01f;
	Amount = FMath::Clamp(Amount, 0.0f, 1.5f);

	
	FVector CylinderLocation = { 0.0f, -5.0f, 30.0f };
	FVector CylinderScale = { 0.5f, 0.5f, Amount };
	FTransform CylinderTransform;
	CylinderTransform.SetLocation(CylinderLocation);
	CylinderTransform.SetRotation(Medicine->GetRelativeTransform().GetRotation());
	CylinderTransform.SetScale3D(CylinderScale);
	Medicine->SetRelativeScale3D(CylinderScale);
=======
>>>>>>> parent of 2c17b86 (syring BP -> cpp 변경 중)
}

void ASyring_Second::CalcMedicineAmount(UStaticMeshComponent* mesh)
{
	Amount = mesh->RelativeLocation.Y * 0.1f;
	FVector Meshoffset{ 0,Amount,0 };
	
	FTransform MeshTransform;
	MeshTransform.SetLocation(mesh->GetComponentLocation());
	MeshTransform.SetRotation(mesh->GetComponentRotation().Quaternion());
	MeshTransform.SetScale3D(Meshoffset);

	mesh->SetWorldTransform(MeshTransform);
	
}

