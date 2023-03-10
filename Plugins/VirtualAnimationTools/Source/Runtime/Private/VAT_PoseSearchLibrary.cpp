// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "VAT_PoseSearchLibrary.h"
#include "VAT_CurveLibrary.h"
#include "VirtualAnimationTools.h"
#include "Curves/RichCurve.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimSequenceBase.h"

DECLARE_CYCLE_STAT(TEXT("ComparePoses"), STAT_ComparePoses, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("CompareWeightoses"), STAT_CompareWeightPoses, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("ComparePosesMontage"), STAT_ComparePosesMontage, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("ComparePosesSequences"), STAT_ComparePosesSequences, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("CalculatePreviousPoseDistance"), STAT_CalculatePreviousPoseDistance, STATGROUP_VirtualAnimTools);

bool UVAT_PoseSearchLibrary::IsMatchingWeightRange(const float& InEvaluatePos, const FRichCurve* InWeightCurve, const float& InMatchValue, float* OutDeltaTime)
{
	// Check the weight curve is valid
	if (InWeightCurve == nullptr || !InWeightCurve->HasAnyData())
	{
		return false;
	}

	SCOPE_CYCLE_COUNTER(STAT_CompareWeightPoses);

	// Each every key
	for (int32 KeyIndex = 1; KeyIndex < InWeightCurve->Keys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = InWeightCurve->Keys[KeyIndex];
		const FRichCurveKey& theCurveLastKey = InWeightCurve->Keys[KeyIndex - 1];

		// Check the range
		if (FMath::IsWithinInclusive(InEvaluatePos, theCurveLastKey.Time, theCurveKey.Time))
		{
			if (OutDeltaTime != nullptr)
			{
				// Calculate the delta time
				float theDeltaTime = InEvaluatePos - theCurveLastKey.Time;

				// The algorithm of the tool is always centered on 3 points
				if (KeyIndex >= 2)
				{
					FRichCurveKey theKeys[3]{ InWeightCurve->Keys[KeyIndex - 2], InWeightCurve->Keys[KeyIndex - 1], InWeightCurve->Keys[KeyIndex] };
					if (theKeys[0].Value == theKeys[1].Value && theKeys[1].Value == theKeys[2].Value)
					{
						theDeltaTime = InEvaluatePos - theKeys[1].Time;
					}
				}
				else if (InWeightCurve->Keys.IsValidIndex(KeyIndex + 1))
				{
					FRichCurveKey theKeys[3]{ InWeightCurve->Keys[KeyIndex - 1], InWeightCurve->Keys[KeyIndex], InWeightCurve->Keys[KeyIndex + 1] };
					if (theKeys[0].Value == theKeys[1].Value && theKeys[1].Value == theKeys[2].Value)
					{
						theDeltaTime = InEvaluatePos - theKeys[1].Time;
					}
				}

				// Output the delta time
				*OutDeltaTime = theDeltaTime;
			}

			// Calculate the weight
			const float theWeight = (InEvaluatePos - theCurveLastKey.Time) / (theCurveKey.Time - theCurveLastKey.Time);

			// Calculate the weight
			const float theValue = theCurveLastKey.Value + (theCurveKey.Value - theCurveLastKey.Value) * theWeight;

			// Return the result
			return FMath::IsNearlyEqual(theValue, InMatchValue, KINDA_SMALL_NUMBER);
		}
		else if (InEvaluatePos <= theCurveKey.Time)
		{
			return false;
		}
	}

	// Failed
	return false;
}

bool UVAT_PoseSearchLibrary::ComparePoses(const float& InPreviousPoseDistance
	, const FVector2D& InLastCurveKey, const FVector2D& InCurrentCurveKey, float& OutPos, float& OutPosValue)
{
	SCOPE_CYCLE_COUNTER(STAT_ComparePoses);

	// Ignore invalid key value
	if (InCurrentCurveKey.Y == 0.f || InLastCurveKey.Y == 0.f)
	{
		return false;
	}

	// Calculate the delta distance
	const float theDeltaDist = InPreviousPoseDistance - InCurrentCurveKey.Y;

	// Compare values ​​closer to 0
	if (FMath::Abs(OutPosValue) > FMath::Abs(theDeltaDist))
	{
		OutPosValue = theDeltaDist;
		OutPos = InCurrentCurveKey.X;

		// Fast path
		if (FMath::IsNearlyEqual(OutPosValue, 0.f, 0.1f))
		{
			return true;
		}
	}

	// Fast path, check the time range is valid
	if (InCurrentCurveKey.Y >= InLastCurveKey.Y)
	{
		if (FMath::IsWithinInclusive(InPreviousPoseDistance, InLastCurveKey.Y, InCurrentCurveKey.Y))
		{
			// Calculate the weight
			const float theValueRange = InCurrentCurveKey.Y - InLastCurveKey.Y;
			const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - InLastCurveKey.Y) / theValueRange;

			// Return the compare result
			OutPos = InLastCurveKey.X + (InCurrentCurveKey.X - InLastCurveKey.X) * theWeight;
			return true;
		}
	}
	else if (FMath::IsWithinInclusive(InPreviousPoseDistance, InCurrentCurveKey.Y, InLastCurveKey.Y))
	{
		// Calculate the weight
		const float theValueRange = InLastCurveKey.Y - InCurrentCurveKey.Y;
		const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - InCurrentCurveKey.Y) / theValueRange;

		// Return the compare result
		OutPos = InLastCurveKey.X + (InCurrentCurveKey.X - InLastCurveKey.X) * theWeight;
		return true;
	}

	// Return the compare result
	return false;
}

float UVAT_PoseSearchLibrary::ComparePoses(const float& InPreviousPoseDistance, const FRichCurve* InFuturePoseCurve)
{
	SCOPE_CYCLE_COUNTER(STAT_ComparePoses);

	// Define the matching pose data
	float theMatchingPoseValue = 1e6f;
	float theMatchingPosePosition = -1.f;

	// Each every key
	for (int32 KeyIndex = 1; KeyIndex < InFuturePoseCurve->Keys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = InFuturePoseCurve->Keys[KeyIndex];
		const FRichCurveKey& theCurveLastKey = InFuturePoseCurve->Keys[KeyIndex - 1];

		// Ignore invalid key value
		if (theCurveKey.Value == 0.f || theCurveLastKey.Value == 0.f)
		{
			continue;
		}

		// Calculate the delta distance
		const float theDeltaDist = InPreviousPoseDistance - theCurveKey.Value;

		// Compare values ​​closer to 0
		if (FMath::Abs(theMatchingPoseValue) > FMath::Abs(theDeltaDist))
		{
			theMatchingPoseValue = theDeltaDist;
			theMatchingPosePosition = theCurveKey.Time;

			// Fast path
			if (FMath::IsNearlyEqual(theMatchingPoseValue, 0.f, 0.1f))
			{
				return theMatchingPosePosition;
			}
		}

		// Fast path, check the time range is valid
		if (theCurveKey.Value >= theCurveLastKey.Value)
		{
			if (FMath::IsWithinInclusive(InPreviousPoseDistance, theCurveLastKey.Value, theCurveKey.Value))
			{
				// Calculate the weight
				const float theValueRange = theCurveKey.Value - theCurveLastKey.Value;
				const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - theCurveLastKey.Value) / theValueRange;

				// Return the compare result
				return theCurveLastKey.Time + (theCurveKey.Time - theCurveLastKey.Time) * theWeight;
			}
		}
		else if (FMath::IsWithinInclusive(InPreviousPoseDistance, theCurveKey.Value, theCurveLastKey.Value))
		{
			// Calculate the weight
			const float theValueRange = theCurveLastKey.Value - theCurveKey.Value;
			const float theWeight = theValueRange == 0.f ? 0.f : (InPreviousPoseDistance - theCurveKey.Value) / theValueRange;

			// Return the compare result
			return theCurveLastKey.Time + (theCurveKey.Time - theCurveLastKey.Time) * theWeight;
		}
	}

	// Return the compare result
	return theMatchingPosePosition;
}

float UVAT_PoseSearchLibrary::ComparePoses(const float& InEvaluatePos, const FRichCurve* InPreviousPoseCurve, const FRichCurve* InFuturePoseCurve)
{
	// Define the previous pose distance
	float thePreviousPoseDistance = 0.f;

	{
		SCOPE_CYCLE_COUNTER(STAT_CalculatePreviousPoseDistance);

		// Each every key
		for (int32 KeyIndex = 1; KeyIndex < InPreviousPoseCurve->Keys.Num(); KeyIndex++)
		{
			const FRichCurveKey& theCurveKey = InPreviousPoseCurve->Keys[KeyIndex];
			const FRichCurveKey& theCurveLastKey = InPreviousPoseCurve->Keys[KeyIndex - 1];

			// Ignore invalid key value
			if (theCurveKey.Value == 0.f || theCurveLastKey.Value == 0.f)
			{
				continue;
			}

			// Check the range
			if (FMath::IsWithinInclusive(InEvaluatePos, theCurveLastKey.Time, theCurveKey.Time))
			{
				// Calculate the weight
				const float theWeight = (InEvaluatePos - theCurveLastKey.Time) / (theCurveKey.Time - theCurveLastKey.Time);

				// Return the result
				thePreviousPoseDistance = theCurveLastKey.Value + (theCurveKey.Value - theCurveLastKey.Value) * theWeight;
				break;
			}
			else if (InEvaluatePos <= theCurveKey.Time)
			{
				thePreviousPoseDistance = theCurveKey.Value;
				break;
			}
		}

		// Check the pose distance is valid
		if (thePreviousPoseDistance == 0.f)
		{
			return 0.f;
		}
	}

	// Return compare result
	return ComparePoses(thePreviousPoseDistance, InFuturePoseCurve);
}

float UVAT_PoseSearchLibrary::ComparePosesAsMontage(float InEvaluatePos, UAnimMontage* InPreviousMontage, UAnimMontage* InFutureAnimMontage
	, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap)
{
	//SCOPE_CYCLE_COUNTER(STAT_ComparePosesMontage);

	// Check the animation montage is valid
	if (InPreviousMontage == nullptr || InPreviousMontage->SlotAnimTracks.Num() == 0)
	{
		return 0.f;
	}

	// Check the animation montage is valid
	if (InFutureAnimMontage == nullptr || InFutureAnimMontage->SlotAnimTracks.Num() == 0)
	{
		return 0.f;
	}

	// Find the previous animation asset
	UAnimSequenceBase* thePreviousAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InPreviousMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		if (theAnimSegment.IsInRange(InEvaluatePos))
		{
			thePreviousAnimSequenceBase = theAnimSegment.AnimReference;
			break;
		}
	}

	// Check previous animation asset is valid
	if (thePreviousAnimSequenceBase == nullptr)
	{
		return 0.f;
	}

	// Find the evaluate animation asset
	float theCompositeStartPos = 0.f;
	UAnimSequenceBase* theEvalAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InFutureAnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		theCompositeStartPos = theAnimSegment.AnimStartTime;
		theEvalAnimSequenceBase = theAnimSegment.AnimReference;
	}

	return ComparePosesAsAnimation(InEvaluatePos, Cast<UAnimSequence>(thePreviousAnimSequenceBase), Cast<UAnimSequence>(theEvalAnimSequenceBase), InReferenceCurveName, InPoseDistanceCurvesMap);
}

float UVAT_PoseSearchLibrary::ComparePosesAsAnimation(float InEvaluatePos, UAnimSequence* InPreviousAnimSequnece, UAnimSequence* InFutureAnimSequnece
	, const FName& InReferenceCurveName, const TMap<float, FName>& InPoseDistanceCurvesMap)
{
	SCOPE_CYCLE_COUNTER(STAT_ComparePosesSequences);

	// Check the previous animation sequence is valid
	if (InPreviousAnimSequnece == nullptr)
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_PoseSearchLibrary::ComparePosesAsAnimation, Invalid previous animation."));
		return -1.f;
	}

	// Check the future animation sequence is valid
	if (InFutureAnimSequnece == nullptr)
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_PoseSearchLibrary::ComparePosesAsAnimation, Invalid previous animation."));
		return -1.f;
	}

	// Check the pose distance curves is valid
	if (InPoseDistanceCurvesMap.Num() == 0)
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_PoseSearchLibrary::ComparePosesAsAnimation, Invalid pose distance curves."));
		return -1.f;
	}

	// Evaluate the reference curve value
	float theReferenceCurveValue = 0.f;
#if 0
	// Find the reference curve
	FRuntimeFloatCurve theReferenceCurve;
	if (!UVAT_CurveLibrary::ConvertAnimCurveToRuntimeCurve(InPreviousAnimSequnece, theReferenceCurve, InReferenceCurveName))
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_PoseSearchLibrary::ComparePosesAsAnimation, Invalid reference curve."));
		return -1.f;
	}

	const TArray<FRichCurveKey>& theReferenceKeys = theReferenceCurve.GetRichCurveConst()->Keys;
	for (int32 KeyIndex = 0; KeyIndex < theReferenceKeys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = theReferenceKeys[KeyIndex];
		if (theReferenceKeys.IsValidIndex(KeyIndex + 1))
		{
			const FRichCurveKey& theNextCurveKey = theReferenceKeys[KeyIndex];
			if (InEvaluatePos >= theCurveKey.Time
				|| FMath::IsWithinInclusive(InEvaluatePos, theCurveKey.Time, theNextCurveKey.Time))
			{
				theReferenceCurveValue = theNextCurveKey.Value;
				break;
			}
		}
		else
		{
			theReferenceCurveValue = theCurveKey.Value;
			break;
		}
	}
#else
	// Check the curve data is valid
	bool bHasCurveData = false;
	bHasCurveData = UVAT_CurveLibrary::GetAnimationAssetCurveValue(InPreviousAnimSequnece, InReferenceCurveName, InEvaluatePos, theReferenceCurveValue);
	if (!bHasCurveData)
	{
		return theReferenceCurveValue;
	}

#endif

#if 0
	// Find the desired pose distance curve
	for (const TPair<float, FName>& thePair : InPoseDistanceCurvesMap)
	{
		// Check the value is equal
		if (!FMath::IsNearlyEqual(thePair.Key, theReferenceCurveValue, 0.001f))
		{
			continue;
		}

		// Find the pose distance curve
		const FRichCurve* theFuturePoseDistanceCurve = &UVAT_CurveLibrary::GetFloatCurveClass<float, FFloatCurve>(InFutureAnimSequnece, thePair.Value)->FloatCurve;
		const FRichCurve* thePreviousPoseDistanceCurve = &UVAT_CurveLibrary::GetFloatCurveClass<float, FFloatCurve>(InPreviousAnimSequnece, thePair.Value)->FloatCurve;
		if (theFuturePoseDistanceCurve != nullptr && thePreviousPoseDistanceCurve != nullptr)
		{
			break;
		}

		// Check the pose distance is valid
		if (theFuturePoseDistanceCurve->IsEmpty() || thePreviousPoseDistanceCurve->IsEmpty())
		{
			return -1.f;
		}

		// Return the desired result
		return ComparePoses(InEvaluatePos, thePreviousPoseDistanceCurve, theFuturePoseDistanceCurve);
	}
#else
	// Find the desired pose distance curve
	for (const TPair<float, FName>& thePair : InPoseDistanceCurvesMap)
	{
		// Check the value is equal
		if (!FMath::IsNearlyEqual(thePair.Key, theReferenceCurveValue, 0.001f))
		{
			continue;
		}

		// Calculate the previous pose distance
		float thePreviousPoseDistance = 0.f;
		bool bHasPreviousCurveData
			= UVAT_CurveLibrary::GetAnimationAssetCurveValue(InPreviousAnimSequnece, thePair.Value, InEvaluatePos, thePreviousPoseDistance);

		// Check the previous pose distance curve is valid
		if (bHasPreviousCurveData == false)
		{
			return -1.f;
		}

		// Define the pose distance data
		float theComparePosTime = -1.f;
		float theComparePosDistance = 1e6f;
		FVector2D theLastPoseKey(0.f, 0.f);

		// Get the animation asset data
#if ENGINE_MAJOR_VERSION > 4
		const int32& theNumberOfKeys = InFutureAnimSequnece->GetNumberOfSampledKeys();
#else
		const int32& theNumberOfKeys = InFutureAnimSequnece->GetNumberOfFrames();
#endif

		// Retrieve smart name for curve
		const FSmartName theCurveSmartName = UVAT_CurveLibrary::GetCurveSmartName(InFutureAnimSequnece, thePair.Value);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Define the pose key 
			FVector2D thePoseKey(0.f, 0.f);

			// Evaluate the key time
			thePoseKey.X = UVAT_CurveLibrary::GetTimeAtFrame(InFutureAnimSequnece, KeyIndex);

			// Evaluate the pose distance
			thePoseKey.Y = InFutureAnimSequnece->EvaluateCurveData(theCurveSmartName.UID, thePoseKey.X);

			// Return the desired result
			if (ComparePoses(thePreviousPoseDistance, theLastPoseKey, thePoseKey, theComparePosTime, theComparePosDistance))
			{
				return theComparePosTime;
			}

			// Cache the last pose key
			theLastPoseKey = thePoseKey;
		}

		// Failed
		return theComparePosTime;
	}
#endif

	// Failed
	return -1.f;
}