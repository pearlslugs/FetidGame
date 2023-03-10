// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_GameFramework.h"
#include "Library/VAT_Library.h"
#include "VAT_CurveLibrary.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimationAsset.h"
#include "AnimationBlueprintLibrary.h"
#include "Library/VAT_PoseSearch.h"
#include "Library/VAT_Curve.h"
#include "Animation/AnimTypes.h"
#include "Library/VAT_Bone.h"

/*-----------------------------------------------------------------------------
	FVirtualFootWeightData Implementation.
-----------------------------------------------------------------------------*/

bool FVirtualFootWeightData::IsValidComparePosesAsset() const
{
	// Check the array number is valid
	if (ComparePosesAsset.Num() != CurveSampleData.CurvesApexRanges.Num())
	{
		return false;
	}

	// Check the pose asset is valid
	for (UAnimSequence* thePoseAsset : ComparePosesAsset)
	{
		if (thePoseAsset == nullptr)
		{
			return false;
		}
	}

	// Success
	return true;
}

/*-----------------------------------------------------------------------------
	UVAT_GameFramework Implementation.
-----------------------------------------------------------------------------*/

void UVAT_GameFramework::SamplingFootLock(UAnimSequence* InAnimSequence, FVirtualFootLockSampleData& InFootLockData, const TArray<FVirtualLegBaseData>& InLegsBaseData)
{
	// Check the animation is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the curve sample data reference
	FVirtualCurveSampleData& theCurveSampleData = InFootLockData.CurveSampleData;

	// Check the output curves number is valid
	if (theCurveSampleData.RuntimeCurvesMap.Num() == 0 || theCurveSampleData.CurvesApexRanges.Num() == 0)
	{
		return;
	}

	// Generate the curves map
	TArray<FName> theCurvesName;
	theCurveSampleData.RuntimeCurvesMap.GenerateKeyArray(theCurvesName);

	// Check the default curve name is valid
	const FName& theDefaultCurveName = theCurvesName.Num() > 0 ? theCurvesName[0] : NAME_None;
	if (theDefaultCurveName == NAME_None)
	{
		return;
	}

	// Check the legs base data is valid
	if (InLegsBaseData.Num() == 0)
	{
		return;
	}

	// Try sample each animation pose
	const float theAnimLength = InAnimSequence->GetPlayLength();
	const double& thePoseSampleRate = theCurveSampleData.SampleRate.AsInterval();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get root bone component space transform
	const FTransform& theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

	// Rebuild the animation curves
	for (const FName& theCurveName : theCurvesName)
	{
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
	}

	// Initialize all legs runtime data
	int32 thePosesNumber = 0;
	TArray<FVirtualLegPoseRuntimeData> theLegsRuntimeData;
	for (const FVirtualLegBaseData& theLegBaseData : InLegsBaseData)
	{
		FVirtualLegPoseRuntimeData& theLegRuntimeData = theLegsRuntimeData.AddDefaulted_GetRef();
		thePosesNumber = theLegRuntimeData.Initialize(InAnimSequence, thePoseSampleRate, theLegBaseData, true, &theCurveSampleData);
	}

	// Check the poses number is valid
	if (thePosesNumber == 0)
	{
		return;
	}

	// Check the animation has motion data condition
	const bool bHasAnyMotionData = UVAT_Bone::HasAnyMotionData(InAnimSequence);

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

		// Get the grounded plane height
		float theGroundedHeight = theCurveSampleData.GroundedHeight;
		if (theCurveSampleData.GroundedHeights.IsValidIndex(DataIndex))
		{
			theGroundedHeight = theCurveSampleData.GroundedHeights[DataIndex];
		}

		// Get the curve apex value
		const FVector2D& theCurveValueRange = theCurveSampleData.CurvesApexRanges.IsValidIndex(DataIndex) ? theCurveSampleData.CurvesApexRanges[DataIndex] : theCurveSampleData.CurvesApexRanges[0];

		// Get the curve name
		const FName& theCurveName = theCurvesName.IsValidIndex(DataIndex) ? theCurvesName[DataIndex] : theCurvesName[0];

		// Get the runtime curve reference
		FRuntimeFloatCurve* theRuntimeCurveRef = theCurveSampleData.RuntimeCurvesMap.Find(theCurveName);
		if (theRuntimeCurveRef == nullptr)
		{
			theRuntimeCurveRef = theCurveSampleData.RuntimeCurvesMap.Find(theDefaultCurveName);
		}
		FRichCurve* theRuntimeCurvePtr = theRuntimeCurveRef->GetRichCurve();
		theRuntimeCurvePtr->Reset();

#if 1
		// Define the max range value
		float theApexValue = 0.f;
		float theLowerValue = 0.f;
		int32 theApexPoseIndex = INDEX_NONE;

		// Each every pose
		for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
		{
			// Calculate the pose time
			const float thePoseValue = theLegRuntimeData.GetMaxPoseHeight(PoseIndex);

			// Calculate the pose time
			const float thePoseTime = (PoseIndex + theCurveSampleData.OffsetFrame) * thePoseSampleRate + theCurveSampleData.OffsetTime;

			// Check the data is landed state
			const bool bIsLanded = theLegRuntimeData.IsLandedPose(PoseIndex
				, theLegRuntimeData.bIsLanded ? theCurveSampleData.AirTolerance : theCurveSampleData.LandedTolerance
				, theGroundedHeight, FLT_MIN, InFootLockData.bUseMinValueInLanded);

			// Handle
			if (PoseIndex == 0 || theLegRuntimeData.bIsLanded != bIsLanded)
			{
				// Calculate the lower value
				theLowerValue = bIsLanded ? theLegRuntimeData.GetMinPoseHeight(PoseIndex) : theLowerValue;

				// Cache the state
				theLegRuntimeData.bIsLanded = bIsLanded;

				// Calculate the max apex value
				if (!bIsLanded)
				{
					// Clear the cached data
					theApexValue = 0.f;
					theApexPoseIndex = INDEX_NONE;

					// Each every future pose
					for (int32 i = PoseIndex; i < thePosesNumber; i++)
					{
						// Calculate the landed state
						const float thePoseHeight = theLegRuntimeData.GetMaxPoseHeight(i);
						const bool theTrajectoryLanded = theLegRuntimeData.IsLandedPose(i
							, theLegRuntimeData.bIsLanded ? theCurveSampleData.AirTolerance : theCurveSampleData.LandedTolerance
							, theGroundedHeight, FLT_MIN, InFootLockData.bUseMinValueInLanded);

						// Trajectory landed state
						if (theTrajectoryLanded)
						{
							theApexPoseIndex = i;
							break;
						}
						else if (thePoseHeight >= theApexValue)
						{
							theApexValue = thePoseHeight;
						}
					}
				}

				// Check if the interval between two keys is less than the minimum interval we expect
				if (theCurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
				{
					// Calculate both key delta time
					const float theDeltaTime = thePoseTime - theRuntimeCurvePtr->GetLastKey().Time;
					if (theDeltaTime < theCurveSampleData.Interval)
					{
						continue;
					}
				}
			}

			// Calculate the alpha value
			const float theAlpha = (theApexValue - theLowerValue) == 0.f ? 0.f : FMath::Abs((thePoseValue - theLowerValue) / (theApexValue - theLowerValue));
			float theOutputValue = bIsLanded ? theCurveValueRange.Y : (InFootLockData.bMakeFullWeight ? 0.f : (1.f - theAlpha));

			// Calculate the motion data
			if (InFootLockData.bUnLockInNoneMotion && bHasAnyMotionData)
			{
				const float theAnimKeyTime = FMath::Clamp((PoseIndex + /*1*/0) * float(thePoseSampleRate), 0.f, theAnimLength);
				if (!UVAT_Bone::HasMotionData(InAnimSequence, theAnimKeyTime - float(thePoseSampleRate), theAnimKeyTime, thePoseSampleRate, true))
				{
					theOutputValue = 0.f;
				}
			}

			// We should avoid situations where the curve reverses
			if (InFootLockData.bAvoidInverseCurve && theRuntimeCurvePtr->Keys.Num() > 1
				&& theOutputValue > KINDA_SMALL_NUMBER && FAnimWeight::IsRelevant(theOutputValue))
			{
				const float& pA = theRuntimeCurvePtr->Keys[theRuntimeCurvePtr->Keys.Num() - 2].Value;
				const float& pB = theRuntimeCurvePtr->Keys[theRuntimeCurvePtr->Keys.Num() - 1].Value;
				if (FAnimWeight::IsRelevant(pA) && FAnimWeight::IsRelevant(pB))
				{
					if (pA < pB) // Is up curve
					{
						if (theOutputValue < pB)
						{
							continue;
						}
					}
					else if (pA != pB) // Is down curve
					{
						if (theOutputValue > pB)
						{
							continue;
						}
					}
				}
			}

			// Add to curve key
			theRuntimeCurvePtr->AddKey(thePoseTime, theOutputValue);
		}

		// Apply curve filter
		UVAT_Curve::ApplyCurveFilter(*theRuntimeCurveRef, theCurveSampleData.CurveFilterType, theCurveSampleData.CurveFilterTolerance);

		// Always keep final track is blend out
		if (InFootLockData.bUnLockInNoneMotion && !bHasAnyMotionData)
		{
			FRuntimeFloatCurve theFilterCurve = *theRuntimeCurveRef;
			if (theRuntimeCurvePtr->Keys.Num() > 3)
			{
				const int32 theFinalKeyIndex = theRuntimeCurvePtr->Keys.Num() - 1;
				if (theRuntimeCurvePtr->Keys[theFinalKeyIndex].Value == theCurveValueRange.Y
					&& theRuntimeCurvePtr->Keys[theFinalKeyIndex - 1].Value == theCurveValueRange.Y
					&& theRuntimeCurvePtr->Keys[theFinalKeyIndex - 2].Value == theCurveValueRange.X)
				{
					theFilterCurve.GetRichCurve()->UpdateOrAddKey(theRuntimeCurvePtr->Keys[theFinalKeyIndex].Time, theCurveValueRange.X);
					theFilterCurve.GetRichCurve()->DeleteKey(theFilterCurve.GetRichCurve()->FindKey(theRuntimeCurvePtr->Keys[theFinalKeyIndex - 1].Time));
				}
			}

			// Move temp the keys
			theRuntimeCurvePtr->Keys = theFilterCurve.GetRichCurve()->Keys;
		}

		// Apply blend in
		FRuntimeFloatCurve theBlendInCurve = *theRuntimeCurveRef;
		for (int32 KeyIndex = 1; KeyIndex < theRuntimeCurvePtr->Keys.Num(); KeyIndex++)
		{
			FRichCurveKey pA = theRuntimeCurvePtr->Keys[KeyIndex - 1];
			FRichCurveKey pB = theRuntimeCurvePtr->Keys[KeyIndex];

			// Choose blend alpha
			if (pB.Time < theAnimLength && pA.Value < pB.Value) // Is up curve, blend in
			{
				FAlphaBlend theBlendIn = InFootLockData.BlendIn;

				// Check the left offset key time
				float theMinKeyTime = 0.f;
				if (theRuntimeCurvePtr->Keys.IsValidIndex(KeyIndex - 2))
				{
					theMinKeyTime = theRuntimeCurvePtr->Keys[KeyIndex - 2].Time;
				}

				// Resize the ratio time
				float theEndTime = FMath::Clamp(pB.Time + theBlendIn.GetBlendTime() * FMath::Clamp(InFootLockData.BlendInOffsetRatio, 0.f, 1.f), 0.f, theAnimLength);
				
				// Blend two key
				theBlendIn.Reset();
				float theBlendTime = FMath::Min(theBlendIn.GetBlendTime(), theEndTime - theMinKeyTime);

				// avoid blend to none motion data
				if (InFootLockData.bUnLockInNoneMotion && bHasAnyMotionData && FAnimWeight::IsRelevant(pB.Value))
				{
					float theKeyTime = theEndTime - theBlendTime;
					float thePoseSampleTime = 0.f;
					while (thePoseSampleTime <= (theBlendTime + KINDA_SMALL_NUMBER))
					{
						// Clamp the pose time
						thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theBlendTime);

						// Calculate the motion data
						bool bHasMotionData = UVAT_Bone::HasMotionData(InAnimSequence, theKeyTime - thePoseSampleRate, theKeyTime, thePoseSampleRate, true);
						if (!bHasMotionData)
						{
							// Rebuild the curve
							theEndTime = theKeyTime;
							theBlendTime = theEndTime - pB.Time;

							// Don't blend it now.
							theBlendTime = 0.f;

							// Delete the keys
							for (int Index = theRuntimeCurvePtr->Keys.Num() - 1; Index >= 0; Index--)
							{
								const FRichCurveKey theDeleteKey = theRuntimeCurvePtr->Keys[Index];
								if (theDeleteKey.Time >= (pB.Time - KINDA_SMALL_NUMBER) && theDeleteKey.Value != 0.f)
								{
									theBlendInCurve.GetRichCurve()->DeleteKey(theBlendInCurve.GetRichCurve()->FindKey(theDeleteKey.Time));
								}
							}
							break;
						}

						// Iterate
						theKeyTime += thePoseSampleRate;
						thePoseSampleTime += thePoseSampleRate;
					}
				}

				// Blend two key
				if (theBlendTime > 0.f)
				{
					// Delete the range time
					theBlendInCurve.GetRichCurve()->DeleteKey(theBlendInCurve.GetRichCurve()->FindKey(pB.Time));

					// Adjust the blend time
					theBlendIn.SetBlendTime(theBlendTime);

					// Sample the pose features
					float theKeyTime = theEndTime - theBlendTime;
					float thePoseSampleTime = 0.f;
					while (thePoseSampleTime <= (theBlendTime + KINDA_SMALL_NUMBER))
					{
						// Clamp the pose time
						thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theBlendTime);

						// Calculate the blend weight
						const float theBlendWeight = theBlendIn.GetBlendedValue();

						// Add key 
						const float theKeyValue = pA.Value + (pB.Value - pA.Value) * theBlendWeight;
						FKeyHandle theKeyHandle = theBlendInCurve.GetRichCurve()->UpdateOrAddKey(theKeyTime, theKeyValue);
						//theBlendInCurve.GetRichCurve()->SetKeyInterpMode(theKeyHandle, theBlendIn.GetBlendOption())

						// Iterate
						theKeyTime += thePoseSampleRate;
						thePoseSampleTime += thePoseSampleRate;
						theBlendIn.Update(thePoseSampleRate + KINDA_SMALL_NUMBER);
					}
				}
			}
		}

		// Move temp the keys
		theRuntimeCurvePtr->Keys = theBlendInCurve.GetRichCurve()->Keys;

		// Apply blend out
		FRuntimeFloatCurve theBlendOutCurve = *theRuntimeCurveRef;
		for (int32 KeyIndex = 1; KeyIndex < theRuntimeCurvePtr->Keys.Num(); KeyIndex++)
		{
			FRichCurveKey pA = theRuntimeCurvePtr->Keys[KeyIndex - 1];
			FRichCurveKey pB = theRuntimeCurvePtr->Keys[KeyIndex];

			// Choose blend alpha
			if (pA.Value > pB.Value) // Is down curve, blend out
			{
				// Calculate the motion data
				bool bHasMotionData = true;
				if (InFootLockData.bUnLockInNoneMotion && bHasAnyMotionData)
				{
					bHasMotionData = UVAT_Bone::HasMotionData(InAnimSequence, pB.Time, pB.Time + thePoseSampleRate, thePoseSampleRate, true);
				}

				// Get the blend out data
				FAlphaBlend theBlendOut = bHasMotionData ? InFootLockData.BlendOut : InFootLockData.NoneMotionBlendOut;
				const float theBlendOutOffsetRatio = bHasMotionData ? InFootLockData.BlendOutOffsetRatio : InFootLockData.NoneMotionBlendOutOffsetRatio;

				// Check the left offset key time
				float theMinKeyTime = 0.f;
				if (theRuntimeCurvePtr->Keys.IsValidIndex(KeyIndex - 2))
				{
					theMinKeyTime = theRuntimeCurvePtr->Keys[KeyIndex - 2].Time;
				}

				// Resize the ratio time
				const float theEndTime = FMath::Clamp(pB.Time + theBlendOut.GetBlendTime() * FMath::Clamp(1.f - theBlendOutOffsetRatio, 0.f, 1.f), 0.f, theAnimLength);

				// Blend two key
				theBlendOut.Reset();
				const float& theBlendTime = FMath::Min(theBlendOut.GetBlendTime(), theEndTime - theMinKeyTime);
				if (theBlendTime > 0.f)
				{
					// Delete the range time
					theBlendOutCurve.GetRichCurve()->DeleteKey(theBlendOutCurve.GetRichCurve()->FindKey(pB.Time));

					// Adjust the blend time
					theBlendOut.SetBlendTime(theBlendTime);

					// Sample the pose features
					float theKeyTime = theEndTime - theBlendTime;
					float thePoseSampleTime = 0.f;
					while (thePoseSampleTime <= (theBlendTime + KINDA_SMALL_NUMBER))
					{
						// Clamp the pose time
						thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theBlendTime);

						// Calculate the blend weight
						const float theBlendWeight = theBlendOut.GetBlendedValue();

						// Add key 
						const float theKeyValue = pA.Value + (pB.Value - pA.Value) * theBlendWeight;
						FKeyHandle theKeyHandle = theBlendOutCurve.GetRichCurve()->UpdateOrAddKey(theKeyTime, theKeyValue);

						// Iterate
						theKeyTime += thePoseSampleRate;
						thePoseSampleTime += thePoseSampleRate;
						theBlendOut.Update(thePoseSampleRate + KINDA_SMALL_NUMBER);
					}
				}
			}
		}

		// Move temp the keys
		theRuntimeCurvePtr->Keys = theBlendOutCurve.GetRichCurve()->Keys;

		// Apply curve filter
		UVAT_Curve::ApplyCurveFilter(*theRuntimeCurveRef, theCurveSampleData.CurveFilterType, theCurveSampleData.CurveFilterTolerance);
#else
		// Calculate the result
		for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
		{
			// Check the data is landed state
			const bool bIsLanded = theLegRuntimeData.IsLandedPose(PoseIndex
				, theLegRuntimeData.bIsLanded ? theCurveSampleData.AirTolerance : theCurveSampleData.LandedTolerance
				, theGroundedHeight, theCurveSampleData.HorizontalTolerance);

			// Handle
			if (theLegRuntimeData.bIsLanded != bIsLanded)
			{
				// Cache the state
				theLegRuntimeData.bIsLanded = bIsLanded;

				// Calculate the pose time
				const float thePoseTime = (PoseIndex + theCurveSampleData.OffsetFrame) * thePoseSampleRate + theCurveSampleData.OffsetTime;

				// Check if the interval between two keys is less than the minimum interval we expect
				if (theCurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
				{
					// Calculate both key delta time
					const float theDeltaTime = thePoseTime - theRuntimeCurvePtr->GetLastKey().Time;
					if (theDeltaTime < theCurveSampleData.Interval)
					{
						continue;
					}
				}

				// Cache the landed curve value
				const float theLandedCurveValue = InLegsBaseData[DataIndex].CurveApexValue;

				// Choose rich curve interp mode
				if (theCurveSampleData.CurveInterpMode == ERichCurveInterpMode::RCIM_Constant)
				{
					// Add both curve key
					const FKeyHandle& theNewKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.Y : theCurveValueRange.X);
					if (PoseIndex != 0)
					{
						const FKeyHandle& theLastKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.X : theCurveValueRange.Y);
					}
				}
				else
				{
					// Add both curve key
					const FKeyHandle& theNewKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.Y : theCurveValueRange.X);
					if (/*!bIsLanded && */PoseIndex != 0)
					{
						const FKeyHandle& theLastKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.X : theCurveValueRange.Y);
					}
				}
			}
			else if (PoseIndex == 0)
			{
				// Add the first key
				const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(0.f, theCurveValueRange.X);
			}
		}

		// Add the final key
		{
			const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(InAnimSequence->GetPlayLength(), theRuntimeCurvePtr->GetLastKey().Value);
			//const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(InAnimSequence->GetPlayLength(), theCurveValueRange.X);
		}

		// Other transition method
		if (false && theCurveSampleData.CurveInterpMode != ERichCurveInterpMode::RCIM_Constant)
		{
			// Traverse all keys and find the last 0-1 range curve
			for (int32 KeyIndex = theRuntimeCurvePtr->Keys.Num() - 1; KeyIndex >= 1; KeyIndex--)
			{
				const FRichCurveKey& theCurveKey = theRuntimeCurvePtr->Keys[KeyIndex];
				const FRichCurveKey& theLastCurveKey = theRuntimeCurvePtr->Keys[KeyIndex - 1];

				// Delete final full weight keys, always unlock foot in last position
				if (theCurveKey.Value == 1.f && theLastCurveKey.Value == 0.f)
				{
					// Delete invalid keys
					for (int32 DeleteKeyIndex = theRuntimeCurvePtr->Keys.Num() - 1; DeleteKeyIndex >= KeyIndex + 1; DeleteKeyIndex--)
					{
						theRuntimeCurvePtr->DeleteKey(theRuntimeCurvePtr->FindKey(theRuntimeCurvePtr->Keys[DeleteKeyIndex].Time));
					}

					// Get the keys number
					const int32 theKeysNumber = theRuntimeCurvePtr->Keys.Num();

					// Adjust last key is zero weight
					theRuntimeCurvePtr->Keys[theKeysNumber - 1].Value = 0.f;

					// Delete last constant key
					if (theKeysNumber >= 3
						&& theRuntimeCurvePtr->Keys[theKeysNumber - 2].Time
						== theRuntimeCurvePtr->Keys[theKeysNumber - 3].Time)
					{
						// Get the source curve keys data
						TArray<FKeyHandle> theSourceKeysHandle;

						// Get all keys handle
						for (auto It = theRuntimeCurvePtr->GetKeyHandleIterator(); It; ++It)
						{
							theSourceKeysHandle.Add(*It);
						}

						// Each every curve keys
						for (int32 ClearKeyIndex = theSourceKeysHandle.Num() - 2; ClearKeyIndex >= 0; ClearKeyIndex--)
						{
							// Get key handle
							const FKeyHandle& theKeyHandle = theSourceKeysHandle[ClearKeyIndex];

							// Delete the key
							if (theRuntimeCurvePtr->GetKeyValue(theKeyHandle) == 0.f)
							{
								theRuntimeCurvePtr->DeleteKey(theKeyHandle);
							}
						}
					}
					break;
				}
			}
		}
#endif

		// Convert the runtime curve to animation curve
		UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, *theRuntimeCurveRef, theCurveName, theCurveSampleData.CurveInterpMode);
	}

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->BakeTrackCurvesToRawAnimation();
#endif

	// Apply the animation asset modify
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_GameFramework::SamplingFootWeight(UAnimSequence* InAnimSequence
	, FVirtualFootWeightData& InFootWeightData, const FVirtualPoseSearchSampleData& InPoseSampleData)
{
	// Check the animation is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Check the output curves number is valid
	if (InFootWeightData.CurveSampleData.RuntimeCurvesMap.Num() == 0 || InFootWeightData.CurveSampleData.CurvesApexRanges.Num() == 0)
	{
		return;
	}

	// Generate the curves map
	TArray<FName> theCurvesName;
	InFootWeightData.CurveSampleData.RuntimeCurvesMap.GenerateKeyArray(theCurvesName);

	// Check the default curve name is valid
	const FName& theDefaultCurveName = theCurvesName.Num() > 0 ? theCurvesName[0] : NAME_None;
	if (theDefaultCurveName == NAME_None)
	{
		return;
	}

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Try sample each animation pose
	const double& thePoseSampleRate = InFootWeightData.CurveSampleData.SampleRate.AsInterval();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get root bone component space transform
	const FTransform& theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

	// Rebuild the animation curves
	for (const FName& theCurveName : theCurvesName)
	{
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
	}

	// Get the curve name
	const FName& theCurveName = theCurvesName[0];

	// Check the runtime curve is valid
	FRuntimeFloatCurve* theRuntimeCurveRef = InFootWeightData.CurveSampleData.RuntimeCurvesMap.Find(theCurveName);
	if (theRuntimeCurveRef == nullptr)
	{
		return;
	}

	// Clear the curve data
	FRichCurve* theRuntimeCurvePtr = theRuntimeCurveRef->GetRichCurve();
	theRuntimeCurvePtr->Reset();

	// Get the curve apex value
	const FVector2D& theCurveValueRange = InFootWeightData.CurveSampleData.CurvesApexRanges[0];

	// Each every compare pose asset
	bool bHasComparePosesAsset = false;
	for (UAnimSequence* thePosesAsset : InFootWeightData.ComparePosesAsset)
	{
		// Check the asset is valid
		if (thePosesAsset == nullptr)
		{
			bHasComparePosesAsset = false;
			continue;
		}

		// Flag state
		bHasComparePosesAsset = true;
	}

	// Sample data
	if (bHasComparePosesAsset)
	{
		// Debug tolerance result
		float theToleranceResult = 0.f;

		// Calculate the result
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Each every compare pose asset
			for (int32 PoseIndex = 0; PoseIndex < InFootWeightData.ComparePosesAsset.Num(); PoseIndex++)
			{
				// Check the asset is valid
				UAnimSequence* thePosesAsset = InFootWeightData.ComparePosesAsset[PoseIndex];
				if (thePosesAsset == nullptr)
				{
					continue;
				}

				// Check the sample pose time is valid
				float theComparePoseTime = 0.f;
				if (InFootWeightData.ComparePosesTime.IsValidIndex(PoseIndex))
				{
					theComparePoseTime = InFootWeightData.ComparePosesTime[PoseIndex];
				}

				// Check the sample pose
				if (UVAT_PoseSearch::IsSampePose(theKeyTime, theComparePoseTime, InAnimSequence, thePosesAsset, InFootWeightData.CompareTolerance, &theToleranceResult))
				{
					// Check if the interval between two keys is less than the minimum interval we expect
					if (InFootWeightData.CurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
					{
						// Calculate both key delta time
						const float theDeltaTime = theKeyTime - theRuntimeCurvePtr->GetLastKey().Time;
						if (theDeltaTime < InFootWeightData.CurveSampleData.Interval)
						{
							continue;
						}
					}

					// Add same pose curve key
					theRuntimeCurvePtr->AddKey(theKeyTime, theCurveValueRange.Y);

					// Add left range key
					if (theKeyTime != 0.f && InFootWeightData.OffsetRange.X < 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.X, 0.f, theSequenceLength);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);
					}

					// Add right range key
					if (theKeyTime < (theSequenceLength - KINDA_SMALL_NUMBER) && InFootWeightData.OffsetRange.Y > 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.Y, 0.f, theSequenceLength);
						if (!FMath::IsNearlyEqual(theRangeKeyTime, theSequenceLength, KINDA_SMALL_NUMBER))
						{
							theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);
						}
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
					}
				}
			}
		}
	}
	else if (InFootWeightData.ComparePosesTime.Num() > 0)
	{
		// Debug tolerance result
		float theToleranceResult = 0.f;
		TArray<float> theRecordComparePoseTimes;

		// Calculate the result
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Each every compare pose time
			for (const float& theComparePoseTime : InFootWeightData.ComparePosesTime)
			{
				// Ignore recorded time
				if (theRecordComparePoseTimes.Contains(theComparePoseTime))
				{
					continue;
				}

				// Check the sample pose
				if (theKeyTime >= theComparePoseTime
					|| FMath::IsNearlyEqual(theKeyTime, theComparePoseTime, KINDA_SMALL_NUMBER))
				{
					theRecordComparePoseTimes.Add(theComparePoseTime);

					// Check if the interval between two keys is less than the minimum interval we expect
					if (InFootWeightData.CurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
					{
						// Calculate both key delta time
						const float theDeltaTime = theKeyTime - theRuntimeCurvePtr->GetLastKey().Time;
						if (theDeltaTime < InFootWeightData.CurveSampleData.Interval)
						{
							continue;
						}
					}

					// Add same pose curve key
					theRuntimeCurvePtr->AddKey(theKeyTime, theCurveValueRange.Y);

					// Add left range key
					if (theKeyTime != 0.f && InFootWeightData.OffsetRange.X < 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.X, 0.f, theSequenceLength);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);
					}

					// Add right range key
					if (theKeyTime < (theSequenceLength - KINDA_SMALL_NUMBER) && InFootWeightData.OffsetRange.Y > 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.Y, 0.f, theSequenceLength);
						if (!FMath::IsNearlyEqual(theRangeKeyTime, theSequenceLength, KINDA_SMALL_NUMBER))
						{
							theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);
						}
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
					}
				}
			}
		}
	}
	else
	{
		// Sample source pose distance
		FVirtualPoseSearchSampleData theSourcePoseSampleData = InPoseSampleData;
		UVAT_PoseSearch::SamplePoseFeatures<float, FFloatCurve>(InAnimSequence, theSourcePoseSampleData, false);

		// Sample reference pose asset
		TArray<FVirtualPoseSearchSampleData> theSamplePosesData;
		for (UAnimSequence* thePosesAsset : InFootWeightData.ComparePosesAsset)
		{
			// Check the asset is valid
			if (thePosesAsset == nullptr)
			{
				theSamplePosesData.AddDefaulted();
				continue;
			}

			// Sample pose distance
			FVirtualPoseSearchSampleData& thePoseSampleData = theSamplePosesData.AddDefaulted_GetRef();
			thePoseSampleData = InPoseSampleData;
			UVAT_PoseSearch::SamplePoseFeatures<float, FFloatCurve>(thePosesAsset, thePoseSampleData, false);
		}

		check(theSamplePosesData.Num() == InFootWeightData.ComparePosesAsset.Num());

		// Calculate the result
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Find can sample curve index
			FVector2D theSampleCurveIndex = UVAT_PoseSearch::GetCanSampleIndex(InAnimSequence, theKeyTime, theSourcePoseSampleData);

			// Check the sample curve index is valid
			if (theSampleCurveIndex == FVector2D(float(INDEX_NONE)))
			{
				continue;
			}

			// Get the source pose distance
			const float theSourcePoseDistance
				= UVAT_PoseSearch::GetPoseDistance(theKeyTime, theSourcePoseSampleData.BonesData[theSampleCurveIndex.X].SampleCurvesData[theSampleCurveIndex.Y]);

			// Sample all delta source pose distance
			TArray<float> theDeltaPoseDistances;

			// Each every sample pose asset
			for (int32 PoseAssetIndex = 0; PoseAssetIndex < InFootWeightData.ComparePosesAsset.Num(); PoseAssetIndex++)
			{
				UAnimSequence* thePosesAsset = InFootWeightData.ComparePosesAsset[PoseAssetIndex];
				FVirtualPoseSearchSampleData& thePosesSampleData = theSamplePosesData[PoseAssetIndex];

				// Check the asset is valid
				if (thePosesAsset == nullptr)
				{
					theDeltaPoseDistances.Add(FLT_MAX);
					continue;
				}

				// Get the other pose distance
				const float theOtherPoseDistance
					= UVAT_PoseSearch::GetPoseDistance(theKeyTime, thePosesSampleData.BonesData[theSampleCurveIndex.X].SampleCurvesData[theSampleCurveIndex.Y]);

				// Check the pose distance is valid
				if (theOtherPoseDistance == 0.f)
				{
					continue;
				}

				// Calculate the abs delta value
				theDeltaPoseDistances.Add(FMath::Abs(FMath::Abs(theSourcePoseDistance) - FMath::Abs(theOtherPoseDistance)));
			}

			// Check the delta pose distances number is valid
			if (theDeltaPoseDistances.Num() == 0)
			{
				continue;
			}

			// Calculate the best pose index and pose distance
			int32 theBestIndex;
			const float theBestDeltaPoseDistance = FMath::Min(theDeltaPoseDistances, &theBestIndex);

			// Calculate the tolerance
			const float theMinTolerance = InFootWeightData.CurveSampleData.LandedTolerance;
			if (theBestDeltaPoseDistance <= theMinTolerance)
			{
				theRuntimeCurvePtr->AddKey(theKeyTime, theCurveValueRange.Y);
			}
		}
	}

	// Convert the runtime curve to animation curve
	UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, *theRuntimeCurveRef, theCurveName, ERichCurveInterpMode::RCIM_Constant);

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->BakeTrackCurvesToRawAnimation();
#endif

	// Apply the animation asset modify
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_GameFramework::SamplingFootLanded(UAnimSequence* InAnimSequence, FVirtualCurveSampleData& InCurveSampleData, const TArray<FVirtualLegBaseData>& InLegsBaseData)
{
	// Check the animation is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Check the output curves number is valid
	if (InCurveSampleData.RuntimeCurvesMap.Num() == 0 || InCurveSampleData.CurvesApexRanges.Num() == 0)
	{
		return;
	}

	// Generate the curves map
	TArray<FName> theCurvesName;
	InCurveSampleData.RuntimeCurvesMap.GenerateKeyArray(theCurvesName);

	// Check the default curve name is valid
	const FName& theDefaultCurveName = theCurvesName.Num() > 0 ? theCurvesName[0] : NAME_None;
	if (theDefaultCurveName == NAME_None)
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
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get root bone component space transform
	const FTransform& theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

	// Rebuild the animation curves
	for (const FName& theCurveName : theCurvesName)
	{
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
	}

	// Initialize all legs runtime data
	int32 thePosesNumber = 0;
	TArray<FVirtualLegPoseRuntimeData> theLegsRuntimeData;
	for (const FVirtualLegBaseData& theLegBaseData : InLegsBaseData)
	{
		FVirtualLegPoseRuntimeData& theLegRuntimeData = theLegsRuntimeData.AddDefaulted_GetRef();
		thePosesNumber = theLegRuntimeData.Initialize(InAnimSequence, thePoseSampleRate, theLegBaseData);
	}

	// Check the poses number is valid
	if (thePosesNumber == 0)
	{
		return;
	}

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
		
		// Get the curve apex value
		const FVector2D& theCurveValueRange = InCurveSampleData.CurvesApexRanges.IsValidIndex(DataIndex) ? InCurveSampleData.CurvesApexRanges[DataIndex] : InCurveSampleData.CurvesApexRanges[0];

		// Get the curve name
		const FName& theCurveName = theCurvesName.IsValidIndex(DataIndex) ? theCurvesName[DataIndex] : theCurvesName[0];

		// Get the runtime curve reference
		FRuntimeFloatCurve* theRuntimeCurveRef = InCurveSampleData.RuntimeCurvesMap.Find(theCurveName);
		if (theRuntimeCurveRef == nullptr)
		{
			theRuntimeCurveRef = InCurveSampleData.RuntimeCurvesMap.Find(theDefaultCurveName);
		}
		FRichCurve* theRuntimeCurvePtr = theRuntimeCurveRef->GetRichCurve();
		theRuntimeCurvePtr->Reset();

		// Calculate the result
		for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
		{
			// Check the data is landed state
			const bool bIsLanded = theLegRuntimeData.IsLandedPose(PoseIndex, theLegRuntimeData.bIsLanded ? InCurveSampleData.AirTolerance : InCurveSampleData.LandedTolerance, InCurveSampleData.GroundedHeight);

			// Handle
			if (theLegRuntimeData.bIsLanded != bIsLanded)
			{
				// Cache the state
				theLegRuntimeData.bIsLanded = bIsLanded;

				// Calculate the pose time
				const float thePoseTime = (PoseIndex + InCurveSampleData.OffsetFrame) * thePoseSampleRate + InCurveSampleData.OffsetTime;

				// Check if the interval between two keys is less than the minimum interval we expect
				if (InCurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
				{
					// Calculate both key delta time
					const float theDeltaTime = thePoseTime - theRuntimeCurvePtr->GetLastKey().Time;
					if (theDeltaTime < InCurveSampleData.Interval)
					{
						continue;
					}
				}

				// Cache the landed curve value
				const float theLandedCurveValue = InLegsBaseData[DataIndex].CurveApexValue;

				// Choose rich curve interp mode
				if (true || InCurveSampleData.CurveInterpMode == ERichCurveInterpMode::RCIM_Constant)
				{
					// Add both curve key
					const FKeyHandle& theNewKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.Y : theCurveValueRange.X);
					if (PoseIndex != 0)
					{
						const FKeyHandle& theLastKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.X : theCurveValueRange.Y);
					}
				}
				else
				{
					// Add both curve key
					const FKeyHandle& theNewKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.Y : theCurveValueRange.X);
					if (!bIsLanded && PoseIndex != 0)
					{
						const FKeyHandle& theLastKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, bIsLanded ? theCurveValueRange.X : theCurveValueRange.Y);
					}
				}
			}
			else if (PoseIndex == 0)
			{
				// Add the first key
				const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(0.f, theCurveValueRange.X);
			}
		}

		// Add the final key
		{
			const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(InAnimSequence->GetPlayLength(), theRuntimeCurvePtr->GetLastKey().Value);
		}

		// Convert the runtime curve to animation curve
		UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, *theRuntimeCurveRef, theCurveName, InCurveSampleData.CurveInterpMode);
	}

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->BakeTrackCurvesToRawAnimation();
#endif

	// Apply the animation asset modify
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_GameFramework::SamplingFootPosition(UAnimSequence* InAnimSequence, FVirtualFootWeightData& InFootWeightData, const TArray<FVirtualLegBaseData>& InLegsBaseData)
{
	// Check the animation is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Check the output curves number is valid
	if (InFootWeightData.CurveSampleData.RuntimeCurvesMap.Num() == 0
		|| InFootWeightData.CurveSampleData.CurvesApexRanges.Num() != InLegsBaseData.Num())
	{
		return;
	}

	// Generate the curves map
	TArray<FName> theCurvesName;
	InFootWeightData.CurveSampleData.RuntimeCurvesMap.GenerateKeyArray(theCurvesName);

	// Check the default curve name is valid
	const FName& theDefaultCurveName = theCurvesName.Num() > 0 ? theCurvesName[0] : NAME_None;
	if (theDefaultCurveName == NAME_None)
	{
		return;
	}

	// Check the legs base data is valid
	if (InLegsBaseData.Num() == 0)
	{
		return;
	}

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Try sample each animation pose
	const double& thePoseSampleRate = InFootWeightData.CurveSampleData.SampleRate.AsInterval();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get root bone component space transform
	const FTransform& theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

	// Clear the previous cached data
	FRuntimeFloatCurve* theRuntimeCurveRef = InFootWeightData.CurveSampleData.RuntimeCurvesMap.Find(theDefaultCurveName);
	FRichCurve* theRuntimeCurvePtr = theRuntimeCurveRef->GetRichCurve();
	theRuntimeCurvePtr->Reset();

	// Rebuild the animation curves
	for (const FName& theCurveName : theCurvesName)
	{
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
	}

	// Choose compare pose assets or sample landed poses
	if (InFootWeightData.IsValidComparePosesAsset())
	{
		// Get the curve apex value
		FVector2D theCurveValueRange = InFootWeightData.CurveSampleData.CurvesApexRanges[0];

		// Debug tolerance result
		float theToleranceResult = 0.f;

		// Calculate the result
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Each every compare pose asset
			for (int32 PoseIndex = 0; PoseIndex < InFootWeightData.ComparePosesAsset.Num(); PoseIndex++)
			{
				// Check the asset is valid
				UAnimSequence* thePosesAsset = InFootWeightData.ComparePosesAsset[PoseIndex];
				if (thePosesAsset == nullptr)
				{
					continue;
				}

				// Check the sample pose time is valid
				float theComparePoseTime = 0.f;
				if (InFootWeightData.ComparePosesTime.IsValidIndex(PoseIndex))
				{
					theComparePoseTime = InFootWeightData.ComparePosesTime[PoseIndex];
				}

				// Get the curve apex value
				if (InFootWeightData.CurveSampleData.CurvesApexRanges.IsValidIndex(PoseIndex))
				{
					theCurveValueRange = InFootWeightData.CurveSampleData.CurvesApexRanges[PoseIndex];
				}

				// Check the sample pose
				if (UVAT_PoseSearch::IsSampePose(theKeyTime, theComparePoseTime, InAnimSequence, thePosesAsset, InFootWeightData.CompareTolerance, &theToleranceResult))
				{
					// Add same pose curve key
					theRuntimeCurvePtr->AddKey(theKeyTime, theCurveValueRange.Y);

					// Add left range key
					if (theKeyTime != 0.f && InFootWeightData.OffsetRange.X < 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.X, 0.f, theSequenceLength);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
						//theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);

						// Add last curve key, keep constant
						if (theRuntimeCurvePtr->Keys.Num() >= 3)
						{
							theRuntimeCurvePtr->AddKey(theRangeKeyTime, theRuntimeCurvePtr->Keys[theRuntimeCurvePtr->Keys.Num() - 4].Value);
						}
					}

					// Add right range key
					if (theKeyTime < (theSequenceLength - KINDA_SMALL_NUMBER) && InFootWeightData.OffsetRange.Y > 0.f)
					{
						const float theRangeKeyTime = FMath::Clamp(theKeyTime + InFootWeightData.OffsetRange.Y, 0.f, theSequenceLength);
						//theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.X);
						theRuntimeCurvePtr->AddKey(theRangeKeyTime, theCurveValueRange.Y);
					}
				}
			}
		}
	}
	else
	{
		// Initialize all legs runtime data
		int32 thePosesNumber = 0;
		TArray<FVirtualLegPoseRuntimeData> theLegsRuntimeData;
		for (const FVirtualLegBaseData& theLegBaseData : InLegsBaseData)
		{
			FVirtualLegPoseRuntimeData& theLegRuntimeData = theLegsRuntimeData.AddDefaulted_GetRef();
			thePosesNumber = theLegRuntimeData.Initialize(InAnimSequence, thePoseSampleRate, theLegBaseData);
		}

		// Check the poses number is valid
		if (thePosesNumber == 0)
		{
			return;
		}

		// Each every leg base data
		for (int32 DataIndex = 0; DataIndex < InLegsBaseData.Num(); DataIndex = DataIndex + 2)
		{
			const FVirtualLegBaseData& theLegBaseData = InLegsBaseData[DataIndex];

			// Check the leg bone is valid
			if (!theLegBaseData.IsValid())
			{
				continue;
			}

			// Define the leader data index
			int32 theLeaderDataIndex = DataIndex;

			// Get the grounded height
			float theLeaderGroundedHeight = InFootWeightData.CurveSampleData.GroundedHeights.IsValidIndex(DataIndex)
				? InFootWeightData.CurveSampleData.GroundedHeights[DataIndex]
				: InFootWeightData.CurveSampleData.GroundedHeight;

			// Get the tolerance value
			{
				const float& theTolerance = theLegsRuntimeData[theLeaderDataIndex].bIsLanded ? InFootWeightData.CurveSampleData.AirTolerance : InFootWeightData.CurveSampleData.LandedTolerance;

				// Define the leader skeleton, change the leader once without a landing
				const bool bIsLanded = theLegsRuntimeData[DataIndex].IsLandedPose(0, theTolerance, theLeaderGroundedHeight);

				// We should check the foot has move?
				if (bIsLanded)
				{
					// Evaluate the bone motion distance
					float aD = 0.f;
					FTransform pA = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, 0, theLegBaseData.FootBone.BoneName);
					for (int32 I = 1; I <= InFootWeightData.CompareFrame; I++)
					{
						const FTransform theTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, I, theLegBaseData.FootBone.BoneName);
						aD += (FVector::Dist(pA.GetLocation(), theTransformCS.GetLocation()));
					}

					// Evaluate the other bone motion distance
					float bD = 0.f;
					FTransform pB = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, 0, InLegsBaseData[DataIndex + 1].FootBone.BoneName);
					for (int32 I = 1; I <= InFootWeightData.CompareFrame; I++)
					{
						const FTransform theTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, I, InLegsBaseData[DataIndex + 1].FootBone.BoneName);
						bD += (FVector::Dist(pB.GetLocation(), theTransformCS.GetLocation()));
					}

					// Compare the bone motion distance
					theLeaderDataIndex = (aD < bD || InFootWeightData.bInverseFootPosition) ? DataIndex + 1 : theLeaderDataIndex;
				}
				else
				{
					// Rebuild the leader data index
					theLeaderDataIndex = bIsLanded && !InFootWeightData.bInverseFootPosition ? DataIndex + 1 : theLeaderDataIndex;
				}
			}

			// Add the first key
			{
				const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(0.f, InFootWeightData.CurveSampleData.CurvesApexRanges[theLeaderDataIndex].Y);
			}

			// Calculate the result
			for (int32 PoseIndex = 1; PoseIndex < thePosesNumber; PoseIndex++)
			{
				// Get the tolerance value
				const float& theTolerance = theLegsRuntimeData[theLeaderDataIndex].bIsLanded ? InFootWeightData.CurveSampleData.AirTolerance : InFootWeightData.CurveSampleData.LandedTolerance;

				// Check the data is landed state
				const bool bIsLanded = theLegsRuntimeData[theLeaderDataIndex].IsLandedPose(PoseIndex, theTolerance, theLeaderGroundedHeight);

				// Define the landed pose height data index
				int32 theLandedDataIndex = theLeaderDataIndex;

				// Check other pose height
				if (false)
				{
					for (int32 j = 0; j < InLegsBaseData.Num(); j++)
					{
						// Check the leg bone is valid
						const FVirtualLegBaseData& theOtherLegBaseData = InLegsBaseData[j];
						if (!theOtherLegBaseData.IsValid() || j == theLeaderDataIndex)
						{
							continue;
						}

						// Get the tolerance value
						const float& theOtherTolerance = theLegsRuntimeData[j].bIsLanded ? InFootWeightData.CurveSampleData.AirTolerance : InFootWeightData.CurveSampleData.LandedTolerance;

						// Check the data is landed state
						const bool theLandedState = theLegsRuntimeData[j].IsLandedPose(PoseIndex, theOtherTolerance, theLeaderGroundedHeight);

						// If is landed state, we should compare lower height
						if (theLandedState)
						{
							const float pA = theLegsRuntimeData[j].GetMinPoseHeight(PoseIndex);
							const float pB = theLegsRuntimeData[theLandedDataIndex].GetMinPoseHeight(PoseIndex);

							// Compare the pose height
							if (pA < pB)
							{
								continue;
								theLandedDataIndex = theLeaderDataIndex;
							}
						}
					}
				}

				theLegsRuntimeData[theLandedDataIndex].bIsLanded = bIsLanded;

				// Check the leader data index is landed
				if (bIsLanded)
				{
					// Calculate the pose time
					const float thePoseTime = (PoseIndex + InFootWeightData.CurveSampleData.OffsetFrame) * thePoseSampleRate + InFootWeightData.CurveSampleData.OffsetTime;

					// Check if the interval between two keys is less than the minimum interval we expect
					if (InFootWeightData.CurveSampleData.Interval > 0.f && theRuntimeCurvePtr->Keys.Num() > 0)
					{
						// Calculate both key delta time
						const float theDeltaTime = thePoseTime - theRuntimeCurvePtr->GetLastKey().Time;
						if (theDeltaTime < InFootWeightData.CurveSampleData.Interval)
						{
							continue;
						}
					}

					// Cache the landed curve value
					const float theLandedCurveValue = InFootWeightData.CurveSampleData.CurvesApexRanges[theLeaderDataIndex].Y;

					// Change the leader data index
					theLeaderDataIndex = theLeaderDataIndex == DataIndex ? DataIndex + 1 : DataIndex;

					// Cache the air curve value
					const float theAirCurveValue = InFootWeightData.CurveSampleData.CurvesApexRanges[theLeaderDataIndex].Y;

					// Add both curve key
					const FKeyHandle& theAirKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, theAirCurveValue);
					const FKeyHandle& theLandedKeyHandle = theRuntimeCurvePtr->AddKey(thePoseTime, theLandedCurveValue);
				}
			}

			// Add the final key
			{
				// Get the tolerance value
				const float& theTolerance = theLegsRuntimeData[DataIndex].bIsLanded ? InFootWeightData.CurveSampleData.AirTolerance : InFootWeightData.CurveSampleData.LandedTolerance;

				// Calculate the last landed pose leader data index
				theLeaderDataIndex = theLegsRuntimeData[DataIndex].IsLandedPose(thePosesNumber - 1, theTolerance, InFootWeightData.CurveSampleData.GroundedHeight) ? DataIndex + 1 : DataIndex;

				// Add the final key
				const FKeyHandle& theKeyHandle = theRuntimeCurvePtr->AddKey(InAnimSequence->GetPlayLength(), InFootWeightData.CurveSampleData.CurvesApexRanges[theLeaderDataIndex].Y);
			}
		}
	}

	// Convert the runtime curve to animation curve
	UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, *theRuntimeCurveRef, theCurvesName[0], ERichCurveInterpMode::RCIM_Constant);

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->BakeTrackCurvesToRawAnimation();
#endif

	// Apply the animation asset modify
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_GameFramework::ExportFootOffset(const int32& InIndex, UAnimSequence* InAnimSequence, FVirtualFootOffsetData& InFootOffsetData, const FVirtualLegBaseData& InLegBaseData)
{
	// Check the bone data is valid
	if (!InLegBaseData.IsValid())
	{
		return;
	}

	// Try sample each animation pose
	FVirtualCurveSampleData& theCurveSampleData = InFootOffsetData.CurveSampleData;
	const double& thePoseSampleRate = theCurveSampleData.SampleRate.AsInterval();

	// Define the curves data
	FRuntimeFloatCurve theFloatCurve;
	FRichCurve* theCurvePtr = theFloatCurve.GetRichCurve();

	// Initialize the leg runtime data
	FVirtualLegPoseRuntimeData theLegRuntimeData;
	const int32& thePosesNumber = theLegRuntimeData.Initialize(InAnimSequence, thePoseSampleRate, InLegBaseData, InFootOffsetData.bUseTipRelativeTransform);

	// Define the max range value
	float theApexValue = 0.f;
	float theLowerValue = 0.f;
	int32 theApexPoseIndex = INDEX_NONE;

	// Get the output curve apex value
	const float theOutputApexValue = InFootOffsetData.CurveSampleData.CurvesApexRanges.IsValidIndex(InIndex) ? InFootOffsetData.CurveSampleData.CurvesApexRanges[InIndex].Y : 1.f;

	// Each every pose
	for (int32 PoseIndex = 0; PoseIndex < thePosesNumber; PoseIndex++)
	{
		// Calculate the pose time
		const float thePoseValue = theLegRuntimeData.GetMaxPoseHeight(PoseIndex);

		// Calculate the pose time
		const float thePoseTime = (PoseIndex + theCurveSampleData.OffsetFrame) * thePoseSampleRate + theCurveSampleData.OffsetTime;

		// Check the data is landed state
		const bool bIsLanded = theLegRuntimeData.IsLandedPose(PoseIndex
		, theLegRuntimeData.bIsLanded ? theCurveSampleData.AirTolerance : theCurveSampleData.LandedTolerance
		, theCurveSampleData.GroundedHeight, FLT_MIN, !InFootOffsetData.bUseMaxValue);

		// Handle
		if (PoseIndex == 0 || theLegRuntimeData.bIsLanded != bIsLanded)
		{
			// Calculate the lower value
			theLowerValue = bIsLanded ? theLegRuntimeData.GetMinPoseHeight(PoseIndex) : theLowerValue;

			// Cache the state
			theLegRuntimeData.bIsLanded = bIsLanded;

			// Calculate the max apex value
			if (!bIsLanded)
			{
				// Clear the cached data
				theApexValue = 0.f;
				theApexPoseIndex = INDEX_NONE;

				// Each every future pose
				for (int32 i = PoseIndex; i < thePosesNumber; i++)
				{
					// Calculate the landed state
					const float thePoseHeight = theLegRuntimeData.GetMaxPoseHeight(i);
					const bool theTrajectoryLanded = theLegRuntimeData.IsLandedPose(i
						, theLegRuntimeData.bIsLanded ? theCurveSampleData.AirTolerance : theCurveSampleData.LandedTolerance
						, theCurveSampleData.GroundedHeight, FLT_MIN, !InFootOffsetData.bUseMaxValue);
					if (theTrajectoryLanded)
					{
						theApexPoseIndex = i;
						break;
					}
					else if (thePoseHeight >= theApexValue)
					{
						theApexValue = thePoseHeight;
					}
				}
			}

			// Check if the interval between two keys is less than the minimum interval we expect
			if (theCurveSampleData.Interval > 0.f && theCurvePtr->Keys.Num() > 0)
			{
				// Calculate both key delta time
				const float theDeltaTime = thePoseTime - theCurvePtr->GetLastKey().Time;
				if (theDeltaTime < theCurveSampleData.Interval)
				{
					continue;
				}
			}
		}

		// Calculate the alpha value
		const float theAlpha = (theApexValue - theLowerValue) == 0.f ? 0.f : FMath::Abs((thePoseValue - theLowerValue) / (theApexValue - theLowerValue));
		const float theOutputValue = bIsLanded ? 0.f : (InFootOffsetData.bMakeFullWeight ? theOutputApexValue : (InFootOffsetData.bMakeAlphaCurve ? theAlpha : thePoseValue));

#if 0
		// We should avoid situations where the curve reverses
		if (InFootOffsetData.bAvoidInverseCurve && theCurvePtr->Keys.Num() > 1
			/*&& theOutputValue > KINDA_SMALL_NUMBER && FAnimWeight::IsRelevant(theOutputValue)*/)
		{
			const float& pA = theCurvePtr->Keys[theCurvePtr->Keys.Num() - 2].Value;
			const float& pB = theCurvePtr->Keys[theCurvePtr->Keys.Num() - 1].Value;
			if (FAnimWeight::IsRelevant(pA) && FAnimWeight::IsRelevant(pB))
			{
				if (pA < pB) // Is up curve
				{
					if (theOutputValue < pB)
					{
						continue;
					}
				}
				else if (pA != pB) // Is down curve
				{
					if (theOutputValue > pB)
					{
						continue;
					}
				}
			}
		}
#endif

		// Add to curve key
		theCurvePtr->AddKey(thePoseTime, theOutputValue);
	}

	// Apply curve filter
	UVAT_Curve::ApplyCurveFilter(theFloatCurve, theCurveSampleData.CurveFilterType, theCurveSampleData.CurveFilterTolerance);

	// Transfer the curve keys
	int32 thePairIndex = 0;
	for (TPair<FName, FRuntimeFloatCurve>& theCurvePair : theCurveSampleData.RuntimeCurvesMap)
	{
		// Find same array index data
		if (thePairIndex == InIndex)
		{
			// Transfer the curve data
			theCurvePair.Value.GetRichCurve()->Keys = theFloatCurve.GetRichCurve()->Keys;

			// Output to animation curve data
			if (InFootOffsetData.bOutputChildCurve && theCurvePair.Key != NAME_None)
			{
				// Convert the runtime curve to animation curve
				UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, theCurvePair.Value, theCurvePair.Key);

				// Apply the animation asset changed
				UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
			}
			break;
		}

		// Iterate
		++thePairIndex;
	}
}

void UVAT_GameFramework::SamplingFootOffset(UAnimSequence* InAnimSequence, FVirtualFootOffsetData& InFootOffsetData, const TArray<FVirtualLegBaseData>& InLegsBaseData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Clear the cached curve data
	FRuntimeFloatCurve& theFootOffsetCurve = InFootOffsetData.OutputCurveData;
	theFootOffsetCurve.GetRichCurve()->Reset();

	// Each every foot offset curve data
	for (int32 LegIndex = 0; LegIndex < InLegsBaseData.Num(); LegIndex++)
	{
		ExportFootOffset(LegIndex, InAnimSequence, InFootOffsetData, InLegsBaseData[LegIndex]);
	}

	// Each every children foot offset curve data
	int32 thePairIndex = 0;
	for (TPair<FName, FRuntimeFloatCurve>& theCurvePair : InFootOffsetData.CurveSampleData.RuntimeCurvesMap)
	{
		// We copy first curve data as base curve
		if (thePairIndex == 0)
		{
			// Iterate
			++thePairIndex;

			// Copy first curve data
			theFootOffsetCurve.GetRichCurve()->Keys = theCurvePair.Value.GetRichCurve()->Keys;
			continue;
		}

		// Each every curve key
		for (const FRichCurveKey& theCurveKey : theCurvePair.Value.GetRichCurve()->Keys)
		{
			// Only merge the valid value
			if (theCurveKey.Value != 0.f || InFootOffsetData.CurveSampleData.CurveFilterType == EVirtualCurveFilterType::Reduce)
			{
				theFootOffsetCurve.GetRichCurve()->UpdateOrAddKey(theCurveKey.Time, theCurveKey.Value);
			}
		}

		// Iterate
		++thePairIndex;
	}

	// Output to animation curve data
	if (InFootOffsetData.bOutputMergeCurve && InFootOffsetData.OutputCurveName != NAME_None)
	{
		// Convert the runtime curve to animation curve
		UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, theFootOffsetCurve, InFootOffsetData.OutputCurveName);

		// Apply the animation asset changed
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_GameFramework::SamplingPosesBlendData(UAnimSequence* InAnimSequence, FVirtualPosesCompareData& InPoseCompareData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Clear the cached data
	FRichCurve* theCurvePtr = InPoseCompareData.CurveData.GetRichCurve();
	theCurvePtr->Reset();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Calculate the result
	for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Each every pose curve data
		for (FVirtualPoseAssetData& thePoseAssetData : InPoseCompareData.PosesAssetData)
		{
			// Get the compare pose asset
			UAnimSequence* thePoseAsset = thePoseAssetData.PoseAsset ? thePoseAssetData.PoseAsset : InPoseCompareData.PosesAssetData[0].PoseAsset;

			// Check the pose asset is valid
			if (thePoseAsset == nullptr)
			{
				continue;
			}

			// Define the tolerance result
			float theToleranceResult = 0.f;

			// Calculate the evaluate pose time
			float theEvaluatePoseTime = thePoseAssetData.EvaluatePos;
			if (thePoseAssetData.EvaluateFrame >= 0)
			{
				// Evaluate the desired frame time
				theEvaluatePoseTime = thePoseAsset->GetTimeAtFrame(thePoseAssetData.EvaluateFrame);
			}

			// Check the sample pose
			if (UVAT_PoseSearch::IsSampePose(theKeyTime, theEvaluatePoseTime, InAnimSequence, thePoseAsset, InPoseCompareData.CompareTolerance, &theToleranceResult))
			{
				// Calculate the output curve value
				float theCurveValue = thePoseAssetData.OutputCurveValue;

				// Add the curve key
				FKeyHandle theKeyHandle = theCurvePtr->AddKey(theKeyTime, theCurveValue);
			}
		}
	}

	// Check and rebuild the curve value
	if (InPoseCompareData.bOutputUnityValue && theCurvePtr->Keys.Num() > 1)
	{
		// Get the first key value
		const float theFirstKeyValue = theCurvePtr->GetFirstKey().Value;

		// Each every curve key
		for (FRichCurveKey& theCurveKey : theCurvePtr->Keys)
		{
			// Check the first key value is max value
			if (theFirstKeyValue == 1.f)
			{
				if (theCurveKey.Value == 0.f)
				{
					theCurveKey.Value = 1.f;
				}
				else if (theCurveKey.Value == 1.f)
				{
					theCurveKey.Value = 0.f;
				}
			}
		}
	}

	// Filter the curve result
	UVAT_Curve::ApplyCurveFilter(InPoseCompareData.CurveData, EVirtualCurveFilterType::Reduce, KINDA_SMALL_NUMBER);

	// Blend the bones data
	if (InPoseCompareData.BlendBones.Num() > 0)
	{
		TArray<FRichCurveKey> theApexCurveKeys;

		// Get the apex time
#if 0
		for (int32 KeyIndex = 1; KeyIndex < theCurvePtr->Keys.Num(); KeyIndex++)
		{
			const FRichCurveKey& theCurveKey = theCurvePtr->Keys[KeyIndex];
			const FRichCurveKey& theLastCurveKey = theCurvePtr->Keys[KeyIndex - 1];
			if (theCurveKey.Value != theLastCurveKey.Value)
			{
				theApexCurveKeys.Add(theLastCurveKey);
				theApexCurveKeys.Add(theLastCurveKey);
				theApexCurveKeys.Add(theCurveKey);
			}
		}
#else
		for (int32 KeyIndex = 2; KeyIndex < theCurvePtr->Keys.Num(); KeyIndex++)
		{
			const FRichCurveKey& theCurveKey = theCurvePtr->Keys[KeyIndex];
			const FRichCurveKey& theLastCurveKey = theCurvePtr->Keys[KeyIndex - 1];
			const FRichCurveKey& theStartCurveKey = theCurvePtr->Keys[KeyIndex - 2];
			if (theCurveKey.Value != theLastCurveKey.Value)
			{
				theApexCurveKeys.Add(theStartCurveKey);
				theApexCurveKeys.Add(theCurveKey);
			}
		}
#endif

		// Check the apex times is valid
		if (theApexCurveKeys.Num() >= 2)
		{
			const FTransform theMinBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theApexCurveKeys[0].Time, InPoseCompareData.BlendBones[0].BoneName);
			const FTransform theMaxBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theApexCurveKeys[1].Time, InPoseCompareData.BlendBones[0].BoneName);

			// Calculate the result
			for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
			{
				// Evaluate the key time
				const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);
				const float theFrameTime = InAnimSequence->GetTimeAtFrame(1);

				// Check the key time is valid
				if (!FMath::IsWithinInclusive(theKeyTime, theApexCurveKeys[0].Time, theApexCurveKeys[1].Time - theFrameTime * 0.5f))
				{
					continue;
				}

				// Each every bones
				for (const FBoneReference& theBoneReference : InPoseCompareData.BlendBones)
				{
					// Check the bone is valid
					const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneReference.BoneName);
					if (theBoneIndex == INDEX_NONE)
					{
						break;
					}

					// Get the bone transform as the time
					const FTransform theBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theBoneReference.BoneName);

					// Calculate the min point
					const FVector theClosestPoint = FMath::ClosestPointOnSegment(theBoneTransform.GetLocation(), theMinBoneTransform.GetLocation(), theMaxBoneTransform.GetLocation());

					// Calculate two point distance
					const float theDistance = FVector::Dist(theMinBoneTransform.GetLocation(), theMaxBoneTransform.GetLocation());

					// Calculate the blend weight
					const float thePointDistance = FMath::PointDistToSegment(theBoneTransform.GetLocation(), theMinBoneTransform.GetLocation(), theMaxBoneTransform.GetLocation());

					// Calculate the output curve value
					float theCurveValue = thePointDistance / theDistance/*theApexCurveKeys[0].Value + (theApexCurveKeys[1].Value - theApexCurveKeys[0].Value) * theWeight*/;
					theCurveValue = FVector::Dist(theClosestPoint, theMinBoneTransform.GetLocation()) / theDistance;

					// Add the curve key
					FKeyHandle theKeyHandle = theCurvePtr->UpdateOrAddKey(theKeyTime, theCurveValue);

					// Calculate one bone now
					break;
				}
			}

			// Filter the curve result
			UVAT_Curve::ApplyCurveFilter(InPoseCompareData.CurveData, EVirtualCurveFilterType::Reduce, KINDA_SMALL_NUMBER);
		}
	}

	// Output to animation curve data
	if (InPoseCompareData.CurveName != NAME_None)
	{
		// Convert the runtime curve to animation curve
		UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(nullptr, InAnimSequence, InPoseCompareData.CurveData, InPoseCompareData.CurveName);

		// Apply the animation asset changed
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// BEGIN DEPRECATED, just some experimental approach.
/////////////////////////////////////////////////////////////////////////////////////

#if 0
template <typename DataType, typename CurveClass>
void UVAT_GameFramework::BakeFootLockCurve(UAnimSequence* InAnimSequence, FVirtualSmartFeetLandNotifyData& InFeetData, FVirtualCurveAttributes& InCurveAttributes)
{
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	InFeetData.Curves.Reset(2);
	const float CurveTolerance = 0.001f;

	for (int32 Index = 0; Index < InFeetData.SmartFeetLandNotifyData.Num(); Index++)
	{
		if (!InFeetData.CurvesName.IsValidIndex(Index))
		{
			continue;
		}

		// Add curve
		const FName& theCurveName = InFeetData.CurvesName[Index];
		if (theCurveName == NAME_None)
		{
			continue;
		}

		FRuntimeFloatCurve NewFootPositionCurve;
		FRichCurve* NewRichCurve = NewFootPositionCurve.GetRichCurve();

		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);

		FRuntimeFloatCurve& theRuntimeCurve = InFeetData.Curves.AddDefaulted_GetRef();

		const FVirtualLegBaseData& theFootLandData = InFeetData.SmartFeetLandNotifyData[Index];

		const int32& theFootBoneIndex = InAnimSequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(theFootLandData.FootBone.BoneName);

		if (theFootBoneIndex != INDEX_NONE)
		{
			const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
			FTransform theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

			bool thePoseIsInAir = false;
			float theLastCurveValue = KINDA_SMALL_NUMBER;

			const int32& AnimFrames = UVAT_Library::GetNumberOfFrames(InAnimSequence);
			const float& AnimSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

			FTransform theRootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theRootBoneName);
			FTransform theFootBoneRefTransformCS;

			float theRefDeltaDist = 0.f;

			if (theFootLandData.TipBone.BoneName != NAME_None)
			{
				theFootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theFootLandData.TipBone.BoneName);
				
				FTransform TipSocketTransformLS = FTransform::Identity;
				TipSocketTransformLS.SetLocation(theFootLandData.TipSocketLocation);
				theFootBoneRefTransformCS = TipSocketTransformLS * theFootBoneRefTransformCS;
				theRefDeltaDist = InFeetData.bOnlyCheckHeight
					? theFootBoneRefTransformCS.GetLocation().Z - theRootBoneRefTransformCS.GetLocation().Z
					: FVector::Dist(theFootBoneRefTransformCS.GetLocation(), theRootBoneRefTransformCS.GetLocation());
			}
			else
			{
				theFootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theFootLandData.FootBone.BoneName);

				theRefDeltaDist = InFeetData.bOnlyCheckHeight
					? theFootBoneRefTransformCS.GetLocation().Z - theRootBoneRefTransformCS.GetLocation().Z
					: FVector::Dist(theFootBoneRefTransformCS.GetLocation(), theRootBoneRefTransformCS.GetLocation());
			}

			float theCurveOffset = -CurveTolerance;
			if (theFootLandData.PositionOffset.IsValidIndex(0))
			{
				theCurveOffset = theFootLandData.NewKeyOffset == 0.f
					? theCurveOffset : FMath::Min(theFootLandData.NewKeyOffset, CurveTolerance);
			}

			for (int i = 0; i < AnimFrames; i++)
			{
				float Position = (AnimSequenceLength / (AnimFrames - 1)) * i;

				//FTransform theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, Position, theRootBoneName);
				FTransform theFootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, Position, theFootLandData.FootBone.BoneName);

				float theDeltaDist = 0.f;
				if (theFootLandData.TipBone.BoneName != NAME_None)
				{
					FTransform TipSocketTransformLS = FTransform::Identity;
					TipSocketTransformLS.SetLocation(theFootLandData.TipSocketLocation);
					FTransform theTipBoneTransformsCS
						= TipSocketTransformLS
						* UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, Position, theFootLandData.TipBone.BoneName)
						*theFootBoneTransformCS;

					FTransform HeelSocketTransformLS = FTransform::Identity;
					HeelSocketTransformLS.SetLocation(theFootLandData.HeelSocketLocation);
					FTransform theHeelBoneTransformsCS = HeelSocketTransformLS * theFootBoneTransformCS;

					if (theTipBoneTransformsCS.GetLocation().Z < theHeelBoneTransformsCS.GetLocation().Z)
					{
						theDeltaDist = InFeetData.bOnlyCheckHeight
							? theTipBoneTransformsCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
							: FVector::Dist(theTipBoneTransformsCS.GetLocation(), theRootBoneTransformCS.GetLocation());
					}
					else
					{
						theDeltaDist = InFeetData.bOnlyCheckHeight
							? theHeelBoneTransformsCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
							: FVector::Dist(theHeelBoneTransformsCS.GetLocation(), theRootBoneTransformCS.GetLocation());
					}
				}
				else
				{
					theDeltaDist = InFeetData.bOnlyCheckHeight
						? theFootBoneTransformCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
						: FVector::Dist(theFootBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
				}

				const float theCurveValue = FMath::IsNearlyEqual(theDeltaDist, theRefDeltaDist, theFootLandData.Tolerance) ?  theFootLandData.CurveApexValue : 0.f;
				
				if (theLastCurveValue != theCurveValue)
				{
					// Offset?
					if (InFeetData.bOffsetRight && theCurveValue == 0.f)
					{
						Position -= theCurveOffset;
					}

					if (theCurveValue == 0.f && theFootLandData.PositionOffset.IsValidIndex(0))
					{
						Position += theFootLandData.PositionOffset[0];
					}

					if (NewRichCurve->Keys.Num() > 0)
					{
						if (!InFeetData.bIncludeFirstAndLastKey && NewRichCurve->Keys.Num() == 1)
						{
							FRichCurveKey& theLastKey = NewRichCurve->Keys.Last();
							if (theLastKey.Time == 0.f && theLastKey.Value == theLastCurveValue)
							{
								NewRichCurve->Keys.Reset();
								theRuntimeCurve.GetRichCurve()->Keys.Reset();
							}
						}

						FKeyHandle theKeyHandleRef = NewRichCurve->AddKey(Position + theCurveOffset, theLastCurveValue);
						FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(Position + theCurveOffset, theLastCurveValue);
						NewRichCurve->SetKeyInterpMode(theKeyHandleRef, theFootLandData.InterpMode);
						theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, theFootLandData.InterpMode);
					}

					// Generate new pos key
					FKeyHandle theKeyHandleRef = NewRichCurve->AddKey(Position, theCurveValue);
					FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(Position, theCurveValue);
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
				}

				theLastCurveValue = theCurveValue;

				if (i != AnimFrames - 1)
				{
					continue;
				}

				if (NewRichCurve->Keys.Num() == 0)
				{
					continue;
				}

#if 1
				if (InFeetData.bClearFinalKeys)
				{
					// Always delete last key is full weight?
					FRichCurveKey& theLastKey = NewRichCurve->Keys.Last();
					if (theLastKey.Value == 1.f)
					{
						NewRichCurve->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
						theRuntimeCurve.GetRichCurve()->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
					}

					// Always delete last key is zero weight?
					if (NewRichCurve->Keys.Num() > 2)
					{
						FRichCurveKey& KeyA = NewRichCurve->Keys.Last();
						FRichCurveKey& KeyB = NewRichCurve->Keys[NewRichCurve->Keys.Num() - 2];
						if (KeyA.Value == 0.f && KeyA.Value == KeyB.Value)
						{
							NewRichCurve->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
							theRuntimeCurve.GetRichCurve()->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
						}
					}
				}
#endif

				const float& AnimLength = InAnimSequence->GetPlayLength();
				const float& AnimRateScale = InAnimSequence->RateScale;
				const float& DeltaTime = InCurveAttributes.GetDeltaTime();

				// Copy first key to end key
				if (InFeetData.bIncludeFirstAndLastKey)
				{
					FKeyHandle theKeyHandleRef = NewRichCurve->UpdateOrAddKey(0.f, NewRichCurve->Eval(0.f));
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);
					theKeyHandleRef = NewRichCurve->AddKey(AnimLength, NewRichCurve->Eval(0.f));
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);

					FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(0.f, NewRichCurve->Eval(0.f));
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
					theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(AnimLength, NewRichCurve->Eval(0.f));
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
				}

				for (int32 KeyIndex = 0; KeyIndex < NewRichCurve->Keys.Num(); KeyIndex++)
				{
					FRichCurveKey& theCurveKey = NewRichCurve->Keys[KeyIndex];

					const FName ContainerName = UAnimationBlueprintLibrary::RetrieveContainerNameForCurve(InAnimSequence, theCurveName);
					if (ContainerName != NAME_None)
					{
						// Retrieve smart name for curve
						const FSmartName CurveSmartName = UAnimationBlueprintLibrary::RetrieveSmartNameForCurve(InAnimSequence, theCurveName, ContainerName);

						// Retrieve the curve by name
						CurveClass* Curve = static_cast<CurveClass*>(InAnimSequence->RawCurveData.GetCurveData(CurveSmartName.UID, ERawCurveTrackTypes::RCT_Float));
						if (Curve)
						{
							FKeyHandle theKeyHandleRef = Curve->FloatCurve.AddKey(theCurveKey.Time, theCurveKey.Value);
							Curve->FloatCurve.SetKeyInterpMode(theKeyHandleRef, theCurveKey.InterpMode);
						}
					}
				}
			}
		}
	}

	InAnimSequence->BakeTrackCurvesToRawAnimation();
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

template <typename DataType, typename CurveClass>
void UVAT_GameFramework::BakeFootIKCurve(UAnimSequence* InAnimSequence, FVirtualSmartFeetLandNotifyData& InFeetData, FVirtualCurveAttributes& InCurveAttributes)
{
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	InFeetData.Curves.Reset(2);
	const float CurveTolerance = 0.001f;

	for (int32 Index = 0; Index < InFeetData.SmartFeetLandNotifyData.Num(); Index++)
	{
		if (!InFeetData.CurvesName.IsValidIndex(Index))
		{
			continue;
		}

		// Add curve
		const FName& theCurveName = InFeetData.CurvesName[Index];
		if (theCurveName == NAME_None)
		{
			continue;
		}

		FRuntimeFloatCurve NewFootPositionCurve;
		FRichCurve* NewRichCurve = NewFootPositionCurve.GetRichCurve();

		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);

		FRuntimeFloatCurve& theRuntimeCurve = InFeetData.Curves.AddDefaulted_GetRef();

		const FVirtualLegBaseData& theFootLandData = InFeetData.SmartFeetLandNotifyData[Index];

		const int32& theFootBoneIndex = InAnimSequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(theFootLandData.FootBone.BoneName);

		if (theFootBoneIndex != INDEX_NONE)
		{
			const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
			FTransform theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

			bool thePoseIsInAir = false;
			float theLastCurveValue = KINDA_SMALL_NUMBER;

			const int32& AnimFrames = UVAT_Library::GetNumberOfFrames(InAnimSequence);
			const float& AnimSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

			FTransform theRootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theRootBoneName);
			FTransform theFootBoneRefTransformCS;

			float theRefDeltaDist = 0.f;

			if (theFootLandData.TipBone.BoneName != NAME_None)
			{
				theFootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theFootLandData.TipBone.BoneName);

				FTransform TipSocketTransformLS = FTransform::Identity;
				TipSocketTransformLS.SetLocation(theFootLandData.TipSocketLocation);
				theFootBoneRefTransformCS = TipSocketTransformLS * theFootBoneRefTransformCS;
				theRefDeltaDist = InFeetData.bOnlyCheckHeight
					? theFootBoneRefTransformCS.GetLocation().Z - theRootBoneRefTransformCS.GetLocation().Z
					: FVector::Dist(theFootBoneRefTransformCS.GetLocation(), theRootBoneRefTransformCS.GetLocation());
			}
			else
			{
				theFootBoneRefTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theFootLandData.FootBone.BoneName);

				theRefDeltaDist = InFeetData.bOnlyCheckHeight
					? theFootBoneRefTransformCS.GetLocation().Z - theRootBoneRefTransformCS.GetLocation().Z
					: FVector::Dist(theFootBoneRefTransformCS.GetLocation(), theRootBoneRefTransformCS.GetLocation());
			}

			float theCurveOffset = -CurveTolerance;
			if (theFootLandData.PositionOffset.IsValidIndex(0))
			{
				theCurveOffset = theFootLandData.NewKeyOffset == 0.f
					? theCurveOffset : FMath::Min(theFootLandData.NewKeyOffset, CurveTolerance);
			}

			for (int i = 0; i < AnimFrames; i++)
			{
				float Position = (AnimSequenceLength / (AnimFrames - 1)) * i;

				//FTransform theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, Position, theRootBoneName);
				FTransform 	theFootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, Position, theFootLandData.FootBone.BoneName);

				float theDeltaDist = 0.f;
				if (theFootLandData.TipBone.BoneName != NAME_None)
				{
					FTransform TipSocketTransformLS = FTransform::Identity;
					TipSocketTransformLS.SetLocation(theFootLandData.TipSocketLocation);
					FTransform theTipBoneTransformsCS
						= TipSocketTransformLS
						* UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, Position, theFootLandData.TipBone.BoneName)
						*theFootBoneTransformCS;

					FTransform HeelSocketTransformLS = FTransform::Identity;
					HeelSocketTransformLS.SetLocation(theFootLandData.HeelSocketLocation);
					FTransform theHeelBoneTransformsCS = HeelSocketTransformLS * theFootBoneTransformCS;

					if (theTipBoneTransformsCS.GetLocation().Z < theHeelBoneTransformsCS.GetLocation().Z)
					{
						theDeltaDist = InFeetData.bOnlyCheckHeight
							? theTipBoneTransformsCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
							: FVector::Dist(theTipBoneTransformsCS.GetLocation(), theRootBoneTransformCS.GetLocation());
					}
					else
					{
						theDeltaDist = InFeetData.bOnlyCheckHeight
							? theHeelBoneTransformsCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
							: FVector::Dist(theHeelBoneTransformsCS.GetLocation(), theRootBoneTransformCS.GetLocation());
					}
				}
				else
				{
					theDeltaDist = InFeetData.bOnlyCheckHeight
						? theFootBoneTransformCS.GetLocation().Z - theRootBoneTransformCS.GetLocation().Z
						: FVector::Dist(theFootBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
				}

				const float theCurveValue = FMath::IsNearlyEqual(theDeltaDist, theRefDeltaDist, theFootLandData.Tolerance) ? 1.f : theFootLandData.CurveApexValue;

				if (theLastCurveValue != theCurveValue)
				{
					// Offset?
					if (InFeetData.bOffsetRight && theCurveValue == 0.f)
					{
						Position -= theCurveOffset;
					}

					if (theCurveValue == 0.f && theFootLandData.PositionOffset.IsValidIndex(0))
					{
						Position += theFootLandData.PositionOffset[0];
					}

					if (NewRichCurve->Keys.Num() > 0)
					{
						if (!InFeetData.bIncludeFirstAndLastKey && NewRichCurve->Keys.Num() == 1)
						{
							FRichCurveKey& theLastKey = NewRichCurve->Keys.Last();
							if (theLastKey.Time == 0.f && theLastKey.Value == theLastCurveValue)
							{
								NewRichCurve->Keys.Reset();
								theRuntimeCurve.GetRichCurve()->Keys.Reset();
							}
						}

						FKeyHandle theKeyHandleRef = NewRichCurve->AddKey(Position + theCurveOffset, theLastCurveValue);
						FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(Position + theCurveOffset, theLastCurveValue);
						NewRichCurve->SetKeyInterpMode(theKeyHandleRef, theFootLandData.InterpMode);
						theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, theFootLandData.InterpMode);
					}

					// Generate new pos key
					FKeyHandle theKeyHandleRef = NewRichCurve->AddKey(Position, theCurveValue);
					FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(Position, theCurveValue);
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
				}

				theLastCurveValue = theCurveValue;

				if (i != AnimFrames - 1)
				{
					continue;
				}

				if (NewRichCurve->Keys.Num() == 0)
				{
					continue;
				}

#if 1
				if (InFeetData.bClearFinalKeys)
				{
					// Always delete last key is full weight?
					FRichCurveKey& theLastKey = NewRichCurve->Keys.Last();
					if (theLastKey.Value == 1.f)
					{
						NewRichCurve->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
						theRuntimeCurve.GetRichCurve()->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
					}

					// Always delete last key is zero weight?
					if (NewRichCurve->Keys.Num() > 2)
					{
						FRichCurveKey& KeyA = NewRichCurve->Keys.Last();
						FRichCurveKey& KeyB = NewRichCurve->Keys[NewRichCurve->Keys.Num() - 2];
						if (KeyA.Value == 0.f && KeyA.Value == KeyB.Value)
						{
							NewRichCurve->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
							theRuntimeCurve.GetRichCurve()->Keys.RemoveAt(NewRichCurve->Keys.Num() - 1);
						}
					}
				}
#endif

				const float& AnimLength = InAnimSequence->GetPlayLength();
				const float& AnimRateScale = InAnimSequence->RateScale;
				const float& DeltaTime = InCurveAttributes.GetDeltaTime();

				// Copy first key to end key
				if (InFeetData.bIncludeFirstAndLastKey)
				{
					FKeyHandle theKeyHandleRef = NewRichCurve->UpdateOrAddKey(0.f, NewRichCurve->Eval(0.f));
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);
					theKeyHandleRef = NewRichCurve->AddKey(AnimLength, NewRichCurve->Eval(0.f));
					NewRichCurve->SetKeyInterpMode(theKeyHandleRef, ERichCurveInterpMode::RCIM_Constant);

					FKeyHandle theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(0.f, NewRichCurve->Eval(0.f));
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
					theRuntimeKeyHandle = theRuntimeCurve.GetRichCurve()->AddKey(AnimLength, NewRichCurve->Eval(0.f));
					theRuntimeCurve.GetRichCurve()->SetKeyInterpMode(theRuntimeKeyHandle, ERichCurveInterpMode::RCIM_Constant);
				}

				for (int32 KeyIndex = 0; KeyIndex < NewRichCurve->Keys.Num(); KeyIndex++)
				{
					FRichCurveKey& theCurveKey = NewRichCurve->Keys[KeyIndex];

					const FName ContainerName = UAnimationBlueprintLibrary::RetrieveContainerNameForCurve(InAnimSequence, theCurveName);
					if (ContainerName != NAME_None)
					{
						// Retrieve smart name for curve
						const FSmartName CurveSmartName = UAnimationBlueprintLibrary::RetrieveSmartNameForCurve(InAnimSequence, theCurveName, ContainerName);

						// Retrieve the curve by name
						CurveClass* Curve = static_cast<CurveClass*>(InAnimSequence->RawCurveData.GetCurveData(CurveSmartName.UID, ERawCurveTrackTypes::RCT_Float));
						if (Curve)
						{
							FKeyHandle theKeyHandleRef = Curve->FloatCurve.AddKey(theCurveKey.Time, theCurveKey.Value);
							Curve->FloatCurve.SetKeyInterpMode(theKeyHandleRef, theCurveKey.InterpMode);
						}
					}
				}
			}
		}
	}

	InAnimSequence->BakeTrackCurvesToRawAnimation();
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_GameFramework::MergeAimOffset(const FVirtualMergeAimOffsetData& InMergeAimOffsetData)
{
//	UBlendSpace* theBlendSpace = InMergeAimOffsetData.AimOffsetBlendSpace;
//	if (!theBlendSpace)
//	{
//		return;
//	}
//
//	// Reset additive animation and cache it.
//	TArray<TEnumAsByte<enum EAdditiveAnimationType>> CachedAdditiveTypes;
//	const TArray<FBlendSample>& theBlendSamples = theBlendSpace->GetBlendSamples();
//	for (const FBlendSample& theBlendSample : theBlendSamples)
//	{
//		UAnimSequence* thePose = theBlendSample.Animation;
//		if (!thePose)
//		{
//			CachedAdditiveTypes.Add(thePose->AdditiveAnimType);
//			thePose->AdditiveAnimType = EAdditiveAnimationType::AAT_None;
//		}
//	}
//
//	TArray<FBlendSampleData> CachedAimOffsetBlendSampleData;
//	for (const float& theOutSampleValue : InMergeAimOffsetData.OutputSampleValues)
//	{
//		// Get asset tool module
//		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
//
//		// Create the asset
//		FString theAssetSuffixString = FTEXT("_0");
//		UObject* NewAsset = nullptr; FString Name; FString PackageName;
//		AssetToolsModule.Get().CreateUniqueAssetName(*InMergeAimOffsetData.OutputAssetName.ToString(), theAssetSuffixString, PackageName, Name);
//		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
//
//		NewAsset = AssetToolsModule.Get().DuplicateAsset(Name, PackagePath, MotionCaptureAnimSequence);
//
//		// Notify asset registry of new asset
//		UAnimSequence* NewAnimSequence = Cast<UAnimSequence>(NewAsset);
//		FAssetRegistryModule::AssetCreated(NewAnimSequence);
//
//		// Get the motion capture record frame
//		float const FrameTime = 1.f / float(30.f);
//		const float NewLength = InMergeAimOffsetData.OutputLength;
//
//		// Set the motion animation length
//		NewAnimSequence->CreateAnimation(GetOwningComponent());
//#if ENGINE_MAJOR_VERSION > 4
//		UAnimDataModel* TheAnimData = NewAnimSequence->GetDataModel();
//		IAnimationDataController& TheAnimController = NewAnimSequence->GetController();
//		TheAnimController.SetPlayLength(FrameTime);
//		TheAnimController.Resize(NewLength, 0.f, NewLength);
//#else
//		NewAnimSequence->SequenceLength = NewLength;
//		NewAnimSequence->SetRawNumberOfFrame(30);
//		NewAnimSequence->ResizeSequence(NewLength, 30, false, 0, 1);
//#endif
//
//		for (const float& theSampleValue : InMergeAimOffsetData.SampleValues)
//		{
//			// Calculate the aim offset blend samples
//			FVector theBlendInput(theSampleValue, theOutSampleValue, 0.f);
//			theBlendSpace->GetSamplesFromBlendInput(theBlendInput, CachedAimOffsetBlendSampleData);
//
//			// Evaluate MeshSpaceRotation additive blend space
//			FCompactPose OutPose;
//			FBlendedCurve OutCurve;
//			theBlendSpace->GetAnimationPose(CachedAimOffsetBlendSampleData, OutPose, OutCurve);
//		}
//	}
//
//	// Reset additive animation and cache it.
//	for (const FBlendSample& theBlendSample : theBlendSamples)
//	{
//		UAnimSequence* thePose = theBlendSample.Animation;
//		thePose->AdditiveAnimType = EAdditiveAnimationType::AAT_RotationOffsetMeshSpace;
//	}
}
#endif