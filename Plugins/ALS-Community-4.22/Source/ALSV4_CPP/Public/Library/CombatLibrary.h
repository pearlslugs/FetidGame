#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/EnumAsByte.h"
#include "UObject/Class.h"
#include "CombatLibrary.generated.h"


UENUM(BlueprintType)
enum class EOffensePosition : uint8 //split in to attack and defend
{
	EOP_None UMETA(DisplayName = "None"),
	EOP_AttackHigh UMETA(DisplayName = "AttackHigh"),
	EOP_AttackMid UMETA(DisplayName = "AttackMid"),
	EOP_AttackLow UMETA(DisplayName = "AttackLow"),
	EOP_KickLow UMETA(DisplayName = "KickLow"),
	EOP_KickHigh UMETA(DisplayName = "KickHigh"),
	EOP_Shove UMETA(DisplayName = "Shove")
};

UENUM(BlueprintType)
enum class EDefencePosition : uint8 //split in to attack and defend
{
	EDP_None UMETA(DisplayName = "None"),
	EDP_BlockHigh UMETA(DisplayName = "BlockHigh"),
	EDP_BlockMid UMETA(DisplayName = "BlockMid"),
	EDP_BlockLow UMETA(DisplayName = "BlockLow"),
	EDP_BlockIdle UMETA(DisplayName = "BlockIdle"),
};

UENUM(BlueprintType)
enum class EArmorLevel : uint8
{
	EAL_None UMETA(DisplayName = "None"),
	EAL_Light UMETA(DisplayName = "Light"),
	EAL_Mid UMETA(DisplayName = "Mid"),
	EAL_Heavy UMETA(DisplayName = "Heavy"),
	EAL_Special UMETA(DisplayName = "Special"),
};

UENUM(BlueprintType)
enum class EBleedingType : uint8
{
	EBT_None UMETA(DisplayName = "None"),
	EBT_Light UMETA(DisplayName = "Light"),
	EBT_Mid UMETA(DisplayName = "Mid"),
	EBT_Heavy UMETA(DisplayName = "Heavy"),
};

UENUM(BlueprintType)
enum class EPhysicalDamageType : uint8
{
	EPD_None UMETA(DisplayName = "None"),
	EPD_Slash UMETA(DisplayName = "Slash"),
	EPD_Thrust UMETA(DisplayName = "Thrust"),
	EPD_Hack UMETA(DisplayName = "Hack"),
	EPD_Blunt UMETA(DisplayName = "Blunt"),
};

UENUM(BlueprintType)
enum class EElementalDamageType : uint8
{
	EED_None UMETA(DisplayName = "None"),
	EED_Fire UMETA(DisplayName = "Fire"),
	EED_Cold UMETA(DisplayName = "Cold"),
	EED_Electric UMETA(DisplayName = "Electric"),
	EED_Holy UMETA(DisplayName = "Holy"),
	EED_Dark UMETA(DisplayName = "Dark"),
	EED_Water UMETA(DisplayName = "Water"),
	EED_Poison UMETA(DisplayName = "Poison"),
	EED_Oil UMETA(DisplayName = "Oil"),
};

UENUM(BlueprintType)
enum class EGeneralDamageType : uint8
{
	EPD_None UMETA(DisplayName = "None"),
	EPD_Physical UMETA(DisplayName = "Physical"),
	EPD_Elemental UMETA(DisplayName = "Elemental"),
	EPD_Both UMETA(DisplayName = "Both"),
};

USTRUCT(BlueprintType)
struct FAttackOutput
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Value")
	float Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Name")
	float Precision;

	void ConstructAttackOutput(float InVelocity, float InPrecision)
	{
		Velocity = InVelocity;
		Precision = InPrecision;
	}
};

USTRUCT(BlueprintType)
struct FDamageTypeProtection
{
	GENERATED_BODY()
		//physical types
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Physical")
		float SlashProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Physical")
		float ThrustProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Physical")
		float HackProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Physical")
		float BluntProtection;
	//elemental types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float FireProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float ColdProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float ElectricProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float HolyProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float DarkProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float WaterProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float PoisonProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection|Elemental")
		float OilProtection;
};

USTRUCT(BlueprintType)
struct FArmorType
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmorType|BaseLevel")
		EArmorLevel ArmorLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmorType|ProtectionLevel")
		FDamageTypeProtection DamageProtection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmorType|ArmorCondition")
		float ArmorCondition = 0;


	// add a reference to the material of the weapon when this is made
};

USTRUCT(BlueprintType)
struct FAttackType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackType")
	EOffensePosition AttackPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackType")
	EGeneralDamageType GeneralAttackType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackType")
	EElementalDamageType ElementalAttackType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackType")
	EPhysicalDamageType CurrentPhysicalDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackInfo")
	float Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackType|AttackInfo")
	float Precision;

	void ReplaceVelocity(float NewVelocity)
	{
		Velocity = NewVelocity;
	}
	void SetDamageValues(FAttackOutput NewAttackValues)
	{
		Velocity = NewAttackValues.Velocity;
		Precision = NewAttackValues.Precision;
	}

	// add a reference to the material of the weapon when this is made
};

USTRUCT(BlueprintType)
struct FDamageDealt
{
	GENERATED_BODY()
public:
	UPROPERTY()
	bool CauseBleeding;
	UPROPERTY()
	float DamageToHP;
};

USTRUCT(BlueprintType)
struct FBodyPart
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Health")
		float HP = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Health")
		float MaxHP = HP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Bleeding")
		bool bIsBleeding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Bleeding")
		EBleedingType BleedingAmount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|ArmorLevel")
		FArmorType CurrentArmorLevel;//change to current armor type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Name")
		FString Name = "head";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Health")
		bool bIsBroken;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyPart|Health")
		bool bIsSevered;

	void HealBodyPart(float Amount)
	{
		if (HP <= MaxHP)
		{
			HP = HP + Amount;
		}
	}

		void SetName(FString NewName) 
		{
		Name = NewName;
		}
		void SetIsBleeding(bool InBleeding)
		{
			bIsBleeding = InBleeding;
		}
		void SetBleedingAmountByInt(int IntAmount)
		{
			switch (IntAmount)
			{
			case 0:
				BleedingAmount = EBleedingType::EBT_None;
				break;
			case 1:
				BleedingAmount = EBleedingType::EBT_Light;
				break;
			case 2:
				BleedingAmount = EBleedingType::EBT_Mid;
				break;
			case 3:
				BleedingAmount = EBleedingType::EBT_Heavy;
				break;
			}
		}
		EBleedingType GetBleedingAmountByInt(int IntAmount)
		{
			switch (IntAmount)
			{
			case 0:
				return EBleedingType::EBT_None;
				break;
			case 1:
				return EBleedingType::EBT_Light;
				break;
			case 2:
				return EBleedingType::EBT_Mid;
				break;
			case 3:
				return EBleedingType::EBT_Heavy;
				break;
			}
			return EBleedingType::EBT_None;
		}
		int GetIntByBleedingAmount()
		{
			switch (BleedingAmount)
			{
			case EBleedingType::EBT_None:
				return 0;
				break;
			case EBleedingType::EBT_Light:
				return 1;
				break;
			case EBleedingType::EBT_Mid:
				return 2;
				break;
			case EBleedingType::EBT_Heavy:
				return 3;
				break;
			}
			return 0;
		}

		FArmorType SetArmorType(FArmorType NewArmorType)
		{
			CurrentArmorLevel = NewArmorType;
		}

	float TryDamageBodyPart(FAttackType IncomingAttack)
	{
		float AmountToDamageOwner = 0;
		int BleedingAmountAsInt = GetIntByBleedingAmount();

	
		// check the general damage type
		if (IncomingAttack.AttackPosition == EOffensePosition::EOP_KickLow || IncomingAttack.AttackPosition == EOffensePosition::EOP_KickHigh || IncomingAttack.AttackPosition == EOffensePosition::EOP_Shove)
		{
			return -1;
		}
		if (IncomingAttack.GeneralAttackType == EGeneralDamageType::EPD_Physical || IncomingAttack.GeneralAttackType == EGeneralDamageType::EPD_Both)
		{
			EPhysicalDamageType CurrentDamageType = GetPhysicalDamageType(IncomingAttack);
			AmountToDamageOwner = DealPhysicalDamageType(IncomingAttack, CurrentDamageType);
			if (CurrentDamageType != EPhysicalDamageType::EPD_Blunt)
			{
				if (BleedingAmountAsInt != 3)
				{
					BleedingAmountAsInt++;
					SetIsBleeding(true);
					if (AmountToDamageOwner > 30)
					{
						BleedingAmountAsInt++;
					}
				}
			}

			HP = HP - AmountToDamageOwner;
			UE_LOG(LogTemp, Warning, TEXT("%f"), AmountToDamageOwner);
			SetBleedingAmountByInt(BleedingAmountAsInt);
			return AmountToDamageOwner;
		}
		else if (IncomingAttack.GeneralAttackType == EGeneralDamageType::EPD_Elemental || IncomingAttack.GeneralAttackType == EGeneralDamageType::EPD_Both)
		{
			return 1;
		}
		return 1;
	};
	float DealPhysicalDamageType(FAttackType IncomingAttack, EPhysicalDamageType ReplacedDamageType)
	{
		float BluntDamage;
		float HackDamage;
		float SlashDamage;
		float ThrustDamage;
		UE_LOG(LogTemp, Error, TEXT("IVelocity: %f Precision: %f"), IncomingAttack.Velocity, IncomingAttack.Precision);
		if (IncomingAttack.Velocity == 0 || IncomingAttack.Precision == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid Attack Data. Velocity: %f Precision: %f"), IncomingAttack.Velocity, IncomingAttack.Precision);
			return 1;
		}
		switch (ReplacedDamageType)
		{
		case EPhysicalDamageType::EPD_Blunt:
			BluntDamage = IncomingAttack.Velocity / 2 - (CurrentArmorLevel.DamageProtection.BluntProtection * FMath::RandRange(0.75f, 1.25f));
			return BluntDamage > 0 ? BluntDamage : 1;
			break;
		case EPhysicalDamageType::EPD_Hack:
			HackDamage = IncomingAttack.Velocity / 2 + (IncomingAttack.Precision / 2) - (CurrentArmorLevel.DamageProtection.HackProtection * FMath::RandRange(0.75f, 1.25f));
			return HackDamage > 0 ? HackDamage : 1;
			//investigate what kind of numbers you want to work with and add bleeding based on how high the number is.
			// i would like to keep numbers low, at least between 0 and 100
			break;
		case EPhysicalDamageType::EPD_Slash:
			SlashDamage = IncomingAttack.Velocity / 2 + (IncomingAttack.Precision / 2) - (CurrentArmorLevel.DamageProtection.SlashProtection * FMath::RandRange(0.75f, 1.25f));
			return SlashDamage > 0 ? SlashDamage : 1;
			break;
		case EPhysicalDamageType::EPD_Thrust:
			ThrustDamage = IncomingAttack.Velocity / 2 + (IncomingAttack.Precision) - (CurrentArmorLevel.DamageProtection.ThrustProtection * FMath::RandRange(0.75f, 1.25f));
			return ThrustDamage > 0 ? ThrustDamage : 1;
			break;
		case EPhysicalDamageType::EPD_None:
			//oops :)
			return 1;
			break;
		}
		return 1;
	}
	void GetElementalDamageType(EElementalDamageType DamageType)
	{
		switch (DamageType)
		{

		case EElementalDamageType::EED_Fire:
			//stuff
			break;
		case EElementalDamageType::EED_Cold:
			//stuff
			break;
		case EElementalDamageType::EED_Dark:
			//stuff
			break;
		case EElementalDamageType::EED_Electric:
			//stuff
			break;
		case EElementalDamageType::EED_Holy:
			//stuff
			break;
		case EElementalDamageType::EED_Oil:
			//stuff
			break;
		case EElementalDamageType::EED_Poison:
			//stuff
			break;
		case EElementalDamageType::EED_Water:

			//stuff
			break;
		}
	}
	EPhysicalDamageType GetPhysicalDamageType(FAttackType IncomingAttack)
	{
		if (CurrentArmorLevel.ArmorLevel == EArmorLevel::EAL_Light)
		{
			return IncomingAttack.CurrentPhysicalDamageType;
		}
		if (CurrentArmorLevel.ArmorLevel == EArmorLevel::EAL_Heavy)
		{
			//if armor not broken
			IncomingAttack.ReplaceVelocity(IncomingAttack.Velocity / 2);
			return EPhysicalDamageType::EPD_Blunt;
		}
		switch (IncomingAttack.CurrentPhysicalDamageType)
		{
		case EPhysicalDamageType::EPD_Hack:
			if (IncomingAttack.Precision / 2 + IncomingAttack.Velocity / 2 > CurrentArmorLevel.DamageProtection.HackProtection) {
				return EPhysicalDamageType::EPD_Hack;
			}
			break;
		case EPhysicalDamageType::EPD_Slash:
			if (IncomingAttack.Velocity / 2 > CurrentArmorLevel.DamageProtection.SlashProtection) {
				return EPhysicalDamageType::EPD_Slash;
			}
			//stuff
			break;
		case EPhysicalDamageType::EPD_Thrust:
			if (IncomingAttack.Precision * 1.5 + IncomingAttack.Velocity > CurrentArmorLevel.DamageProtection.HackProtection) {
				return EPhysicalDamageType::EPD_Thrust;
			}
			//stuff
			break;
		}
		IncomingAttack.ReplaceVelocity(IncomingAttack.Velocity / 2);
		return EPhysicalDamageType::EPD_Blunt;
	}
};
