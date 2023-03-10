// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Montage.generated.h"

class USkeleton;
class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;

/** Struct of animation asset montage group data */
USTRUCT(BlueprintType)
struct FVirtualMontageGroupData
{
	GENERATED_USTRUCT_BODY()

	/** Montage slots name in montage group */
	UPROPERTY(VisibleAnywhere, Category = "Montage Params")
	TArray<FName> SlotsName;
};

/** Struct of animation asset montages slot data */
USTRUCT(BlueprintType)
struct FVirtualMontageSlotData
{
	GENERATED_USTRUCT_BODY()

	/** Animation montage slots asset */
	UPROPERTY(VisibleAnywhere, Category = "Montage Params")
	TArray<UAnimMontage*> Montages;
	
	/** Slots name used by the current montages */
	UPROPERTY(EditAnywhere, Category = "Montage Params")
	TArray<FName> SlotsName;

	FVirtualMontageSlotData()
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Montage : public UObject
{
	GENERATED_BODY()

public:

#pragma region Montage Group

	// Returns all the group names of the skeleton
	static void GetSkeletonMontageGroupNames(USkeleton* InSkeleton, TArray<FName>& OutMontageGroupNames);

	// Returns group name according to the slot name
	static FName GetSkeletonMontageGroupNameFromSlot(USkeleton* InSkeleton, const FName& InSlotName);

	// Returns the first group name used by the montage
	static FName GetMontageGroupName(UAnimMontage* InMontage);

	// Returns the all group name used by the montage
	static void GetMontageGroupNames(UAnimMontage* InMontage, TArray<FName>& OutMontageGroupNames);

#pragma endregion


#pragma region Montage Slot

	// Returns all the slot names of the skeleton
	static void GetSkeletonMontageSlotNames(USkeleton* InSkeleton, TArray<FName>& OutMontageSlotNames);

	// Returns slot name according to the group name
	static void GetSkeletonMontageSlotNamesFromGroup(USkeleton* InSkeleton, const FName& InGroupName, TArray<FName>& OutMontageSlotNames);

	/**
	* Return the slot used by the current montage.
	*
	* @param InAnimMontage		Animation montage asset
	* @param OutMontageSlotNames		Out montage slots name
	*
	*/
	static void GetMontageSlotNames(UAnimMontage* InAnimMontage, TArray<FName>& OutMontageSlotNames);

	// Returns the all slot name used by the montage group
	static void GetMontageSlotNamesFromGroup(UAnimMontage* InMontage, const FName& InGroupName, TArray<FName>& OutMontageSlotNames);

#pragma endregion


#pragma region Montage Slot Event

	/**
	* Set montage slots section is looping.
	*
	* @param InAnimMontage		Animation montage asset
	*
	*/
	static void SetMontageLoopSection(UAnimMontage* InAnimMontage);

	/**
	* Sort montage slots name.
	*
	* @param InAnimMontage		Animation montage asset
	* @param InSlotName		Desired slot name
	*
	*/
	static void SortMontageSlots(USkeleton* InSkeleton, TArray<FName>& OutSlotsName);

	/**
	* Add a slot in the specified montage. If the slot is not found on the skeleton, it will be added automatically.
	*
	* @param InAnimMontage		Animation montage asset
	* @param InSlotName		Desired slot name
	*
	*/
	static void AddMontageSlot(UAnimMontage* InAnimMontage, const FName& InSlotName);

	/**
	* Delete a slot in the specified montage
	*
	* @param InAnimMontage		Animation montage asset
	* @param InSlotName		Desired slot name
	*
	*/
	static void RemoveMontageSlot(UAnimMontage* InAnimMontage, const FName& InSlotName);

	/**
	* Replace the specified montage slot name
	*
	* @param InAnimMontage		Animation montage asset
	* @param InOldSlotName		Last slot name
	* @param InSlotName		Desired slot name
	*
	*/
	static void OnReplaceMontageSlot(UAnimMontage* InAnimMontage, const FName& InOldSlotName, const FName& InSlotName);

	/**
	* Replace the specified montage slots name
	*
	* @param InAnimMontage		Animation montage asset
	* @param InSlotNames		Desired slots name
	*
	*/
	static void OnReplaceMontageSlots(UAnimMontage* InAnimMontage, const TArray<FName>& InSlotNames);

	/**
	* Transfer source skeleton montage slots to target skeleton
	*
	* @param InSourceSkeleton		Source skeleton
	* @param InTargetSkeleton		Target skeleton
	*
	*/
	static void OnTransferMontageSlots(USkeleton* InSourceSkeleton, USkeleton* InTargetSkeleton, bool InClearInvalidData = false);

#pragma endregion


};
