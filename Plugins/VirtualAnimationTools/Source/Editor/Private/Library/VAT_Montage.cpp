// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Montage.h"
#include "Library/VAT_Library.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"

#if ENGINE_MAJOR_VERSION > 4
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#pragma region Montage Group
void UVAT_Montage::GetSkeletonMontageGroupNames(USkeleton* InSkeleton, TArray<FName>& OutMontageGroupNames)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Get the slot groups
	const TArray<FAnimSlotGroup>& theSlotGroups = InSkeleton->GetSlotGroups();

	// Each every slot group
	for (const FAnimSlotGroup& theAnimSlotGroup : theSlotGroups)
	{
		OutMontageGroupNames.Add(theAnimSlotGroup.GroupName);
	}
}

FName UVAT_Montage::GetSkeletonMontageGroupNameFromSlot(USkeleton* InSkeleton, const FName& InSlotName)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return NAME_None;
	}

	// Return the slot group name
	return InSkeleton->GetSlotGroupName(InSlotName);
}

FName UVAT_Montage::GetMontageGroupName(UAnimMontage* InMontage)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return NAME_None;
	}

	// Check the skeleton is valid
	USkeleton* theSkeleton = InMontage->GetSkeleton();
	if (theSkeleton == nullptr)
	{
		return NAME_None;
	}

	// Get the slot animation tracks
	const TArray<struct FSlotAnimationTrack>& theSlotAnimationTracks = InMontage->SlotAnimTracks;

	// Each the slot animation track
	for (const FSlotAnimationTrack& theSlotAnimTrack : theSlotAnimationTracks)
	{
		return theSkeleton->GetSlotGroupName(theSlotAnimTrack.SlotName);
	}

	// INVALID
	return NAME_None;
}

void UVAT_Montage::GetMontageGroupNames(UAnimMontage* InMontage, TArray<FName>& OutMontageGroupNames)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return;
	}

	// Check the skeleton is valid
	USkeleton* theSkeleton = InMontage->GetSkeleton();
	if (theSkeleton == nullptr)
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<struct FSlotAnimationTrack>& theSlotAnimationTracks = InMontage->SlotAnimTracks;

	// Each the slot animation track
	for (const FSlotAnimationTrack& theSlotAnimTrack : theSlotAnimationTracks)
	{
		const FName theMontageGroupName = theSkeleton->GetSlotGroupName(theSlotAnimTrack.SlotName);
		OutMontageGroupNames.Add(theMontageGroupName);
	}
}
#pragma endregion


#pragma region Montage Slot
void UVAT_Montage::GetSkeletonMontageSlotNames(USkeleton* InSkeleton, TArray<FName>& OutMontageSlotNames)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Get the slot groups
	const TArray<FAnimSlotGroup>& theSlotGroups = InSkeleton->GetSlotGroups();

	// Each every slot group
	for (const FAnimSlotGroup& theAnimSlotGroup : theSlotGroups)
	{
		OutMontageSlotNames.Append(theAnimSlotGroup.SlotNames);
	}
}

void UVAT_Montage::GetSkeletonMontageSlotNamesFromGroup(USkeleton* InSkeleton, const FName& InGroupName, TArray<FName>& OutMontageSlotNames)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Get the slot groups
	const TArray<FAnimSlotGroup>& theSlotGroups = InSkeleton->GetSlotGroups();

	// Each every slot group
	for (const FAnimSlotGroup& theAnimSlotGroup : theSlotGroups)
	{
		if (theAnimSlotGroup.GroupName == InGroupName)
		{
			OutMontageSlotNames.Append(theAnimSlotGroup.SlotNames);
			break;
		}
	}
}

void UVAT_Montage::GetMontageSlotNames(UAnimMontage* InAnimMontage, TArray<FName>& OutMontageSlotNames)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr)
	{
		return;
	}

	// Each every slots name
	for (FSlotAnimationTrack SlotAnimTrack : InAnimMontage->SlotAnimTracks)
	{
		OutMontageSlotNames.Add(SlotAnimTrack.SlotName);
	}
}

void UVAT_Montage::GetMontageSlotNamesFromGroup(UAnimMontage* InMontage, const FName& InGroupName, TArray<FName>& OutMontageSlotNames)
{
	// Check the montage is valid
	if (InMontage == nullptr)
	{
		return;
	}

	// Check the skeleton is valid
	USkeleton* theSkeleton = InMontage->GetSkeleton();
	if (theSkeleton == nullptr)
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<struct FSlotAnimationTrack>& theSlotAnimationTracks = InMontage->SlotAnimTracks;

	// Each the slot animation track
	for (const FSlotAnimationTrack& theSlotAnimTrack : theSlotAnimationTracks)
	{
		const FName theMontageGroupName = theSkeleton->GetSlotGroupName(theSlotAnimTrack.SlotName);
		if (theMontageGroupName == InGroupName)
		{
			OutMontageSlotNames.Add(theSlotAnimTrack.SlotName);
		}
	}
}
#pragma endregion


#pragma region Montage Slot Event
void UVAT_Montage::SetMontageLoopSection(UAnimMontage* InAnimMontage)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr)
	{
		return;
	}
	
	// Define loop section
	FCompositeSection* theLoopSection = nullptr;

	// Each every composite section
	for (FCompositeSection& theCompositeSection : InAnimMontage->CompositeSections)
	{
		if (theCompositeSection.SectionName == "Loop")
		{
			theLoopSection = &theCompositeSection;
		}
	}

	// Flag next section is current section equal looping state
	if (theLoopSection == nullptr)
	{
		// Each every composite section
		for (FCompositeSection& theCompositeSection : InAnimMontage->CompositeSections)
		{
			theCompositeSection.NextSectionName = theCompositeSection.SectionName;
		}
	}
	else
	{
		theLoopSection->NextSectionName = theLoopSection->SectionName;
	}

	// Apply the animation montage changed
	UVAT_Library::ModifyFromAnimMontage(InAnimMontage);
}

void UVAT_Montage::SortMontageSlots(USkeleton* InSkeleton, TArray<FName>& OutSlotsName)
{
	// Check the animation skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Define the slots name
	TArray<FName> theSortSlotsName, theSkeletonSlotsName;
	GetSkeletonMontageSlotNames(InSkeleton, theSkeletonSlotsName);

	// Each every skeleton slots name
	for (FName& theSlotName : theSkeletonSlotsName)
	{
		if (OutSlotsName.Contains(theSlotName))
		{
			theSortSlotsName.AddUnique(theSlotName);
		}
	}

	// Return the result
	OutSlotsName = theSortSlotsName;
}

void UVAT_Montage::AddMontageSlot(UAnimMontage* InAnimMontage, const FName& InSlotName)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr
		|| !InAnimMontage->IsValidSlot(InSlotName)
		|| !InAnimMontage->SlotAnimTracks.IsValidIndex(0))
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<FSlotAnimationTrack>& theSlotAnimTracks = InAnimMontage->SlotAnimTracks;

	// Add the new slot animation track
	FSlotAnimationTrack& theNewSlotAnimTrack = InAnimMontage->AddSlot(InSlotName);

	// Copy the first animation track
	theNewSlotAnimTrack.AnimTrack = theSlotAnimTracks[0].AnimTrack;

	// Apply the animation montage changed
	UVAT_Library::ModifyFromAnimMontage(InAnimMontage);
}

void UVAT_Montage::RemoveMontageSlot(UAnimMontage* InAnimMontage, const FName& InSlotName)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr
		|| !InAnimMontage->SlotAnimTracks.IsValidIndex(0))
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<FSlotAnimationTrack>& theSlotAnimTracks = InAnimMontage->SlotAnimTracks;

	// Find the desired montage slot and remove it
	for (int32 SlotIndex = 0; SlotIndex < theSlotAnimTracks.Num(); SlotIndex++)
	{
		if (theSlotAnimTracks[SlotIndex].SlotName == InSlotName)
		{
			InAnimMontage->SlotAnimTracks.RemoveAt(SlotIndex);
			break;
		}
	}

	// Apply the animation montage changed
	UVAT_Library::ModifyFromAnimMontage(InAnimMontage);
}

void UVAT_Montage::OnReplaceMontageSlot(UAnimMontage* InAnimMontage, const FName& InOldSlotName, const FName& InSlotName)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr
		|| !InAnimMontage->SlotAnimTracks.IsValidIndex(0))
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<FSlotAnimationTrack>& theSlotAnimTracks = InAnimMontage->SlotAnimTracks;

	// Find the desired montage slot and remove it
	for (int32 SlotIndex = 0; SlotIndex < theSlotAnimTracks.Num(); SlotIndex++)
	{
		if (theSlotAnimTracks[SlotIndex].SlotName == InSlotName)
		{
			InAnimMontage->SlotAnimTracks[SlotIndex].SlotName = InSlotName;
			break;
		}
	}

	// Apply the animation montage changed
	UVAT_Library::ModifyFromAnimMontage(InAnimMontage);
}

void UVAT_Montage::OnReplaceMontageSlots(UAnimMontage* InAnimMontage, const TArray<FName>& InSlotNames)
{
	// Check the animation montage is valid
	if (InAnimMontage == nullptr
		|| !InAnimMontage->SlotAnimTracks.IsValidIndex(0))
	{
		return;
	}

	// Get the slot animation tracks
	const TArray<FSlotAnimationTrack>& theSlotAnimTracks = InAnimMontage->SlotAnimTracks;

	// Cache the first slot animation track data
	const FSlotAnimationTrack theSourceSlotAnimTrack = theSlotAnimTracks[0];

	// Reset the slot animation tracks
	InAnimMontage->SlotAnimTracks.Reset();

	// Add the desired slot animation track
	for (const FName& theSlotName : InSlotNames)
	{
		FSlotAnimationTrack& theNewSlotAnimTrack = InAnimMontage->AddSlot(theSlotName);
		theNewSlotAnimTrack.AnimTrack = theSourceSlotAnimTrack.AnimTrack;
	}

	// Apply the animation montage changed
	UVAT_Library::ModifyFromAnimMontage(InAnimMontage);
}

void UVAT_Montage::OnTransferMontageSlots(USkeleton* InSourceSkeleton, USkeleton* InTargetSkeleton, bool InClearInvalidData)
{
	// Check both skeleton is valid
	if (InSourceSkeleton == nullptr || InTargetSkeleton == nullptr)
	{
		return;
	}

	// Get the slot groups
	const TArray<FAnimSlotGroup>& theSlotGroups = InSourceSkeleton->GetSlotGroups();

	// We will clear other slots and groups
	if (InClearInvalidData)
	{
		// Each every slot group
		const TArray<FAnimSlotGroup> theAnimSlotGroups = InTargetSkeleton->GetSlotGroups();
		for (const FAnimSlotGroup& theAnimSlotGroup : theAnimSlotGroups)
		{
			// Each every slot names
			for (const FName& theSlotName : theAnimSlotGroup.SlotNames)
			{
				// Ignore default slot name
				if (theSlotName == FAnimSlotGroup::DefaultSlotName)
				{
					continue;
				}
				InTargetSkeleton->RemoveSlotName(theSlotName);
			}

			// Ignore default group name
			if (theAnimSlotGroup.GroupName == FAnimSlotGroup::DefaultGroupName)
			{
				continue;
			}
			InTargetSkeleton->RemoveSlotGroup(theAnimSlotGroup.GroupName);
		}
	}

	// Each every slot group
	for (const FAnimSlotGroup& theAnimSlotGroup : theSlotGroups)
	{
		InTargetSkeleton->AddSlotGroupName(theAnimSlotGroup.GroupName);

		// Each every slot names
		for (const FName& theSlotName : theAnimSlotGroup.SlotNames)
		{
			InTargetSkeleton->RemoveSlotName(theSlotName);
			InTargetSkeleton->SetSlotGroupName(theSlotName, theAnimSlotGroup.GroupName);
		}
	}

	// Apply the animation montage changed
	UVAT_Library::ModifyFromObject(InTargetSkeleton);
}
#pragma endregion