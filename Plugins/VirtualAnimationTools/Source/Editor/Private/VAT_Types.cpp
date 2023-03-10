// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "VAT_Types.h"
#include "Library/VAT_Bone.h"
#include "Library/VAT_Curve.h"

/*-----------------------------------------------------------------------------
	FVirtualResizeCurveData implementation.
-----------------------------------------------------------------------------*/

void FVirtualResizeCurveData::Resize(FVector2D* InTimeRange, FVector2D* InValueRange)
{
	// If the range reference is valid, we will override it.
	if (InTimeRange != nullptr)
	{
		TimeRange = *InTimeRange;
	}

	// If the range reference is valid, we will override it.
	if (InValueRange != nullptr)
	{
		ValueRange = *InValueRange;
	}

	FRichCurve* theCurvePtr = FloatCurve.GetRichCurve();

	// Check the float curve data is valid
	if (!theCurvePtr->HasAnyData() || theCurvePtr->Keys.Num() <= 1)
	{
		return;
	}

	// Define the keys data
	TArray<float> theKeysTime, theKeysAlpha;
	TArray<FRichCurveKey> theSourceKeys = theCurvePtr->Keys;

	// Get the source keys data
	const FRichCurveKey theFirstCurveKey = theCurvePtr->GetFirstKey();
	const FRichCurveKey theLastCurveKey = theCurvePtr->GetLastKey();

	// Each every keys
	for (FRichCurveKey& theKey : theCurvePtr->Keys)
	{
		theKeysTime.Add(theKey.Time);
		theKeysAlpha.Add(theKey.Value / theLastCurveKey.Value);
	}

	// Get the source keys time range
	const FVector2D theSourceTimeRange(theFirstCurveKey.Time, theLastCurveKey.Time);
	const float theSourceDeltaTime = (theSourceTimeRange.Y - theSourceTimeRange.X) / theCurvePtr->Keys.Num();

	// Get the resize keys time range
	const FVector2D theResizeTimeRange(TimeRange);
	const float theResizeDeltaTime = (theResizeTimeRange.Y - theResizeTimeRange.X) / (theSourceTimeRange.Y - theSourceTimeRange.X);

	// Resize the curve time range
	if (theSourceTimeRange != theResizeTimeRange && theResizeTimeRange.X != INDEX_NONE && theResizeTimeRange.Y != INDEX_NONE)
	{
		// Each every keys
		for (int32 KeyIndex = 0; KeyIndex < theKeysAlpha.Num(); KeyIndex++)
		{
			// First key pivot axis
			theKeysTime[KeyIndex] *= theResizeDeltaTime;
			theSourceKeys[KeyIndex].Time *= theResizeDeltaTime;
		}
	}

	// Get the source keys value range
	const FVector2D theSourceValueRange(theFirstCurveKey.Value, theLastCurveKey.Value);
	const float theSourceDeltaValue = (theSourceValueRange.Y - theSourceValueRange.X) / theCurvePtr->Keys.Num();

	// Get the resize keys time range
	const FVector2D theResizeValueRange(FMath::Min(ValueRange.X, theSourceValueRange.X), FMath::Min(ValueRange.Y, theSourceValueRange.Y));
	const float theResizeDeltaValue = (theResizeValueRange.Y - theResizeValueRange.X) / theCurvePtr->Keys.Num();

	// Resize the curve time range
	if (theSourceValueRange != theResizeValueRange)
	{
		// Each every keys
		for (int32 KeyIndex = 0; KeyIndex < theKeysAlpha.Num(); KeyIndex++)
		{
			theCurvePtr->AddKey(theKeysTime[KeyIndex], theKeysAlpha[KeyIndex] * theLastCurveKey.Value);
		}
	}

	// Clear cached curve data
	theCurvePtr->Reset();

#if 0
	// Each every keys
	for (int32 KeyIndex = 0; KeyIndex < theKeysAlpha.Num(); KeyIndex++)
	{
		theCurvePtr->AddKey(theKeysTime[KeyIndex], theKeysAlpha[KeyIndex] * theLastCurveKey.Value);
	}
#else
	theCurvePtr->Keys = theSourceKeys;
	UVAT_Curve::ApplyCurveEulerFilter(FloatCurve);
#endif
}

void FVirtualResizeCurveData::Cleanup()
{
	// Define the state
	bool bHasValidData = false;

	// Each every keys
	for (FRichCurveKey& theKeys : FloatCurve.GetRichCurve()->Keys)
	{
		if (FMath::Abs(theKeys.Value) >= 1e-4f)
		{
			bHasValidData = true;
			break;
		}
	}

	// If is invalid data, we clear the curve
	if (!bHasValidData)
	{
		FloatCurve.GetRichCurve()->Reset();
	}
}

/*-----------------------------------------------------------------------------
	FVirtualBoneCurvesData implementation.
-----------------------------------------------------------------------------*/

FTransform FVirtualBoneCurvesData::Evaluate(const float& InKeyTime) const
{
	FVector theTranslationKey = FVector::ZeroVector;
	theTranslationKey.X = TranslationResizeCurvesData[0].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	theTranslationKey.Y = TranslationResizeCurvesData[1].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	theTranslationKey.Z = TranslationResizeCurvesData[2].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);

	FRotator theRotationKey = FRotator::ZeroRotator;
	theRotationKey.Roll = RotationResizeCurvesData[0].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	theRotationKey.Pitch = RotationResizeCurvesData[1].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	theRotationKey.Yaw = RotationResizeCurvesData[2].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);

	// Avoid orientation errors due to conversion
	if (FMath::IsNearlyEqual(FMath::Abs(theRotationKey.Yaw), 180.f, KINDA_SMALL_NUMBER))
	{
		theRotationKey.Yaw = (180.f - KINDA_SMALL_NUMBER) * (theRotationKey.Yaw >= 0.f ? 1.f : -1.f);
	}

	// Return result
	return FTransform(theRotationKey.Quaternion(), theTranslationKey);
}

void FVirtualBoneCurvesData::Evaluate(const float& InKeyTime, FRotator& OutRotation, FVector& OutTranslation) const
{
	OutTranslation.X = TranslationResizeCurvesData[0].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	OutTranslation.Y = TranslationResizeCurvesData[1].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	OutTranslation.Z = TranslationResizeCurvesData[2].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);

	OutRotation.Roll = RotationResizeCurvesData[0].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	OutRotation.Pitch = RotationResizeCurvesData[1].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);
	OutRotation.Yaw = RotationResizeCurvesData[2].FloatCurve.GetRichCurveConst()->Eval(InKeyTime);

	// Avoid orientation errors due to conversion
	if (FMath::IsNearlyEqual(FMath::Abs(OutRotation.Yaw), 180.f, KINDA_SMALL_NUMBER))
	{
		OutRotation.Yaw = (180.f - KINDA_SMALL_NUMBER) * (OutRotation.Yaw >= 0.f ? 1.f : -1.f);
	}
}

void FVirtualBoneCurvesData::AddTransformKey(const float& InKeyTime, const FTransform& InTransform)
{
	TranslationResizeCurvesData[0].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, InTransform.GetTranslation().X);
	TranslationResizeCurvesData[1].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, InTransform.GetTranslation().Y);
	TranslationResizeCurvesData[2].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, InTransform.GetTranslation().Z);

	FRotator theRotation = InTransform.Rotator();

	// Avoid orientation errors due to conversion
	if (FMath::IsNearlyEqual(FMath::Abs(theRotation.Yaw), 180.f, KINDA_SMALL_NUMBER))
	{
		theRotation.Yaw = (180.f - KINDA_SMALL_NUMBER) * (theRotation.Yaw >= 0.f ? 1.f : -1.f);
	}

	RotationResizeCurvesData[0].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, theRotation.Roll);
	RotationResizeCurvesData[1].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, theRotation.Pitch);
	RotationResizeCurvesData[2].FloatCurve.GetRichCurve()->UpdateOrAddKey(InKeyTime, theRotation.Yaw);
}

bool FVirtualBoneCurvesData::HasCurveData()
{
	bool bHasCurveData = false;
	bHasCurveData |= RotationResizeCurvesData[0].FloatCurve.GetRichCurve()->HasAnyData();
	bHasCurveData |= RotationResizeCurvesData[1].FloatCurve.GetRichCurve()->HasAnyData();
	bHasCurveData |= RotationResizeCurvesData[2].FloatCurve.GetRichCurve()->HasAnyData();
	bHasCurveData |= TranslationResizeCurvesData[0].FloatCurve.GetRichCurve()->HasAnyData();
	bHasCurveData |= TranslationResizeCurvesData[1].FloatCurve.GetRichCurve()->HasAnyData();
	bHasCurveData |= TranslationResizeCurvesData[2].FloatCurve.GetRichCurve()->HasAnyData();
	return bHasCurveData;
}

void FVirtualBoneCurvesData::Resize(FVector2D* InTimeRange, FVector2D* InValueRange)
{
	RotationResizeCurvesData[0].Resize(InTimeRange, InValueRange);
	RotationResizeCurvesData[1].Resize(InTimeRange, InValueRange);
	RotationResizeCurvesData[2].Resize(InTimeRange, InValueRange);
	TranslationResizeCurvesData[0].Resize(InTimeRange, InValueRange);
	TranslationResizeCurvesData[1].Resize(InTimeRange, InValueRange);
	TranslationResizeCurvesData[2].Resize(InTimeRange, InValueRange);
}

void FVirtualBoneCurvesData::Filter(const EVirtualCurveFilterType& InFilterType, const float InTolerance)
{
	UVAT_Curve::ApplyCurveFilter(RotationResizeCurvesData[0].FloatCurve, InFilterType, InTolerance);
	UVAT_Curve::ApplyCurveFilter(RotationResizeCurvesData[1].FloatCurve, InFilterType, InTolerance);
	UVAT_Curve::ApplyCurveFilter(RotationResizeCurvesData[2].FloatCurve, InFilterType, InTolerance);
	UVAT_Curve::ApplyCurveFilter(TranslationResizeCurvesData[0].FloatCurve, InFilterType, InTolerance);
	UVAT_Curve::ApplyCurveFilter(TranslationResizeCurvesData[1].FloatCurve, InFilterType, InTolerance);
	UVAT_Curve::ApplyCurveFilter(TranslationResizeCurvesData[2].FloatCurve, InFilterType, InTolerance);
}

void FVirtualBoneCurvesData::Cleanup()
{
	RotationResizeCurvesData[0].Cleanup();
	RotationResizeCurvesData[1].Cleanup();
	RotationResizeCurvesData[2].Cleanup();
	TranslationResizeCurvesData[0].Cleanup();
	TranslationResizeCurvesData[1].Cleanup();
	TranslationResizeCurvesData[2].Cleanup();
}

void FVirtualBoneCurvesData::Reset()
{
	for (FVirtualResizeCurveData& theResizeCurveData : RotationResizeCurvesData)
	{
		theResizeCurveData.FloatCurve.GetRichCurve()->Reset();
	}
	for (FVirtualResizeCurveData& theResizeCurveData : TranslationResizeCurvesData)
	{
		theResizeCurveData.FloatCurve.GetRichCurve()->Reset();
	}
}

/*-----------------------------------------------------------------------------
	FVirtualLegPoseRuntimeData implementation.
-----------------------------------------------------------------------------*/

int32 FVirtualLegPoseRuntimeData::Initialize(UAnimSequence* InAnimSequence, const float& InSampleRate
	, const FVirtualLegBaseData& InLegBaseData, bool InUseTipRelativeTM, FVirtualCurveSampleData* InCurveSampleData)
{
	int32 thePosesNumber = 0;

	// Check the leg bone is valid
	if (!InLegBaseData.IsValid())
	{
		return thePosesNumber;
	}

	// Get the animation sequence data
	const float& theSequenceLength = InAnimSequence->GetPlayLength();

	// Cache the idle pose transform data
	if (InCurveSampleData != nullptr && InCurveSampleData->IdlePoseFrame != INDEX_NONE)
	{
		// Get the foot bone component space transform
		TipIdlePoseTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, InCurveSampleData->IdlePoseFrame, InLegBaseData.TipBone.BoneName);

		// Calculate the relative transform data
		if (InUseTipRelativeTM)
		{
			TipIdlePoseTransform = FTransform(InLegBaseData.TipSocketLocation) * TipIdlePoseTransform.GetValue();
		}

		// Get the foot tip bone component space transform
		HeelIdlePoseTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, InCurveSampleData->IdlePoseFrame, InLegBaseData.FootBone.BoneName);
		HeelIdlePoseTransform = FTransform(InLegBaseData.HeelSocketLocation) * HeelIdlePoseTransform.GetValue();
	}

	// Clear cached reference output curves data
	if (InCurveSampleData != nullptr)
	{
		{
			FRuntimeFloatCurve& theBoneHeightCurve = InCurveSampleData->HeelBoneHeightsCurvesMap.FindOrAdd(InLegBaseData.FootBone.BoneName);
			theBoneHeightCurve.GetRichCurve()->Reset();
		}
		{
			FRuntimeFloatCurve& theBoneHeightCurve = InCurveSampleData->TipBoneHeightsCurvesMap.FindOrAdd(InLegBaseData.TipBone.BoneName);
			theBoneHeightCurve.GetRichCurve()->Reset();
		}
	}

	// Sample the pose features
	float thePoseSampleTime = 0.f;
	while (thePoseSampleTime <= (theSequenceLength + 0.0001f))
	{
		// Clamp the pose time
		thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theSequenceLength);

		// Get the foot bone component space transform
		FTransform theFootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, thePoseSampleTime, InLegBaseData.FootBone.BoneName);

		// Get the foot tip bone component space transform
		FTransform theTipBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, thePoseSampleTime, InLegBaseData.TipBone.BoneName);

		// Calculate the relative transform data
		if (InUseTipRelativeTM)
		{
			FTransform theTipSocketTransformLS = FTransform::Identity;
			theTipSocketTransformLS.SetLocation(InLegBaseData.TipSocketLocation);
			theTipBoneTransformCS = theTipSocketTransformLS * theTipBoneTransformCS;
		}
		
		TipLowerHeights.Add(theTipBoneTransformCS.GetLocation().Z);
		TipTransformCS.Add(theTipBoneTransformCS);

		// Calculate the tip bone lower height
		if (TipLowerHeightPoseIndex == INDEX_NONE || TipLowerHeight > theTipBoneTransformCS.GetLocation().Z)
		{
			TipLowerHeight = theTipBoneTransformCS.GetLocation().Z;
			TipLowerHeightPoseTime = thePoseSampleTime;
			TipLowerHeightPoseIndex = thePosesNumber;
		}

		// Get the foot heel bone component space transform
		FTransform theHeelSocketTransformLS = FTransform::Identity;
		theHeelSocketTransformLS.SetLocation(InLegBaseData.HeelSocketLocation);
		FTransform theHeelBoneTransformCS = theHeelSocketTransformLS * theFootBoneTransformCS;
		HeelLowerHeights.Add(theHeelBoneTransformCS.GetLocation().Z);
		HeelTransformCS.Add(theHeelBoneTransformCS);

		// Calculate the heel bone lower height
		if (HeelLowerHeightPoseIndex == INDEX_NONE || HeelLowerHeight > theHeelBoneTransformCS.GetLocation().Z)
		{
			HeelLowerHeight = theHeelBoneTransformCS.GetLocation().Z;
			HeelLowerHeightPoseTime = thePoseSampleTime;
			HeelLowerHeightPoseIndex = thePosesNumber;
		}

		// Output the bone reference value
		if (InCurveSampleData != nullptr)
		{
			{
				FRuntimeFloatCurve& theBoneHeightCurve = InCurveSampleData->HeelBoneHeightsCurvesMap.FindOrAdd(InLegBaseData.FootBone.BoneName);
				theBoneHeightCurve.GetRichCurve()->AddKey(thePoseSampleTime, HeelLowerHeights.Last());
			}
			{
				FRuntimeFloatCurve& theBoneHeightCurve = InCurveSampleData->TipBoneHeightsCurvesMap.FindOrAdd(InLegBaseData.TipBone.BoneName);
				theBoneHeightCurve.GetRichCurve()->AddKey(thePoseSampleTime, TipLowerHeights.Last());
			}
		}

		// Iterate
		++thePosesNumber;
		thePoseSampleTime += InSampleRate;
	}

	return thePosesNumber;
}

float FVirtualLegPoseRuntimeData::GetMinPoseHeight(const int32& InPoseIndex)
{
	return FMath::Min(TipLowerHeights[InPoseIndex], HeelLowerHeights[InPoseIndex]);
}

float FVirtualLegPoseRuntimeData::GetMaxPoseHeight(const int32& InPoseIndex)
{
	return FMath::Max(TipLowerHeights[InPoseIndex], HeelLowerHeights[InPoseIndex]);
}

float FVirtualLegPoseRuntimeData::GetMinPoseValue(const int32& InPoseIndex)
{
	return FMath::Min(FVector::Dist(TipTransformCS[InPoseIndex].GetLocation(), TipIdlePoseTransform->GetLocation())
		, FVector::Dist(HeelTransformCS[InPoseIndex].GetLocation(), HeelIdlePoseTransform->GetLocation()));
}

float FVirtualLegPoseRuntimeData::GetMaxPoseValue(const int32& InPoseIndex)
{
	return FMath::Max(FVector::Dist(TipTransformCS[InPoseIndex].GetLocation(), TipIdlePoseTransform->GetLocation())
		, FVector::Dist(HeelTransformCS[InPoseIndex].GetLocation(), HeelIdlePoseTransform->GetLocation()));
}

bool FVirtualLegPoseRuntimeData::IsLandedPose(const int32& InPoseIndex, const float InTolerance
	, const float InGroundedHeight, const float InHorizontalTolerance, const bool InCalculateMinVale)
{
	// Check the cached data is valid
	if (!TipTransformCS.IsValidIndex(InPoseIndex) || !HeelTransformCS.IsValidIndex(InPoseIndex))
	{
		return false;
	}

	// Calculate the min difference value
	const float theTipDiff = FMath::Min(TipLowerHeights[InPoseIndex] - TipLowerHeight, TipLowerHeights[InPoseIndex] - InGroundedHeight);
	const float theHeelDiff = FMath::Min(HeelLowerHeights[InPoseIndex] - HeelLowerHeight, HeelLowerHeights[InPoseIndex] - InGroundedHeight);
	const float& theMinDiff = InCalculateMinVale ? FMath::Min(theTipDiff, theHeelDiff) : FMath::Max(theTipDiff, theHeelDiff);

	// Check the horizontal offset
	if (HeelIdlePoseTransform.IsSet() && TipIdlePoseTransform.IsSet())
	{
		const float theTipOffset = FVector::Dist(TipTransformCS[InPoseIndex].GetLocation(), TipIdlePoseTransform->GetLocation());
		const float theHeelOffset = FVector::Dist(HeelTransformCS[InPoseIndex].GetLocation(), HeelIdlePoseTransform->GetLocation());
		const float theMaxOffset = FMath::Max(theTipOffset, theHeelOffset);
		return theMaxOffset <= InHorizontalTolerance;
	}

	// If the lowest difference altitude is greater than the leave ground tolerance, the current pose should be in air
	return theMinDiff <= InTolerance;
}

/*-----------------------------------------------------------------------------
	FVirtualLegBaseData implementation.
-----------------------------------------------------------------------------*/

bool FVirtualLegBaseData::IsValid() const
{
	return FootBone.BoneName != NAME_None;
}
