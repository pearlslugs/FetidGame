#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/Class.h"
#include "Library/CombatLibrary.h"
#include "WeaponStructLibrary.generated.h"


USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Name")
	FString WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "damage|Physical")
	EPhysicalDamageType PrimaryDamageType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "damage|Physical")
	EPhysicalDamageType SecondaryDamageType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "damage|Physical")
	EPhysicalDamageType TertiaryPhysicalDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float SlashDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float HackDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float ThrustDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float BluntDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Scaling")
	float StrengthScaling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Scaling")
	float DexterityScaling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Scaling")
	float WisdomScaling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Scaling")
	float SpiritScaling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Scaling")
	float IntelligenceScaling;

};

	USTRUCT(BlueprintType)
	struct FWeaponFunctions
	{
		GENERATED_BODY()
		static float GetPrecision(float WeaponDamage, float DextarityScaling, float CharacterDextarity)
		{
			return FMath::RoundHalfFromZero(WeaponDamage + (DextarityScaling * FMath::RandRange(1.f, 1.5f)) * CharacterDextarity);
		}

		static float GetVelocity(float WeaponDamage, float StrengthScaling, float CharacterStrength)
		{
			return FMath::RoundHalfFromZero(WeaponDamage + (StrengthScaling * FMath::RandRange(1.f, 1.5f)) * CharacterStrength);
		}

		static float GetBluntVelocity(float WeaponDamage, float StrengthScaling, float CharacterStrength)
		{
			return FMath::RoundHalfFromZero(WeaponDamage + (StrengthScaling * FMath::RandRange(1.f, 2.f)) * CharacterStrength);
		}
		static bool GetIsWeaponLessAttack(EOffensePosition CurrentOffensePosition)
		{
			if (CurrentOffensePosition == EOffensePosition::EOP_KickLow || CurrentOffensePosition == EOffensePosition::EOP_KickHigh || CurrentOffensePosition == EOffensePosition::EOP_Shove)
			{
				return true;
			}
			return false;
		}

	};
