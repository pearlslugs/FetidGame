// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Library.h"
#include "Library/VAT_Retarget.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Kismet/KismetMathLibrary.h"

#if ENGINE_MAJOR_VERSION > 4
#include "AnimationDataController/Public/IAnimationDataControllerModule.h"
#endif


#define LOCTEXT_NAMESPACE "UVAT_Library"

static float CorrectionAngle(const float& InAngle)
{
	// Avoid orientation errors due to conversion
	if (FMath::IsNearlyEqual(FMath::Abs(InAngle), 180.f, KINDA_SMALL_NUMBER))
	{
		return (180.f - KINDA_SMALL_NUMBER) * (InAngle >= 0.f ? 1.f : -1.f);
	}

	// Ignore small tolerance
	return FMath::Abs(InAngle) >= KINDA_SMALL_NUMBER ? InAngle : 0.f;
}

void UVAT_Library::NormailizeTransform(FTransform& InTransform)
{
	// Corrected the translation tolerance
	FVector theTranslation = InTransform.GetTranslation();
	theTranslation.X = FMath::Abs(theTranslation.X) >= KINDA_SMALL_NUMBER ? theTranslation.X : 0.f;
	theTranslation.Y = FMath::Abs(theTranslation.Y) >= KINDA_SMALL_NUMBER ? theTranslation.Y : 0.f;
	theTranslation.Z = FMath::Abs(theTranslation.Z) >= KINDA_SMALL_NUMBER ? theTranslation.Z: 0.f;
	InTransform.SetTranslation(theTranslation);

	// Corrected the rotation angle
	FRotator theRotation = InTransform.GetRotation().Rotator();
	theRotation.Pitch = CorrectionAngle(theRotation.Pitch);
	theRotation.Roll = CorrectionAngle(theRotation.Roll);
	theRotation.Yaw = CorrectionAngle(theRotation.Yaw);
	InTransform.SetRotation(theRotation.Quaternion());

	// Normalize the rotation
	InTransform.NormalizeRotation();
}

FTransform UVAT_Library::BlendTransform(const FTransform& InTransform, const FTransform& InOtherTransform
	, FVector InTranslationWeight, FVector InRotationWeight, FVector InScaleWeight)
{
	// Define the new transform
	FTransform NewTransform = InTransform;

	// Blend the translation
	FVector theNewTranslation = InTransform.GetTranslation();
	theNewTranslation.X = UKismetMathLibrary::Lerp(theNewTranslation.X, InOtherTransform.GetTranslation().X, InTranslationWeight.X);
	theNewTranslation.Y = UKismetMathLibrary::Lerp(theNewTranslation.Y, InOtherTransform.GetTranslation().Y, InTranslationWeight.Y);
	theNewTranslation.Z = UKismetMathLibrary::Lerp(theNewTranslation.Z, InOtherTransform.GetTranslation().Z, InTranslationWeight.Z);
	NewTransform.SetTranslation(theNewTranslation);

	// Blend the rotation
	FRotator theNewRotation = InTransform.Rotator();
	theNewRotation.Roll = UKismetMathLibrary::Lerp(theNewRotation.Roll, InOtherTransform.Rotator().Roll, InRotationWeight.X);
	theNewRotation.Pitch = UKismetMathLibrary::Lerp(theNewRotation.Pitch, InOtherTransform.Rotator().Pitch, InRotationWeight.Y);
	theNewRotation.Yaw = UKismetMathLibrary::Lerp(theNewRotation.Yaw, InOtherTransform.Rotator().Yaw, InRotationWeight.Z);
	NewTransform.SetRotation(theNewRotation.Quaternion());

	// Blend the scale
	FVector theNewScale = InTransform.GetScale3D();
	theNewScale.X = UKismetMathLibrary::Lerp(theNewScale.X, InOtherTransform.GetScale3D().X, InScaleWeight.X);
	theNewScale.Y = UKismetMathLibrary::Lerp(theNewScale.Y, InOtherTransform.GetScale3D().Y, InScaleWeight.Y);
	theNewScale.Z = UKismetMathLibrary::Lerp(theNewScale.Z, InOtherTransform.GetScale3D().Z, InScaleWeight.Z);
	NewTransform.SetScale3D(theNewScale);

	// Normalize the rotation
	NewTransform.NormalizeRotation();

	// Return the result
	return NewTransform;
}

FTransform UVAT_Library::Add_TransformTransform(const FTransform& InTransform, const FTransform& InOtherTransform)
{
	// Define the new transform
	FTransform NewTransform = InTransform;

	// Additive the translation
	NewTransform.AddToTranslation(InOtherTransform.GetTranslation());

	// Additive the rotation
	NewTransform.SetRotation(InTransform.GetRotation() * InOtherTransform.GetRotation());

	// Normalize the rotation
	NewTransform.NormalizeRotation();

	// Return the result
	return NewTransform;
}

FTransform UVAT_Library::Subtract_TransformTransform(const FTransform& InTransform, const FTransform& InOtherTransform)
{
	// Define the new transform
	FTransform NewTransform = InTransform;

	// Additive the translation
	NewTransform.AddToTranslation(-InOtherTransform.GetTranslation());

	// Additive the rotation
	NewTransform.SetRotation(InTransform.GetRotation() * InOtherTransform.GetRotation().Inverse());

	// Normalize the rotation
	NewTransform.NormalizeRotation();

	// Return the result
	return NewTransform;
}

float UVAT_Library::GetFrameTime(UAnimSequenceBase* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return 0.f;
    }

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	return InAnimSequence->GetTimeAtFrame(1);
#else
	return InAnimSequence->GetTimeAtFrame(1);
#endif
}

int32 UVAT_Library::GetNumberOfKeys(UAnimSequence* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	return theAnimDataModel ? theAnimDataModel->GetNumberOfKeys() : 1;
#else
	return InAnimSequence->GetNumberOfFrames();
#endif
}

int32 UVAT_Library::GetNumberOfFrames(UAnimSequence* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	return theAnimDataModel ? theAnimDataModel->GetNumberOfFrames() : 1;
#else
	return InAnimSequence->GetNumberOfFrames();
#endif
}

float UVAT_Library::GetSequenceLength(UAnimSequenceBase* InAnimSequence)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return 0.f;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	return theAnimDataModel ? theAnimDataModel->GetPlayLength() : 0.f;
#else
	return InAnimSequence->GetPlayLength();
#endif
}

bool UVAT_Library::GetBoneTrackData(UAnimSequence* InAnimSequence, const FName& InBoneName, FRawAnimSequenceTrack& OutBoneTack, const bool InFillTrack)
{
	return GetBoneTrackData(InAnimSequence, GetBoneTrackIndex(InAnimSequence, InBoneName), OutBoneTack, InFillTrack);
}

bool UVAT_Library::GetBoneTrackData(UAnimSequence* InAnimSequence, const int32& InBoneTrackIndex, FRawAnimSequenceTrack& OutBoneTack, const bool InFillTrack)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return false;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();

	// Check the bone track index is valid
	if (theAnimDataModel == nullptr || !theAnimDataModel->IsValidBoneTrackIndex(InBoneTrackIndex))
	{
		return false;
	}

	// Get the track data
	OutBoneTack = theAnimDataModel ? theAnimDataModel->GetBoneTrackByIndex(InBoneTrackIndex).InternalTrackData : FRawAnimSequenceTrack();

	// Output the data is valid
	const bool bValidData = OutBoneTack.PosKeys.Num() > 0 || OutBoneTack.RotKeys.Num() > 0;

	// Fill the bone track data
	if (bValidData && InFillTrack)
	{
		const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
		for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
		{
			if (!OutBoneTack.PosKeys.IsValidIndex(KeyIndex))
			{
				OutBoneTack.PosKeys.Add(OutBoneTack.PosKeys.Num() > 0 ? OutBoneTack.PosKeys.Last() : FVector3f::ZeroVector);
			}
			if (!OutBoneTack.RotKeys.IsValidIndex(KeyIndex))
			{
				OutBoneTack.RotKeys.Add(OutBoneTack.RotKeys.Num() > 0 ? OutBoneTack.RotKeys.Last() : FQuat4f::Identity);
			}
			if (!OutBoneTack.ScaleKeys.IsValidIndex(KeyIndex))
			{
				OutBoneTack.ScaleKeys.Add(OutBoneTack.ScaleKeys.Num() > 0 ? OutBoneTack.ScaleKeys.Last() : FVector3f::OneVector);
			}
		}
	}
#else
	// Check the bone track index is valid
	if (!InAnimSequence->GetRawAnimationData().IsValidIndex(InBoneTrackIndex))
	{
		return false;
	}

	// Get the track data
	OutBoneTack = InAnimSequence->GetRawAnimationTrack(InBoneTrackIndex);

	// Output the data is valid
	const bool bValidData = OutBoneTack.PosKeys.Num() > 0 || OutBoneTack.RotKeys.Num() > 0;

	// Fill the bone track data
	if (bValidData && InFillTrack)
	{
		const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
		for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
		{
			if (!OutBoneTack.PosKeys.IsValidIndex(KeyIndex))
			{
				const FVector thePos = OutBoneTack.PosKeys.Num() > 0 ? OutBoneTack.PosKeys.Last() : FVector::ZeroVector;
				OutBoneTack.PosKeys.Add(thePos);
			}
			if (!OutBoneTack.RotKeys.IsValidIndex(KeyIndex))
			{
				const FQuat theRot = OutBoneTack.RotKeys.Num() > 0 ? OutBoneTack.RotKeys.Last() : FQuat::Identity;
				OutBoneTack.RotKeys.Add(theRot);
			}
			if (!OutBoneTack.ScaleKeys.IsValidIndex(KeyIndex))
			{
				const FVector theSclae = OutBoneTack.ScaleKeys.Num() > 0 ? OutBoneTack.ScaleKeys.Last() : FVector::OneVector;
				OutBoneTack.ScaleKeys.Add(theSclae);
			}
		}
	}
#endif

	// Return the track data is valid
	return bValidData;
}

int32 UVAT_Library::GetBoneTrackIndex(UAnimSequence* InAnimSequence, const FName& InBoneName, bool InAddDefaultTrack)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	int32 theTrackIndex = theAnimDataModel ? theAnimDataModel->GetBoneTrackIndexByName(InBoneName) : INDEX_NONE;
#else
	int32 theTrackIndex = InAnimSequence->GetAnimationTrackNames().IndexOfByKey(InBoneName);
#endif

	// If the bone track is invalid, we check and add default track
	if (InAddDefaultTrack && theTrackIndex == INDEX_NONE)
	{
		AddBoneTrack(InAnimSequence, InBoneName);
#if ENGINE_MAJOR_VERSION > 4
		UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
		theTrackIndex = theAnimDataModel ? theAnimDataModel->GetBoneTrackIndexByName(InBoneName) : INDEX_NONE;
#else
		theTrackIndex = InAnimSequence->GetAnimationTrackNames().IndexOfByKey(InBoneName);
#endif
	}

	// Return the result
	return theTrackIndex;
}

void UVAT_Library::AddBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	InAnimSequence->GetController().AddBoneTrack(InBoneName);
#else
	FRawAnimSequenceTrack theDefaultTrack = FRawAnimSequenceTrack();
	InAnimSequence->AddNewRawTrack(InBoneName, &theDefaultTrack);
#endif
#if 0
	FRawAnimSequenceTrack theSkeletonTrackData;
	const FTransform theSkeletonTransformLS = UVAT_Retarget::GetSkeletonBoneTransformLS(InAnimSequence->GetSkeleton(), InBoneName);
	theSkeletonTrackData.PosKeys.Add(theSkeletonTransformLS.GetLocation());
	theSkeletonTrackData.RotKeys.Add(theSkeletonTransformLS.GetRotation());
	theSkeletonTrackData.ScaleKeys.Add(theSkeletonTransformLS.GetScale3D());
#endif
}

int32 UVAT_Library::InsertBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName, const int32& InInsertBoneIndex)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

#if ENGINE_MAJOR_VERSION > 4
	return InAnimSequence->GetController().InsertBoneTrack(InBoneName, InInsertBoneIndex);
#else
	//return InAnimSequence->InsertTrack(InBoneName);
	FRawAnimSequenceTrack theDefaultTrack = FRawAnimSequenceTrack();
	return InAnimSequence->AddNewRawTrack(InBoneName, &theDefaultTrack);
#endif
}

int32 UVAT_Library::RemoveBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName, const bool InRemoveTranslation, const bool InRemoveRotation, const bool InRemoveScale)
{
	// Check the asset is valid
	if (InAnimSequence == nullptr)
	{
		return INDEX_NONE;
	}

	// Choose clear bone track or modify it
	if (InRemoveTranslation && InRemoveRotation)
	{
#if ENGINE_MAJOR_VERSION > 4
		return InAnimSequence->GetController().RemoveBoneTrack(InBoneName);
#else
		FRawAnimSequenceTrack theBoneTrackData;
		SetBoneTrackData(InAnimSequence, InBoneName, &theBoneTrackData);
#endif
	}
	else
	{
		// Get the bone track data
		FRawAnimSequenceTrack theBoneTrackData;
		GetBoneTrackData(InAnimSequence, InBoneName, theBoneTrackData, false);

		// Handle the translation keys
		if (InRemoveTranslation)
		{
			theBoneTrackData.PosKeys.Reset();
		}

		// Handle the rotation keys
		if (InRemoveRotation)
		{
			theBoneTrackData.RotKeys.Reset();
		}

		// Handle the scale keys
		if (InRemoveScale)
		{
			theBoneTrackData.ScaleKeys.Reset();
		}

		// Modify the bone track keys
		SetBoneTrackData(InAnimSequence, InBoneName, &theBoneTrackData);
	}

	// Return the bone track index
	return GetBoneTrackIndex(InAnimSequence, InBoneName);
}

bool UVAT_Library::SetBoneTrackData(UAnimSequence* InAnimSequence, const FName& InBoneName, FRawAnimSequenceTrack* InTrackData, bool InShouldTransact)
{
	if (!InAnimSequence)
	{
		return false;
	}

#if ENGINE_MAJOR_VERSION > 4
	return InAnimSequence->GetController().SetBoneTrackKeys(InBoneName, InTrackData->PosKeys, InTrackData->RotKeys, InTrackData->ScaleKeys, InShouldTransact);
#else
	return InAnimSequence->AddNewRawTrack(InBoneName, InTrackData) != INDEX_NONE;
#endif
}

void UVAT_Library::AddBoneTrackKey(FRawAnimSequenceTrack& InTrackData, const FTransform& InTransformKey)
{
#if ENGINE_MAJOR_VERSION > 4
	InTrackData.PosKeys.Add(FVector3f(InTransformKey.GetTranslation()));
	InTrackData.RotKeys.Add(FQuat4f(InTransformKey.GetRotation()));
	InTrackData.ScaleKeys.Add(FVector3f(InTransformKey.GetScale3D()));
#else
	InTrackData.PosKeys.Add(InTransformKey.GetTranslation());
	InTrackData.RotKeys.Add(InTransformKey.GetRotation());
	InTrackData.ScaleKeys.Add(InTransformKey.GetScale3D());
#endif
}

void UVAT_Library::ModifyBoneTrackKey(FRawAnimSequenceTrack& InTrackData, const FTransform& InTransformKey, const int32& InKeyIndex)
{
#if ENGINE_MAJOR_VERSION > 4
	if (InTrackData.PosKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.PosKeys[InKeyIndex] = FVector3f(InTransformKey.GetTranslation());
	}
	if (InTrackData.RotKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.RotKeys[InKeyIndex] = FQuat4f(InTransformKey.GetRotation());
	}
	if (InTrackData.ScaleKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.ScaleKeys[InKeyIndex] = FVector3f(InTransformKey.GetScale3D());
	}
#else
	if (InTrackData.PosKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.PosKeys[InKeyIndex] = InTransformKey.GetTranslation();
	}
	if (InTrackData.RotKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.RotKeys[InKeyIndex] = InTransformKey.GetRotation();
	}
	if (InTrackData.ScaleKeys.IsValidIndex(InKeyIndex))
	{
		InTrackData.ScaleKeys[InKeyIndex] = InTransformKey.GetScale3D();
	}
#endif
}

void UVAT_Library::TransferBoneTrackKey(const FRawAnimSequenceTrack& InSourceTrackData, FRawAnimSequenceTrack& InNewTrackData, const int32& InKeyIndex)
{
#if ENGINE_MAJOR_VERSION > 4
	if (InSourceTrackData.PosKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.PosKeys.Add(FVector3f(InSourceTrackData.PosKeys[InKeyIndex]));
	}
	if (InSourceTrackData.RotKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.RotKeys.Add(FQuat4f(InSourceTrackData.RotKeys[InKeyIndex]));
	}
	if (InSourceTrackData.ScaleKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.ScaleKeys.Add(FVector3f(InSourceTrackData.ScaleKeys[InKeyIndex]));
	}
#else
	if (InSourceTrackData.PosKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.PosKeys.Add(InSourceTrackData.PosKeys[InKeyIndex]);
	}
	if (InSourceTrackData.RotKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.RotKeys.Add(InSourceTrackData.RotKeys[InKeyIndex]);
	}
	if (InSourceTrackData.ScaleKeys.IsValidIndex(InKeyIndex))
	{
		InNewTrackData.ScaleKeys.Add(InSourceTrackData.ScaleKeys[InKeyIndex]);
	}
#endif
}

void UVAT_Library::ModifyFromObject(UObject* InObject)
{
	if (UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(InObject))
	{
		ModifyFromAnimSequence(AnimSequenceBase);
	}
	else if (InObject)
	{
		InObject->PostEditChange();
		InObject->MarkPackageDirty();
	}
}

void UVAT_Library::ModifyFromAnimSequence(UAnimSequenceBase* InAnimSequence)
{
	if (!InAnimSequence)
	{
		return;
	}

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
	UpdateCompressedAnimationDataAsSequence(Cast<UAnimSequence>(InAnimSequence));
#endif
}

void UVAT_Library::ModifyFromAnimMontage(UAnimMontage* InAnimMontage)
{
	if (!InAnimMontage)
	{
		return;
	}

	if (InAnimMontage->GetSkeleton())
	{
		InAnimMontage->GetSkeleton()->PostEditChange();
	}

	InAnimMontage->PostEditChange();
	InAnimMontage->MarkPackageDirty();
	InAnimMontage->RefreshCacheData();

#if ENGINE_MAJOR_VERSION < 5
	InAnimMontage->RefreshCurveData();
	InAnimMontage->MarkRawDataAsModified();
#endif
}

void UVAT_Library::UpdateCompressedAnimationDataAsSequence(UAnimSequence* InAnimSequnence)
{
#if ENGINE_MAJOR_VERSION < 5
	if (!InAnimSequnence)
	{
		return;
	}

	if (InAnimSequnence->DoesNeedRebake())
	{
		InAnimSequnence->BakeTrackCurvesToRawAnimation();
	}

	if (InAnimSequnence->DoesNeedRecompress())
	{
		InAnimSequnence->RequestSyncAnimRecompression(false);
	}
#endif
}

#undef LOCTEXT_NAMESPACE