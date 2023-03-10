// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "VAT_CurveLibrary.h"
#include "VirtualAnimationTools.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCompositeBase.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(TEXT("GetAssetCurveValue"), STAT_GetAssetCurveValue, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("GetMontageCurveValue"), STAT_GetMontageCurveValue, STATGROUP_VirtualAnimTools);
DECLARE_CYCLE_STAT(TEXT("GetSequenceCurveValue"), STAT_GetSequenceCurveValue, STATGROUP_VirtualAnimTools);

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

int32 UVAT_CurveLibrary::GetFrameAtTime(UAnimSequence* InAnimSequence, const float Time)
{
	check(InAnimSequence);
#if ENGINE_MAJOR_VERSION > 4
	return FMath::Clamp(InAnimSequence->GetSamplingFrameRate().AsFrameTime(Time).RoundToFrame().Value, 0, InAnimSequence->GetNumberOfSampledKeys() - 1);
#else
	const int32& theNumberOfKeys = InAnimSequence->GetNumberOfFrames();
	const float& theAnimLength = InAnimSequence->GetPlayLength();
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FMath::RoundToInt(Time / FrameTime), 0, theNumberOfKeys - 1);
#endif
}

float UVAT_CurveLibrary::GetTimeAtFrame(UAnimSequence* InAnimSequence, const int32 Frame)
{
	check(InAnimSequence);
#if ENGINE_MAJOR_VERSION > 4
	return FMath::Clamp((float)InAnimSequence->GetSamplingFrameRate().AsSeconds(Frame), 0.f, InAnimSequence->GetPlayLength());
#else
	const int32& theNumberOfKeys = InAnimSequence->GetNumberOfFrames();
	const float& theAnimLength = InAnimSequence->GetPlayLength();
	const float FrameTime = theNumberOfKeys > 1 ? theAnimLength / (float)(theNumberOfKeys - 1) : 0.0f;
	return FMath::Clamp(FrameTime * Frame, 0.0f, theAnimLength);
#endif
}

FSmartName UVAT_CurveLibrary::GetCurveSmartName(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	check(InAnimSequence);

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_CurveLibrary::GetCurveSmartName, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		return FSmartName();
	}

	// Retrieve smart name for curve
	return RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);
}

template <typename DataType, typename CurveClass>
const CurveClass* UVAT_CurveLibrary::GetCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName, const ERawCurveTrackTypes& InCurveType)
{
	checkf(InAnimSequence != nullptr, TEXT("Invalid Animation Sequence ptr"));

	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_CurveLibrary::GetCurveClass, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		return nullptr;
	}

	// Retrieve smart name for curve
	const FSmartName theCurveSmartName = RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);

	// Retrieve the curve by name
	return static_cast<const CurveClass*>(InAnimSequence->GetCurveData().GetCurveData(theCurveSmartName.UID, InCurveType));
}

template <typename DataType, typename CurveClass>
const FFloatCurve* UVAT_CurveLibrary::GetFloatCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName)
{
	// Check the curve class reference is valid
	const CurveClass* theCurvePtr = GetCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName, ERawCurveTrackTypes::RCT_Float);
	if (theCurvePtr == nullptr)
	{
		//UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_CurveLibrary::GetFloatCurveClass, Invalid desired animation curve data, curve name: %s."), *InCurveName.ToString());
		return nullptr;
	}

	// Convert to float curve reference
	return static_cast<const FFloatCurve*>(theCurvePtr);
}

float UVAT_CurveLibrary::K2_EvaluateRuntimeCurve(float InEvaluatePos, const FRuntimeFloatCurve& InCurve)
{
	return InCurve.GetRichCurveConst()->Eval(InEvaluatePos);
}

void UVAT_CurveLibrary::K2_CalculateCurveRootMotion(float InDeltaSeconds, UAnimInstance* InAnimInstance, FTransform& OutRootMotion)
{
	// Check the animation instance is valid
	if (InAnimInstance == nullptr)
	{
		return;
	}

	// Check the owning mesh component is valid
	USkeletalMeshComponent* theMeshComponent = InAnimInstance->GetOwningComponent();
	if (theMeshComponent == nullptr)
	{
		return;
	}

	// Check the root motion transform is valid
	if (OutRootMotion.GetTranslation().IsNearlyZero() && OutRootMotion.GetRotation().IsIdentity())
	{
		return;
	}

	// Adjust the delta seconds multiply
	OutRootMotion.SetTranslation(OutRootMotion.GetTranslation() * InDeltaSeconds);
	OutRootMotion.SetRotation((OutRootMotion.Rotator() * InDeltaSeconds).Quaternion());

	// Convert to world space
	OutRootMotion = theMeshComponent->ConvertLocalRootMotionToWorld(OutRootMotion);
}

bool UVAT_CurveLibrary::K2_GetCurveValueFromMontage(UAnimMontage* InAnimMontage, FName InCurveName, float InEvaluatePos, float& OutCurveValue)
{
	return GetCurveValueFromMontage(InAnimMontage, InCurveName, InEvaluatePos, OutCurveValue);
}

bool UVAT_CurveLibrary::GetCurveValueFromMontage(UAnimMontage* InAnimMontage, const FName& InCurveName, const float& InEvaluatePos, float& OutCurveValue)
{
	SCOPE_CYCLE_COUNTER(STAT_GetMontageCurveValue);

	// Check the animation montage is valid
	if (InAnimMontage == nullptr)
	{
		OutCurveValue = 0.f;
		return false;
	}

	// Check the slot track is valid
	if (InAnimMontage->SlotAnimTracks.Num() == 0)
	{
		OutCurveValue = 0.f;
		return false;
	}

	// Find the evaluate animation asset
	float theCompositeStartPos = 0.f;
	UAnimSequenceBase* theEvalAnimSequenceBase = nullptr;

	// Always evaluate first slot track
	for (const FAnimSegment& theAnimSegment : InAnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments)
	{
		if (theAnimSegment.IsInRange(InEvaluatePos))
		{
			theCompositeStartPos = theAnimSegment.StartPos;
			theEvalAnimSequenceBase = theAnimSegment.AnimReference;
			break;
		}
	}

	// Return the result
	return GetAnimationAssetCurveValue(Cast<UAnimSequence>(theEvalAnimSequenceBase), InCurveName, InEvaluatePos - theCompositeStartPos, OutCurveValue);
}

bool UVAT_CurveLibrary::K2_GetCurveValueFromSequence(UAnimSequence* InAnimSequence, FName InCurveName, float InEvaluatePos, float& OutCurveValue)
{
	return GetCurveValueFromSequence(InAnimSequence, InCurveName, InEvaluatePos, OutCurveValue);
}

bool UVAT_CurveLibrary::GetCurveValueFromSequence(UAnimSequence* InAnimSequence, const FName& InCurveName, const float& InEvaluatePos, float& OutCurveValue)
{
	SCOPE_CYCLE_COUNTER(STAT_GetSequenceCurveValue);
	return GetAnimationAssetCurveValue(InAnimSequence, InCurveName, InEvaluatePos, OutCurveValue);
}

bool UVAT_CurveLibrary::GetAnimationAssetCurveValue(UAnimSequence* InAnimSequence, const FName& InCurveName, const float& InEvaluatePos, float& OutCurveValue)
{
	//SCOPE_CYCLE_COUNTER(STAT_GetAssetCurveValue);

	// Always check the animation sequence is valid
	if (InAnimSequence == nullptr)
	{
		// Invalid
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_CurveLibrary::ConvertAnimCurveToRuntimeCurve, Invalid animation sequence asset."));
		OutCurveValue = 0.f;
		return false;
	}

#if WITH_EDITOR && 0
	// Check the curve data is valid
	const FFloatCurve* theFloatCurvePtr = GetFloatCurveClass<float, FFloatCurve>(InAnimSequence, InCurveName);
	if (theFloatCurvePtr == nullptr || !theFloatCurvePtr->FloatCurve.HasAnyData())
	{
		OutCurveValue = 0.f;
		return false;
	}

	// Get the keys reference
	const TArray<FRichCurveKey>& theCurveKeys = theFloatCurvePtr->FloatCurve.Keys;

	// Check the keys is valid
	if (theCurveKeys.Num() == 0)
	{
		OutCurveValue = 0.f;
		return false;
	}

	// Each every keys
	for (int32 KeyIndex = 1; KeyIndex < theCurveKeys.Num(); KeyIndex++)
	{
		const FRichCurveKey& keyA = theCurveKeys[KeyIndex - 1];
		const FRichCurveKey& keyB = theCurveKeys[KeyIndex];

		// Check value range
		if (KeyIndex == 1 && InEvaluatePos < keyA.Time)
		{
			return keyA.Value;
		}
		else if (FMath::IsNearlyEqual(InEvaluatePos, keyA.Time, KINDA_SMALL_NUMBER))
		{
			return keyA.Value;
		}
		else if (FMath::IsNearlyEqual(InEvaluatePos, keyB.Time, KINDA_SMALL_NUMBER))
		{
			return keyB.Value;
		}
		else if (FMath::IsWithinInclusive(InEvaluatePos, keyA.Time, keyB.Time))
		{
			// Calculate the weight
			const float theWeight = (InEvaluatePos - keyA.Time) / (keyB.Time - keyA.Time);

			// Return the result
			return keyA.Value + (keyB.Value - keyA.Value) * theWeight;
		}
	}

	// Return default value
	OutCurveValue = theCurveKeys.Last().Value;
	return true;
#else
	// Get the curve container name from curve name
	const FName& theContainerName = RetrieveContainerNameForCurve(InAnimSequence, InCurveName);
	if (theContainerName == NAME_None)
	{
		// Invalid
		UE_LOG(LogVirtualAnimTools, Warning, TEXT("UVAT_CurveLibrary::GetAnimationAssetCurveValue, Invalid curve container name, curve name: %s."), *InCurveName.ToString());
		OutCurveValue = 0.f;
		return false;
	}

	// Retrieve smart name for curve
	const FSmartName theCurveSmartName = RetrieveSmartNameForCurve(InAnimSequence, InCurveName, theContainerName);

	// Check the curve data is valid
	if (!InAnimSequence->HasCurveData(theCurveSmartName.UID, false))
	{
		OutCurveValue = 0.f;
		return false;
	}

	// Return the result
	OutCurveValue = InAnimSequence->EvaluateCurveData(theCurveSmartName.UID, InEvaluatePos);
	return true;
#endif
}