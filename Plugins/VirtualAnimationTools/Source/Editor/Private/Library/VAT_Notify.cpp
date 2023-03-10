// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Notify.h"
#include "Library/VAT_Montage.h"
#include "Library/VAT_Library.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimMontage.h"
#include "VAT_AnimNotifyInstance.h"
#if ENGINE_MAJOR_VERSION > 4
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#pragma region Notify Range
void UVAT_Notify::LimitPositionRange(const float& InSequenceLength, float& InPosition)
{
	InPosition = FMath::Clamp(InPosition, 0.f, InSequenceLength);
}

void UVAT_Notify::LimitPositionRange(UAnimSequenceBase* InAnimSequenceBase, float& InPosition, float& InDuration)
{
	check(InAnimSequenceBase);
	InPosition = FMath::Clamp(InPosition, 0.f, UVAT_Library::GetSequenceLength(InAnimSequenceBase));
	InDuration = FMath::Clamp(InPosition + InDuration, InAnimSequenceBase->GetTimeAtFrame(1), UVAT_Library::GetSequenceLength(InAnimSequenceBase) - InPosition);
}
#pragma endregion


#pragma region Notify Event
void UVAT_Notify::GetNotifiesData(UAnimSequenceBase* InAnimSequenceBase, TArray<FVirtualNotifiesData>& OutNotifiesData)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Clear cached data
	OutNotifiesData.Reset();

	// Each every notifies track
	for (FAnimNotifyTrack& theNotifyTrack : InAnimSequenceBase->AnimNotifyTracks)
	{
		// Check the track notifies is valid
		if (theNotifyTrack.Notifies.Num() == 0)
		{
			continue;
		}

		// Add the track data
		FVirtualNotifiesData& theNotifiesData = OutNotifiesData.AddDefaulted_GetRef();
		theNotifiesData.TrackName = theNotifyTrack.TrackName;

		// Each every notifies event
		for (FAnimNotifyEvent& theAnimNotifyEvent : InAnimSequenceBase->Notifies)
		{
			if (theAnimNotifyEvent.Notify != nullptr)
			{
				FVirtualNotifyData& theNotifyData = theNotifiesData.Notifies.AddDefaulted_GetRef();
				theNotifyData.NotifyInstance = theAnimNotifyEvent.Notify;
				theNotifyData.Position = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();
			}

			if (theAnimNotifyEvent.NotifyStateClass != nullptr)
			{
				FVirtualNotifyStateData& theNotifyStateData = theNotifiesData.NotifiesState.AddDefaulted_GetRef();
				theNotifyStateData.NotifyStateInstance = theAnimNotifyEvent.NotifyStateClass;
				theNotifyStateData.Range.X = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();
				theNotifyStateData.Range.Y = theAnimNotifyEvent.GetDuration();
			}
		}
	}
}

void UVAT_Notify::TransferNotifyEventToData(const FAnimNotifyEvent& InNotifyEvent, FVirtualNotifyEventData& InNotifyEventData)
{
	InNotifyEventData.NotifyColor = InNotifyEvent.NotifyColor;
	InNotifyEventData.TriggerWeightThreshold = InNotifyEvent.TriggerWeightThreshold;
	InNotifyEventData.MontageTickType = InNotifyEvent.MontageTickType;
	InNotifyEventData.NotifyTriggerChance = InNotifyEvent.NotifyTriggerChance;
	InNotifyEventData.NotifyFilterLOD = InNotifyEvent.NotifyFilterLOD;
	InNotifyEventData.bTriggerOnFollower = InNotifyEvent.bTriggerOnFollower;
	InNotifyEventData.bTriggerOnDedicatedServer = InNotifyEvent.bTriggerOnDedicatedServer;
	InNotifyEventData.NotifyFilterType = InNotifyEvent.NotifyFilterType;
}

void UVAT_Notify::TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent, const FVirtualNotifyEventData& InNotifyEventData)
{
	InNotifyEvent.NotifyColor = InNotifyEventData.NotifyColor != FColor(0.f) ? InNotifyEventData.NotifyColor : InNotifyEvent.NotifyColor;
	InNotifyEvent.TriggerWeightThreshold = InNotifyEventData.TriggerWeightThreshold;
	InNotifyEvent.MontageTickType = InNotifyEventData.MontageTickType;
	InNotifyEvent.NotifyTriggerChance = InNotifyEventData.NotifyTriggerChance;
	InNotifyEvent.NotifyFilterLOD = InNotifyEventData.NotifyFilterLOD;
	InNotifyEvent.bTriggerOnFollower = InNotifyEventData.bTriggerOnFollower;
	InNotifyEvent.bTriggerOnDedicatedServer = InNotifyEventData.bTriggerOnDedicatedServer;
	InNotifyEvent.NotifyFilterType = InNotifyEventData.NotifyFilterType;
}
#pragma endregion


#pragma region Add Notify
void UVAT_Notify::AddNotifiesEvent(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Each every notifies data
	for (const FVirtualNotifiesData& theNotifiesData : InNotifiesData)
	{
		// Each every notify data
		for (const FVirtualNotifyData& theNotifyData : theNotifiesData.Notifies)
		{
			AddNotifyEvent(InAnimSequenceBase, theNotifyData, theNotifiesData.TrackName);
		}

		// Each every notify state data
		for (const FVirtualNotifyStateData& theNotifyStateData : theNotifiesData.NotifiesState)
		{
			AddNotifyStateEvent(InAnimSequenceBase, theNotifyStateData, theNotifiesData.TrackName);
		}
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequenceBase);
}

TArray<UAnimNotify*> UVAT_Notify::AddNotifyEvent(UAnimSequenceBase* InAnimSequenceBase
	, const FVirtualNotifyData& InNotifyData, const FName& InNotifyTrackName)
{
	// Define the animation notifies
	TArray<UAnimNotify*> theAnimNotifies;

	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return theAnimNotifies;
	}

	// Check the notify data is valid
	if (!InNotifyData.IsValid())
	{
		return theAnimNotifies;
	}

	// Check the track name is valid
	if (false && InNotifyTrackName == NAME_None)
	{
		return theAnimNotifies;
	}
	
	// If the track name is valid, we always find or create it
	AddNotifyTrack(InAnimSequenceBase, InNotifyTrackName);

	// Define the source position
	float theDesiredPosition = InNotifyData.Position;

	// Limit the position range in animation length
	const float& theSequenceLength = InAnimSequenceBase->GetPlayLength();
	LimitPositionRange(theSequenceLength, theDesiredPosition);

	// If there is a duplicate position, we should add a new track
	bool bShouldAddNotifyTrack = false;

	// Get the notify track index
	int32 theDesiredTrackIndex = GetNotifyTrackIndex(InAnimSequenceBase, InNotifyTrackName);

	// Check current animation notifies to prevent duplicate positions
	for (FAnimNotifyEvent& theAnimNotifyEvent : InAnimSequenceBase->Notifies)
	{
		// Check the track index is desired
		if (theAnimNotifyEvent.TrackIndex != theDesiredTrackIndex)
		{
			continue;
		}

		// Get the notify event data
		const float& theDuration = theAnimNotifyEvent.GetDuration();
		const float& thePosition = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();

		// Check the position range is valid
		if (theAnimNotifyEvent.Notify && theAnimNotifyEvent.Notify == InNotifyData.NotifyInstance && thePosition == theDesiredPosition)
		{
			theAnimNotifies.AddUnique(theAnimNotifyEvent.Notify);
		}
		else if (FMath::IsWithinInclusive(theDesiredPosition, thePosition, thePosition + theDuration))
		{
			bShouldAddNotifyTrack = true;
		}
	}

	// Check and create default track data
	if (bShouldAddNotifyTrack)
	{
		// Rebuild the track index
		theDesiredTrackIndex = InAnimSequenceBase->AnimNotifyTracks.Num();

		// Create the new notify track
		const FName theNewNotifyTrackName = FName(*FString::FromInt(theDesiredTrackIndex));
		AddNotifyTrack(InAnimSequenceBase, theNewNotifyTrackName);
	}

	// Create new animation notify
	UAnimNotify* theNewNotify = DuplicateObject<UAnimNotify>(InNotifyData.NotifyInstance, InAnimSequenceBase, NAME_None);
	FAnimNotifyEvent& theNewAnimNotifyEvent = InAnimSequenceBase->Notifies.AddDefaulted_GetRef();
	theNewAnimNotifyEvent.Notify = theNewNotify;
	theNewAnimNotifyEvent.NotifyName = FName(*theNewAnimNotifyEvent.Notify->GetNotifyName());
	theNewAnimNotifyEvent.Link(InAnimSequenceBase, theDesiredPosition);
	theNewAnimNotifyEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(InAnimSequenceBase->CalculateOffsetForNotify(theDesiredPosition));
	theNewAnimNotifyEvent.TriggerTimeOffset = FMath::Abs(theNewAnimNotifyEvent.TriggerTimeOffset) == KINDA_SMALL_NUMBER ? 0.f : theNewAnimNotifyEvent.TriggerTimeOffset;
	theNewAnimNotifyEvent.TrackIndex = theDesiredTrackIndex;
	theNewAnimNotifyEvent.NotifyStateClass = nullptr;

	// Transfer notify event data
	if (UVAT_AnimNotifyInstance* theAnimNotifyInstance = Cast<UVAT_AnimNotifyInstance>(theNewNotify))
	{
		theAnimNotifyInstance->TransferNotifyEventDataToInstance(theNewAnimNotifyEvent);
	}

	// Return the result
	theAnimNotifies.AddUnique(theNewNotify);
	return theAnimNotifies;
}

TArray<UAnimNotifyState*> UVAT_Notify::AddNotifyStateEvent(UAnimSequenceBase* InAnimSequenceBase
	, const FVirtualNotifyStateData& InNotifyStateData, const FName& InNotifyTrackName)
{
	// Define the animation notifies state
	TArray<UAnimNotifyState*> AnimNotifiesState;

	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return AnimNotifiesState;
	}

	// Check the notify state data is valid
	if (!InNotifyStateData.IsValid())
	{
		return AnimNotifiesState;
	}

	// Check the track name is valid
	if (false && InNotifyTrackName == NAME_None)
	{
		return AnimNotifiesState;
	}

	// If the track name is valid, we always find or create it
	AddNotifyTrack(InAnimSequenceBase, InNotifyTrackName);

	// Get the desired position and duration
	float theDesiredPosition = InNotifyStateData.GetStartPosition();
	float theDeisredDuration = InNotifyStateData.GetDuration();

	// Limit the position range in animation length
	const float& theSequenceLength = InAnimSequenceBase->GetPlayLength();
	LimitPositionRange(InAnimSequenceBase, theDesiredPosition, theDeisredDuration);
	
	// If there is a duplicate position, we should add a new track
	bool bShouldAddNotifyTrack = false;

	// Get the notify track index
	int32 theDesiredTrackIndex = GetNotifyTrackIndex(InAnimSequenceBase, InNotifyTrackName);

	// Check current animation notifies to prevent duplicate positions
	for (FAnimNotifyEvent& theAnimNotifyEvent : InAnimSequenceBase->Notifies)
	{
		// Check the track index is desired
		if (theAnimNotifyEvent.TrackIndex != theDesiredTrackIndex)
		{
			continue;
		}

		// Get the notify event data
		const float& theDuration = theAnimNotifyEvent.GetDuration();
		const float& thePosition = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();

		// Check the position range is valid
		if (theAnimNotifyEvent.NotifyStateClass && theAnimNotifyEvent.NotifyStateClass == InNotifyStateData.NotifyStateInstance
			&& thePosition == theDesiredPosition && theDuration == theDeisredDuration)
		{
			AnimNotifiesState.AddUnique(theAnimNotifyEvent.NotifyStateClass);
		}
		else if (theDesiredPosition == 0.f && theDeisredDuration == theSequenceLength)
		{
			bShouldAddNotifyTrack = true;
		}
		else if (FMath::IsWithinInclusive(theDesiredPosition, thePosition, thePosition + theDuration)
			|| FMath::IsWithinInclusive(theDesiredPosition + theDeisredDuration, thePosition, thePosition + theDuration))
		{
			bShouldAddNotifyTrack = true;
		}
	}

	// Check and create default track data
	if (bShouldAddNotifyTrack)
	{
		// Rebuild the track index
		theDesiredTrackIndex = InAnimSequenceBase->AnimNotifyTracks.Num();

		// Create the new notify track
		const FName theNewNotifyTrackName = FName(*FString::FromInt(theDesiredTrackIndex));
		AddNotifyTrack(InAnimSequenceBase, theNewNotifyTrackName);
	}

	// Create new notify state
	UAnimNotifyState* theNotifyState = DuplicateObject<UAnimNotifyState>(InNotifyStateData.NotifyStateInstance, InAnimSequenceBase, NAME_None);
	FAnimNotifyEvent& theAnimNotifyEvent = InAnimSequenceBase->Notifies.AddDefaulted_GetRef();
	theAnimNotifyEvent.NotifyStateClass = theNotifyState;
	theAnimNotifyEvent.Link(InAnimSequenceBase, theDesiredPosition);
	theAnimNotifyEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(InAnimSequenceBase->CalculateOffsetForNotify(theDesiredPosition));
	theAnimNotifyEvent.TriggerTimeOffset = FMath::Abs(theAnimNotifyEvent.TriggerTimeOffset) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.TriggerTimeOffset;
	theAnimNotifyEvent.TrackIndex = theDesiredTrackIndex;
	theAnimNotifyEvent.Notify = nullptr;
	theAnimNotifyEvent.NotifyName = FName(*theAnimNotifyEvent.NotifyStateClass->GetNotifyName());
	theAnimNotifyEvent.SetDuration(theDeisredDuration);
	theAnimNotifyEvent.EndLink.Link(InAnimSequenceBase, theAnimNotifyEvent.EndLink.GetTime());

	// Transfer notify event data
	if (UVAT_AnimNotifyStateInstance* theAnimNotifyStateInstance = Cast<UVAT_AnimNotifyStateInstance>(theNotifyState))
	{
		theAnimNotifyStateInstance->TransferNotifyEventDataToInstance(theAnimNotifyEvent);
	}

	// Success
	AnimNotifiesState.AddUnique(theNotifyState);
	return AnimNotifiesState;
}
#pragma endregion


#pragma region Modify Notify
void UVAT_Notify::ModifyNotifies(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Clear old animation notifies
	InAnimSequenceBase->Notifies.Reset();

	// Add new notifies data
	AddNotifiesEvent(InAnimSequenceBase, InNotifiesData);
}
#pragma endregion


#pragma region Remove Notify
void UVAT_Notify::RemoveNotifies(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Each every notifies data
	for (const FVirtualNotifiesData& theNotifiesData : InNotifiesData)
	{
		RemoveNotifiesByClass(InAnimSequenceBase, theNotifiesData.Notifies, &theNotifiesData.TrackName);
		RemoveNotifiesStateByClass(InAnimSequenceBase, theNotifiesData.NotifiesState, &theNotifiesData.TrackName);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequenceBase);
}

bool UVAT_Notify::RemoveNotifiesByIndex(UAnimSequenceBase* InAnimSequenceBase, const TArray<int32>& InNotifiesIndex)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Check the indices is valid
	if (InNotifiesIndex.Num() == 0)
	{
		return false;
	}

	// Define the new notifies
	TArray<FAnimNotifyEvent> theNewNotifies;

	// Each every notifies
	for (int32 NotifyIndex = 0; NotifyIndex < InAnimSequenceBase->Notifies.Num(); NotifyIndex++)
	{
		// Check the contains condition
		if (!InNotifiesIndex.Contains(NotifyIndex))
		{
			continue;
		}

		// Cache the valid notify
		theNewNotifies.Add(InAnimSequenceBase->Notifies[NotifyIndex]);
	}

	InAnimSequenceBase->Notifies = theNewNotifies;
	return true;
}

bool UVAT_Notify::RemoveNotifiesByClass(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifyData>& InNotifiesData, const FName* InNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Check the data is valid
	if (InNotifiesData.Num() == 0)
	{
		return false;
	}

	// Get the notify track name
	const FName theNotifyTrackName = InNotifyTrackName == nullptr ? NAME_None : *InNotifyTrackName;

	// Get the notify track index
	const int32 theDesiredTrackIndex = theNotifyTrackName == NAME_None ? INDEX_NONE : GetNotifyTrackIndex(InAnimSequenceBase, *InNotifyTrackName);

	// Each every notifies
	for (int32 NotifyIndex = InAnimSequenceBase->Notifies.Num() - 1; NotifyIndex >= 0; NotifyIndex--)
	{
		const FAnimNotifyEvent& theAnimNotifyEvent = InAnimSequenceBase->Notifies[NotifyIndex];

		// Check the track index is desired
		if (theDesiredTrackIndex != INDEX_NONE && theAnimNotifyEvent.TrackIndex != theDesiredTrackIndex)
		{
			continue;
		}

		// Get the notify event data
		const float& thePosition = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();

		// Each every remove notifies data
		for (const FVirtualNotifyData& theNotifyData : InNotifiesData)
		{
			// Check the notify is valid
			if (theAnimNotifyEvent.Notify == nullptr)
			{
				continue;
			}

			// Check the class is valid
			if (theAnimNotifyEvent.Notify->GetClass() != theNotifyData.NotifyInstance->GetClass())
			{
				continue;
			}

			// Check the position range is valid
			if (theNotifyData.Position < 0.f || thePosition == theNotifyData.Position)
			{
				InAnimSequenceBase->Notifies.RemoveAtSwap(NotifyIndex, 1, false);
			}
		}
	}
	
	// Success
	InAnimSequenceBase->Notifies.Shrink();
	return true;
}

bool UVAT_Notify::RemoveNotifiesStateByClass(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifyStateData>& InNotifiesStateData, const FName* InNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Check the data is valid
	if (InNotifiesStateData.Num() == 0)
	{
		return false;
	}

	// Get the notify track name
	const FName theNotifyTrackName = InNotifyTrackName == nullptr ? NAME_None : *InNotifyTrackName;

	// Get the notify track index
	const int32 theDesiredTrackIndex = theNotifyTrackName == NAME_None ? INDEX_NONE : GetNotifyTrackIndex(InAnimSequenceBase, *InNotifyTrackName);

	// Each every notifies
	for (int32 NotifyIndex = InAnimSequenceBase->Notifies.Num() - 1; NotifyIndex >= 0; NotifyIndex--)
	{
		const FAnimNotifyEvent& theAnimNotifyEvent = InAnimSequenceBase->Notifies[NotifyIndex];

		// Check the track index is desired
		if (theDesiredTrackIndex != INDEX_NONE && theAnimNotifyEvent.TrackIndex != theDesiredTrackIndex)
		{
			continue;
		}

		// Get the notify event data
		const float& theDuration = theAnimNotifyEvent.GetDuration();
		const float& thePosition = FMath::Abs(theAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : theAnimNotifyEvent.GetTriggerTime();

		// Each every remove notifies state data
		for (const FVirtualNotifyStateData& theNotifyStateData : InNotifiesStateData)
		{
			// Check the notify state is valid
			if (theAnimNotifyEvent.NotifyStateClass == nullptr)
			{
				continue;
			}

			// Check the class is valid
			if (theAnimNotifyEvent.NotifyStateClass->GetClass() != theNotifyStateData.NotifyStateInstance->GetClass())
			{
				continue;
			}

			// Check the position range is valid
			if ((theNotifyStateData.GetStartPosition() < 0.f || thePosition == theNotifyStateData.GetStartPosition())
				&& (theNotifyStateData.GetDuration() < 0.f || theDuration == theNotifyStateData.GetDuration()))
			{
				InAnimSequenceBase->Notifies.RemoveAtSwap(NotifyIndex, 1, false);
			}
		}
	}

	// Success
	InAnimSequenceBase->Notifies.Shrink();
	return true;
}
#pragma endregion


#pragma region Notify Track
bool UVAT_Notify::HasNotifyTrack(const UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Return the result
	return GetNotifyTrackIndex(InAnimSequenceBase, InNotifyTrackName) != INDEX_NONE;
}

int32 UVAT_Notify::GetNotifyTrackIndex(const UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return INDEX_NONE;
	}

	// Return the result
	return InAnimSequenceBase->AnimNotifyTracks.IndexOfByPredicate(
		[&](const FAnimNotifyTrack& Track)
		{
			return Track.TrackName == InNotifyTrackName;
		});
}

void UVAT_Notify::GetNotifiesTrackName(const UAnimSequenceBase* InAnimSequenceBase, TMap<FName, FName>& InTrackNameMap)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Clear cached data
	InTrackNameMap.Reset();

	// Each every animation notify tracks
	for (const FAnimNotifyTrack& theNotifyTrack : InAnimSequenceBase->AnimNotifyTracks)
	{
		InTrackNameMap.FindOrAdd(theNotifyTrack.TrackName);
	}
}

void UVAT_Notify::GetNotifiesTrackName(const UAnimSequenceBase* InAnimSequenceBase, TArray<FVirtualNotifyTrackData>& InTrackData)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return;
	}

	// Clear cached data
	InTrackData.Reset();

	// Each every animation notify tracks
	for (const FAnimNotifyTrack& theNotifyTrack : InAnimSequenceBase->AnimNotifyTracks)
	{
		FVirtualNotifyTrackData& theNotifyTrackData = InTrackData.AddDefaulted_GetRef();
		theNotifyTrackData.TrackName = theNotifyTrack.TrackName;
		theNotifyTrackData.TrackColor = theNotifyTrack.TrackColor;
	}
}

bool UVAT_Notify::AddNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName, const FLinearColor InTrackColor)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Check the notify is valid
	const bool bHasNotifyTrack = HasNotifyTrack(InAnimSequenceBase, InNotifyTrackName);
	if (bHasNotifyTrack)
	{
		return false;
	}

	// Create the new notify track data
	FAnimNotifyTrack theNewNotifyTrack;
	theNewNotifyTrack.TrackName = InNotifyTrackName;
	theNewNotifyTrack.TrackColor = InTrackColor;
	InAnimSequenceBase->AnimNotifyTracks.Add(theNewNotifyTrack);
	return true;
}

bool UVAT_Notify::ModifyNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName, const FName& InLastNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Each every animation notify tracks
	for (FAnimNotifyTrack& theNotifyTrack : InAnimSequenceBase->AnimNotifyTracks)
	{
		if (theNotifyTrack.TrackName == InLastNotifyTrackName)
		{
			theNotifyTrack.TrackName = InNotifyTrackName;
			return true;
		}
	}

	// Failed
	return false;
}

bool UVAT_Notify::RemoveNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName)
{
	// Check the animation asset is valid
	if (InAnimSequenceBase == nullptr)
	{
		return false;
	}

	// Each every animation notify tracks
	for (int32 TrackIndex = InAnimSequenceBase->AnimNotifyTracks.Num() - 1; TrackIndex >= 0; TrackIndex--)
	{
		if (InAnimSequenceBase->AnimNotifyTracks[TrackIndex].TrackName == InNotifyTrackName)
		{
			InAnimSequenceBase->AnimNotifyTracks.RemoveAt(TrackIndex);
			return true;
		}
	}

	// Failed
	return false;
}
#pragma endregion


#pragma region Foot Step
void UVAT_Notify::SampleFootStepNotifies(UAnimSequence* InAnimSequenceBase, const FVirtualCurveSampleData& InCurveSampleData
	, const TArray<FVirtualLegBaseData>& InLegsBaseData, TArray<FVirtualNotifiesData>& InNotifiesData)
{
	// Check the animation is valid
	if (!InAnimSequenceBase || !InAnimSequenceBase->GetSkeleton())
	{
		return;
	}

	// Check the legs base data is valid
	if (InLegsBaseData.Num() == 0)
	{
		return;
	}

	// Try sample each animation pose
	const double& thePoseSampleRate = InCurveSampleData.SampleRate.AsInterval();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequenceBase->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get root bone component space transform
	const FTransform& theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

	// Initialize all legs runtime data
	int32 thePosesNumber = 0;
	TArray<FVirtualLegPoseRuntimeData> theLegsRuntimeData;
	for (const FVirtualLegBaseData& theLegBaseData : InLegsBaseData)
	{
		FVirtualLegPoseRuntimeData& theLegRuntimeData = theLegsRuntimeData.AddDefaulted_GetRef();
		thePosesNumber = theLegRuntimeData.Initialize(InAnimSequenceBase, thePoseSampleRate, theLegBaseData);
	}

	// Check the poses number is valid
	if (thePosesNumber == 0)
	{
		return;
	}

	// Define last success sample time
	float theLastSampleTime = 0.f;

	// Response add foot step notifies
	auto OnAddFootStepNotifies = [&](const float& InPosition)
	{
		// Each every notifies data
		for (FVirtualNotifiesData& theNotifiesData : InNotifiesData)
		{
			// Each every notify data
			for (FVirtualNotifyData& theNotifyData : theNotifiesData.Notifies)
			{
				theNotifyData.Position = InPosition;
			}

			// Each every notify state data
			for (FVirtualNotifyStateData& theNotifyStateData : theNotifiesData.NotifiesState)
			{
				theNotifyStateData.Range.X = InPosition;
			}
		}

		theLastSampleTime = InPosition;
		AddNotifiesEvent(InAnimSequenceBase, InNotifiesData);
	};

	// Each every leg base data
	for (int32 DataIndex = 0; DataIndex < InLegsBaseData.Num(); DataIndex++)
	{
		const FVirtualLegBaseData& theLegBaseData = InLegsBaseData[DataIndex];
		FVirtualLegPoseRuntimeData& theLegRuntimeData = theLegsRuntimeData[DataIndex];

		// Check the leg bone is valid
		if (!theLegBaseData.IsValid())
		{
			continue;
		}

#if 1
		// Define the leader skeleton, change the leader once without a landing
		int32 theLeaderDataIndex = theLegsRuntimeData[DataIndex].IsLandedPose(0, InLegsBaseData[DataIndex].Tolerance) ? DataIndex + 1 : DataIndex;
		theLeaderDataIndex = FMath::Clamp(theLeaderDataIndex, 0, theLegsRuntimeData.Num() - 1);

		// Calculate the result
		for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
		{
			// Get the tolerance value
			const float& theTolerance = theLegsRuntimeData[theLeaderDataIndex].bIsLanded ? InCurveSampleData.AirTolerance : InCurveSampleData.LandedTolerance;

			// Check the data is landed state
			const bool bIsLanded = theLegsRuntimeData[theLeaderDataIndex].IsLandedPose(PoseIndex, theTolerance);

			// Check the leader data index is landed
			if (theLegsRuntimeData[theLeaderDataIndex].bIsLanded != bIsLanded)
			{
				theLegsRuntimeData[theLeaderDataIndex].bIsLanded = bIsLanded;

				// Calculate the pose time
				const float thePoseTime = (PoseIndex + InCurveSampleData.OffsetFrame) * thePoseSampleRate + InCurveSampleData.OffsetTime;

				// Response add foot step notifies
				if (PoseIndex != 0)
				{
					// Check if the interval between two keys is less than the minimum interval we expect
					if (InCurveSampleData.Interval > 0.f)
					{
						// Calculate both key delta time
						const float theDeltaTime = thePoseTime - theLastSampleTime;
						if (theDeltaTime < InCurveSampleData.Interval)
						{
							continue;
						}
					}

					// Response add foot step
					OnAddFootStepNotifies(thePoseTime);
				}

				// Change the leader data index
				theLeaderDataIndex = theLeaderDataIndex == DataIndex ? DataIndex + 1 : DataIndex, 0, theLegsRuntimeData.Num() - 1;
				theLeaderDataIndex = FMath::Clamp(theLeaderDataIndex, 0, theLegsRuntimeData.Num() - 1);
			}
		}
#else
		// Calculate the result
		for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
		{
			// Check the data is landed state
			const bool bIsLanded = theLegRuntimeData.IsLandedPose(PoseIndex, theLegRuntimeData.bIsLanded ? InCurveSampleData.AirTolerance : InCurveSampleData.LandedTolerance);

			// Handle
			if (theLegRuntimeData.bIsLanded != bIsLanded)
			{
				// Cache the state
				theLegRuntimeData.bIsLanded = bIsLanded;

				// Calculate the pose time
				const float thePoseTime = (PoseIndex + InCurveSampleData.OffsetFrame) * thePoseSampleRate + InCurveSampleData.OffsetTime;

				// Response add foot step notifies
				if (bIsLanded && PoseIndex != 0)
				{
					// Check if the interval between two keys is less than the minimum interval we expect
					if (InCurveSampleData.Interval > 0.f)
					{
						// Calculate both key delta time
						const float theDeltaTime = thePoseTime - theLastSampleTime;
						if (theDeltaTime < InCurveSampleData.Interval)
						{
							continue;
						}
					}

					// Response add foot step
					OnAddFootStepNotifies(thePoseTime);
				}
			}
			else if (false && PoseIndex == 0)
			{
				// Response add foot step notifies
				if (bIsLanded)
				{
					OnAddFootStepNotifies(0.f);
				}
			}
		}
		#endif
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequenceBase);
}
#pragma endregion