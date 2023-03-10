// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BaseStatsComponent.h"
#include "Library/WeaponStructLibrary.h"
#include "Library/CombatLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "Character/ALSBaseCharacter.h"

// Sets default values for this component's properties
UBaseStatsComponent::UBaseStatsComponent()
{

	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	Blood.Name = "Blood";
	Blood.Value = 100.f;
	Blood.MaxValue = 100.f;
	

	// ...
}


// Called when the game starts
void UBaseStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	

	//StatsList.Add(Strength);
	//StatsList.Add(Vitality);
	//StatsList.Add(Endurance);
	//StatsList.Add(Dexterity);
	//StatsList.Add(Wisdom);
	//StatsList.Add(Spirit);
	//StatsList.Add(Intelligence);

	//BodyPartsList.Add(Head);
	//BodyPartsList.Add(Torso);
	//BodyPartsList.Add(LeftArm);
	//BodyPartsList.Add(RightArm);
	//BodyPartsList.Add(LeftLeg);
	//BodyPartsList.Add(RightLeg);
	// ...
	
}


// Called every frame
void UBaseStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBaseStatsComponent::SelectBodyPart(FBodyPart ThisBodyPart)
{
	CurrentBodyPart = ThisBodyPart;
}

float UBaseStatsComponent::TryDamageCurrentBodyPart()
{
	// take pain, composure and stats in to account, dont have attacks directly lower health. pain and bleeding are what will lower health, maybe just a little bump from attacks
	float DamageFromAttack = CurrentBodyPart.TryDamageBodyPart(IncomingAttack);
	if (DamageFromAttack == -1)
	{
		Composure.DamageStat(15);
		// this needs to be expanded. I think we will need access to more stats
		return 0;
	}

		OverallHealth.DamageStat(FMath::RoundHalfToZero((100.0 * (DamageFromAttack)) / 100.0));
		return DamageFromAttack;
}
// weappn as param
FAttackOutput UBaseStatsComponent::SetAttackValues(FWeaponData InWeapon)
{
	FAttackOutput OutAttackValues;
	float Precision = 1;
	float Velocity = 1;
	UE_LOG(LogTemp, Error, TEXT("it got here"));
	switch(CurrentWeaponData.PrimaryDamageType)
	{
	case EPhysicalDamageType::EPD_Blunt:
		Velocity = FWeaponFunctions::GetBluntVelocity(CurrentWeaponData.BluntDamage, Strength.Value, CurrentWeaponData.StrengthScaling);
		Precision = 1;
		break;
	case EPhysicalDamageType::EPD_Hack:
		Velocity = FWeaponFunctions::GetVelocity(CurrentWeaponData.HackDamage, Strength.Value, CurrentWeaponData.StrengthScaling);
		Precision = FWeaponFunctions::GetPrecision(CurrentWeaponData.HackDamage, Dexterity.Value, CurrentWeaponData.DexterityScaling);
		break;
	case EPhysicalDamageType::EPD_Slash:
		Velocity = FWeaponFunctions::GetVelocity(CurrentWeaponData.SlashDamage, Strength.Value, CurrentWeaponData.StrengthScaling);
		Precision = FWeaponFunctions::GetPrecision(CurrentWeaponData.SlashDamage, Dexterity.Value, CurrentWeaponData.DexterityScaling);
		break;
	case EPhysicalDamageType::EPD_Thrust:
		Velocity = FWeaponFunctions::GetVelocity(CurrentWeaponData.ThrustDamage, Strength.Value, CurrentWeaponData.StrengthScaling);
		Precision = FWeaponFunctions::GetPrecision(CurrentWeaponData.ThrustDamage, Dexterity.Value, CurrentWeaponData.DexterityScaling);
		break;
	case EPhysicalDamageType::EPD_None:
		//oops
		Velocity = 1;
		Precision = 1;
		break;
	}
	
	OutAttackValues.ConstructAttackOutput(Velocity, Precision);

	if (OutAttackValues.Precision == 1 && OutAttackValues.Velocity == 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Damage Type Is None, Check the Weapon Data"));
	}
	UBaseStatsComponent::CausePain(Velocity, Precision, IncomingAttack.CurrentPhysicalDamageType);
	return OutAttackValues;
}

void UBaseStatsComponent::BleedSelf()
{
	for (auto BodyPart : BodyPartsList)
	{
		if (BodyPart.bIsBleeding)
		{
			switch (BodyPart.BleedingAmount)
			{
			case EBleedingType::EBT_Light:
				Blood.DamageStat(2.f);
				OverallHealth.DamageStat(2.f);
			break;
			case EBleedingType::EBT_Mid:
				Blood.DamageStat(2.f);
				OverallHealth.DamageStat(2.f);
			break;
			case EBleedingType::EBT_Heavy:
				Blood.DamageStat(5.f);
				OverallHealth.DamageStat(5.f);
			break;
			}
		}
	}
}

int UBaseStatsComponent::CheckForBleedingByLimb(FBodyPart InCurrentBodyPart)
{
	return InCurrentBodyPart.GetIntByBleedingAmount();
}

EBleedingType UBaseStatsComponent::GetBleedingByInt(FBodyPart InCurrentBodyPart, int NewBleedingAmount)
{
	return InCurrentBodyPart.GetBleedingAmountByInt(NewBleedingAmount);
}

void UBaseStatsComponent::DamageComposure(float Damage)
{
	if (Composure.Value > 0) 
	{
		Composure.DamageStat(Damage);
	}
	if (Composure.Value < 0) 
	{
		Composure.Value = 0;
	}
}

void UBaseStatsComponent::HealStat(float Amount)
{
	Composure.HealStat(Amount);
}

FSingleStat UBaseStatsComponent::DamageStat(float Damage, FSingleStat StatToDamage)
{
	FSingleStat CurrentStat = StatToDamage;
	if (StatToDamage.Value > 0)
	{
		StatToDamage.DamageStat(Damage);
	}
	if (StatToDamage.Value < 0)
	{
		StatToDamage.Value = 0;
	}
	return StatToDamage;
}

void UBaseStatsComponent::HealComposure(float Amount)
{
	Composure.HealStat(Amount);
}

void UBaseStatsComponent::CausePain(float Velocity, float Precision, EPhysicalDamageType CurrentDamageType)
{
	switch (CurrentDamageType)
	{
	case EPhysicalDamageType::EPD_Blunt:
		PainLevel.DamageStat(Velocity / 2 * -1);
		break;
	case EPhysicalDamageType::EPD_Hack:
		PainLevel.DamageStat(Velocity / 4 + Precision / 5 * -1);
		break;
	case EPhysicalDamageType::EPD_Slash:
		PainLevel.DamageStat(Velocity / 6 + Precision / 4 * -1);
		break;
	case EPhysicalDamageType::EPD_Thrust:
		PainLevel.DamageStat(Velocity / 8 + Precision / 3 * -1);
		break;
	case EPhysicalDamageType::EPD_None:
		//oops

		break;
	}
}

void UBaseStatsComponent::HealSelf(float Amount, TArray<FBodyPart> BodyParts, FSingleStat InHealth)
{
	for (auto& BodyPart : BodyParts)
	{
		BodyPart.HealBodyPart(Amount);
	}
	InHealth.HealStat(Amount);
}

FBodyPart UBaseStatsComponent::HealBodyPart(FBodyPart InBodyPart, float Amount)
{
	InBodyPart.HealBodyPart(Amount);
	return InBodyPart;
}
