// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include <HaptxPrimitives/Public/Components/hx_1d_translator_component.h>

#include "Syringe.generated.h"


class UStaticMeshComponent;
class UHx1DTranslatorComponent;

UCLASS()
class HAPTXMEDICALPROJECT1_API ASyringe : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASyringe();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Driver;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Driver_Bit;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Liquid;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UHx1DTranslatorComponent* Hx;

	FTransform* Bit_Transform;

public:

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);



};
