// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_PoseSearch.h"
#include "Library/VAT_Bone.h"
#include "Library/VAT_Asset.h"
#include "Library/VAT_Library.h"
#include "Kismet/KismetMathLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Runtime/Public/VAT_PoseSearchLibrary.h"

/*-----------------------------------------------------------------------------
	FVirtualPoseSampleCurveData Implementation.
-----------------------------------------------------------------------------*/

bool FVirtualPoseSampleCurveData::CanSamplePose(UAnimSequence* InAnimSequence, const float& InSamplePoseTime
	, int32 InSampleToleranceFrame, float InSampleRate) const
{
	// If the tolerance is zero, we always sample all pose
	if (InSampleToleranceFrame < 0)
	{
		return true;
	}

	// Check the curves name is valid
	if (ReferenceCurveName == NAME_None || OutputCurveName == NAME_None)
	{
		return false;
	}

#if 1
	// Check the reference curve data is valid
	FFloatCurve* theReferenceCurvePtr = UVAT_Curve::GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, ReferenceCurveName);
	if (theReferenceCurvePtr == nullptr || !theReferenceCurvePtr->FloatCurve.HasAnyData())
	{
		return false;
	}

#if 1
	// Get the last animation feet position
	const float& theEvalCurveValue = theReferenceCurvePtr->FloatCurve.Eval(InSamplePoseTime);
	const float& theEvalToleranceCurveValue = theReferenceCurvePtr->FloatCurve.Eval(InSamplePoseTime + 1e-3f);

	// Check the condition
	if (FMath::IsNearlyEqual(theEvalCurveValue, ReferenceCurveValue, 0.01f))
	{
		return true;
	}
	else if (FMath::IsNearlyEqual(theEvalToleranceCurveValue, ReferenceCurveValue, 0.01f))
	{
		return true;
	}
	else if (InSampleToleranceFrame > 0)
	{
		// Calculate the sample tolerance time
		for (int32 ToleranceIndex = 1; ToleranceIndex <= InSampleToleranceFrame; ToleranceIndex++)
		{
			const float theSampleToleranceTime = ToleranceIndex * InSampleRate;
			const float& cA = theReferenceCurvePtr->FloatCurve.Eval(InSamplePoseTime + theSampleToleranceTime);
			const float& cB = theReferenceCurvePtr->FloatCurve.Eval(InSamplePoseTime + theSampleToleranceTime + 1e-3f);
			if (FMath::IsNearlyEqual(cA, ReferenceCurveValue, 0.01f) || FMath::IsNearlyEqual(cB, ReferenceCurveValue, 0.01f))
			{
				return true;
			}
		}
	}

	// Failed
	return false;
#endif

	// Get the curve value of the current animation pose
	TArray<FRichCurveKey>& theCurveKeys = theReferenceCurvePtr->FloatCurve.Keys;
	for (int32 CurveIndex = 0; CurveIndex < theCurveKeys.Num(); CurveIndex++)
	{
		const float& theCurveTime = theCurveKeys[CurveIndex].Time;
		const float& theCurveValue = theCurveKeys[CurveIndex].Value;

		// Check next curve time is valid
		if (!theCurveKeys.IsValidIndex(CurveIndex + 1))
		{
			return FMath::IsNearlyEqual(theCurveValue, ReferenceCurveValue, 0.01f);
		}

		// Check the desired value is in range
		const float& theCurveNextTime = theCurveKeys[CurveIndex + 1].Time;
		const float& theCurveNextValue = theCurveKeys[CurveIndex + 1].Value;
		if (FMath::IsWithinInclusive(InSamplePoseTime, theCurveTime, theCurveNextTime))
		{
			// If is min tolerance, we use next curve value
			if ((theCurveNextTime - InSamplePoseTime) <= 1e-3f)
			{
				return FMath::IsNearlyEqual(theCurveNextValue, ReferenceCurveValue, 0.01f);
			}

			return FMath::IsNearlyEqual(theCurveValue, ReferenceCurveValue, 0.01f);
		}
	}
#else

	// Get the curve data
	TArray<float> theCurveTimes, theCurveValues;
	UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, ReferenceCurveName, theCurveTimes, theCurveValues);

	// Check the curve data is valid
	if (theCurveTimes.Num() == 0 || theCurveTimes.Num() != theCurveValues.Num())
	{
		return false;
	}

	// Get the curve value of the current animation pose
	for (int32 CurveIndex = 0; CurveIndex < theCurveTimes.Num(); CurveIndex++)
	{
		const float& theCurveTime = theCurveTimes[CurveIndex];
		const float& theCurveValue = theCurveValues[CurveIndex];

		// Check next curve time is valid
		if (!theCurveTimes.IsValidIndex(CurveIndex + 1))
		{
			return FMath::IsNearlyEqual(theCurveValue, ReferenceCurveValue, 0.01f);
		}

		// Check the desired value is in range
		const float& theCurveNextTime = theCurveTimes[CurveIndex + 1];
		const float& theCurveNextValue = theCurveValues[CurveIndex + 1];
		if (FMath::IsWithinInclusive(InSamplePoseTime, theCurveTime, theCurveNextTime))
		{
			// If is min tolerance, we use next curve value
			if ((theCurveNextTime - InSamplePoseTime) <= 1e-3f)
			{
				return FMath::IsNearlyEqual(theCurveNextValue, ReferenceCurveValue, 0.01f);
			}

			return FMath::IsNearlyEqual(theCurveValue, ReferenceCurveValue, 0.01f);
		}
	}
#endif

	// Failed
	return false;
}

/*-----------------------------------------------------------------------------
	Pose Search Implementation.
-----------------------------------------------------------------------------*/

float UVAT_PoseSearch::GetPoseDistance(const float& InEvaluatePos, const FVirtualPoseSampleCurveData& InPoseSampleCurveData)
{
	// Evaluate the curve value from pose time
	return InPoseSampleCurveData.OutputCurve.GetRichCurveConst()->Eval(InEvaluatePos);
}

float UVAT_PoseSearch::GetPoseDistance(UAnimSequence* InAnimSequence, const float& InEvaluatePos, const FVirtualPoseSearchSampleData& InPoseSampleData)
{
	check(InAnimSequence);
	for (const FVirtualPoseSampleBoneData& theSampleBoneData : InPoseSampleData.BonesData)
	{
		for (const FVirtualPoseSampleCurveData& theSampleCurveData : theSampleBoneData.SampleCurvesData)
		{
			if (theSampleCurveData.CanSamplePose(InAnimSequence, InEvaluatePos))
			{
				// Evaluate the curve value from pose time
				const float thePoseDistance = theSampleCurveData.OutputCurve.GetRichCurveConst()->Eval(InEvaluatePos);

				// Check the pose distance is valid
				if (thePoseDistance != 0.f)
				{
					return thePoseDistance;
				}
			}
		}
	}

	// Failed
	return 0.f;
}

FVector2D UVAT_PoseSearch::GetCanSampleIndex(UAnimSequence* InAnimSequence, const float& InEvaluatePos, const FVirtualPoseSearchSampleData& InPoseSampleData)
{
	check(InAnimSequence);
	for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.BonesData.Num(); BoneIndex++)
	{
		const FVirtualPoseSampleBoneData& theSampleBoneData = InPoseSampleData.BonesData[BoneIndex];
		for (int32 CurveIndex = 0; CurveIndex < theSampleBoneData.SampleCurvesData.Num(); CurveIndex++)
		{
			const FVirtualPoseSampleCurveData& theSampleCurveData = theSampleBoneData.SampleCurvesData[CurveIndex];
			if (theSampleCurveData.CanSamplePose(InAnimSequence, InEvaluatePos))
			{
				return FVector2D(float(BoneIndex), float(CurveIndex));
			}
		}
	}

	// Failed
	return FVector2D(float(INDEX_NONE), float(INDEX_NONE));
}

bool UVAT_PoseSearch::IsLoopAnimationAsset(UAnimSequence* InAnimSequence, const float InTolerance)
{
	return IsSampePose(0.f, InAnimSequence->GetPlayLength(), InAnimSequence, InAnimSequence, InTolerance);
}

bool UVAT_PoseSearch::IsSampePose(const float& InEvaluatePos, const float& InComparePos
	, UAnimSequence* InAnimSequence, UAnimSequence* InCompareAnimSequence
	, const float InTolerance, float* OutTolerance)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return false;
	}

	// Check the compare animation asset is valid
	if (!InCompareAnimSequence || !InCompareAnimSequence->GetSkeleton())
	{
		return false;
	}

	// Get the skeleton reference data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Each every mesh bone info
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// Get the mesh bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);

		// Don't check first bone data
		if (theBoneIndex == 0)
		{
			continue;
		}

		// Compare both poses
		FTransform theSourcePoseTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, InEvaluatePos, theBoneName);
		FTransform theComparePoseTransform = UVAT_Bone::GetBoneTransformCS(InCompareAnimSequence, false, InComparePos, theBoneName);

		// Check the poses tolerance
		if (!theSourcePoseTransform.Equals(theComparePoseTransform, InTolerance))
		{
			// Check and return the tolerance result
			if (OutTolerance != nullptr)
			{
				*OutTolerance = 0.f;
				*OutTolerance += FMath::Abs((theSourcePoseTransform.GetLocation().X - theComparePoseTransform.GetLocation().X));
				*OutTolerance += FMath::Abs((theSourcePoseTransform.GetLocation().Y - theComparePoseTransform.GetLocation().Y));
				*OutTolerance += FMath::Abs((theSourcePoseTransform.GetLocation().Z - theComparePoseTransform.GetLocation().Z));
				*OutTolerance += FMath::Abs((theSourcePoseTransform.Rotator().Roll - theComparePoseTransform.Rotator().Roll));
				*OutTolerance += FMath::Abs((theSourcePoseTransform.Rotator().Pitch - theComparePoseTransform.Rotator().Pitch));
				*OutTolerance += FMath::Abs((theSourcePoseTransform.Rotator().Yaw - theComparePoseTransform.Rotator().Yaw));
			}

			return false;
		}
	}

	// Success
	return true;
}

template <typename DataType, typename CurveClass>
void UVAT_PoseSearch::SamplePoseFeatures(UAnimSequence* InAnimSequence, FVirtualPoseSearchSampleData& InPoseSampleData, bool InBuildCurve)
{
	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	if (theAnimDataModel == nullptr)
	{
		return;
	}
#endif

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const FName& thePelvisBoneName = theReferenceSkeleton.GetBoneName(1);

	// Each every bone try sample pose data
	for (int32 DataIndex = 0; DataIndex < InPoseSampleData.BonesData.Num(); DataIndex++)
	{
		FVirtualPoseSampleBoneData& theSearchBoneData = InPoseSampleData.BonesData[DataIndex];

		// Check the bone name is valid
		const FName & theBoneName = theSearchBoneData.Bone.BoneName;
		if (theBoneName == NAME_None)
		{
			continue;
		}

		// Clear output curves value
		for (FVirtualPoseSampleCurveData& theSampleCurveData : theSearchBoneData.SampleCurvesData)
		{
			// Check the curve name is valid
			if (theSampleCurveData.OutputCurveName == NAME_None)
			{
				continue;
			}

			theSampleCurveData.OutputCurve.GetRichCurve()->Reset();

			// Check the build curve condition
			if (InBuildCurve)
			{
				UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theSampleCurveData.OutputCurveName);
				UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theSampleCurveData.OutputCurveName);
			}
		}

		// Try sample each animation pose
		float thePoseSampleTime = 0.f;
		const float& theSequenceLength = InAnimSequence->GetPlayLength();
		const double& thePoseSampleRate = InPoseSampleData.SampleRate.AsInterval();
		while (thePoseSampleTime <= (theSequenceLength + 0.0001f))
		{
			// Clamp the pose time
			thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theSequenceLength);

			// Define the sample condition state
			bool bCanSamplePose = true;

			// Check if the specified curve data needs to be sampled for comparison
			for (FVirtualPoseSampleCurveData& theSampleCurveData : theSearchBoneData.SampleCurvesData)
			{
				// Check the runtime curve is valid
				FRichCurve* theOutputRuntimeCurve = theSampleCurveData.OutputCurve.GetRichCurve();
				if (theOutputRuntimeCurve == nullptr)
				{
					continue;
				}

				// Check the build curve condition
				CurveClass* theOutputCurve = nullptr;
				if (InBuildCurve)
				{
					// Check the curve is valid
					theOutputCurve = UVAT_Curve::GetCurveClass<float, FFloatCurve>(InAnimSequence, theSampleCurveData.OutputCurveName, ERawCurveTrackTypes::RCT_Float);
					if (theOutputCurve == nullptr)
					{
						continue;
					}
				}

				// Calculate the can sample pose condition
				bCanSamplePose = theSampleCurveData.CanSamplePose(InAnimSequence, thePoseSampleTime, InPoseSampleData.SampleToleranceFrame, thePoseSampleRate);

				// Try sample the animation pose data
				if (bCanSamplePose == false)
				{
					// Calculate the delta key time
					const float theLastKeyValue = theOutputRuntimeCurve->Keys.Num() > 0 ? theOutputRuntimeCurve->GetLastKey().Value : FLT_MAX;
					const float theDeltaKeyTime = theOutputRuntimeCurve->Keys.Num() > 0 ? thePoseSampleTime - theOutputRuntimeCurve->GetLastKey().Time : FLT_MAX;
					if (thePoseSampleTime == 0.f)
					{
						theOutputRuntimeCurve->AddKey(thePoseSampleTime, 0.f);

						// Check the build curve condition
						if (theOutputCurve != nullptr)
						{
							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, 0.f);
						}
					}
					else if (theLastKeyValue != 0.f)
					{
						const float& theLastKeyTime = theOutputRuntimeCurve->GetLastKey().Time;

						theOutputRuntimeCurve->AddKey(thePoseSampleTime, 0.f);
						theOutputRuntimeCurve->AddKey(thePoseSampleTime, theLastKeyValue);

						// Check the build curve condition
						if (theOutputCurve != nullptr)
						{
							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, 0.f);
							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, theLastKeyValue);
						}

// 						theOutputRuntimeCurve->UpdateOrAddKey(thePoseSampleTime, 0.f);
// 						theOutputRuntimeCurve->AddKey(thePoseSampleTime, theLastKeyValue);
// 
// 						// Check the build curve condition
// 						if (theOutputCurve != nullptr)
// 						{
// 							FKeyHandle theLastKeyHandle = theOutputCurve->FloatCurve.FindKey(theLastKeyTime);
// 							theOutputCurve->FloatCurve.DeleteKey(theLastKeyHandle);
// 							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, 0.f);
// 							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, theLastKeyValue);
// 						}
					}
					continue;
				}

				// Calculate root motion transform
				FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime - thePoseSampleRate);
				FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime);
				FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
				const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleRate).Size();

				// Get need bones component space transform
				const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theRootBoneName);
				const FTransform& thePelvisBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, thePelvisBoneName);
				const FTransform& theBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theBoneName);

				// Check the reference bone name is valid
				TOptional<FTransform> theReferenceBoneTransformCS;
				const FName& theReferenceBoneName = theSearchBoneData.ReferenceBone.BoneName;
				if (theReferenceBoneName != NAME_None)
				{
					theReferenceBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theReferenceBoneName);
				}

				// Calculate the pose distance
				float thePoseDistance = 0.f;
#if 0
				thePoseDistance = FVector::Dist(theBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
#else
				// Get other compare bone data
				FVirtualPoseSampleBoneData* theOtherBoneData = nullptr;
				if (InPoseSampleData.BonesData.IsValidIndex(DataIndex - 1))
				{
					theOtherBoneData = &InPoseSampleData.BonesData[DataIndex - 1];
				}
				else if (InPoseSampleData.BonesData.IsValidIndex(DataIndex + 1))
				{
					theOtherBoneData = &InPoseSampleData.BonesData[DataIndex + 1];
				}

				// If has other bone, we will compare two bone pose distance
				if (theOtherBoneData != nullptr && theOtherBoneData->Bone.BoneName != NAME_None)
				{
					const FTransform& theOtherBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theOtherBoneData->Bone.BoneName);
#if 0 // We should use vector curve compare poses
					const float thePoseDistanceXYZ = FVector::Dist(theBoneTransformCS.GetLocation(), theOtherBoneTransformCS.GetLocation());
					const float thePoseDistanceXY = FVector::Dist2D(theBoneTransformCS.GetLocation(), theOtherBoneTransformCS.GetLocation());
					thePoseDistance = thePoseDistanceXY + FMath::Square(thePoseDistanceXYZ - thePoseDistanceXY);
#else
					thePoseDistance = FVector::Dist(theBoneTransformCS.GetLocation(), theOtherBoneTransformCS.GetLocation());
#endif

#if 0
					thePoseDistance = FVector::Dist(theBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
					thePoseDistance += FVector::Dist(theOtherBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
#endif

					theReferenceBoneTransformCS = theOtherBoneTransformCS;
				}
				else
				{
					thePoseDistance = FVector::Dist(theBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation());
				}
#endif			

				// Calculate the bone right axis value
				float theAxisX = 1.f;
				if (InPoseSampleData.SampleAxis == EAxis::X)
				{
					if (theReferenceBoneTransformCS.IsSet())
					{
						theAxisX = theBoneTransformCS.GetTranslation().X >= theReferenceBoneTransformCS.GetValue().GetTranslation().X ? 1.f : -1.f;
					}
					else
					{
						theAxisX = theBoneTransformCS.GetTranslation().X >= theRootBoneTransformCS.GetTranslation().X ? 1.f : -1.f;
					}
					theAxisX *= (theSearchBoneData.SampleBoneAxis.X == 0.f ? 1.f : theSearchBoneData.SampleBoneAxis.X);
				}

				// Calculate the bone forward axis value
				float theAxisY = 1.f;
				if (InPoseSampleData.SampleAxis == EAxis::Y)
				{
					if (theReferenceBoneTransformCS.IsSet())
					{
						theAxisY = theBoneTransformCS.GetTranslation().Y >= theReferenceBoneTransformCS.GetValue().GetTranslation().Y ? 1.f : -1.f;
					}
					else
					{
						theAxisY = theBoneTransformCS.GetTranslation().Y >= theRootBoneTransformCS.GetTranslation().Y ? 1.f : -1.f;
					}
					theAxisY *= (theSearchBoneData.SampleBoneAxis.Y == 0.f ? 1.f : theSearchBoneData.SampleBoneAxis.Y);
				}

				// Calculate the bone up axis value
				float theAxisZ = 1.f;
				if (InPoseSampleData.SampleAxis == EAxis::Z)
				{
					if (theReferenceBoneTransformCS.IsSet())
					{
						theAxisZ = theBoneTransformCS.GetTranslation().Z >= theReferenceBoneTransformCS.GetValue().GetTranslation().Z ? 1.f : -1.f;
					}
					else
					{
						theAxisZ = theBoneTransformCS.GetTranslation().Z >= theRootBoneTransformCS.GetTranslation().Z ? 1.f : -1.f;
					}
					theAxisZ *= (theSearchBoneData.SampleBoneAxis.Z == 0.f ? 1.f : theSearchBoneData.SampleBoneAxis.Z);
				}

				// Calculate the axis direction
				thePoseDistance = thePoseDistance * theAxisX * theAxisY * theAxisZ;

				// If the position between two frames is greater than the sampling interval, we should zero the two frames
				if (theOutputRuntimeCurve->Keys.Num() > 0)
				{
					// Get last curve data
					const FRichCurveKey theLastCurveKey = theOutputRuntimeCurve->Keys.Last();
					const float theLastCurveAxis = theLastCurveKey.Value / FMath::Abs(theLastCurveKey.Value);

					// Calculate current pose axis direction
					const float thePoseCurveAxis = thePoseDistance / FMath::Abs(thePoseDistance);

					// Calculate the delta curve time
					const float theDeltaCurveTime = thePoseSampleTime - theLastCurveKey.Time;

					// Check condition
					if (theDeltaCurveTime > (thePoseSampleRate + thePoseSampleRate * 0.5f) || (theLastCurveAxis != thePoseCurveAxis))
					{
						// Check for duplicate keys
						const int32 theLaseSecondKeyIndex = theOutputRuntimeCurve->Keys.Num() - 2;
						if (theOutputRuntimeCurve->Keys.IsValidIndex(theLaseSecondKeyIndex))
						{
							const FRichCurveKey theLastSecondsKey = theOutputRuntimeCurve->Keys[theLaseSecondKeyIndex];
							if (theLastSecondsKey.Time == theLastCurveKey.Time && theLastSecondsKey.Value == 0.f)
							{
								theOutputRuntimeCurve->AddKey(thePoseSampleTime, thePoseDistance);
								theOutputRuntimeCurve->AddKey(thePoseSampleTime, 0.f);

								if (theOutputCurve != nullptr)
								{
									theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, thePoseDistance);
									theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, 0.f);
								}
								continue;
							}
						}

						// Add the curve key
						if (theLastCurveKey.Value != 0.f)
						{
							theOutputRuntimeCurve->UpdateOrAddKey(theLastCurveKey.Time, 0.f);
							theOutputRuntimeCurve->AddKey(theLastCurveKey.Time, theLastCurveKey.Value);
						}
						theOutputRuntimeCurve->AddKey(thePoseSampleTime, thePoseDistance);
						theOutputRuntimeCurve->AddKey(thePoseSampleTime, 0.f);

						// Check the build curve condition
						if (theOutputCurve != nullptr)
						{
							if (theLastCurveKey.Value != 0.f)
							{
								FKeyHandle theLastKeyHandle = theOutputCurve->FloatCurve.FindKey(theLastCurveKey.Time);
								theOutputCurve->FloatCurve.DeleteKey(theLastKeyHandle);
								theOutputCurve->FloatCurve.AddKey(theLastCurveKey.Time, 0.f);
								theOutputCurve->FloatCurve.AddKey(theLastCurveKey.Time, theLastCurveKey.Value);
							}
							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, thePoseDistance);
							theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, 0.f);
						}
						continue;
					}
				}

				// Add the curve key
				theOutputRuntimeCurve->AddKey(thePoseSampleTime, thePoseDistance);

				// Check the build curve condition
				if (theOutputCurve != nullptr)
				{
					theOutputCurve->FloatCurve.AddKey(thePoseSampleTime, thePoseDistance);
				}
			}

			// Iterate
			thePoseSampleTime += thePoseSampleRate;
		}

		// Remove invalid curve reference
		if (InBuildCurve)
		{
			for (FVirtualPoseSampleCurveData& theSampleCurveData : theSearchBoneData.SampleCurvesData)
			{
				if (theSampleCurveData.OutputCurveName == NAME_None)
				{
					continue;
				}

				// Check the runtime curve is valid
				FRichCurve* theOutputRuntimeCurve = theSampleCurveData.OutputCurve.GetRichCurve();
				if (theOutputRuntimeCurve == nullptr || !theOutputRuntimeCurve->HasAnyData())
				{
					UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theSampleCurveData.OutputCurveName);
				}
			}
		}
	}

	// Apply sequence changed
	if (InBuildCurve)
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_PoseSearch::SampleTwoPoseFeatures(UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence, FVirtualPoseSearchSampleData& InPoseSampleData, FRuntimeFloatCurve& OutPositionCurve)
{
	SamplePoseFeatures<float, FFloatCurve>(InAnimSequence, InPoseSampleData, false);
	SamplePoseFeatures<float, FFloatCurve>(InNextSequence, InPoseSampleData, false);

	// Clear the curve data
	OutPositionCurve.GetRichCurve()->Reset();

	// Define the pose search data
	FName theReferenceCurveName = NAME_None;
	TMap<float, FName> thePoseDistanceCurvesMap;

	// Each every bone try sample pose data
	for (FVirtualPoseSampleBoneData& theSearchBoneData : InPoseSampleData.BonesData)
	{
		for (FVirtualPoseSampleCurveData& theSampleCurveData : theSearchBoneData.SampleCurvesData)
		{
			theReferenceCurveName = theSampleCurveData.ReferenceCurveName;
			thePoseDistanceCurvesMap.Add(theSampleCurveData.ReferenceCurveValue, theSampleCurveData.OutputCurveName);
		}
	}

	// Try sample each animation pose
	float thePoseSampleTime = 0.f;
	const float& theSequenceLength = InAnimSequence->GetPlayLength();
	const double& thePoseSampleRate = InPoseSampleData.SampleRate.AsInterval();
	while (thePoseSampleTime <= (theSequenceLength + 0.0001f))
	{
		// Clamp the pose time
		thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theSequenceLength);

		// Compare poses both animation sequence
		const float theComparePose = UVAT_PoseSearchLibrary::ComparePosesAsAnimation(thePoseSampleTime, InAnimSequence, InNextSequence
			, theReferenceCurveName, thePoseDistanceCurvesMap);

		// Output the pose value to runtime curve
		OutPositionCurve.GetRichCurve()->AddKey(thePoseSampleTime, theComparePose);

		// Iterate
		thePoseSampleTime += thePoseSampleRate;
	}
}

template <typename DataType, typename CurveClass>
void UVAT_PoseSearch::SamplePoseTimeSyncFeatures(UAnimSequence* InAnimSequence, FVirtualPoseSearchSampleData& InPoseSampleData, FVirtualPoseSearchTimeSyncData& InPoseTimeSyncData, bool InBuildCurve)
{
	// Check the time sync data is valid
	if (InPoseTimeSyncData.CurvesName.Num() == 0)
	{
		return;
	}

	// Always rebuild the leader pose features data
	UVAT_PoseSearch::SamplePoseFeatures<float, FFloatCurve>(InAnimSequence, InPoseSampleData);

	// Build the transfer curves data
	FVirtualTransferCurveData theTransferCurveData;
	theTransferCurveData.Assets = InPoseTimeSyncData.FollowAssets;
	theTransferCurveData.CurvesName = InPoseTimeSyncData.CurvesName;

	// Copy the time sync curves data
	UVAT_Curve::TransferAnimationCurvesData(InAnimSequence, theTransferCurveData);
}

/////////////////////////////////////////////////////////////////////////////////////
// BEGIN DEPRECATED, just some experimental approach.
/////////////////////////////////////////////////////////////////////////////////////

#if 0
float UVAT_PoseSearch::PoseBakedMatching(const TArray<FName>& InCurvesName, UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence)
{
	// Check the curves name is valid
	if (InCurvesName.Num() == 0)
	{
		return -1.f;
	}

	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return -1.f;
	}

	// Check the next sequence is valid
	if (InNextSequence == nullptr)
	{
		return -1.f;
	}

	// Check the curve value matching
	for (const FName& theCurveName : InCurvesName)
	{
		TArray<float> CurveTimesA, CurveValuesA;
		UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, theCurveName, CurveTimesA, CurveValuesA);
	}

	return -1.f;
}

float UVAT_PoseSearch::PoseMergeMatching(const float InDesiredPos, const FName& InCurveName, UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence)
{
	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return -1.f;
	}

	// Check the next sequence is valid
	if (InNextSequence == nullptr)
	{
		return -1.f;
	}

	// Get the both curve data
	TArray<float> CurveTimesA, CurveValuesA;
	UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, InCurveName, CurveTimesA, CurveValuesA);

	// Get the both curve data
	TArray<float> CurveTimesB, CurveValuesB;
	UAnimationBlueprintLibrary::GetFloatKeys(InNextSequence, InCurveName, CurveTimesB, CurveValuesB);

	// Define the matching pose data
	float theMatchingPoseValue = 1e6f;
	float theMatchingPosePosition = -1.f;

	// Matching
	if (InDesiredPos >= 0.f)
	{
		FRuntimeFloatCurve theFloatCurveA = FRuntimeFloatCurve();
		for (int32 Pos = 0; Pos < CurveValuesA.Num(); Pos++)
		{
			const float& theCurveTimeA = CurveTimesA[Pos];
			const float& theCurveValueA = CurveValuesA[Pos];
			theFloatCurveA.GetRichCurve()->AddKey(theCurveTimeA, theCurveValueA);
		}

		// Calculate the value from desired position
		const float theDesiredValue = theFloatCurveA.GetRichCurve()->Eval(InDesiredPos);

		// Search
		for (int32 NextPos = 0; NextPos < CurveValuesB.Num(); NextPos++)
		{
			const float& theCurveTimeB = CurveTimesB[NextPos];
			const float& theCurveValueB = CurveValuesB[NextPos];

			// Calculate the delta distance
			const float theDeltaDist = theDesiredValue - theCurveValueB;

			// Compare values ​​closer to 0
			if (FMath::Abs(theMatchingPoseValue) > FMath::Abs(theDeltaDist))
			{
				theMatchingPoseValue = theDeltaDist;
				theMatchingPosePosition = theCurveTimeB;
			}
		}
	}
	else
	{
		for (int32 Pos = 0; Pos < CurveValuesA.Num(); Pos++)
		{
			const float& theCurveTimeA = CurveTimesA[Pos];
			const float& theCurveValueA = CurveValuesA[Pos];

			for (int32 NextPos = 0; NextPos < CurveValuesB.Num(); NextPos++)
			{
				const float& theCurveTimeB = CurveTimesB[NextPos];
				const float& theCurveValueB = CurveValuesB[NextPos];

				// Calculate the delta distance
				const float theDeltaDist = theCurveValueA - theCurveValueB;

				// Compare values ​​closer to 0
				if (FMath::Abs(theMatchingPoseValue) > FMath::Abs(theDeltaDist))
				{
					theMatchingPoseValue = theDeltaDist;
					theMatchingPosePosition = theCurveTimeB;
				}
			}
		}
	}

	return theMatchingPosePosition;
}

void UVAT_PoseSearch::PoseBaked(FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InAnimSequence, bool InBuildCurve /*= true*/)
{
	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Check the curves name is valid
	if (InPoseSampleData.Bones.Num() > InPoseSampleData.BakeCurvesName.Num())
	{
		return;
	}

	// Clear the pose search data
	bool HasNewCurve = false;
	InPoseSampleData.BonesData.Reset();

	// Calculate the pose sample time
	const float thePoseSampleTime = 1.f / float(InPoseSampleData.SampleRate);

	// Get the sequence length
	const float& CurrentSequenceLength = InAnimSequence->GetPlayLength();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Calculate all bone transform
	for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
	{
		// Get the bone curve name
		const FName& theCurveName = InPoseSampleData.BakeCurvesName[BoneIndex];

		// Create the float curve reference
		FRuntimeFloatCurve& theRuntimeFloatCurve = InPoseSampleData.BakedCurves.FindOrAdd(theCurveName);
		theRuntimeFloatCurve.GetRichCurve()->Reset();

		// Check build curve condition
		HasNewCurve = false;
		if (theCurveName != NAME_None && InBuildCurve)
		{
			HasNewCurve = true;
			UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
			UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
		}

		// Each
		float thePoseSampleTime = 0.f;
		while (thePoseSampleTime < CurrentSequenceLength)
		{
			// Create the pose search data
			FVirtualPoseSampleBoneData NewPoseSearchData = FVirtualPoseSampleBoneData();
			NewPoseSearchData.PoseSampleTime = thePoseSampleTime;

			// Calculate root motion transform
			FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime - thePoseSampleTime);
			FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime);
			FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
			const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

			// Get the root bone transform
			const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theRootBoneName);

			// Get the bone component space transform
			const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
			const FTransform& CurrentBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theBoneReference.BoneName);

			// Calculate the bone to root bone distance
			NewPoseSearchData.PoseSamplePosition = FVector::Dist(CurrentBoneTransform.GetLocation(), theRootBoneTransformCS.GetLocation());
			
			// Add the curve key
			theRuntimeFloatCurve.GetRichCurve()->AddKey(thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
			if (HasNewCurve)
			{
				UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, theCurveName
					, thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
			}

			// Loop
			thePoseSampleTime += thePoseSampleTime;
			InPoseSampleData.BonesData.Add(NewPoseSearchData);
		}
	}

	// Baked the curve
	if (HasNewCurve)
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_PoseSearch::PoseMerge(FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InAnimSequence, bool InBuildCurve /*= true*/)
{
	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Check the curves name is valid
	if (InPoseSampleData.Bones.Num() > InPoseSampleData.BakeCurvesName.Num())
	{
		return;
	}

	// Clear the pose search data
	bool HasNewCurve = false;
	InPoseSampleData.BonesData.Reset();

	// Calculate the pose sample time
	const float thePoseSampleTime = 1.f / float(InPoseSampleData.SampleRate);

	// Get the sequence length
	const float& CurrentSequenceLength = InAnimSequence->GetPlayLength();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const FName& thePelvisBoneName = theReferenceSkeleton.GetBoneName(1);

	// Get the bone curve name
	const FName& theCurveName = InPoseSampleData.BakeCurveName;

	// Create the float curve reference
	FRuntimeFloatCurve& theRuntimeFloatCurve = InPoseSampleData.BakedCurve;
	theRuntimeFloatCurve.GetRichCurve()->Reset();

	// Check build curve condition
	HasNewCurve = false;
	if (theCurveName != NAME_None && InBuildCurve)
	{
		HasNewCurve = true;
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
	}

#if 0
	TArray<float> theFeetPosCurveTimes, theFeetPosCurveValues;
	UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, InPoseSampleData.FeetPositionCurveName, theFeetPosCurveTimes, theFeetPosCurveValues);
	FRuntimeFloatCurve theFeetPosCurve = FRuntimeFloatCurve();
	for (int32 Pos = 0; Pos < theFeetPosCurveTimes.Num(); Pos++)
	{
		const float& theCurveTime = theFeetPosCurveTimes[Pos];
		const float& theCurveValue = theFeetPosCurveValues[Pos];
		theFeetPosCurve.GetRichCurve()->AddKey(theCurveTime, theCurveValue);
	}
#endif

	// Each
	float thePoseSampleTime = 0.f;
	while (thePoseSampleTime < CurrentSequenceLength)
	{
		// Create the pose search data
		FVirtualPoseSampleBoneData NewPoseSearchData = FVirtualPoseSampleBoneData();
		NewPoseSearchData.PoseSampleTime = thePoseSampleTime;

		// Calculate root motion transform
		FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime - thePoseSampleTime);
		FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime);
		FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
		const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

		// Get the root bone transform
		const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theRootBoneName);
		const FTransform& thePelvisBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, thePelvisBoneName);

		// Define the accumulate pose distance
		float thePoseAccumulateDistance = 0.f;

		TArray<float> BonesY;
		TArray<float> BonesAxis;
		TArray<float> BonesDistance;

		// Calculate all bone transform
		for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
		{
			// Get the bone component space transform
			const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
			const FTransform& CurrentBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theBoneReference.BoneName);

#if 0
			const FVector Normal = (CurrentBoneTransform.GetLocation() - theRootBoneTransformCS.GetLocation()).GetSafeNormal();
			const FVector AxisNormal = thePelvisBoneTransformCS.GetRotation().Vector();
			const float AxisValue = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::DegAcos(FVector::DotProduct(Normal, AxisNormal)), 0.f, 90.f) ? 1.0f : -1.0f;
#else 
			// 			const float AxisValue = theFeetPosCurve.GetRichCurve()->HasAnyData()
			// 				? theFeetPosCurve.GetRichCurveConst()->Eval(thePoseSampleTime)
			// 			: CurrentBoneTransform.GetLocation().Y >= 0.f ? 1.f : -1.f;
			float AxisValue = CurrentBoneTransform.GetLocation().Y >= 0.f ? 1.f : -1.f;
			BonesAxis.Add(AxisValue);
			BonesY.Add(CurrentBoneTransform.GetLocation().Y);
#endif

			float theSampleDistance
				= FVector::Dist(CurrentBoneTransform.GetLocation(), theRootBoneTransformCS.GetLocation());

			thePoseAccumulateDistance += theSampleDistance;
		}

// 		check(BonesDistance.Num() >= 2);
// 
// 		const bool bIsRightFoot = BonesY[1] >= BonesY[0];
// 		BonesAxis[0] = bIsRightFoot ? -1.f : 1.f;
// 		BonesAxis[1] = bIsRightFoot ? 1.f : -1.f;
// 
// 		for (int32 i = 0; i < BonesDistance.Num(); i++)
// 		{
// 			thePoseAccumulateDistance += BonesDistance[i];
// 			//thePoseAccumulateDistance *= (bIsRightFoot ? 1.f : -1.f);
// 		}

		// Calculate the bone to root bone distance
		NewPoseSearchData.PoseSamplePosition = thePoseAccumulateDistance;

		// Add the curve key
		theRuntimeFloatCurve.GetRichCurve()->AddKey(thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
		if (HasNewCurve)
		{
			UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, theCurveName
				, thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
		}

		// Loop
		thePoseSampleTime += thePoseSampleTime;
		InPoseSampleData.BonesData.Add(NewPoseSearchData);
	}

	// Baked the curve
	if (HasNewCurve)
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_PoseSearch::FeetPoseSearch(FVirtualPoseSearchSampleData& InPoseSampleData, FVirtualSmartFeetLandNotifyData& InFeetData, UAnimSequence* InAnimSequence, bool InBuildCurve /*= true*/)
{
	// Check the current sequence is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Check the curves name is valid
	if (InPoseSampleData.Bones.Num() > InPoseSampleData.BakeCurvesName.Num())
	{
		return;
	}

	// Clear the pose search data
	bool HasNewCurve = false;
	InPoseSampleData.BonesData.Reset();

	// Calculate the pose sample time
	const float thePoseSampleTime = 1.f / float(InPoseSampleData.SampleRate);

	// Get the sequence length
	const float& CurrentSequenceLength = InAnimSequence->GetPlayLength();

	// Get the skeleton reference
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const FName& thePelvisBoneName = theReferenceSkeleton.GetBoneName(1);

	TArray<float> theFeetPosCurveTimes, theFeetPosCurveValues;
	UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, InPoseSampleData.FeetPositionCurveName, theFeetPosCurveTimes, theFeetPosCurveValues);
	FRuntimeFloatCurve theFeetPosCurve = FRuntimeFloatCurve();
	for (int32 Pos = 0; Pos < theFeetPosCurveTimes.Num(); Pos++)
	{
		const float& theCurveTime = theFeetPosCurveTimes[Pos];
		const float& theCurveValue = theFeetPosCurveValues[Pos];
		theFeetPosCurve.GetRichCurve()->AddKey(theCurveTime, theCurveValue);
	}

	// Calculate all bone transform
	for (int32 BoneDataIndex = 0; BoneDataIndex < InFeetData.SmartFeetLandNotifyData.Num(); BoneDataIndex++)
	{
		const FVirtualLegBaseData& theFootLandData = InFeetData.SmartFeetLandNotifyData[BoneDataIndex];

		// Get the bone curve name
		const FName& theCurveName = theFootLandData.FootBone.BoneName;
		const float& theCurveApexValue = theFootLandData.CurveApexValue;

		// Create the float curve reference
		FRuntimeFloatCurve& theRuntimeFloatCurve = InPoseSampleData.BakedCurve;
		theRuntimeFloatCurve.GetRichCurve()->Reset();

		// Check build curve condition
		HasNewCurve = false;
		if (theCurveName != NAME_None && InBuildCurve)
		{
			HasNewCurve = true;
			UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
			UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);
		}

		// Each
		float thePoseSampleTime = 0.f;
		while (thePoseSampleTime < CurrentSequenceLength)
		{
			// Eval the foot position curve value
			float theFootPosCurveValue = theFeetPosCurve.GetRichCurveConst()->Eval(thePoseSampleTime);
			for (int32 Pos = 0; Pos < theFeetPosCurveTimes.Num(); Pos++)
			{
				const float& theCurveTime = theFeetPosCurveTimes[Pos];
				const float& theCurveValue = theFeetPosCurveValues[Pos];

				theFootPosCurveValue = theCurveValue;
				if (theFeetPosCurveTimes.IsValidIndex(Pos + 1))
				{
					const float& theCurveNextTime = theFeetPosCurveTimes[Pos + 1];
					if (FMath::IsWithinInclusive(thePoseSampleTime, theCurveTime, theCurveNextTime))
					{
						theFootPosCurveValue = theCurveValue;
						break;
					}
				}
			}

			if (theFootPosCurveValue != theCurveApexValue)
			{
				if (theRuntimeFloatCurve.GetRichCurve()->HasAnyData())
				{
					const float theLastValue = theRuntimeFloatCurve.GetRichCurve()->GetLastKey().Value;
					// Add the curve key
					theRuntimeFloatCurve.GetRichCurve()->AddKey(thePoseSampleTime, theLastValue);
					if (HasNewCurve)
					{
						UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, theCurveName, thePoseSampleTime, theLastValue);
					}
				}

				// Loop
				thePoseSampleTime += thePoseSampleTime;
				continue;
			}

			// Create the pose search data
			FVirtualPoseSampleBoneData NewPoseSearchData = FVirtualPoseSampleBoneData();
			NewPoseSearchData.PoseSampleTime = thePoseSampleTime;

			// Calculate root motion transform
			FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime - thePoseSampleTime);
			FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InAnimSequence, 0.f, thePoseSampleTime);
			FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
			const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

			// Get the root bone transform
			const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theRootBoneName);
			const FTransform& thePelvisBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, thePelvisBoneName);

			// Define the accumulate pose distance
			float thePoseAccumulateDistance = 0.f;

			TArray<float> BonesY;
			TArray<float> BonesAxis;
			TArray<float> BonesDistance;

#if 0
			// Calculate all bone transform
			for (int32 BoneIndex = 0; BoneIndex < InFeetData.SmartFeetLandNotifyData.Num(); BoneIndex++)
			{
				// Define the axis direction
				float theAxisValue = 1.f;

				// Get the bone component space transform
				const FBoneReference& theBoneReference = InFeetData.SmartFeetLandNotifyData[BoneIndex].FootBone;
				const FTransform& theBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theBoneReference.BoneName);

#if 0
				const FVector Normal = (theBoneTransformCS.GetLocation() - theRootBoneTransformCS.GetLocation()).GetSafeNormal();
				const FVector AxisNormal = thePelvisBoneTransformCS.GetRotation().Vector();
				const float AxisValue = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::DegAcos(FVector::DotProduct(Normal, AxisNormal)), 0.f, 90.f) ? 1.0f : -1.0f;
#else 
				const float AxisValue = theFeetPosCurve.GetRichCurveConst()->Eval(thePoseSampleTime);
#endif

#if 1 // Calculate the foot in other foot direction
				// Get the mirror bone component space transform
				const FBoneReference& theMirrorBoneReference = InFeetData.SmartFeetLandNotifyData[BoneIndex == 0 ? 1 : 0].FootBone;
				const FTransform& theMirrorBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theMirrorBoneReference.BoneName);

				// Calculate the direction in both forward distance
				theAxisValue = theBoneTransformCS.GetTranslation().Y >= theMirrorBoneTransformCS.GetTranslation().Y ? 1.f : -1.f;
#endif
				const float theSampleDistance = FVector::Dist2D(theBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation()) * theAxisValue;
				thePoseAccumulateDistance += theSampleDistance;
			}
#else
			// Define the axis direction
			float theAxisValue = 1.f;

			// Get the bone component space transform
			const FBoneReference& theBoneReference = theFootLandData.FootBone;
			const FTransform& theBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theBoneReference.BoneName);

			const float AxisValue = theFeetPosCurve.GetRichCurveConst()->Eval(thePoseSampleTime);

			// Get the mirror bone component space transform
			const FBoneReference& theMirrorBoneReference = InFeetData.SmartFeetLandNotifyData[BoneDataIndex == 0 ? 1 : 0].FootBone;
			const FTransform& theMirrorBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theMirrorBoneReference.BoneName);

			// Calculate the direction in both forward distance
			theAxisValue = theBoneTransformCS.GetTranslation().Y >= theMirrorBoneTransformCS.GetTranslation().Y ? 1.f : -1.f;

			// Calculate the sample distance
			float theSampleDistance = FVector::Dist2D(theBoneTransformCS.GetLocation(), theRootBoneTransformCS.GetLocation()) * theAxisValue;

			// Calculate thigh - knee - foot angle
			{
				// Check the bone index is valid
				const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneReference.BoneName);
				if (theBoneIndex == INDEX_NONE)
				{
					return;
				}

				// Check the knee bone index is valid
				const int32& theKneeBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
				if (theKneeBoneIndex == INDEX_NONE)
				{
					return;
				}

				// Get the knee bone name
				const FName& theKneeBoneName = theReferenceSkeleton.GetBoneName(theKneeBoneIndex);
				if (theKneeBoneName == NAME_None)
				{
					return;
				}
				const FTransform& theKneeBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theKneeBoneName);

				// Check the thigh bone index is valid
				const int32& theThighBoneIndex = theReferenceSkeleton.GetParentIndex(theKneeBoneIndex);
				if (theThighBoneIndex == INDEX_NONE)
				{
					return;
				}

				// Get the thigh bone name
				const FName& theThighBoneName = theReferenceSkeleton.GetBoneName(theThighBoneIndex);
				if (theThighBoneName == NAME_None)
				{
					return;
				}
				const FTransform& theThighBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, thePoseSampleTime, theThighBoneName);

				FVector thePositions[3];
				thePositions[0] = theThighBoneTransformCS.GetLocation();
				thePositions[1] = theKneeBoneTransformCS.GetLocation();
				thePositions[2] = theBoneTransformCS.GetLocation();

				// Define the bone axis
				const FVector theBoneAxis = FVector::ForwardVector;

				// We always only calculate the angle between the first 3 points
				const FVector StartNormal = (FVector::PointPlaneProject(thePositions[0], thePositions[1], theBoneAxis) - thePositions[1]).GetSafeNormal();
				const FVector DesiredNormal = (FVector::PointPlaneProject(thePositions[2], thePositions[1], theBoneAxis) - thePositions[1]).GetSafeNormal();
				theSampleDistance = FMath::RadiansToDegrees(acosf(FVector::DotProduct(StartNormal, DesiredNormal)));

				//// Calculate pre-translation vector between this bone and child
				//const FVector InitialDir = (CurrentTransform.GetLocation() - ParentTransform.GetLocation()).GetSafeNormal();

				//// Get vector from the post-translation bone to it's child
				//const FVector TargetDir = (CurrentLink.Location - ParentLink.Location).GetSafeNormal();

				//const FQuat DeltaRotation = FQuat::FindBetweenNormals(InitialDir, TargetDir);
//
//#if WITH_EDITORONLY_DATA
//				FRotator theDeltaRotation = DeltaRotation.Rotator();
//				FRotator theTargetRotation = TargetDir.ToOrientationRotator();
//#endif // WITH_EDITORONLY_DATA
			}

			// Calculate the bone to root bone distance
			NewPoseSearchData.PoseSamplePosition = theSampleDistance;

			// Add the curve key
			theRuntimeFloatCurve.GetRichCurve()->AddKey(thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
			if (HasNewCurve)
			{
				UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, theCurveName
					, thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
			}

			// Loop
			thePoseSampleTime += thePoseSampleTime;
			InPoseSampleData.BonesData.Add(NewPoseSearchData);
		}
#endif
	}

	// Baked the curve
	if (HasNewCurve)
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_PoseSearch::PoseSearch(FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InCurrentSequence, UAnimSequence* InNextSequence, bool InBuildCurve)
{
	// Check the current sequence is valid
	if (InCurrentSequence == nullptr)
	{
		return;
	}

	// Check the next sequence is valid
	if (InNextSequence == nullptr)
	{
		return;
	}

	// Clear the pose search data
	InPoseSampleData.BonesData.Reset();

	// Create the float curve reference
	FRuntimeFloatCurve& theRuntimeFloatCurve = InPoseSampleData.PositionsCurves.FindOrAdd(InNextSequence);
	theRuntimeFloatCurve.GetRichCurve()->Reset();

	// Calculate the pose sample time
	const float thePoseSampleTime = 1.f / float(InPoseSampleData.SampleRate);

	// Get both sequence length
	const float& NextSequenceLength = InNextSequence->GetPlayLength();
	const float& CurrentSequenceLength = InCurrentSequence->GetPlayLength();

	// Get the skeleton reference
	USkeleton* theSkeleton = InCurrentSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);


	// Check build curve condition
	bool HasNewCurve = false;
	if (InPoseSampleData.BakeCurveName != NAME_None && InBuildCurve)
	{
		HasNewCurve = true;
		UAnimationBlueprintLibrary::RemoveCurve(InCurrentSequence, InPoseSampleData.BakeCurveName);
		UAnimationBlueprintLibrary::AddCurve(InCurrentSequence, InPoseSampleData.BakeCurveName);
	}

	// Each
	float thePoseSampleTime = 0.f;
	while (thePoseSampleTime < CurrentSequenceLength)
	{
		// Create the pose search data
		FVirtualPoseSampleBoneData NewPoseSearchData = FVirtualPoseSampleBoneData();
		NewPoseSearchData.PoseSampleTime = thePoseSampleTime;

		// Calculate root motion transform
		FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InCurrentSequence, 0.f, thePoseSampleTime - thePoseSampleTime);
		FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InCurrentSequence, 0.f, thePoseSampleTime);
		FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
		const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

		// Calculate all bone transform
		TArray<FTransform> CurrentBonesTransform;
		for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
		{
			const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
			const FTransform& CurrentBoneTransform = UVAT_Bone::GetBoneTransformCS(InCurrentSequence, false, thePoseSampleTime, theBoneReference.BoneName);
			CurrentBonesTransform.Add(CurrentBoneTransform);
		}

		// Each current sequence pose to next sequence all frame pose
		float NextPoseSampleTime = 0.f;
		while (NextPoseSampleTime < NextSequenceLength)
		{
			// Calculate root motion transform
			FTransform NextPreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InNextSequence, 0.f, NextPoseSampleTime - thePoseSampleTime);
			FTransform NextCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InNextSequence, 0.f, NextPoseSampleTime);
			FTransform NextAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(NextCurrentRootMotion, NextPreviousRootMotion);
			const float NextSpeed = (NextAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

			// Calculate sample velocity
			float SampleSpeed = theCurrentSpeed - NextSpeed;
			FVector SampleVelocity = theCurrentAccumulateRootMotionTransform.GetTranslation() - NextAccumulateRootMotionTransform.GetTranslation();

			float thePoseAccumulateDistance = 0.f;
			const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InNextSequence, false, NextPoseSampleTime, theRootBoneName);

			// Calculate all bone transform
			for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
			{
				const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
				const FTransform& CurrentBoneTransform = CurrentBonesTransform[BoneIndex];
				const FTransform& NextBoneTransform = UVAT_Bone::GetBoneTransformCS(InNextSequence, false, NextPoseSampleTime, theBoneReference.BoneName);

				const FVector Normal = (NextBoneTransform.GetLocation() - theRootBoneTransformCS.GetLocation()).GetSafeNormal();
				const FVector AxisNormal = theRootBoneTransformCS.GetRotation().Vector();
				const float AxisValue = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::DegAcos(FVector::DotProduct(Normal, AxisNormal)), 0.f, 90.f) ? 1.0f : -1.0f;

				float theSampleDistance = FVector::Dist(CurrentBoneTransform.GetLocation(), NextBoneTransform.GetLocation())/* * AxisValue*/;
				thePoseAccumulateDistance += theSampleDistance;
				FVector SampleOffset = CurrentBoneTransform.GetTranslation() - NextBoneTransform.GetTranslation();
			}

			NewPoseSearchData.PoseSampleTimes.Add(NextPoseSampleTime);
			NewPoseSearchData.PoseSampleSpeeds.Add(SampleSpeed);
			//NewPoseSearchData.PoseSampleOffsets.Add(SampleOffset);
			NewPoseSearchData.PoseSampleDistances.Add(thePoseAccumulateDistance);

			NextPoseSampleTime += thePoseSampleTime;
		}

		// Search best speed pose index
		int32 BestSpeedPoseIndex;
		FMath::Max<float>(NewPoseSearchData.PoseSampleSpeeds, &BestSpeedPoseIndex);

		// Search best distance pose index
		int32 BestDistancePoseIndex;
		FMath::Min<float>(NewPoseSearchData.PoseSampleDistances, &BestDistancePoseIndex);

		// Search pose is forward
		NewPoseSearchData.PoseSampleIndex = BestDistancePoseIndex;
		NewPoseSearchData.PoseSamplePosition = NewPoseSearchData.PoseSampleTimes[BestDistancePoseIndex];

		// Add the curve key
		theRuntimeFloatCurve.GetRichCurve()->AddKey(thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
		if (HasNewCurve)
		{
			UAnimationBlueprintLibrary::AddFloatCurveKey(InCurrentSequence, InPoseSampleData.BakeCurveName
			, thePoseSampleTime, NewPoseSearchData.PoseSamplePosition);
		}

		// Loop
		thePoseSampleTime += thePoseSampleTime;
		InPoseSampleData.BonesData.Add(NewPoseSearchData);
	}

	// Baked the curve
	if (HasNewCurve)
	{
		UVAT_Library::ModifyFromAnimSequence(InCurrentSequence);
	}
}

float UVAT_PoseSearch::PoseSimpleSearch(FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InCurrentSequence, UAnimSequence* InNextSequence, float InCurrentPos)
{
	// Check the current sequence is valid
	if (InCurrentSequence == nullptr)
	{
		return -1.f;
	}

	// Check the next sequence is valid
	if (InNextSequence == nullptr)
	{
		return -1.f;
	}

	// Calculate the pose sample time
	const float thePoseSampleTime = 1.f / float(InPoseSampleData.SampleRate);

	// Get both sequence length
	const float& NextSequenceLength = InNextSequence->GetPlayLength();
	const float& CurrentSequenceLength = InCurrentSequence->GetPlayLength();

	// Get the skeleton reference
	USkeleton* theSkeleton = InCurrentSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Each
	float thePoseSampleTime = InCurrentPos;

	// Create the pose search data
	FVirtualPoseSampleBoneData NewPoseSearchData = FVirtualPoseSampleBoneData();
	NewPoseSearchData.PoseSampleTime = thePoseSampleTime;

	// Calculate root motion transform
	FTransform thePreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InCurrentSequence, 0.f, thePoseSampleTime - thePoseSampleTime);
	FTransform theCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InCurrentSequence, 0.f, thePoseSampleTime);
	FTransform theCurrentAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentRootMotion, thePreviousRootMotion);
	const float theCurrentSpeed = (theCurrentAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

	// Calculate all bone transform
	TArray<FTransform> CurrentBonesTransform;
	for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
	{
		const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
		const FTransform& CurrentBoneTransform = UVAT_Bone::GetBoneTransformCS(InCurrentSequence, false, thePoseSampleTime, theBoneReference.BoneName);
		CurrentBonesTransform.Add(CurrentBoneTransform);
	}

	// Each current sequence pose to next sequence all frame pose
	float NextPoseSampleTime = 0.f;
	while (NextPoseSampleTime < NextSequenceLength)
	{
		// Calculate root motion transform
		FTransform NextPreviousRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InNextSequence, 0.f, NextPoseSampleTime - thePoseSampleTime);
		FTransform NextCurrentRootMotion = UVAT_Asset::OnExtractRootMotionFromRange(InNextSequence, 0.f, NextPoseSampleTime);
		FTransform NextAccumulateRootMotionTransform = UVAT_Library::Subtract_TransformTransform(NextCurrentRootMotion, NextPreviousRootMotion);
		const float NextSpeed = (NextAccumulateRootMotionTransform.GetTranslation() / thePoseSampleTime).Size();

		// Calculate sample velocity
		float SampleSpeed = theCurrentSpeed - NextSpeed;
		FVector SampleVelocity = theCurrentAccumulateRootMotionTransform.GetTranslation() - NextAccumulateRootMotionTransform.GetTranslation();

		float thePoseAccumulateDistance = 0.f;
		const FTransform& theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InNextSequence, false, NextPoseSampleTime, theRootBoneName);

		// Calculate all bone transform
		for (int32 BoneIndex = 0; BoneIndex < InPoseSampleData.Bones.Num(); BoneIndex++)
		{
			const FBoneReference& theBoneReference = InPoseSampleData.Bones[BoneIndex];
			const FTransform& CurrentBoneTransform = CurrentBonesTransform[BoneIndex];
			const FTransform& NextBoneTransform = UVAT_Bone::GetBoneTransformCS(InNextSequence, false, NextPoseSampleTime, theBoneReference.BoneName);

			const FVector Normal = (NextBoneTransform.GetLocation() - theRootBoneTransformCS.GetLocation()).GetSafeNormal();
			const FVector AxisNormal = theRootBoneTransformCS.GetRotation().Vector();
			const float AxisValue = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::DegAcos(FVector::DotProduct(Normal, AxisNormal)), 0.f, 90.f) ? 1.0f : -1.0f;

			float theSampleDistance = FVector::Dist(CurrentBoneTransform.GetLocation(), NextBoneTransform.GetLocation())/* * AxisValue*/;
			thePoseAccumulateDistance += theSampleDistance;
			FVector SampleOffset = CurrentBoneTransform.GetTranslation() - NextBoneTransform.GetTranslation();
		}

		NewPoseSearchData.PoseSampleTimes.Add(NextPoseSampleTime);
		NewPoseSearchData.PoseSampleSpeeds.Add(SampleSpeed);
		//NewPoseSearchData.PoseSampleOffsets.Add(SampleOffset);
		NewPoseSearchData.PoseSampleDistances.Add(thePoseAccumulateDistance);

		NextPoseSampleTime += thePoseSampleTime;
	}

	// Search best speed pose index
	int32 BestSpeedPoseIndex;
	FMath::Max<float>(NewPoseSearchData.PoseSampleSpeeds, &BestSpeedPoseIndex);

	// Search best distance pose index
	int32 BestDistancePoseIndex;
	FMath::Min<float>(NewPoseSearchData.PoseSampleDistances, &BestDistancePoseIndex);

	const float theBestPose = NewPoseSearchData.PoseSampleTimes[BestDistancePoseIndex];

	// Search pose is forward
	NewPoseSearchData.PoseSampleIndex = BestDistancePoseIndex;
	NewPoseSearchData.PoseSamplePosition = theBestPose;

	return theBestPose;
}
#endif
