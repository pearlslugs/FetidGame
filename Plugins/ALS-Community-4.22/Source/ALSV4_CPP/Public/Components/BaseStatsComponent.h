// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Library/CombatLibrary.h"
#include "Library/WeaponStructLibrary.h"
#include "BaseStatsComponent.generated.h"

class AALSBaseCharacter;

USTRUCT(BlueprintType)
struct FSingleStat
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Value")
	float Value;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Value")
		float MaxValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Name")
	FString Name;

	void DamageStat(float Damage)
	{
		Value = Value - Damage;
	}
	void HealStat(float Amount)
	{
		if (Value < MaxValue)
		{
			Value = Value + Amount;
		}
		if (Value > MaxValue)
		{
			Value = MaxValue;
		}
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class ALSV4_CPP_API UBaseStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBaseStatsComponent();
	//stat variables
	//body partss
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	AALSBaseCharacter* RefToOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart FullBody;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart LeftArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart RightArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart Torso;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart Head;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart LeftLeg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart RightLeg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	FBodyPart CurrentBodyPart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|IncomingAttack")
	FAttackType IncomingAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|IncomingAttack")
	FAttackType OutGoingAttack;
	

	//stamina
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
	float Breath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
	float TotalEnergy;

	//level up stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Strength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
	FSingleStat Vitality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Endurance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Dexterity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Wisdom;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Spirit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Intelligence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat PainLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat OverallHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Blood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		FSingleStat Composure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stats")
		int ArmorRatingForBashing;

	// lists
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	TArray<FBodyPart> BodyPartsList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
	TArray<FSingleStat> StatsList;

	//health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|BodyPart")
		bool bShouldRegen;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SelectBodyPart(FBodyPart ThisBodyPart);

	UFUNCTION(BlueprintCallable)
	float TryDamageCurrentBodyPart();

	UFUNCTION(BlueprintCallable)
	void BleedSelf();

	UFUNCTION(BlueprintCallable)// get weapon, make attack type
	FAttackOutput SetAttackValues(FWeaponData InWeapon);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|CurrentWeapon")
	FWeaponData CurrentWeaponData;

	UFUNCTION(BlueprintCallable)
	int CheckForBleedingByLimb(FBodyPart InCurrentBodyPart);
	UFUNCTION(BlueprintCallable)
	EBleedingType GetBleedingByInt(FBodyPart InCurrentBodyPart, int NewBleedingAmount);

	UFUNCTION(BlueprintCallable)
	void DamageComposure(float Damage);
	UFUNCTION(BlueprintCallable)
	void HealComposure(float Amount);
	UFUNCTION(BlueprintCallable)
	FSingleStat DamageStat(float Damage, FSingleStat StatToDamage);
	UFUNCTION(BlueprintCallable)
	void HealStat(float Amount);

	void CausePain(float Velocity, float Precision, EPhysicalDamageType CurrentDamageType);
	UFUNCTION(BlueprintCallable)
	void HealSelf(float Amount, TArray<FBodyPart> BodyParts, FSingleStat InHealth);
	UFUNCTION(BlueprintCallable)
	FBodyPart HealBodyPart(FBodyPart InBodyPart, float Amount);
};