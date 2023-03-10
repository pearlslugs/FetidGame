// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Library/VAT_Curve.h"
#include "Library/VAT_Asset.h"
#include "Library/VAT_Library.h"
#include "Library/VAT_PoseSearch.h"

#include "Math/Axis.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"

#if ENGINE_MAJOR_VERSION > 4
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimationDataController/Public/AnimDataController.h"
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#include "Curves/RichCurve.h"

#define LOCTEXT_NAMESPACE "VAT_Curve"

static bool RetrieveSmartNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName, const FName& InContainerName, FSmartName& OutSmartName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));
	return InAnimSequence->GetSkeleton()->GetSmartNameByName(InContainerName, InCurveName, OutSmartName);
}

static FSmartName RetrieveSmartNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName, const FName& InContainerName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));
	FSmartName SmartCurveName;
	InAnimSequence->GetSkeleton()->GetSmartNameByName(InContainerName, InCurveName, SmartCurveName);
	return SmartCurveName;
}

static FName RetrieveContainerNameForCurve(const UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));

	{
		const FSmartNameMapping* theCurveMapping = InAnimSequence->GetSkeleton()->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
		if (theCurveMapping && theCurveMapping->Exists(InCurveName))
		{
			return USkeleton::AnimCurveMappingName;
		}
	}

	{
		const FSmartNameMapping* theCurveMapping = InAnimSequence->GetSkeleton()->GetSmartNameContainer(USkeleton::AnimTrackCurveMappingName);
		if (theCurveMapping && theCurveMapping->Exists(InCurveName))
		{
			return USkeleton::AnimTrackCurveMappingName;
		}
	}

	return NAME_None;
}

int32 UVAT_Curve::GetFrameAtTime(UAnimSequence* InAnimSequence, const float Time)
{
	check(InAnimSequence);
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfFrames(InAnimSequence);
	const float& theAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FMath::RoundToInt(Time / FrameTime), 0, theNumberOfKeys - 1);
}

float UVAT_Curve::GetTimeAtFrame(UAnimSequence* InAnimSequence, const int32 Frame)
{
	check(InAnimSequence);
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfFrames(InAnimSequence);
	const float& theAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FrameTime * Frame, 0.0f, theAnimLength);
}

FSmartName UVAT_Curve::GetCurveSmartName(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	check(InAnimSequence);

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		return FSmartName();
	}

	// Retrieve smart name for curve
	return RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);
}

template <typename DataType, typename CurveClass>
CurveClass* UVAT_Curve::GetCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName, const ERawCurveTrackTypes& InCurveType)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		return nullptr;
	}

	// Retrieve smart name for curve
	const FSmartName theCurveSmartName = RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);

#if ENGINE_MAJOR_VERSION > 4
	if (UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel())
	{
		for (FFloatCurve& theFloatCurve : theAnimDataModel->GetNonConstCurveData().FloatCurves)
		{
			if (theFloatCurve.Name.UID == theCurveSmartName.UID)
			{
				return &theFloatCurve;
			}
		}
	}
	return nullptr;
#else
	// Retrieve the curve by name
	return static_cast<CurveClass*>(InAnimSequence->RawCurveData.GetCurveData(theCurveSmartName.UID, InCurveType));
#endif
}

template <typename DataType, typename CurveClass>
FFloatCurve* UVAT_Curve::GetFloatCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	// Check the curve class reference is valid
	CurveClass* theCurvePtr = GetCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Float);
	if (theCurvePtr == nullptr)
	{
		return nullptr;
	}

	// Convert to float curve reference
	return static_cast<FFloatCurve*>(theCurvePtr);
}

/*-----------------------------------------------------------------------------
	FVirtualCurveAttributes implementation.
-----------------------------------------------------------------------------*/

void FVirtualCurveAttributes::RebuildCurveFilterMap(const FVirtualCurvePresetData& InPresetData)
{
	if (InPresetData.CurvesFilterType.Num() > 0)
	{
		if (InPresetData.CurvesFilterType.Num() == 1)
		{
			for (TPair<EVirtualCurveType, EVirtualCurveFilterType>& theCurvePair : AnimCurvesFilterMap)
			{
				theCurvePair.Value = InPresetData.CurvesFilterType[0];
			}
		}
		else
		{
			for (int32 Index = 0; Index < InPresetData.CurvesType.Num(); Index++)
			{
				if (InPresetData.CurvesFilterType.IsValidIndex(Index))
				{
					if (EVirtualCurveFilterType* theFilterTypePtr = AnimCurvesFilterMap.Find(InPresetData.CurvesType[Index]))
					{
						*theFilterTypePtr = InPresetData.CurvesFilterType[Index];
					}
				}
			}
		}
	}
}

#pragma region Curve
bool UVAT_Curve::IsAlphaType(const EVirtualCurveType& InVirtualCurveType)
{
	return
		InVirtualCurveType == EVirtualCurveType::SpeedAlpha
		||
		InVirtualCurveType == EVirtualCurveType::ForwardMovementAlpha || InVirtualCurveType == EVirtualCurveType::RightMovementAlpha || InVirtualCurveType == EVirtualCurveType::UpMovementAlpha
		||
		InVirtualCurveType == EVirtualCurveType::PitchRotationAlpha || InVirtualCurveType == EVirtualCurveType::YawRotationAlpha || InVirtualCurveType == EVirtualCurveType::RollRotationAlpha
		||
		InVirtualCurveType == EVirtualCurveType::TranslationAlpha || InVirtualCurveType == EVirtualCurveType::RotationAlpha;
}

float UVAT_Curve::GetCurveValueFromType(const EVirtualCurveType& InCurveType, const FTransform& InRootMotionTransform, float InDeltaTime)
{
	float CurveValue = 0.f;

	switch (InCurveType)
	{
	case EVirtualCurveType::Speed:
		CurveValue = InDeltaTime == 0.f ? 0.f : (InRootMotionTransform.GetTranslation() / InDeltaTime).Size();
		break;

	case EVirtualCurveType::SpeedAlpha:
		CurveValue = InDeltaTime == 0.f ? 0.f : (InRootMotionTransform.GetTranslation() / InDeltaTime).Size();
		break;

	case EVirtualCurveType::ForwardMovement:
		CurveValue = InRootMotionTransform.GetLocation().Y;
		break;

	case EVirtualCurveType::ForwardMovementAlpha:
		CurveValue = InRootMotionTransform.GetLocation().Y;
		break;

	case EVirtualCurveType::RightMovement:
		CurveValue = InRootMotionTransform.GetLocation().X;
		break;

	case EVirtualCurveType::RightMovementAlpha:
		CurveValue = InRootMotionTransform.GetLocation().X;
		break;

	case EVirtualCurveType::UpMovement:
		CurveValue = InRootMotionTransform.GetLocation().Z;
		break;

	case EVirtualCurveType::UpMovementAlpha:
		CurveValue = InRootMotionTransform.GetLocation().Z;
		break;

	case EVirtualCurveType::TranslationAlpha:
		CurveValue = InRootMotionTransform.GetLocation().Size();
		break;

	case EVirtualCurveType::PitchRotation:
		CurveValue = InRootMotionTransform.Rotator().Pitch;
		break;

	case EVirtualCurveType::PitchRotationAlpha:
		CurveValue = InRootMotionTransform.Rotator().Pitch;
		break;

	case EVirtualCurveType::YawRotation:
		CurveValue = InRootMotionTransform.Rotator().Yaw;
		break;

	case EVirtualCurveType::YawRotationAlpha:
		CurveValue = InRootMotionTransform.Rotator().Yaw;
		break;

	case EVirtualCurveType::RollRotation:
		CurveValue = InRootMotionTransform.Rotator().Roll;
		break;

	case EVirtualCurveType::RollRotationAlpha:
		CurveValue = InRootMotionTransform.Rotator().Roll;
		break;

	case EVirtualCurveType::RotationAlpha:
		CurveValue = InRootMotionTransform.Rotator().Yaw;
		break;
	}

	return FMath::Abs(CurveValue) >= KINDA_SMALL_NUMBER ? CurveValue : 0.f;
}

void UVAT_Curve::GetAnimationCurvesType(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, TArray<EVirtualCurveType>& OutCurvesType)
{
	for (const TPair<EVirtualCurveType, FName>& CurvePair : InCurveAttributes.AnimCurvesNameMap)
	{
		const FName& theCurveName = CurvePair.Value;
		const EVirtualCurveType& theCurveType = CurvePair.Key;
#if ENGINE_MAJOR_VERSION > 4
		/*FAnimationCurveIdentifier& MyCurveIdentifier = InCurveAttributes.TMap_AnimCurveIdentifier.FindOrAdd(CurveType);
		MyCurveIdentifier = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(theSkeleton, CurveName, ERawCurveTrackTypes::RCT_Float);
		if (MyDataModel->FindCurve(MyCurveIdentifier))
		{

		}*/
#else
		if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, theCurveName, ERawCurveTrackTypes::RCT_Float))
		{
			OutCurvesType.Add(theCurveType);
		}
#endif
	}
}
#pragma endregion


#pragma region Sample Curves
void UVAT_Curve::SampleBoneCurve(UAnimSequence* InAnimSequence, const FVirtualBoneCurveData& InBoneCurveData, const FVirtualCurveAttributes& InCurveAttributes)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Add transform curve key 
	UAnimationBlueprintLibrary::AddTransformationCurveKey(InAnimSequence, InBoneCurveData.OutputName, 0.f, FTransform::Identity);

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::SampleAnimationAssetCurves(UAnimSequence* InAnimSequence, const FName& InCurveName, TArray<FRuntimeFloatCurve>& OutAnimCurves)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* MyDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& MyDataController = InAnimSequence->GetController();
	MyDataController.OpenBracket(LOCTEXT("AddFloatCurve_Description", "Adding float curve."), false);
#endif

	// Clear previous curves data
	OutAnimCurves.Reset();

	// Search float curve
	if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Float))
	{
		TArray<float> theCurveTimes, theCurveValues;
		UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, InCurveName, theCurveTimes, theCurveValues);

		// Output the curve
		if (theCurveTimes.Num() > 0)
		{
			FRuntimeFloatCurve& theFloatCurve = OutAnimCurves.AddDefaulted_GetRef();
			for (int32 KeyIndex = 0; KeyIndex < theCurveTimes.Num(); KeyIndex++)
			{
				theFloatCurve.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex]);
			}
		}
	}
	else if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Vector))
	{
		TArray<float> theCurveTimes;
		TArray<FVector> theCurveValues;
		UAnimationBlueprintLibrary::GetVectorKeys(InAnimSequence, InCurveName, theCurveTimes, theCurveValues);

		// Output the curve
		if (theCurveTimes.Num() > 0)
		{
			FRuntimeFloatCurve& theFloatCurveX = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theFloatCurveY = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theFloatCurveZ = OutAnimCurves.AddDefaulted_GetRef();
			for (int32 KeyIndex = 0; KeyIndex < theCurveTimes.Num(); KeyIndex++)
			{
				theFloatCurveX.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].X);
				theFloatCurveY.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].Y);
				theFloatCurveZ.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].Z);
			}
		}
	}
	else if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Transform))
	{
		TArray<float> theCurveTimes;
		TArray<FTransform> theCurveValues;
		UAnimationBlueprintLibrary::GetTransformationKeys(InAnimSequence, InCurveName, theCurveTimes, theCurveValues);

		// Output the curve
		if (theCurveTimes.Num() > 0)
		{
			FRuntimeFloatCurve& theTranslationX = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theTranslationY = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theTranslationZ = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theRotationX = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theRotationY = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theRotationZ = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theScaleX = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theScaleY = OutAnimCurves.AddDefaulted_GetRef();
			FRuntimeFloatCurve& theScaleZ = OutAnimCurves.AddDefaulted_GetRef();
			for (int32 KeyIndex = 0; KeyIndex < theCurveTimes.Num(); KeyIndex++)
			{
				theTranslationX.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetTranslation().X);
				theTranslationY.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetTranslation().Y);
				theTranslationZ.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetTranslation().Z);
				theRotationX.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].Rotator().Roll);
				theRotationY.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].Rotator().Pitch);
				theRotationZ.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].Rotator().Yaw);
				theScaleX.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetScale3D().X);
				theScaleY.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetScale3D().Y);
				theScaleZ.GetRichCurve()->AddKey(theCurveTimes[KeyIndex], theCurveValues[KeyIndex].GetScale3D().Z);
			}
		}
	}
}

void UVAT_Curve::SampleRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, TMap<EVirtualCurveType, FRuntimeFloatCurve>& OutAnimCurvesMap)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the skeleton root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Define the local variables
	TMap<EVirtualCurveType, float> theAccumulateCurvesMap;
	TMap<EVirtualCurveType, FRuntimeFloatCurve> theAnimCurvesMap;

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* MyDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& MyDataController = InAnimSequence->GetController();
	MyDataController.OpenBracket(LOCTEXT("AddFloatCurve_Description", "Adding float curve."), false);
#endif

	// Clear previous curves data
	for (TPair<EVirtualCurveType, FRuntimeFloatCurve>& theCurvePair : OutAnimCurvesMap)
	{
		theCurvePair.Value.GetRichCurve()->Reset();
	}

	// Get the animation asset data
	const float& theAnimLength = InAnimSequence->GetPlayLength();
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Calculate the motion apex transform data
	float theMaxSpeed = 0.f;
	const FTransform theMotionApexTransform = UVAT_Asset::GetRootMotionApexTransform(InAnimSequence, theMaxSpeed);
	const FTransform theMotionFirstTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, 0.f, theRootBoneName);
	const FTransform theMotionLastTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, theAnimLength, theRootBoneName);

	// Each every keys
	for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Get the last key index
		const int32 theLastKeyIndex = KeyIndex - 1;

		// Evaluate the keys time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);
		const float theLastKeyTime = InAnimSequence->GetTimeAtFrame(theLastKeyIndex);
		const float theDeltaTime = theKeyTime - theLastKeyTime;

		// Calculate the root motion transform data
		FTransform thePreviousMotionTransform, theCurrentMotionTransform;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theRootBoneName, KeyIndex, true, theCurrentMotionTransform);
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theRootBoneName, theLastKeyIndex, true, thePreviousMotionTransform);
		const FTransform theDeltaMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentMotionTransform, thePreviousMotionTransform);

		// Add curve value
		for (TPair<EVirtualCurveType, FRuntimeFloatCurve>& theCurvePair : OutAnimCurvesMap)
		{
			// Get the curve type
			const EVirtualCurveType& theCurveType = theCurvePair.Key;
			FRuntimeFloatCurve& theFloatCurve = theCurvePair.Value;

			// Check the curve name is valid
			const FName* theCurveName = InCurveAttributes.AnimCurvesNameMap.Find(theCurveType);
			if (theCurveName == nullptr)
			{
				continue;
			}

			// Get the alpha curve condition
			const bool bIsAlphaCurve = IsAlphaType(theCurveType);

			// Calculate the curve value
			float theCurveValue = 0.f;
			switch (theCurveType)
			{
			case EVirtualCurveType::Speed:
				theCurveValue = (theDeltaMotionTransform.GetTranslation() / theDeltaTime).Size();
				break;

			case EVirtualCurveType::SpeedAlpha:
				theCurveValue = (theDeltaMotionTransform.GetTranslation() / theDeltaTime).Size();
				theCurveValue = theMaxSpeed == 0.f ? 0.f : theCurveValue / theMaxSpeed;
				break;

			case EVirtualCurveType::AnimPosition:
				theCurveValue = theKeyTime;
				break;

			default:
				theCurveValue = GetCurveValueFromType(theCurveType, theDeltaMotionTransform, theDeltaTime);
				if (bIsAlphaCurve || InCurveAttributes.bSampleAccumulateCurve)
				{
#if 0
					// Accumulate the curve value
					float& theAccumulateValue = theAccumulateCurvesMap.FindOrAdd(theCurveType);
					theAccumulateValue += theCurveValue;
#else
					float theAccumulateValue = GetCurveValueFromType(theCurveType, theCurrentMotionTransform, theDeltaTime);

					// Compare the last curve, keep line track
					if (InCurveAttributes.bSampleAccumulateLineCurve && theCurveType >= EVirtualCurveType::PitchRotation)
					{
						if (theAccumulateValue != 0.f && theFloatCurve.GetRichCurve()->Keys.Num() > 0)
						{
							const FRichCurveKey theLastCurveKey = theFloatCurve.GetRichCurve()->GetLastKey();
							if (theLastCurveKey.Value != 0.f)
							{
								const float theLastAxis = theLastCurveKey.Value / FMath::Abs(theLastCurveKey.Value);
								const float theCurrentAxis = theAccumulateValue / FMath::Abs(theAccumulateValue);
								if (theLastAxis != theCurrentAxis)
								{
									const float theLastAccumulateValue = GetCurveValueFromType(theCurveType, thePreviousMotionTransform, theDeltaTime);
									if (theCurrentAxis > 0.f)
									{
										theAccumulateValue = theLastCurveKey.Value - (FMath::Abs(theLastAccumulateValue) - theAccumulateValue);
									}
									else
									{
										theAccumulateValue = theLastCurveKey.Value + (FMath::Abs(theLastAccumulateValue) + theAccumulateValue);
									}
								}
							}
						}
					}
#endif 
					// Calculate the curve apex value
					float theApexValue = GetCurveValueFromType(theCurveType, theMotionApexTransform);

					// Calculate the curve final value
					if (bIsAlphaCurve)
					{
						// Check the abs alpha curve value
						switch (InCurveAttributes.CurveAbsType)
						{
						case EVirtualCurveAbsoluteType::Absolute:
							theCurveValue = theApexValue == 0.f ? 0.f : FMath::Abs(theAccumulateValue / theApexValue);
							break;

						case EVirtualCurveAbsoluteType::AbsoluteInFirst:
						{
							theApexValue = GetCurveValueFromType(theCurveType, theMotionFirstTransform);
							theCurveValue = theApexValue == 0.f ? 0.f : theAccumulateValue / theApexValue;
						}
						break;

						case EVirtualCurveAbsoluteType::AbsoluteInLast:
						{
							theApexValue = GetCurveValueFromType(theCurveType, theMotionLastTransform);
							theCurveValue = theApexValue == 0.f ? 0.f : theAccumulateValue / theApexValue;
						}
						break;

						default:
							theCurveValue = theApexValue == 0.f ? 0.f : theAccumulateValue / theApexValue;
							break;
						}
					}
					else
					{
						theCurveValue = theAccumulateValue;
					}
				}
				break;
			}

			// Smaller values ​​are ignored
			if (FMath::IsNearlyEqual(theCurveValue, 0.f, KINDA_SMALL_NUMBER))
			{
				theCurveValue = 0.f;
			}

			// Keep the final decimal place to what we expect,
			if (InCurveAttributes.DecimalPlaces >= 0)
			{
				if (InCurveAttributes.DecimalPlaces == 0)
				{
					theCurveValue = (int)FMath::RoundToFloat(theCurveValue);
				}
				else
				{
					const float theDecimalPercentage = FMath::Pow(0.1, InCurveAttributes.DecimalPlaces);
					theCurveValue = (int)FMath::RoundToFloat(theCurveValue * (1.f / theDecimalPercentage)) * theDecimalPercentage;
				}
			}

#if 0 // Please use curve filter
			// Get the last key data
			const int32 theKeyNumber = theFloatCurve.GetRichCurve()->Keys.Num();
			if (theKeyNumber > 2)
			{
				const FRichCurveKey theLastKeyTimes[3]{ theFloatCurve.GetRichCurve()->Keys[theKeyNumber - 3]
				,theFloatCurve.GetRichCurve()->Keys[theKeyNumber - 2]
				, theFloatCurve.GetRichCurve()->Keys[theKeyNumber - 1] };
				if (theLastKeyTimes[0].Value == theLastKeyTimes[1].Value && theLastKeyTimes[2].Value == theLastKeyTimes[1].Value)
				{
					// Delete the float curve
					theFloatCurve.GetRichCurve()->DeleteKey(theFloatCurve.GetRichCurve()->FindKey(theLastKeyTimes[1].Time));
				}
				else if (theLastKeyTimes[1].Value == theCurveValue && theLastKeyTimes[2].Value == theLastKeyTimes[1].Value)
				{
					// Delete the float curve
					theFloatCurve.GetRichCurve()->DeleteKey(theFloatCurve.GetRichCurve()->FindKey(theLastKeyTimes[2].Time));
				}
			}
			else if (theKeyNumber == 2)
			{
				const FRichCurveKey theLastKeyTimes[2]{ theFloatCurve.GetRichCurve()->Keys[theKeyNumber - 2], theFloatCurve.GetRichCurve()->Keys[theKeyNumber - 1] };
				if (theLastKeyTimes[0].Value == theCurveValue && theLastKeyTimes[1].Value == theCurveValue)
				{
					// Delete the float curve
					theFloatCurve.GetRichCurve()->DeleteKey(theFloatCurve.GetRichCurve()->FindKey(theLastKeyTimes[0].Time));
				}
			}
#endif
			// Add the float curve
			theFloatCurve.GetRichCurve()->AddKey(theKeyTime, theCurveValue);
		}
	}

	// Check all curves for valid data
	for (TPair<EVirtualCurveType, FRuntimeFloatCurve>& theCurvePair : OutAnimCurvesMap)
	{
		// Get the curve data
		const EVirtualCurveType& theCurveType = theCurvePair.Key;
		FRuntimeFloatCurve& theFloatCurve = theCurvePair.Value;

		// Check the curve name is valid
		const FName* theCurveName = InCurveAttributes.AnimCurvesNameMap.Find(theCurveType);
		if (theCurveName == nullptr)
		{
			continue;
		}

		// Check the curve data is valid
		if (!theFloatCurve.GetRichCurve()->HasAnyData())
		{
			continue;
		}

		// Add the first key, usually 0, if it is a loop pose, it will get the data of the last frame
		if ((!InCurveAttributes.bSampleAccumulateCurve && theCurveType != EVirtualCurveType::AnimPosition)
			|| theCurveType == EVirtualCurveType::Speed || theCurveType == EVirtualCurveType::SpeedAlpha)
		{
			if (InCurveAttributes.bIsLoopMotionAsset && UVAT_PoseSearch::IsLoopAnimationAsset(InAnimSequence, InCurveAttributes.LoopMotionAssetTolerance))
			{
				theFloatCurve.GetRichCurve()->UpdateOrAddKey(0.f, theFloatCurve.GetRichCurve()->GetLastKey().Value);
			}
			else
			{
				theFloatCurve.GetRichCurve()->UpdateOrAddKey(0.f, theFloatCurve.GetRichCurve()->Keys[0].Value);
			}
		}
		else
		{
			//theFloatCurve.GetRichCurve()->UpdateOrAddKey(0.f, 0.f);
		}

#if 0 // Please use curve filter
		// Get the last key data
		const int32 theKeyNumber = theFloatCurve.GetRichCurve()->Keys.Num();
		if (theKeyNumber > 2)
		{
			const FRichCurveKey theLastKeyTimes[3]{ theFloatCurve.GetRichCurve()->Keys[0], theFloatCurve.GetRichCurve()->Keys[1], theFloatCurve.GetRichCurve()->Keys[2] };
			if (theLastKeyTimes[0].Value == theLastKeyTimes[1].Value && theLastKeyTimes[2].Value == theLastKeyTimes[1].Value)
			{
				// Delete the float curve
				theFloatCurve.GetRichCurve()->DeleteKey(theFloatCurve.GetRichCurve()->FindKey(theLastKeyTimes[1].Time));
			}
		}
#endif

		// Apply the curve filter
		if (const EVirtualCurveFilterType* theFilterTypePtr = InCurveAttributes.AnimCurvesFilterMap.Find(theCurveType))
		{
			UVAT_Curve::ApplyCurveFilter(theFloatCurve, *theFilterTypePtr, InCurveAttributes.FilterTolerance);
		}
	}
}

void UVAT_Curve::SampleRootMotionCurvesToAnimSequence(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, const TMap<EVirtualCurveType, FRuntimeFloatCurve>& InAnimCurvesMap)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theAnimDataController = InAnimSequence->GetController();
	theAnimDataController.OpenBracket(LOCTEXT("AddFloatCurve_Description", "Sample RootMotionCurves To AnimSequence."), false);
#endif

	// Check all curves for valid data
	for (const TPair<EVirtualCurveType, FRuntimeFloatCurve>& theCurvePair : InAnimCurvesMap)
	{
		// Get the curve data
		const EVirtualCurveType& theCurveType = theCurvePair.Key;
		const FRuntimeFloatCurve& theFloatCurve = theCurvePair.Value;

		// Check the curve name is valid
		const FName* theCurveName = InCurveAttributes.AnimCurvesNameMap.Find(theCurveType);
		if (theCurveName == nullptr)
		{
			continue;
		}

		// Get the curve attributes data
		const FColor* theCurveColor = InCurveAttributes.AnimCurvesColorMap.Find(theCurveType);

		// Check the curve data is valid
		if (!theFloatCurve.GetRichCurveConst()->HasAnyData())
		{
			// Remove the animation asset curve
			if (InCurveAttributes.bAlwaysClearCurves)
			{
				UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, *theCurveName, false);
			}
			continue;
		}

		// Check the curve data is valid
		bool bHasValidData = false;
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			if (FMath::Abs(theKey.Value) > KINDA_SMALL_NUMBER)
			{
				bHasValidData = true;
				break;
			}
		}

		// If is invalid curve data, we don't output it.
		if (bHasValidData == false)
		{
			// Remove the animation asset curve
			if (InCurveAttributes.bAlwaysClearCurves)
			{
				UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, *theCurveName, false);
			}
			continue;
		}

		// Rebuild the animation asset curve
		UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, *theCurveName, false);
		UAnimationBlueprintLibrary::AddCurve(InAnimSequence, *theCurveName);
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, *theCurveName, theKey.Time, theKey.Value);
		}
	}

#if ENGINE_MAJOR_VERSION > 4
	theAnimDataController.CloseBracket();
#endif
	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::SampleRootMotionCurvesToCurvesFloat(TArray<UCurveFloat*> InCurvesFloat, const FVirtualCurveAttributes& InCurveAttributes, const TArray<FRuntimeFloatCurve>& InAnimCurves)
{
	// Check the output curves float is valid
	if (InCurvesFloat.Num() == 0)
	{
		return;
	}

	// Define the iterate index
	int32 theIterateIndex = INDEX_NONE;

	// Check all curves for valid data
	for (const FRuntimeFloatCurve& theFloatCurve : InAnimCurves)
	{
		++theIterateIndex;

		// Check the desired curve float asset is valid
		if (!InCurvesFloat.IsValidIndex(theIterateIndex))
		{
			return;
		}

		// Check the output curve float asset is valid
		UCurveFloat* theCurveFloat = InCurvesFloat[theIterateIndex];
		if (theCurveFloat == nullptr)
		{
			continue;
		}

		// Check the curve data is valid
		if (!theFloatCurve.GetRichCurveConst()->HasAnyData())
		{
			continue;
		}

		// Check the curve data is valid
		bool bHasValidData = false;
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			if (FMath::Abs(theKey.Value) > KINDA_SMALL_NUMBER)
			{
				bHasValidData = true;
				break;
			}
		}

		// If is invalid curve data, we don't output it.
		if (bHasValidData == false)
		{
			continue;
		}

		// Clear the curve data
		theCurveFloat->FloatCurve.Reset();

		// Add the curve key to curve float asset
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			theCurveFloat->FloatCurve.AddKey(theKey.Time, theKey.Value);
		}

		// Apply the curve float asset changed
		UVAT_Library::ModifyFromObject(theCurveFloat);
	}
}

void UVAT_Curve::SampleRootMotionCurvesToCurvesVector(TArray<UCurveVector*> InCurvesVector, const FVirtualCurveAttributes& InCurveAttributes, const TArray<FRuntimeFloatCurve>& InAnimCurves)
{
	// Check the output curves vector is valid
	if (InCurvesVector.Num() == 0)
	{
		return;
	}

	// Define the iterate index
	int32 theIterateIndex = INDEX_NONE;
	int32 theCurveVectorIterateIndex = 0;
	int32 theCurveFloatIterateIndex = 0;

	// Check all curves for valid data
	for (const FRuntimeFloatCurve& theFloatCurve : InAnimCurves)
	{
		++theIterateIndex;
		
		// Iterate curve vector index
		if (theCurveFloatIterateIndex >= 2)
		{
			theCurveFloatIterateIndex = 0;
			++theCurveVectorIterateIndex;
		}

		// Check the desired curve vector asset is valid
		if (!InCurvesVector.IsValidIndex(theCurveVectorIterateIndex))
		{
			return;
		}

		// Check the output curve vector asset is valid
		UCurveVector* theCurveVector = InCurvesVector[theCurveVectorIterateIndex];
		if (theCurveVector == nullptr)
		{
			continue;
		}

		// Check the curve data is valid
		if (!theFloatCurve.GetRichCurveConst()->HasAnyData())
		{
			continue;
		}

		// Check the curve data is valid
		bool bHasValidData = false;
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			if (FMath::Abs(theKey.Value) > KINDA_SMALL_NUMBER)
			{
				bHasValidData = true;
				break;
			}
		}

		// If is invalid curve data, we don't output it.
		if (bHasValidData == false)
		{
			continue;
		}

		// Clear the curve data
		theCurveVector->FloatCurves[theCurveFloatIterateIndex].Reset();

		// Add the curve key to curve float asset
		for (const FRichCurveKey& theKey : theFloatCurve.GetRichCurveConst()->Keys)
		{
			theCurveVector->FloatCurves[theCurveFloatIterateIndex].AddKey(theKey.Time, theKey.Value);
		}

		// Iterate the curve index 
		++theCurveFloatIterateIndex;

		// Apply the curve float asset changed
		UVAT_Library::ModifyFromObject(theCurveVector);
	}
}

void UVAT_Curve::RebuildAnimationRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, bool InAccumulateCurve)
{
	FVirtualCurveAttributes theCurveAttributes = InCurveAttributes;
	theCurveAttributes.bSampleAccumulateCurve = InAccumulateCurve;
	TArray<EVirtualCurveType> theRebuildCurvesType;
	UVAT_Curve::GetAnimationCurvesType(InAnimSequence, theCurveAttributes, theRebuildCurvesType);
	UVAT_Curve::SampleAnimationRootMotionCurves(InAnimSequence, theCurveAttributes, theRebuildCurvesType);
}

void UVAT_Curve::SampleAnimationRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, const TArray<EVirtualCurveType>& InAnimCurvesType)
{
	// Generate the curves data map
	TMap<EVirtualCurveType, FRuntimeFloatCurve> theCurvesDataMap;
	for (const EVirtualCurveType& theCurveType : InAnimCurvesType)
	{
		theCurvesDataMap.FindOrAdd(theCurveType);
	}

	// Sample root motion curves data
	UVAT_Curve::SampleRootMotionCurves(InAnimSequence, InCurveAttributes, theCurvesDataMap);
	UVAT_Curve::SampleRootMotionCurvesToAnimSequence(InAnimSequence, InCurveAttributes, theCurvesDataMap);
	UVAT_Curve::SortAnimationAssetCurves(InAnimSequence, InCurveAttributes);
}
#pragma endregion


#pragma region Curve Event
void UVAT_Curve::CopyAnimationCurvesData(UAnimSequence* InAnimSequence, const TArray<FVirtualCopyCurveData>& InCurvesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Each every copy curve data
	for (const FVirtualCopyCurveData& theCurveData : InCurvesData)
	{
		// Check the curve data is valid
		if (!theCurveData.IsValid())
		{
			continue;
		}

		// Check the source curve data is valid
		FFloatCurve* theSourceCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, theCurveData.SourceName);
		if (theSourceCurvePtr == nullptr || !theSourceCurvePtr->FloatCurve.HasAnyData())
		{
			continue;
		}

		// Check the desired asset is valid
		if (theCurveData.Assets.Num() > 0)
		{
			// Each every asset
			for (UAnimSequence* theDesiredAsset : theCurveData.Assets)
			{
				// Check the asset is valid
				if (theDesiredAsset == nullptr)
				{
					continue;
				}

				// Check the curve data is valid
				FFloatCurve* theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(theDesiredAsset, theCurveData.OutputName);
				if (theOutputCurvePtr == nullptr)
				{
					UAnimationBlueprintLibrary::AddCurve(theDesiredAsset, theCurveData.OutputName);
					theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(theDesiredAsset, theCurveData.OutputName);
					if (theOutputCurvePtr == nullptr)
					{
						continue;
					}
				}

				// Copy curve data
				theOutputCurvePtr->FloatCurve = theSourceCurvePtr->FloatCurve;

				// Apply the animation asset changed
				UVAT_Library::ModifyFromAnimSequence(theDesiredAsset);
			}
		}
		else
		{
			// Check the curve data is valid
			FFloatCurve* theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, theCurveData.OutputName);
			if (theOutputCurvePtr == nullptr)
			{
				UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveData.OutputName);
				theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, theCurveData.OutputName);
				if (theOutputCurvePtr == nullptr)
				{
					continue;
				}
			}

			// Copy curve data
			theOutputCurvePtr->FloatCurve = theSourceCurvePtr->FloatCurve;
		}

		// Check and remove source curve data
		if (theCurveData.bRemoveSourceData)
		{
			UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveData.SourceName);
		}
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::TransferAnimationCurvesData(UAnimSequence* InAnimSequence, FVirtualTransferCurveData& InTransferCurveData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	if (theAnimDataModel == nullptr)
	{
		return;
	}

	IAnimationDataController& theAnimDataController = InAnimSequence->GetController();
#endif

	// Each every output asset
	for (UAnimSequence* theOutputAnimSequence : InTransferCurveData.Assets)
	{
		// Check the animation asset is valid
		if (!theOutputAnimSequence || !theOutputAnimSequence->GetSkeleton())
		{
			return;
		}

		// If the curves name is invalid, we will transfer all curves data
		if (InTransferCurveData.CurvesName.Num() == 0)
		{
#if ENGINE_MAJOR_VERSION > 4
			IAnimationDataController& theOutputAnimDataController = theOutputAnimSequence->GetController();
			theOutputAnimDataController.RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Float);

			// Get all float curves name
			TArray<FName> theFloatCurvesName;
			UAnimationBlueprintLibrary::GetAnimationCurveNames(InAnimSequence, ERawCurveTrackTypes::RCT_Float, theFloatCurvesName);

			// Each every curves name
			for (const FName& theCurveName : theFloatCurvesName)
			{
				// Check the curve data is valid
				FFloatCurve* theSourceCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, theCurveName);
				if (theSourceCurvePtr == nullptr)
				{
					continue;
				}

				// Rebuild the curve data
				UAnimationBlueprintLibrary::RemoveCurve(theOutputAnimSequence, theCurveName);
				UAnimationBlueprintLibrary::AddCurve(theOutputAnimSequence, theCurveName);

				// Check the output curve reference is valid
				FFloatCurve* theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(theOutputAnimSequence, theCurveName);
				if (theOutputCurvePtr == nullptr)
				{
					continue;
				}

				// Copy the curve
				theOutputCurvePtr->FloatCurve = theSourceCurvePtr->FloatCurve;
			}
#else
			theOutputAnimSequence->RawCurveData = InAnimSequence->RawCurveData;
#endif
		}
		else
		{
			// Each every curves name
			for (const FName& theCurveName : InTransferCurveData.CurvesName)
			{
				// Check the curve data is valid
				FFloatCurve* theSourceCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, theCurveName);
				if (theSourceCurvePtr == nullptr)
				{
					continue;
				}

				// Rebuild the curve data
				UAnimationBlueprintLibrary::RemoveCurve(theOutputAnimSequence, theCurveName);
				UAnimationBlueprintLibrary::AddCurve(theOutputAnimSequence, theCurveName);

				// Check the output curve reference is valid
				FFloatCurve* theOutputCurvePtr = GetFloatCurveClass<float, FFloatCurve>(theOutputAnimSequence, theCurveName);
				if (theOutputCurvePtr == nullptr)
				{
					continue;
				}

				// Copy the curve
				theOutputCurvePtr->FloatCurve = theSourceCurvePtr->FloatCurve;
			}
		}

		// Flag modify asset
		UVAT_Library::ModifyFromAnimSequence(theOutputAnimSequence);
	}
}

void UVAT_Curve::SortAnimationAssetCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the skeleton data is valid
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theAnimDataController = InAnimSequence->GetController();
	theAnimDataController.OpenBracket(LOCTEXT("SortFloatCurve_Description", "Sorting float curve."), false);
#endif

	// Each every curve type
	for (const TPair<EVirtualCurveType, FName>& theCurvePair : InCurveAttributes.AnimCurvesNameMap)
	{
		const FName& theCurveName = theCurvePair.Value;
		const EVirtualCurveType& theCurveType = theCurvePair.Key;

		// Check the curve is valid
		if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, theCurveName, ERawCurveTrackTypes::RCT_Float))
		{
			TArray<float> theKeyTimes, theKeyValues;
			UAnimationBlueprintLibrary::GetFloatKeys(InAnimSequence, theCurveName, theKeyTimes, theKeyValues);
			UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
			UAnimationBlueprintLibrary::AddCurve(InAnimSequence, theCurveName);

			// Each every curve key time
			for (int32 i = 0; i < theKeyTimes.Num(); i++)
			{
				UAnimationBlueprintLibrary::AddFloatCurveKey(InAnimSequence, theCurveName, theKeyTimes[i], theKeyValues[i]);
			}
		}
	}

#if ENGINE_MAJOR_VERSION > 4
	theAnimDataController.CloseBracket();
#endif

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::RemoveAnimationAssetCurve(UAnimSequence* InAnimSequence, const TArray<FName>& InCurvesName)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theAnimDataController = InAnimSequence->GetController();
	theAnimDataController.OpenBracket(LOCTEXT("RemoveFloatCurve_Description", "Removing float curve."), false);
#endif

	// Each every curves name
	for (const FName& theCurveName : InCurvesName)
	{
		if (UAnimationBlueprintLibrary::DoesCurveExist(InAnimSequence, theCurveName, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, theCurveName);
		}
	}

#if ENGINE_MAJOR_VERSION > 4
	theAnimDataController.CloseBracket();
#endif
	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::ClearAnimationAssetCurve(UAnimSequence* InAnimSequence)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();

#if ENGINE_MAJOR_VERSION > 4
	const UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theAnimDataController = InAnimSequence->GetController();
	theAnimDataController.OpenBracket(LOCTEXT("RemoveFloatCurve_Description", "Removing float curve."), false);
#endif

#if ENGINE_MAJOR_VERSION > 4
	theAnimDataController.RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Float, false);
#else
	UAnimationBlueprintLibrary::RemoveAllCurveData(InAnimSequence);
#endif

#if ENGINE_MAJOR_VERSION > 4
	theAnimDataController.CloseBracket();
#endif

	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Curve::ResizeAnimationAssetCurves(UAnimSequence* InAnimSequence)
{

}
#pragma endregion


#pragma region Curve Filter
/* Solve Cubic Euqation using Cardano's forumla
* Adopted from Graphic Gems 1
* https://github.com/erich666/GraphicsGems/blob/master/gems/Roots3And4.c
*  Solve cubic of form
*
* @param Coeff Coefficient parameters of form  Coeff[0] + Coeff[1]*x + Coeff[2]*x^2 + Coeff[3]*x^3 + Coeff[4]*x^4 = 0
* @param Solution Up to 3 real solutions. We don't include imaginary solutions, would need a complex number objecct
* @return Returns the number of real solutions returned in the Solution array.
*/
static int SolveCubic(double Coeff[4], double Solution[3])
{
	auto cbrt = [](double x) -> double
	{
		return ((x) > 0.0 ? pow((x), 1.0 / 3.0) : ((x) < 0.0 ? -pow((double)-(x), 1.0 / 3.0) : 0.0));
	};
	int     NumSolutions = 0;

	/* normal form: x^3 + Ax^2 + Bx + C = 0 */

	double A = Coeff[2] / Coeff[3];
	double B = Coeff[1] / Coeff[3];
	double C = Coeff[0] / Coeff[3];

	/*  substitute x = y - A/3 to eliminate quadric term:
	x^3 +px + q = 0 */

	double SqOfA = A * A;
	double P = 1.0 / 3 * (-1.0 / 3 * SqOfA + B);
	double Q = 1.0 / 2 * (2.0 / 27 * A * SqOfA - 1.0 / 3 * A * B + C);

	/* use Cardano's formula */

	double CubeOfP = P * P * P;
	double D = Q * Q + CubeOfP;

	if (FMath::IsNearlyZero(D))
	{
		if (FMath::IsNearlyZero(Q)) /* one triple solution */
		{
			Solution[0] = 0;
			NumSolutions = 1;
		}
		else /* one single and one double solution */
		{
			double u = cbrt(-Q);
			Solution[0] = 2 * u;
			Solution[1] = -u;
			NumSolutions = 2;
		}
	}
	else if (D < 0) /* Casus irreducibilis: three real solutions */
	{
		double phi = 1.0 / 3 * acos(-Q / sqrt(-CubeOfP));
		double t = 2 * sqrt(-P);

		Solution[0] = t * cos(phi);
		Solution[1] = -t * cos(phi + PI / 3);
		Solution[2] = -t * cos(phi - PI / 3);
		NumSolutions = 3;
	}
	else /* one real solution */
	{
		double sqrt_D = sqrt(D);
		double u = cbrt(sqrt_D - Q);
		double v = -cbrt(sqrt_D + Q);

		Solution[0] = u + v;
		NumSolutions = 1;
	}

	/* resubstitute */

	double Sub = 1.0 / 3 * A;

	for (int i = 0; i < NumSolutions; ++i)
	{
		Solution[i] -= Sub;

	}
	return NumSolutions;
}

/** Util to find float value on bezier defined by 4 control points */
FORCEINLINE_DEBUGGABLE static float BezierInterp(float P0, float P1, float P2, float P3, float Alpha)
{
	const float P01 = FMath::Lerp(P0, P1, Alpha);
	const float P12 = FMath::Lerp(P1, P2, Alpha);
	const float P23 = FMath::Lerp(P2, P3, Alpha);
	const float P012 = FMath::Lerp(P01, P12, Alpha);
	const float P123 = FMath::Lerp(P12, P23, Alpha);
	const float P0123 = FMath::Lerp(P012, P123, Alpha);

	return P0123;
}

/*
*   Convert the control values for a polynomial defined in the Bezier
*		basis to a polynomial defined in the power basis (t^3 t^2 t 1).
*/
static void BezierToPower(double A1, double B1, double C1, double D1,
	double* A2, double* B2, double* C2, double* D2)
{
	double A = B1 - A1;
	double B = C1 - B1;
	double C = D1 - C1;
	double D = B - A;
	*A2 = C - B - D;
	*B2 = 3.0 * D;
	*C2 = 3.0 * A;
	*D2 = A1;
}

static float WeightedEvalForTwoKeys(
	float Key1Value, float Key1Time, float Key1LeaveTangent, float Key1LeaveTangentWeight, ERichCurveTangentWeightMode Key1TangentWeightMode,
	float Key2Value, float Key2Time, float Key2ArriveTangent, float Key2ArriveTangentWeight, ERichCurveTangentWeightMode Key2TangentWeightMode,
	float InTime)
{
	const float Diff = Key2Time - Key1Time;
	const float Alpha = (InTime - Key1Time) / Diff;
	const float P0 = Key1Value;
	const float P3 = Key2Value;
	const float OneThird = 1.0f / 3.0f;
	const double Time1 = Key1Time;
	const double Time2 = Key2Time;
	const float X = Time2 - Time1;
	float CosAngle, SinAngle;
	float Angle = FMath::Atan(Key1LeaveTangent);
	FMath::SinCos(&SinAngle, &CosAngle, Angle);
	float LeaveWeight;
	if (Key1TangentWeightMode == RCTWM_WeightedNone || Key1TangentWeightMode == RCTWM_WeightedArrive)
	{
		const float LeaveTangentNormalized = Key1LeaveTangent;
		const float Y = LeaveTangentNormalized * X;
		LeaveWeight = FMath::Sqrt(X * X + Y * Y) * OneThird;
	}
	else
	{
		LeaveWeight = Key1LeaveTangentWeight;
	}
	const float Key1TanX = CosAngle * LeaveWeight + Time1;
	const float Key1TanY = SinAngle * LeaveWeight + Key1Value;

	Angle = FMath::Atan(Key2ArriveTangent);
	FMath::SinCos(&SinAngle, &CosAngle, Angle);
	float ArriveWeight;
	if (Key2TangentWeightMode == RCTWM_WeightedNone || Key2TangentWeightMode == RCTWM_WeightedLeave)
	{
		const float ArriveTangentNormalized = Key2ArriveTangent;
		const float Y = ArriveTangentNormalized * X;
		ArriveWeight = FMath::Sqrt(X * X + Y * Y) * OneThird;
	}
	else
	{
		ArriveWeight = Key2ArriveTangentWeight;
	}
	const float Key2TanX = -CosAngle * ArriveWeight + Time2;
	const float Key2TanY = -SinAngle * ArriveWeight + Key2Value;

	//Normalize the Time Range
	const float RangeX = Time2 - Time1;

	const float Dx1 = Key1TanX - Time1;
	const float Dx2 = Key2TanX - Time1;

	// Normalize values
	const float NormalizedX1 = Dx1 / RangeX;
	const float NormalizedX2 = Dx2 / RangeX;

	double Coeff[4];
	double Results[3];

	//Convert Bezier to Power basis, also float to double for precision for root finding.
	BezierToPower(
		0.0, NormalizedX1, NormalizedX2, 1.0,
		&(Coeff[3]), &(Coeff[2]), &(Coeff[1]), &(Coeff[0])
	);

	Coeff[0] = Coeff[0] - Alpha;

	int NumResults = SolveCubic(Coeff, Results);
	float NewInterp = Alpha;
	if (NumResults == 1)
	{
		NewInterp = Results[0];
	}
	else
	{
		NewInterp = TNumericLimits<float>::Lowest(); //just need to be out of range
		for (double Result : Results)
		{
			if ((Result >= 0.0f) && (Result <= 1.0f))
			{
				if (NewInterp < 0.0f || Result > NewInterp)
				{
					NewInterp = Result;
				}
			}
		}

		if (NewInterp == TNumericLimits<float>::Lowest())
		{
			NewInterp = 0.f;
		}

	}
	//now use NewInterp and adjusted tangents plugged into the Y (Value) part of the graph.
	//const float P0 = Key1.Value;
	const float P1 = Key1TanY;
	//const float P3 = Key2.Value;
	const float P2 = Key2TanY;

	float OutValue = BezierInterp(P0, P1, P2, P3, NewInterp);
	return OutValue;
}

static bool IsItNotWeighted(const FRichCurveKey& Key1, const FRichCurveKey& Key2)
{
	return ((Key1.TangentWeightMode == RCTWM_WeightedNone || Key1.TangentWeightMode == RCTWM_WeightedArrive)
		&& (Key2.TangentWeightMode == RCTWM_WeightedNone || Key2.TangentWeightMode == RCTWM_WeightedLeave));
}

static float EvalForTwoKeys(const FRichCurveKey& Key1, const FRichCurveKey& Key2, const float InTime)
{
	const float Diff = Key2.Time - Key1.Time;

	if (Diff > 0.f && Key1.InterpMode != RCIM_Constant)
	{
		const float Alpha = (InTime - Key1.Time) / Diff;
		const float P0 = Key1.Value;
		const float P3 = Key2.Value;

		if (Key1.InterpMode == RCIM_Linear)
		{
			return FMath::Lerp(P0, P3, Alpha);
		}
		else
		{
			if (IsItNotWeighted(Key1, Key2))
			{
				const float OneThird = 1.0f / 3.0f;
				const float P1 = P0 + (Key1.LeaveTangent * Diff * OneThird);
				const float P2 = P3 - (Key2.ArriveTangent * Diff * OneThird);

				return BezierInterp(P0, P1, P2, P3, Alpha);
			}
			else //it's weighted
			{
				return  WeightedEvalForTwoKeys(
					Key1.Value, Key1.Time, Key1.LeaveTangent, Key1.LeaveTangentWeight, Key1.TangentWeightMode,
					Key2.Value, Key2.Time, Key2.ArriveTangent, Key2.ArriveTangentWeight, Key2.TangentWeightMode,
					InTime);
			}
		}
	}
	else
	{
		return Key1.Value;
	}
}

void UVAT_Curve::ApplyCurveFilter(FRuntimeFloatCurve& InFloatCurve, const EVirtualCurveFilterType& InFilterType, const float InTolerance)
{
	switch (InFilterType)
	{
	case EVirtualCurveFilterType::Bake:
		// Default is always exact curve
		break;

	case EVirtualCurveFilterType::Euler:
		ApplyCurveEulerFilter(InFloatCurve, InTolerance);
		break;

	case EVirtualCurveFilterType::FourierTransform:
		break;

	case EVirtualCurveFilterType::Reduce:
		ApplyCurveReduceFilter(InFloatCurve, InTolerance);
		break;
	}
}

void UVAT_Curve::ApplyCurveEulerFilter(FRuntimeFloatCurve& InFloatCurve, const float InTolerance)
{
	// Get the rich curve reference
	FRichCurve* theSourceCurve = InFloatCurve.GetRichCurve();

	// Get the source curve keys data
	TArray<FKeyHandle> theSourceKeysHandle;

	// Get all keys handle
	for (auto It = theSourceCurve->GetKeyHandleIterator(); It; ++It)
	{
		theSourceKeysHandle.Add(*It);
	}

	// Check the keys number is valid
	if (theSourceKeysHandle.Num() <= 2)
	{
		return;
	}

	// Each every curve keys
	for (int32 KeyIndex = 1; KeyIndex < theSourceKeysHandle.Num() - 1; ++KeyIndex)
	{
		// Get the key data
		const FKeyHandle& theKeyHandle = theSourceKeysHandle[KeyIndex];
		const FRichCurveKey& theCurveKey = theSourceCurve->GetKey(theKeyHandle);
		FRichCurveKey& theNextCurveKey = theSourceCurve->GetKey(theSourceKeysHandle[KeyIndex + 1]);

		// calculate the Euler-filtered key
		float theNextKeyValue = theNextCurveKey.Value;
		FMath::WindRelativeAnglesDegrees(theCurveKey.Value, theNextKeyValue);

		// Rebuild the key value
		theNextCurveKey.Value = theNextKeyValue;
	}
}

void UVAT_Curve::ApplyCurveReduceFilter(FRuntimeFloatCurve& InFloatCurve, const float InTolerance)
{
	// Get the rich curve reference
	FRichCurve* theSourceCurve = InFloatCurve.GetRichCurve();

	// Get the source curve keys data
	TArray<FKeyHandle> theSourceKeysHandle;

	// Get all keys handle
	for (auto It = theSourceCurve->GetKeyHandleIterator(); It; ++It)
	{
		theSourceKeysHandle.Add(*It);
	}

	// Check the keys number is valid
	if (theSourceKeysHandle.Num() <= 2)
	{
		return;
	}

	// Define the local variables
	int32 theMostRecentKeepKeyIndex = 0;
	TArray<FKeyHandle> theRemoveKeysHandle;

	// Each every curve keys
	for (int32 KeyIndex = 1; KeyIndex < theSourceKeysHandle.Num() - 1; ++KeyIndex)
	{
		// Get the key data
		const FKeyHandle& theKeyHandle = theSourceKeysHandle[KeyIndex];
		const FRichCurveKey& theCurveKey = theSourceCurve->GetKey(theKeyHandle);

		// Evaluate two key tolerance
		const float theValueWithoutKey = EvalForTwoKeys(
			theSourceCurve->GetKey(theSourceKeysHandle[theMostRecentKeepKeyIndex])
			, theSourceCurve->GetKey(theSourceKeysHandle[KeyIndex + 1])
			, theCurveKey.Time);

		// Check if there is a great enough change in value to consider this key needed.
		if (FMath::Abs(theValueWithoutKey - theCurveKey.Value) > InTolerance)
		{
			theMostRecentKeepKeyIndex = KeyIndex;
		}
		else
		{
			theRemoveKeysHandle.Add(theKeyHandle);
		}
	}

	// Remove invalid curve keys
	for (int32 KeyIndex = theRemoveKeysHandle.Num() - 1; KeyIndex >= 0; KeyIndex--)
	{
		theSourceCurve->DeleteKey(theRemoveKeysHandle[KeyIndex]);
	}
}

void UVAT_Curve::K2_ResizeRuntimeCurve(UObject* InWorldContextObject, FVirtualResizeCurveData& InCurve)
{
	InCurve.Resize();
}
#pragma endregion


#pragma region Convert
bool UVAT_Curve::K2_ConvertAnimCurveToRuntimeCurve(UObject* InWorldContextObject, UAnimSequence* InAnimSequence, FRuntimeFloatCurve& InCurve, FName InCurveName)
{
#if WITH_EDITOR
	// Flag the object is modify state
	if (InWorldContextObject != nullptr)
	{
		InWorldContextObject->Modify();
	}
#endif // WITH_EDITOR

	// Response the event
	const bool bValid = ConvertAnimCurveToRuntimeCurve(InAnimSequence, InCurve, InCurveName);

#if WITH_EDITOR
	// Flag the object is dirty
	if (InWorldContextObject != nullptr)
	{
		InWorldContextObject->PostEditChange();
		InWorldContextObject->MarkPackageDirty();
	}
#endif // WITH_EDITOR

	// Return the result
	return bValid;
}

bool UVAT_Curve::ConvertAnimCurveToRuntimeCurve(UAnimSequence* InAnimSequence, FRuntimeFloatCurve& InCurve, const FName& InCurveName)
{
	// Always check the animation sequence is valid
	if (InAnimSequence == nullptr)
	{
		return false;
	}

	// Check the runtime rich curve is valid
	FRichCurve* theRichCurvePtr = InCurve.GetRichCurve();
	if (theRichCurvePtr == nullptr)
	{
		return false;
	}

	// Reset the curve cached data
	theRichCurvePtr->Reset();

	// Check the curve data is valid
	FFloatCurve* theFloatCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName);
	if (theFloatCurvePtr == nullptr || !theFloatCurvePtr->FloatCurve.HasAnyData())
	{
		return 0.f;
	}

	auto OnCopyCurveKeyDat = [&](const FRichCurveKey& InKey, const FKeyHandle& InKeyHandle)
	{
		if (InKey.InterpMode == ERichCurveInterpMode::RCIM_Constant)
		{
			theRichCurvePtr->SetKeyInterpMode(InKeyHandle, RCIM_Constant);
		}
		else
		{
			// Get the source curve keys data
			TArray<FKeyHandle> theSourceKeysHandle;

			// Get all keys handle
			for (auto It = theRichCurvePtr->GetKeyHandleIterator(); It; ++It)
			{
				theSourceKeysHandle.Add(*It);
			}

			for (int32 KeyIndex = theSourceKeysHandle.Num() - 1; KeyIndex >= 0; KeyIndex--)
			{
				FKeyHandle& theCurveKeyHandle = theSourceKeysHandle[KeyIndex];
				if (theCurveKeyHandle == InKeyHandle)
				{
					FRichCurveKey& theKey = theRichCurvePtr->GetKey(theCurveKeyHandle);
					theKey = InKey;
				}
			}
		}
	};

	// Get the keys reference
	const TArray<FRichCurveKey>& theCurveKeys = theFloatCurvePtr->FloatCurve.Keys;

	// Get the curve interp model
	ERichCurveInterpMode theCurveInterpMode = theCurveKeys[0].InterpMode;

	// Each every curve keys data
	for (int32 KeyIndex = 0; KeyIndex < theCurveKeys.Num(); KeyIndex++)
	{
		const float& theKeyTime = theCurveKeys[KeyIndex].Time;
		const float& theKeyValue = theCurveKeys[KeyIndex].Value;
		theCurveInterpMode = theCurveKeys[KeyIndex].InterpMode;

		// If there are keys of the same time, we should add a little error
		if (theCurveKeys.IsValidIndex(KeyIndex - 1))
		{
			const float& theLastKeyTime = theCurveKeys[KeyIndex - 1].Time;
			const float& theLastKeyValue = theCurveKeys[KeyIndex - 1].Value;
			if (theKeyTime == theLastKeyTime)
			{
				// Avoid adding the same key
				if (theKeyValue == theLastKeyValue)
				{
					// Add the tolerance key
					const FKeyHandle& theKeyHandle = theRichCurvePtr->AddKey(theKeyTime + 0.001f, theKeyValue);
					OnCopyCurveKeyDat(theCurveKeys[KeyIndex], theKeyHandle);
					continue;
				}

				// Add the tolerance key
				const FKeyHandle& theKeyHandle = theRichCurvePtr->AddKey(theKeyTime + 0.001f, theKeyValue);
				OnCopyCurveKeyDat(theCurveKeys[KeyIndex], theKeyHandle);
				continue;
			}
		}

		if (theCurveInterpMode == ERichCurveInterpMode::RCIM_Constant)
		{
			// Check for the existence of constant curves
			if (KeyIndex >= 2 && theCurveKeys.Num() >= KeyIndex + 2)
			{
				float theLastKeyTimes[2]{ theCurveKeys[KeyIndex - 2].Time, theCurveKeys[KeyIndex - 1].Time };
				float theLastKeyValues[2]{ theCurveKeys[KeyIndex - 2].Value, theCurveKeys[KeyIndex - 1].Value };
				float theNextKeyTimes[2]{ theCurveKeys[KeyIndex].Time, theCurveKeys[KeyIndex + 1].Time };
				float theNextKeyValues[2]{ theCurveKeys[KeyIndex].Value, theCurveKeys[KeyIndex + 1].Value };

				// Constant curves must have two identical keys
				if (theLastKeyValues[0] == theLastKeyValues[1]
					&& theNextKeyValues[0] == theNextKeyValues[1])
				{
					{
						const FKeyHandle& theKeyHandle = theRichCurvePtr->AddKey(theKeyTime, theKeyValue);
						OnCopyCurveKeyDat(theCurveKeys[KeyIndex], theKeyHandle);
					}
					{
						const FKeyHandle& theKeyHandle = theRichCurvePtr->AddKey(theKeyTime, theLastKeyValues[1]);
						OnCopyCurveKeyDat(theCurveKeys[KeyIndex], theKeyHandle);
					}
					continue;
				}
			}
		}

		// Add default key data
		{
			const FKeyHandle& theKeyHandle = theRichCurvePtr->AddKey(theKeyTime, theKeyValue);
			OnCopyCurveKeyDat(theCurveKeys[KeyIndex], theKeyHandle);
		}
	}

	// Handle constant curve
	if (theCurveInterpMode == ERichCurveInterpMode::RCIM_Constant)
	{
		// Each every previous keys
		TArray<FRichCurveKey> thePreviousKeys = theRichCurvePtr->Keys;
		for (int32 KeyIndex = 0; KeyIndex < thePreviousKeys.Num(); KeyIndex++)
		{
			// Get the previous key
			const FRichCurveKey& theKey = thePreviousKeys[KeyIndex];

			// Check the previous last key
			if (thePreviousKeys.IsValidIndex(KeyIndex - 1))
			{
				// Get the last key
				FRichCurveKey& theLastKey = thePreviousKeys[KeyIndex - 1];
				if (theKey.Value != theLastKey.Value)
				{
					theRichCurvePtr->AddKey(theKey.Time, theLastKey.Value);
				}
			}
		}
	}

	// Success
	return true;
}

bool UVAT_Curve::K2_ConvertRuntimeCurveToAnimCurve(UObject* InWorldContextObject, UAnimSequence* InAnimSequence
	, const FRuntimeFloatCurve& InCurve, FName InCurveName, TEnumAsByte<ERichCurveInterpMode> InCurveInterpMode)
{
#if WITH_EDITOR
	// Flag the object is modify state
	if (InWorldContextObject != nullptr)
	{
		InWorldContextObject->Modify();
	}
#endif // WITH_EDITOR

	// Response the event
	const bool bValid = ConvertRuntimeCurveToAnimCurve<float, FFloatCurve>(InAnimSequence, InCurve, InCurveName, InCurveInterpMode);

#if WITH_EDITOR
	// Flag the object is dirty
	if (InWorldContextObject != nullptr)
	{
		InWorldContextObject->PostEditChange();
		InWorldContextObject->MarkPackageDirty();
	}
#endif // WITH_EDITOR

	// Return the result
	return bValid;
}

template <typename DataType, typename CurveClass>
bool UVAT_Curve::ConvertRuntimeCurveToAnimCurve(UAnimSequence* InAnimSequence, const FRuntimeFloatCurve& InCurve
	, const FName& InCurveName, const TEnumAsByte<ERichCurveInterpMode> InCurveInterpMode)
{
	// Always check the animation sequence is valid
	if (InAnimSequence == nullptr)
	{
		return false;
	}

	// Check the runtime rich curve is valid
	const FRichCurve* theRichCurvePtr = InCurve.GetRichCurveConst();
	if (theRichCurvePtr == nullptr)
	{
		return false;
	}

#if WITH_EDITOR
	UAnimationBlueprintLibrary::RemoveCurve(InAnimSequence, InCurveName);
	UAnimationBlueprintLibrary::AddCurve(InAnimSequence, InCurveName);
#endif // WITH_EDITOR

	// Check the curve class reference is valid
	CurveClass* theCurvePtr = GetCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Float);
	if (theCurvePtr == nullptr)
	{
		return false;
	}

	// Each every curve keys data
	for (int32 KeyIndex = 0; KeyIndex < theRichCurvePtr->Keys.Num(); KeyIndex++)
	{
		const FRichCurveKey& theCurveKey = theRichCurvePtr->Keys[KeyIndex];

		// Check for the existence of constant curves
		// Sort the current Keys to prevent non-constant curves from appearing
		//if (InCurveInterpMode == ERichCurveInterpMode::RCIM_Constant || true)
		if (KeyIndex >= 2 /*&& theRichCurvePtr->Keys.Num() >= KeyIndex + 2*/)
		{
			FRichCurveKey theLastKey[2] = { theRichCurvePtr->Keys[KeyIndex - 2] , theRichCurvePtr->Keys[KeyIndex - 1] };

			// Constant curves must have two identical keys
			if (theLastKey[0].Value == theLastKey[1].Value
				&& theLastKey[1].Time == theCurveKey.Time)
			{
				FKeyHandle theLastKeyHandle = theCurvePtr->FloatCurve.FindKey(theLastKey[1].Time);
				theCurvePtr->FloatCurve.DeleteKey(theLastKeyHandle);
				{
					const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theCurveKey.Time, theCurveKey.Value);
					theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
				}
				{
					const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theLastKey[1].Time, theLastKey[1].Value);
					theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
				}
				continue;
			}
			else if (theCurveKey.Value == theLastKey[1].Value
				&& theLastKey[0].Time == theLastKey[1].Time)
			{
				FKeyHandle theLastFirstKeyHandle = theCurvePtr->FloatCurve.FindKey(theLastKey[0].Time);
				theCurvePtr->FloatCurve.DeleteKey(theLastFirstKeyHandle);
				FKeyHandle theLastSecondKeyHandle = theCurvePtr->FloatCurve.FindKey(theLastKey[1].Time);
				theCurvePtr->FloatCurve.DeleteKey(theLastSecondKeyHandle);
				{
					const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theCurveKey.Time, theCurveKey.Value);
					theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
				}
				{
					const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theLastKey[1].Time, theLastKey[1].Value);
					theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
				}
				{
					const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theLastKey[0].Time, theLastKey[0].Value);
					theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
				}
				continue;
			}
		}

		// Add default key data
		{
			const FKeyHandle& theKeyHandle = theCurvePtr->FloatCurve.AddKey(theCurveKey.Time, theCurveKey.Value);
			theCurvePtr->FloatCurve.SetKeyInterpMode(theKeyHandle, InCurveInterpMode);
		}
	}

#if WITH_EDITOR
	if (InAnimSequence->GetSkeleton())
	{
		InAnimSequence->GetSkeleton()->PostEditChange();
	}
	InAnimSequence->PostEditChange();
	InAnimSequence->RefreshCacheData();
	InAnimSequence->MarkPackageDirty();

#if ENGINE_MAJOR_VERSION < 5
	InAnimSequence->RefreshCurveData();
	InAnimSequence->MarkRawDataAsModified();
	if (InAnimSequence->DoesNeedRebake())
	{
		InAnimSequence->BakeTrackCurvesToRawAnimation();
	}
	if (InAnimSequence->DoesNeedRecompress())
	{
		InAnimSequence->RequestSyncAnimRecompression(false);
	}
#endif // ENGINE_MAJOR_VERSION
#endif //WITH_EDITOR

	// Success
	return true;
}
#pragma endregion
#undef LOCTEXT_NAMESPACE