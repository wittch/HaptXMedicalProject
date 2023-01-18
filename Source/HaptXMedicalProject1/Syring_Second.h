// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Syring_Second.generated.h"

UCLASS()
class HAPTXMEDICALPROJECT1_API ASyring_Second : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASyring_Second();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	float Amount;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void CalcMedicineAmount(UStaticMeshComponent* mesh);

	FORCEINLINE float GetAmount() { return Amount; }

<<<<<<< HEAD
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Cylinder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Piston;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Medicine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsOverlapped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	class UHx1DTranslatorComponent* TranslatorComponent;

	
=======
>>>>>>> parent of 2c17b86 (syring BP -> cpp 변경 중)
};
