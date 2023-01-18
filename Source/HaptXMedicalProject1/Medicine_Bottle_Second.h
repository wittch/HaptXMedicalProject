// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Medicine_Bottle_Second.generated.h"

UCLASS()
class HAPTXMEDICALPROJECT1_API AMedicine_Bottle_Second : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMedicine_Bottle_Second();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable)
	void CalcMedicineAmount();
private:



	class ASyring_Second* SyringValue;

	float MedicineBottleAmount;
	float StartValue;



	bool IsOverlapped;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MedicineBottle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MedicineBottleCap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Medicine;

	

	FORCEINLINE float GetAmount() { return MedicineBottleAmount; }
	FORCEINLINE ASyring_Second* GetSyringValue() { return SyringValue; }
	FORCEINLINE void SetSyringValue(ASyring_Second* value) { SyringValue = value; }
};
