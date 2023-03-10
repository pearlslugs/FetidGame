// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Asset.h"
#include "Library/VAT_Bone.h"
#include "Library/VAT_Library.h"

#include "Kismet/KismetMathLibrary.h"

#include "PersonaModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "SequenceRecorder/Public/ISequenceRecorder.h"

#include "Animation/AnimTypes.h"
#include "AnimationEditorUtils.h"
#include "Animation/AnimSequence.h"

#if ENGINE_MAJOR_VERSION > 4
#include "Animation/AnimSequenceHelpers.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#include "LODUtilities.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Animation/AnimCurveTypes.h"
#include "Library/VAT_Retarget.h"

#define LOCTEXT_NAMESPACE "UVAT_Asset"

static void GetValidFrameRange(const int32& InMaxFrame, const FVector2D& InFrameRange, int32& OutStartFrame, int32& OutEndFrame)
{
	OutStartFrame = FMath::Max(0, int32(InFrameRange.X));
	OutEndFrame = InFrameRange.Y >= 0.f ? FMath::Min(InMaxFrame, int32(InFrameRange.Y)) : InMaxFrame;
}

#pragma region Animation Asset
#if ENGINE_MAJOR_VERSION > 4
/** Utility function to crop data from a RawAnimSequenceTrack */
static int32 CropRawTrack(FRawAnimSequenceTrack& RawTrack, int32 StartKey, int32 NumKeys, int32 theRawNumberOfFrames)
{
	check(RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == theRawNumberOfFrames);
	check(RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == theRawNumberOfFrames);
	// scale key can be empty
	check(RawTrack.ScaleKeys.Num() == 0 || RawTrack.ScaleKeys.Num() == 1 || RawTrack.ScaleKeys.Num() == theRawNumberOfFrames);

	if (RawTrack.PosKeys.Num() > 1)
	{
		RawTrack.PosKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.PosKeys.Num() > 0);
		RawTrack.PosKeys.Shrink();
	}

	if (RawTrack.RotKeys.Num() > 1)
	{
		RawTrack.RotKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.RotKeys.Num() > 0);
		RawTrack.RotKeys.Shrink();
	}

	if (RawTrack.ScaleKeys.Num() > 1)
	{
		RawTrack.ScaleKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.ScaleKeys.Num() > 0);
		RawTrack.ScaleKeys.Shrink();
	}

	// Update NumFrames below to reflect actual number of keys.
	return FMath::Max<int32>(RawTrack.PosKeys.Num(), FMath::Max<int32>(RawTrack.RotKeys.Num(), RawTrack.ScaleKeys.Num()));
}

/** Utility function to append data from a RawAnimSequenceTrack */
static void AppendRawTrack(const bool IsRootTrack, FRawAnimSequenceTrack& RawTrack, FRawAnimSequenceTrack& InOtherRawTrack, const int32& InOtherTrackFrame, const FVector2D& InFrameRange = FVector2D(-1.f))
{
	int32 AppendStartFrame, AppendEndFrame;
	GetValidFrameRange(InOtherTrackFrame, InFrameRange, AppendStartFrame, AppendEndFrame);

	if (IsRootTrack)
	{
		FTransform StartTransform = FTransform::Identity;
		if (RawTrack.PosKeys.Num() > 0)
		{
			StartTransform.SetLocation(FVector(RawTrack.PosKeys.Last()));
		}
		if (RawTrack.RotKeys.Num() > 0)
		{
			StartTransform.SetRotation(FQuat(RawTrack.RotKeys.Last()));
		}
		if (RawTrack.ScaleKeys.Num() > 0)
		{
			StartTransform.SetScale3D(FVector(RawTrack.ScaleKeys.Last()));
		}

		for (int32 Index = AppendStartFrame; Index < AppendEndFrame; Index++)
		{
			if (InOtherRawTrack.PosKeys.IsValidIndex(Index))
			{
				const FVector theOtherPosKey(InOtherRawTrack.PosKeys[Index]);
				RawTrack.PosKeys.Add(FVector3f(StartTransform.GetLocation() + StartTransform.GetRotation().RotateVector(theOtherPosKey)));
			}
			if (InOtherRawTrack.RotKeys.IsValidIndex(Index))
			{
				RawTrack.RotKeys.Add(FQuat4f(FQuat(InOtherRawTrack.RotKeys[Index]) * StartTransform.GetRotation()));
			}
			if (InOtherRawTrack.ScaleKeys.IsValidIndex(Index))
			{
				RawTrack.ScaleKeys.Add(FVector3f(FVector(InOtherRawTrack.ScaleKeys[Index]) * StartTransform.GetScale3D()));
			}
		}
	}
	else
	{
		for (int32 Index = AppendStartFrame; Index < AppendEndFrame; Index++)
		{
			if (InOtherRawTrack.PosKeys.IsValidIndex(Index))
			{
				RawTrack.PosKeys.Add(FVector3f(InOtherRawTrack.PosKeys[Index]));
			}
			if (InOtherRawTrack.RotKeys.IsValidIndex(Index))
			{
				RawTrack.RotKeys.Add(FQuat4f(InOtherRawTrack.RotKeys[Index]));
			}
			if (InOtherRawTrack.ScaleKeys.IsValidIndex(Index))
			{
				RawTrack.ScaleKeys.Add(FVector3f(InOtherRawTrack.ScaleKeys[Index]));
			}
		}
	}
}
#else
/** Utility function to crop data from a RawAnimSequenceTrack */
static int32 CropRawTrack(FRawAnimSequenceTrack& RawTrack, int32 StartKey, int32 NumKeys, int32 theRawNumberOfFrames)
{
	check(RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == theRawNumberOfFrames);
	check(RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == theRawNumberOfFrames);
	// scale key can be empty
	check(RawTrack.ScaleKeys.Num() == 0 || RawTrack.ScaleKeys.Num() == 1 || RawTrack.ScaleKeys.Num() == theRawNumberOfFrames);

	if (RawTrack.PosKeys.Num() > 1)
	{
		RawTrack.PosKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.PosKeys.Num() > 0);
		RawTrack.PosKeys.Shrink();
	}

	if (RawTrack.RotKeys.Num() > 1)
	{
		RawTrack.RotKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.RotKeys.Num() > 0);
		RawTrack.RotKeys.Shrink();
	}

	if (RawTrack.ScaleKeys.Num() > 1)
	{
		RawTrack.ScaleKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.ScaleKeys.Num() > 0);
		RawTrack.ScaleKeys.Shrink();
	}

	// Update NumFrames below to reflect actual number of keys.
	return FMath::Max<int32>(RawTrack.PosKeys.Num(), FMath::Max<int32>(RawTrack.RotKeys.Num(), RawTrack.ScaleKeys.Num()));
}

/** Utility function to append data from a RawAnimSequenceTrack */
static void AppendRawTrack(const bool IsRootTrack, FRawAnimSequenceTrack& RawTrack, FRawAnimSequenceTrack& InOtherRawTrack, const int32& InOtherTrackFrame, const FVector2D& InFrameRange = FVector2D(-1.f))
{
	int32 AppendStartFrame, AppendEndFrame;
	GetValidFrameRange(InOtherTrackFrame, InFrameRange, AppendStartFrame, AppendEndFrame);

	if (IsRootTrack)
	{
		FTransform StartTransform = FTransform::Identity;
		if (RawTrack.PosKeys.Num() > 0)
		{
			StartTransform.SetLocation(RawTrack.PosKeys.Last());
		}
		if (RawTrack.RotKeys.Num() > 0)
		{
			StartTransform.SetRotation(RawTrack.RotKeys.Last());
		}
		if (RawTrack.ScaleKeys.Num() > 0)
		{
			StartTransform.SetScale3D(RawTrack.ScaleKeys.Last());
		}

		for (int32 Index = AppendStartFrame; Index < AppendEndFrame; Index++)
		{
			if (InOtherRawTrack.PosKeys.IsValidIndex(Index))
			{
				RawTrack.PosKeys.Add(StartTransform.GetLocation() + StartTransform.GetRotation().RotateVector(InOtherRawTrack.PosKeys[Index]));
			}
			if (InOtherRawTrack.RotKeys.IsValidIndex(Index))
			{
				RawTrack.RotKeys.Add(InOtherRawTrack.RotKeys[Index] * StartTransform.GetRotation());
			}
			if (InOtherRawTrack.ScaleKeys.IsValidIndex(Index))
			{
				RawTrack.ScaleKeys.Add(InOtherRawTrack.ScaleKeys[Index] * StartTransform.GetScale3D());
			}
		}
	}
	else
	{
		for (int32 Index = AppendStartFrame; Index < AppendEndFrame; Index++)
		{
			if (InOtherRawTrack.PosKeys.IsValidIndex(Index))
			{
				RawTrack.PosKeys.Add(InOtherRawTrack.PosKeys[Index]);
			}
			if (InOtherRawTrack.RotKeys.IsValidIndex(Index))
			{
				RawTrack.RotKeys.Add(InOtherRawTrack.RotKeys[Index]);
			}
			if (InOtherRawTrack.ScaleKeys.IsValidIndex(Index))
			{
				RawTrack.ScaleKeys.Add(InOtherRawTrack.ScaleKeys[Index]);
			}
		}
	}
}
#endif // ENGINE_MAJOR_VERSION < 5

void UVAT_Asset::CropAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetCropData& InAssetCropFrameData)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Get animation asset data
	float const theFrameTime = UVAT_Library::GetFrameTime(InAnimSequence);
#if ENGINE_MAJOR_VERSION > 4
	const int32 theRawNumberOfFrames = InAnimSequence->GetNumberOfSampledKeys();
#else
	const int32 theRawNumberOfFrames = InAnimSequence->GetRawNumberOfFrames();
#endif

	// Get the crop start to end range
	const int32 theCropStartFrame = FMath::Max(0, InAssetCropFrameData.StartFrame);
	const int32 theCropEndFrame = FMath::Min(theRawNumberOfFrames, InAssetCropFrameData.EndFrame);

	// Check the crop range is valid
	if (theCropStartFrame < 0 || theCropStartFrame > theCropEndFrame)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	UE::Anim::AnimationData::Trim(InAnimSequence, ((float)theCropStartFrame) * theFrameTime, ((float)theCropEndFrame) * theFrameTime);
#else

	// Calculate the crop keys number
	const int32 theCropNumKeys = FMath::Min(theCropEndFrame - theCropStartFrame, theRawNumberOfFrames - 1);

	// Calculate the new animation asset frames number
	const int32 theNewRawNumberOfFrames = theRawNumberOfFrames - theCropNumKeys;

	// Crop desired raw track
	for (int32 i = 0; i < InAnimSequence->GetRawAnimationData().Num(); i++)
	{
		// Update NewNumFrames below to reflect actual number of keys while we crop the animation data
		CropRawTrack(InAnimSequence->GetRawAnimationTrack(i), theCropStartFrame, theCropNumKeys, theRawNumberOfFrames);
	}

	// Update sequence length to match new number of frames.
	InAnimSequence->ResizeSequence((float)theNewRawNumberOfFrames * theFrameTime, theNewRawNumberOfFrames
		, false, FMath::Min(theRawNumberOfFrames - 1, theCropEndFrame), FMath::Max(theNewRawNumberOfFrames, theRawNumberOfFrames));
#endif

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Asset::InsertAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetInsertData& InAssetInsertData)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Define the frame range
	int32 theStartFrame = InAssetInsertData.StartFrame;
	int32 theEndFrame = InAssetInsertData.EndFrame;

	// Check the frame range is valid
	if (theStartFrame == theEndFrame)
	{
		return;
	}

	// Each every copy count
	for (int32 CopyIndex = 0; CopyIndex < InAssetInsertData.CopyCount; CopyIndex++)
	{
		++theStartFrame;
		++theEndFrame;
#if ENGINE_MAJOR_VERSION > 4
		UE::Anim::AnimationData::DuplicateKeys(InAnimSequence, theStartFrame, theEndFrame, InAssetInsertData.CopyFrame);
#else
		InAnimSequence->InsertFramesToRawAnimData(theStartFrame, theEndFrame, InAssetInsertData.CopyFrame);
#endif
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Asset::ResizeAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetResizeData& InAssetResizeData)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Get the resize frame rate
	const float theResizeFrameRate = InAssetResizeData.bEnableFrameRateModifier ? InAssetResizeData.FrameRate.AsInterval() : UVAT_Library::GetFrameTime(InAnimSequence);

	// Get the animation asset source play length
	const float theSourceAssetLength = InAnimSequence->GetPlayLength();

	// Get the animation asset frames number
	const int32 theSourceNumberFrames = UVAT_Library::GetNumberOfFrames(InAnimSequence);

	// Get the source frame rate
	const float theSourceFrameRate = theSourceAssetLength / (float)(theSourceNumberFrames - 1);
	const int32 theSampleFPS = 1.f / theSourceFrameRate;

	// Calculate the new animation asset length
	float theNewAssetLength = InAssetResizeData.bEnableLengthModifier && InAssetResizeData.Length > 0.f ? InAssetResizeData.Length : InAnimSequence->GetPlayLength();

	// Check and apply play rate modifier
	if (InAssetResizeData.bEnablePlayRateModifier && InAssetResizeData.PlayRate != 0.f)
	{
		theNewAssetLength *= FMath::Abs(InAssetResizeData.PlayRate);
	}

	// because of float error, add small margin at the end, so it can clamp correctly
	const int32 theNewNumberFrames = theResizeFrameRate == 0.f ? 1 : (int32(theNewAssetLength / theResizeFrameRate + KINDA_SMALL_NUMBER) + 1);

	// Get animation asset raw tracks data
#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	if (!theAnimDataModel)
	{
		return;
	}

	TArray<FName> theTrackNames;
	TArray<FRawAnimSequenceTrack> theRawAnimationData;
	for (const FBoneAnimationTrack& theBoneAnimTrack : theAnimDataModel->GetBoneAnimationTracks())
	{
		theTrackNames.Add(theBoneAnimTrack.Name);
		theRawAnimationData.Add(theBoneAnimTrack.InternalTrackData);
	}
#else
	const TArray<FName>& theTrackNames = InAnimSequence->GetAnimationTrackNames();
	TArray<FRawAnimSequenceTrack> theRawAnimationData = InAnimSequence->GetRawAnimationData();
#endif

	// Check the track names equal tracks data
	if (theTrackNames.Num() != theRawAnimationData.Num())
	{
		return;
	}

	// Response resize sequence
	if (theNewAssetLength != theSourceAssetLength)
	{
#if ENGINE_MAJOR_VERSION > 4
		//InAnimSequence->GetController().SetPlayLength(theNewAssetLength);
		const FFrameRate& theOldFrameRate = theAnimDataModel->GetFrameRate();
		if (InAssetResizeData.bEnableFrameRateModifier && theOldFrameRate.Numerator != InAssetResizeData.FrameRate.Numerator)
		{
			InAnimSequence->GetController().SetFrameRate(InAssetResizeData.FrameRate);
			InAnimSequence->GetController().NotifyPopulated();
		}
		InAnimSequence->GetController().Resize(theNewAssetLength, 0, theNewAssetLength);
#else
		InAnimSequence->SequenceLength = theNewAssetLength;
		InAnimSequence->SetRawNumberOfFrame(theNewNumberFrames);
		InAnimSequence->ResizeSequence(theNewAssetLength, theNewNumberFrames, false, 0, theSourceNumberFrames);
#endif
		}

	// Calculate the pose insert count
	const int32 theInsertCount = theNewNumberFrames > theSourceNumberFrames
		? (FMath::Max(1, (theNewNumberFrames - 1)) / FMath::Max(1, (theSourceNumberFrames - 1)))
		: (FMath::Max(1, (theSourceNumberFrames - 1)) / FMath::Max(1, (theNewNumberFrames - 1)) * -1);

	// Calculate the iterate data
	const float theResizeScale = theNewAssetLength / theSourceAssetLength;
	const int32 theIterateCount = theInsertCount > 0 ? 1 : FMath::Max(0, FMath::Abs(theInsertCount));

	// Each every track data
	for (int32 TrackIndex = 0; TrackIndex < theTrackNames.Num(); TrackIndex++)
	{
		// Define the source bone raw track data
		FTransform theSourceBoneTransform;
		FVirtualAssetTrackData theSourceBoneTrackData;
		const FRawAnimSequenceTrack& theSourceBoneRawTrack = theRawAnimationData[TrackIndex];

		// Sample source animation asset bone poses data
		for (int32 PoseIndex = 0; PoseIndex < theSourceNumberFrames; PoseIndex++)
		{
#if ENGINE_MAJOR_VERSION > 4
			if (theSourceBoneRawTrack.PosKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetTranslation(FVector(theSourceBoneRawTrack.PosKeys[PoseIndex]));
			}
			if (theSourceBoneRawTrack.RotKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetRotation(FQuat(theSourceBoneRawTrack.RotKeys[PoseIndex]));
			}
			if (theSourceBoneRawTrack.ScaleKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetScale3D(FVector(theSourceBoneRawTrack.ScaleKeys[PoseIndex]));
			}
#else
			if (theSourceBoneRawTrack.PosKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetTranslation(theSourceBoneRawTrack.PosKeys[PoseIndex]);
			}
			if (theSourceBoneRawTrack.RotKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetRotation(theSourceBoneRawTrack.RotKeys[PoseIndex]);
			}
			if (theSourceBoneRawTrack.ScaleKeys.IsValidIndex(PoseIndex))
			{
				theSourceBoneTransform.SetScale3D(theSourceBoneRawTrack.ScaleKeys[PoseIndex]);
			}
#endif
			// Calculate the curve key time
			const float theCurveKeyTime = (PoseIndex == (theSourceNumberFrames - 1))
				? theSourceAssetLength : FMath::Clamp(PoseIndex * theSourceFrameRate, 0.f, theSourceAssetLength);

			// Add the transform curve key
			theSourceBoneTrackData.PosesTime.Add(theCurveKeyTime);
			theSourceBoneTrackData.PosesTransform.Add(theSourceBoneTransform);
		}

		// Resize the bone poses curve
		FRawAnimSequenceTrack theNewBoneRawTrack = FRawAnimSequenceTrack();

		// Resize the source bone poses
		for (float& thePoseTime : theSourceBoneTrackData.PosesTime)
		{
			thePoseTime *= theResizeScale;
		}

		// Bake new animation asset bone poses data
		for (int32 PoseIndex = 0; PoseIndex < theNewNumberFrames; PoseIndex++)
		{
			// Calculate the curve key time
			const float theCurveKeyTime = (PoseIndex == (theNewNumberFrames - 1))
				? theNewAssetLength : FMath::Clamp(PoseIndex * theResizeFrameRate, 0.f, theNewAssetLength);

			// Find nearest pose time
			for (int32 SourcePoseIndex = 0; SourcePoseIndex < theSourceBoneTrackData.PosesTime.Num(); SourcePoseIndex++)
			{
				// Get the current pose data
				const float& thePoseTime = theSourceBoneTrackData.PosesTime[SourcePoseIndex];
				const FTransform& thePoseTransform = theSourceBoneTrackData.PosesTransform[SourcePoseIndex];

				// Define the blend pose transform
				FTransform theBlendPoseTransform = thePoseTransform;

				// Check the two pose time range
				if (theSourceBoneTrackData.PosesTime.IsValidIndex(SourcePoseIndex + 1))
				{
					const float& theNextPoseTime = theSourceBoneTrackData.PosesTime[SourcePoseIndex + 1];
					const FTransform& theNextPoseTransform = theSourceBoneTrackData.PosesTransform[SourcePoseIndex + 1];

					// If is two pose time range, should baked the pose transform
					if (FMath::IsWithinInclusive(theCurveKeyTime, thePoseTime, theNextPoseTime))
					{
						const float theBlendWeight = (theCurveKeyTime - thePoseTime) / (theNextPoseTime - thePoseTime);
						theBlendPoseTransform.Blend(thePoseTransform, theNextPoseTransform, theBlendWeight);
					}
					else
					{
						continue;
					}
				}
				else if (theCurveKeyTime < thePoseTime)
				{
					continue;
				}

				UVAT_Library::AddBoneTrackKey(theNewBoneRawTrack, theBlendPoseTransform);
				break;
			}
		}
#if 0
		// Each every position keys
		for (int32 PoseIndex = 0; PoseIndex < theSourceBoneRawTrack.PosKeys.Num(); PoseIndex = PoseIndex + theIterateCount)
		{
			// Cache the current pose position key
			const FVector thePoseKey = theSourceBoneRawTrack.PosKeys[PoseIndex];
			theNewBoneRawTrack.PosKeys.Add(thePoseKey);

			// Intermediate poses will be blended based on the number of inserted poses
			if (theInsertCount > 0 && theSourceBoneRawTrack.PosKeys.IsValidIndex(PoseIndex + 1))
			{
				// Get the next pose position key
				const FVector theNextPoseKey = theSourceBoneRawTrack.PosKeys[PoseIndex + 1];

				// Insert the poses
				for (int32 InsertIndex = 1; InsertIndex < theInsertCount; InsertIndex++)
				{
					const float theWeight = float(InsertIndex) / float(theInsertCount);
					theNewBoneRawTrack.PosKeys.Add(FMath::Lerp(thePoseKey, theNextPoseKey, theWeight));
				}
			}
		}

		// Each every rotation keys
		for (int32 PoseIndex = 0; PoseIndex < theSourceBoneRawTrack.RotKeys.Num(); PoseIndex = PoseIndex + theIterateCount)
		{
			// Cache the current pose rotation key
			const FQuat thePoseKey = theSourceBoneRawTrack.RotKeys[PoseIndex];
			theNewBoneRawTrack.RotKeys.Add(thePoseKey);

			// Intermediate poses will be blended based on the number of inserted poses
			if (theInsertCount > 0 && theSourceBoneRawTrack.RotKeys.IsValidIndex(PoseIndex + 1))
			{
				// Get the next pose rotation key
				const FQuat theNextPoseKey = theSourceBoneRawTrack.RotKeys[PoseIndex + 1];

				// Insert the poses
				for (int32 InsertIndex = 1; InsertIndex < theInsertCount; InsertIndex++)
				{
					const float theWeight = float(InsertIndex) / float(theInsertCount);
					theNewBoneRawTrack.RotKeys.Add(FMath::Lerp(thePoseKey, theNextPoseKey, theWeight));
				}
			}
		}
#endif

		// If the play rate is < 0, we should reverse the bone tracks data
		if (InAssetResizeData.bEnablePlayRateModifier && InAssetResizeData.PlayRate < 0.f)
		{
			FRawAnimSequenceTrack thReverseoneRawTrack = FRawAnimSequenceTrack();

			// Each every track key
			for (int32 i = theNewBoneRawTrack.PosKeys.Num() - 1; i >= 0; i--)
			{
				thReverseoneRawTrack.PosKeys.Add(theNewBoneRawTrack.PosKeys[i]);
			}
			for (int32 i = theNewBoneRawTrack.RotKeys.Num() - 1; i >= 0; i--)
			{
				thReverseoneRawTrack.RotKeys.Add(theNewBoneRawTrack.RotKeys[i]);
			}
			for (int32 i = theNewBoneRawTrack.ScaleKeys.Num() - 1; i >= 0; i--)
			{
				thReverseoneRawTrack.ScaleKeys.Add(theNewBoneRawTrack.ScaleKeys[i]);
			}

			// Apply
			UVAT_Library::SetBoneTrackData(InAnimSequence, theTrackNames[TrackIndex], &thReverseoneRawTrack);
			continue;
		}

		// Apply
		UVAT_Library::SetBoneTrackData(InAnimSequence, theTrackNames[TrackIndex], &theNewBoneRawTrack);
	}

	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Asset::ReplaceAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetReplaceData& InAssetReplaceData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Check the replace data is valid
	if (InAssetReplaceData.SourceFrames.Num() == 0 || InAssetReplaceData.CopyFrames.Num() == 0)
	{
		return;
	}

	GEngine->BeginTransaction(*FString("ReplaceAnimationAsset")
		, FText(NSLOCTEXT("ReplaceAnimationAsset", "Edit Animation Asset Bone Track", "Edit Animation Asset Bone Track")), InAnimSequence);

	// Check the replace animation asset is valid
	UAnimSequence* theTargetAsset = InAssetReplaceData.ReferenceAnimSequence;
	if (theTargetAsset == nullptr)
	{
		theTargetAsset = InAnimSequence;
	}

	// Get the reference skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Get the animation basic frame time, need to handle frame rates of different animations
	const float theSampleRate = UVAT_Library::GetFrameTime(InAnimSequence);

	// Cache the source frames number
	const int32 theSourceNumberFrames = UVAT_Library::GetNumberOfFrames(InAnimSequence);
	const int32 theTargetNumberFrames = UVAT_Library::GetNumberOfFrames(theTargetAsset);

	// Each very mesh bone info
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// Check the bone track data is valid
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);
		const int32& theBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theBoneName);
		if (theBoneTrackIndex == INDEX_NONE)
		{
			continue;
		}

		// Ignore desired bone
		if (theBoneName == theRootBoneName || InAssetReplaceData.IgnoreBones.Contains(FBoneReference(theBoneName)))
		{
			continue;
		}

		// Get the source asset track data
		FRawAnimSequenceTrack theSourceRawTrack;
		if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theBoneName, theSourceRawTrack, true))
		{
			return;
		}

		// Each every replace frames
		for (int32 i = 0; i < InAssetReplaceData.SourceFrames.Num(); i++)
		{
			// Get replace pose frame
			const int32 theSourceFrame = FMath::Clamp(InAssetReplaceData.SourceFrames[i], 0, theSourceNumberFrames);

			// Get target pose frame
			int32 theTargetFrame = InAssetReplaceData.CopyFrames.IsValidIndex(i) ? InAssetReplaceData.CopyFrames[i] : InAssetReplaceData.CopyFrames[0];
			theTargetFrame = FMath::Clamp(theTargetFrame, 0, theTargetNumberFrames);

			// Get target bone transform data
			FTransform theTargetBoneTransformLS = UVAT_Bone::GetBoneTransformLS(theTargetAsset, false, theTargetFrame, theBoneName);

			// Replace the transform
			UVAT_Library::ModifyBoneTrackKey(theSourceRawTrack, theTargetBoneTransformLS, theSourceFrame);
		}

		// Apply the bone track data
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theSourceRawTrack, true);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	GEngine->EndTransaction();
}

void UVAT_Asset::OnCompositesAnimationAsset(UAnimSequence* InAnimSequence, TArray<FVirtualAssetCompositeData>& InAssetCompositesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the reference skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Get the animation basic frame time, need to handle frame rates of different animations
	const float theSampleRate = UVAT_Library::GetFrameTime(InAnimSequence);
	 
	 // Cache the source frames number
	const int32 theSourceNumberFrames = UVAT_Library::GetNumberOfFrames(InAnimSequence);
	int32 theNewNumberFrames = theSourceNumberFrames;

	// Define the bone tracks data
	TMap<FName, FRawAnimSequenceTrack> theBoneTracksMap;

	// Each very mesh bone info
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// Check the bone track data is valid
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);
		const int32& theBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theBoneName);
		if (theBoneTrackIndex == INDEX_NONE)
		{
			continue;
		}

		// Get the source animation asset bone track data
		FRawAnimSequenceTrack theCompositeRawTrack;
		UVAT_Library::GetBoneTrackData(InAnimSequence, theBoneName, theCompositeRawTrack, true);

		// Add the bone default track data
		if (theCompositeRawTrack.PosKeys.Num() == 0 || theCompositeRawTrack.RotKeys.Num() == 0)
		{
			// Add the bone default track
			const FTransform theSkeletonTransformLS = UVAT_Retarget::GetSkeletonBoneTransformLS(theSkeleton, theBoneName);
			UVAT_Library::AddBoneTrackKey(theCompositeRawTrack, theSkeletonTransformLS);
		}

		// Each every composite animation asset
		for (FVirtualAssetCompositeData& theCompositeData : InAssetCompositesData)
		{
			// Check the composite asset is valid
			if (theCompositeData.AnimSequence == nullptr)
			{
				continue;
			}

			// Get the last frame root motion transform
#if ENGINE_MAJOR_VERSION > 4
			const FTransform theRootMotionTransform = FTransform(FQuat(theCompositeRawTrack.RotKeys.Last()), FVector(theCompositeRawTrack.PosKeys.Last()));
#else
			const FTransform theRootMotionTransform = FTransform(theCompositeRawTrack.RotKeys.Last(), theCompositeRawTrack.PosKeys.Last());
#endif
			// Get the composite asset raw tack
			FRawAnimSequenceTrack theAssetRawTrack;
			UVAT_Library::GetBoneTrackData(theCompositeData.AnimSequence, theBoneName, theAssetRawTrack, true);

			// Add the bone default track data
			if (theAssetRawTrack.PosKeys.Num() == 0 || theAssetRawTrack.RotKeys.Num() == 0)
			{
				// Add the bone default track
				const FTransform theSkeletonTransformLS = UVAT_Retarget::GetSkeletonBoneTransformLS(theSkeleton, theBoneName);
				UVAT_Library::AddBoneTrackKey(theAssetRawTrack, theSkeletonTransformLS);
			}

#if 0
			// If is root bone, we should convert to last pose frame space
			if (theBoneName == theRootBoneName)
			{
				for (FVector& theKey : theAssetRawTrack.PosKeys)
				{
					theKey = theRootMotionTransform.GetRotation().RotateVector(theKey);
				}
				for (FQuat& theKey : theAssetRawTrack.RotKeys)
				{
					theKey *= theRootMotionTransform.GetRotation();
					FRotator test1 = theRootMotionTransform.Rotator();
					FRotator test2 = theKey.Rotator();
					FRotator test3 = theKey.Rotator();
				}
			}
#endif
			// Blend two poses
			theCompositeData.BlendIn.Reset();
			FRawAnimSequenceTrack theBlendRawTrack;
			const float& theBlendTime = theCompositeData.BlendIn.GetBlendTime();
			if (theBlendTime > 0.f)
			{
				// Calculate the blend pose number
				const int32 theBlendPoseNumber = int32(theBlendTime / theSampleRate);
				const int32 theLastPoseKeyIndex = theCompositeRawTrack.PosKeys.Num() /*- 1 */- theBlendPoseNumber;

				// Choose blend mode
				if (theCompositeData.bBlendLastAsset
					&& theCompositeRawTrack.PosKeys.IsValidIndex(theLastPoseKeyIndex)
					&& theCompositeRawTrack.RotKeys.IsValidIndex(theLastPoseKeyIndex))
				{
					// Get the last frame root motion transform
#if ENGINE_MAJOR_VERSION > 4
					const FTransform theLastBonePose(FQuat(theCompositeRawTrack.RotKeys[theLastPoseKeyIndex])
						, FVector(theCompositeRawTrack.PosKeys[theLastPoseKeyIndex])
						, theCompositeRawTrack.ScaleKeys.IsValidIndex(theLastPoseKeyIndex) ? FVector(theCompositeRawTrack.ScaleKeys[theLastPoseKeyIndex]) : FVector::OneVector);
#else
					const FTransform theLastBonePose(theCompositeRawTrack.RotKeys[theLastPoseKeyIndex]
						, theCompositeRawTrack.PosKeys[theLastPoseKeyIndex]
						, theCompositeRawTrack.ScaleKeys.IsValidIndex(theLastPoseKeyIndex) ? theCompositeRawTrack.ScaleKeys[theLastPoseKeyIndex] : FVector::OneVector);
#endif

					// Evaluate the blend end bone pose
					const int32 theStartCompositeFrame = theCompositeData.FrameRange.X > 0.f ? int32(theCompositeData.FrameRange.X) : 0;
					FTransform theBlendEndBonePose
						= UVAT_Bone::GetBoneTransformLS(theCompositeData.AnimSequence, false, theStartCompositeFrame, theBoneName);

					// Sample the pose features
					int32 thePosesNumber = 0;
					float thePoseSampleTime = theSampleRate;
					theCompositeData.BlendIn.Update(theSampleRate);
					while (thePoseSampleTime <= (theBlendTime + 0.0001f))
					{
						// Clamp the pose time
						thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theBlendTime);

						// Calculate the blend weight
						const float theBlendWeight = theCompositeData.BlendIn.GetBlendedValue();

						// Blend the weight
						FTransform theBlendBonePose = theLastBonePose;
						theBlendBonePose.BlendWith(theBlendEndBonePose, theBlendWeight);
						UVAT_Library::AddBoneTrackKey(theBlendRawTrack, theBlendBonePose);

						// Replace the asset frame pose
						const int32 theReplaceFrame = theLastPoseKeyIndex + thePosesNumber;
						if (theCompositeRawTrack.PosKeys.IsValidIndex(theReplaceFrame))
						{
							theCompositeRawTrack.PosKeys[theReplaceFrame] = theBlendRawTrack.PosKeys.Last();
						}
						if (theCompositeRawTrack.RotKeys.IsValidIndex(theReplaceFrame))
						{
							theCompositeRawTrack.RotKeys[theReplaceFrame] = theBlendRawTrack.RotKeys.Last();
						}
						if (theCompositeRawTrack.ScaleKeys.IsValidIndex(theReplaceFrame))
						{
							theCompositeRawTrack.ScaleKeys[theReplaceFrame] = theBlendRawTrack.ScaleKeys.Last();
						}

						// Iterate
						++thePosesNumber;
						thePoseSampleTime += theSampleRate;
						theCompositeData.BlendIn.Update(theSampleRate);
					}
				}
				else
				{
#if ENGINE_MAJOR_VERSION > 4
					const FTransform theLastBonePose(FQuat(theCompositeRawTrack.RotKeys[theLastPoseKeyIndex])
						, FVector(theCompositeRawTrack.PosKeys[theLastPoseKeyIndex])
						, theCompositeRawTrack.ScaleKeys.IsValidIndex(theLastPoseKeyIndex) ? FVector(theCompositeRawTrack.ScaleKeys[theLastPoseKeyIndex]) : FVector::OneVector);
#else
					const FTransform theLastBonePose(theCompositeRawTrack.RotKeys[theLastPoseKeyIndex]
						, theCompositeRawTrack.PosKeys[theLastPoseKeyIndex]
						, theCompositeRawTrack.ScaleKeys.IsValidIndex(theLastPoseKeyIndex) ? theCompositeRawTrack.ScaleKeys[theLastPoseKeyIndex] : FVector::OneVector);
#endif
					// Evaluate the blend end bone pose
					const int32 theStartCompositeFrame = theCompositeData.FrameRange.X > 0.f ? int32(theCompositeData.FrameRange.X) : 0;
					FTransform theBlendEndBonePose
						= UVAT_Bone::GetBoneTransformLS(theCompositeData.AnimSequence, false, theStartCompositeFrame + theBlendPoseNumber, theBoneName);

#if 0
					// If is root bone, we should convert to last pose frame space
					if (theBoneName == theRootBoneName)
					{
						theBlendEndBonePose.SetLocation(theRootMotionTransform.GetRotation().RotateVector(theBlendEndBonePose.GetLocation()));
						theBlendEndBonePose.SetRotation(theBlendEndBonePose.GetRotation() * theRootMotionTransform.GetRotation());
					}
#endif

					// Sample the pose features
					int32 thePosesNumber = 0;
					float thePoseSampleTime = theSampleRate;
					theCompositeData.BlendIn.Update(theSampleRate);
					while (thePoseSampleTime <= (theBlendTime + 0.0001f))
					{
						// Clamp the pose time
						thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theBlendTime);

						// Calculate the blend weight
#if 1
						const float theBlendWeight = theCompositeData.BlendIn.GetBlendedValue();
#else
						const float theBlendWeight = thePoseSampleTime / theBlendTime;
#endif

						// Blend the weight
						FTransform theBlendBonePose = theLastBonePose;
						theBlendBonePose.BlendWith(theBlendEndBonePose, theBlendWeight);
						UVAT_Library::AddBoneTrackKey(theBlendRawTrack, theBlendBonePose);

						// Replace the asset frame pose
						const int32 theReplaceFrame = theStartCompositeFrame + thePosesNumber;
						if (theAssetRawTrack.PosKeys.IsValidIndex(theReplaceFrame))
						{
							theAssetRawTrack.PosKeys[theReplaceFrame] = theBlendRawTrack.PosKeys.Last();
						}
						if (theAssetRawTrack.RotKeys.IsValidIndex(theReplaceFrame))
						{
							theAssetRawTrack.RotKeys[theReplaceFrame] = theBlendRawTrack.RotKeys.Last();
						}
						if (theAssetRawTrack.ScaleKeys.IsValidIndex(theReplaceFrame))
						{
							theAssetRawTrack.ScaleKeys[theReplaceFrame] = theBlendRawTrack.ScaleKeys.Last();
						}

						// Iterate
						++thePosesNumber;
						thePoseSampleTime += theSampleRate;
						theCompositeData.BlendIn.Update(theSampleRate);
					}
				}
				// Append blend raw track
				//AppendRawTrack(theBoneTrackIndex == 0, theCompositeRawTrack, theBlendRawTrack, thePosesNumber);
			}

			// Append all composite asset raw track
			AppendRawTrack(theBoneTrackIndex == 0, theCompositeRawTrack, theAssetRawTrack
				, UVAT_Library::GetNumberOfFrames(theCompositeData.AnimSequence), theCompositeData.FrameRange);
		}

		// Cache the bone track data
		theBoneTracksMap.Add(theBoneName, theCompositeRawTrack);
	}

	// Calculate the composite asset frames number
	for (FVirtualAssetCompositeData& theCompositeData : InAssetCompositesData)
	{
		// Check the composite asset is valid
		if (theCompositeData.AnimSequence == nullptr)
		{
			continue;
		}

		// Get the animation asset frames number
		const int32 theNumberFrame = UVAT_Library::GetNumberOfFrames(theCompositeData.AnimSequence);

		// Calculate the valid composite frames range
		int32 theAppendStartFrame, theAppendEndFrame;
		GetValidFrameRange(UVAT_Library::GetNumberOfFrames(theCompositeData.AnimSequence), theCompositeData.FrameRange, theAppendStartFrame, theAppendEndFrame);
	
		// Merge the valid frames number
		theNewNumberFrames += (theAppendEndFrame - theAppendStartFrame);
	}

	// Calculate the new animation asset length
	const float theNewAnimLength = theSampleRate * float(theNewNumberFrames);

	// Resize the modifier animation asset
#if ENGINE_MAJOR_VERSION > 4
	InAnimSequence->GetController().Resize(theNewAnimLength, 0, theNewAnimLength);
#else
	InAnimSequence->ResizeSequence(theNewAnimLength, theNewNumberFrames, false, 0, theSourceNumberFrames);
#endif

	// Resize all bones track data
	for (TPair<FName, FRawAnimSequenceTrack>& theBoneTrackPair : theBoneTracksMap)
	{
		// Check the bone track is valid
		FRawAnimSequenceTrack& theBoneTrack = theBoneTrackPair.Value;
		if (theBoneTrack.PosKeys.Num() == 0 && theBoneTrack.RotKeys.Num() == 0)
		{
			continue;
		}

		// Apply the bone track data
		check(theBoneTrack.PosKeys.Num() != 0 || theBoneTrack.RotKeys.Num() != 0)
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneTrackPair.Key, &theBoneTrack, true);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

UAnimSequence* UVAT_Asset::RebuildAnimationAsset(UAnimSequence* InAnimSequence, EVirtualToolsRebuildAssetType InRebuildAssetType, const FName* InSuffixName)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return nullptr;
	}

	// Ignore override rebuild asset model
	if (InRebuildAssetType == EVirtualToolsRebuildAssetType::VTAT_Override)
	{
		return InAnimSequence;
	}

	// Define the local variables
	FString thePackageString;
	FString theAnimationAssetName; 
	const FString theSuffixString = InSuffixName == nullptr ? FString() : InSuffixName->ToString();

	// Get the asset tools module
	FAssetToolsModule& theAssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

	// Create unique asset name
	theAssetToolsModule.Get().CreateUniqueAssetName(InAnimSequence->GetOutermost()->GetName(), theSuffixString, thePackageString, theAnimationAssetName);
	
	// Get the package path
	const FString thePackagePath = FPackageName::GetLongPackagePath(thePackageString);

	// Duplicate the animation asset
	UObject* theDuplicatedAsset = theAssetToolsModule.Get().DuplicateAsset(theAnimationAssetName, thePackagePath, InAnimSequence);
	if (theDuplicatedAsset == NULL)
	{
		return nullptr;
	}

	// Convert the animation asset to animation sequence
	UAnimSequence* theDuplicatedAnimSequence = Cast<UAnimSequence>(theDuplicatedAsset);

	// If rebuild mode is create asset, we will clear cached data
	if (InRebuildAssetType == EVirtualToolsRebuildAssetType::VTAT_CreateAsset)
	{
#if ENGINE_MAJOR_VERSION > 4
		UAnimDataModel* theDataModel = theDuplicatedAnimSequence->GetDataModel();
		UAnimationBlueprintLibrary::RemoveAllCurveData(theDuplicatedAnimSequence);
#else
		theDuplicatedAnimSequence->RawCurveData.Empty();
		theDuplicatedAnimSequence->ClearCompressedBoneData();
		theDuplicatedAnimSequence->ClearCompressedCurveData();
#endif
		theDuplicatedAnimSequence->Notifies.Empty();
		theDuplicatedAnimSequence->AnimNotifyTracks.Empty();
	}

	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(theDuplicatedAnimSequence);

	// Success
	return theDuplicatedAnimSequence;
}
#pragma endregion


#pragma region Root Motion
FTransform UVAT_Asset::GetRootMotionApexTransform(UAnimSequence* InAnimSequence, float& OutMaxSpeed)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		OutMaxSpeed = 0.f;
		return FTransform().Identity;
	}

	// Get the animation asset data
	const float& theAnimLength = InAnimSequence->GetPlayLength();
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Define the local variables
	TArray<float> ApexSpeed;
	TArray<float> ApexTranslationsX, ApexTranslationsY, ApexTranslationsZ;
	TArray<float> ApexRotationsPitch, ApexRotationsYaw, ApexRotationsRoll;
	FTransform theMotionApexTransform, theAccumulateMotionTransform;

	// Each every keys
	for (int32 KeyIndex = 1; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Get the last key index
		const int32 theLastKeyIndex = KeyIndex - 1;
	
		// Evaluate the keys time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);
		const float theLastKeyTime = InAnimSequence->GetTimeAtFrame(theLastKeyIndex);
		const float theDeltaTime = theKeyTime - theLastKeyTime;

		// Calculate the root motion transform data
		const FTransform thePreviousMotionTransform = OnExtractRootMotionFromRange(InAnimSequence, 0.f, theLastKeyTime);
		const FTransform theCurrentMotionTransform = OnExtractRootMotionFromRange(InAnimSequence, 0.f, theKeyTime);
		const FTransform theDeltaMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentMotionTransform, thePreviousMotionTransform);
		
		// Calculate the motion speed
		const float theMotionSpeed = (theDeltaMotionTransform.GetTranslation() / theDeltaTime).Size();
		ApexSpeed.Add(theMotionSpeed);

		// Calculate the accumulate motion transform
		theAccumulateMotionTransform = UVAT_Library::Add_TransformTransform(theAccumulateMotionTransform, theDeltaMotionTransform);

		// Cache the translation data
		ApexTranslationsX.Add(FMath::Abs(theAccumulateMotionTransform.GetLocation().X));
		ApexTranslationsY.Add(FMath::Abs(theAccumulateMotionTransform.GetLocation().Y));
		ApexTranslationsZ.Add(FMath::Abs(theAccumulateMotionTransform.GetLocation().Z));

		// Cache the rotation data
		ApexRotationsRoll.Add(FMath::Abs(theAccumulateMotionTransform.Rotator().Roll));
		ApexRotationsPitch.Add(FMath::Abs(theAccumulateMotionTransform.Rotator().Pitch));
		ApexRotationsYaw.Add(FMath::Abs(theAccumulateMotionTransform.Rotator().Yaw));
	}

	// Output the max speed
	OutMaxSpeed = FMath::Max(ApexSpeed);

	// Calculate the apex rotation
	FRotator theApexRotation;
	theApexRotation.Pitch = FMath::Max(ApexRotationsPitch);
	theApexRotation.Yaw = FMath::Max(ApexRotationsYaw);
	theApexRotation.Roll = FMath::Max(ApexRotationsRoll);

	// Calculate the apex translation
	FVector theApexTranslation;
	theApexTranslation.X = FMath::Max(ApexTranslationsX);
	theApexTranslation.Y = FMath::Max(ApexTranslationsY);
	theApexTranslation.Z = FMath::Max(ApexTranslationsZ);

	// Convert to transform
	theMotionApexTransform.SetLocation(theApexTranslation);
	theMotionApexTransform.SetRotation(theApexRotation.Quaternion());
	theMotionApexTransform.NormalizeRotation();
	return theMotionApexTransform;
}

FTransform UVAT_Asset::OnExtractRootMotion(UAnimSequence* InAnimSequence, const float& InStartTime, const float& InDeltaTime, const bool& InLooping)
{
#if 0 // This method will export invalid data when there is no compressed bone data
	if (InAnimSequence && InStartTime >= 0.f)
	{
		FTransform theRootMotionTransfrom = InAnimSequence->ExtractRootMotion(InStartTime, InDeltaTime, InLooping);
		return theRootMotionTransfrom;
	}
#else
	// Check the animation asset is valid
	if (InAnimSequence->GetSkeleton() == nullptr)
	{
		return InAnimSequence->ExtractRootMotion(InStartTime, InDeltaTime, InLooping);
	}

	// Get the root bone name
	const FName& theRootBoneName = InAnimSequence->GetSkeleton()->GetReferenceSkeleton().GetBoneName(0);

	// Calculate the root motion transform data
	FTransform thePreviousMotionTransform, theCurrentMotionTransform;
	UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, theRootBoneName, InStartTime, true, thePreviousMotionTransform);
	UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, theRootBoneName, InStartTime + InDeltaTime, true, theCurrentMotionTransform);
	return UVAT_Library::Subtract_TransformTransform(theCurrentMotionTransform, thePreviousMotionTransform);
#endif
}

FTransform UVAT_Asset::OnExtractRootMotionFromRange(UAnimSequence* InAnimSequence, const float& InStartTrackPosition, const float& InEndTrackPosition)
{
#if 0 // This method will export invalid data when there is no compressed bone data
	if (InAnimSequence && InStartTrackPosition >= 0.f && InEndTrackPosition >= 0.f)
	{
		FTransform theRootMotionTransfrom = InAnimSequence->ExtractRootMotionFromRange(InStartTrackPosition, InEndTrackPosition);
		UVAT_Library::NormailizeTransform(theRootMotionTransfrom);
		return theRootMotionTransfrom;
	}
#else
	// Check the animation asset is valid
	if (InAnimSequence->GetSkeleton() == nullptr)
	{
		return InAnimSequence->ExtractRootMotionFromRange(InStartTrackPosition, InEndTrackPosition);
	}

	// Get the root bone name
	const FName& theRootBoneName = InAnimSequence->GetSkeleton()->GetReferenceSkeleton().GetBoneName(0);

	// Calculate the root motion transform data
	FTransform thePreviousMotionTransform, theCurrentMotionTransform;
	UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, theRootBoneName, InStartTrackPosition, true, thePreviousMotionTransform);
	UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, theRootBoneName, InEndTrackPosition, true, theCurrentMotionTransform);
	return UVAT_Library::Subtract_TransformTransform(theCurrentMotionTransform, thePreviousMotionTransform);
#endif
}
#pragma endregion


#pragma region Motion Capture
void UVAT_Asset::SampleMotionCurvesData(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData, FVirtualBoneCurvesData& OutMotionCurvesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Check the root bone is valid
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (theRootBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Check the motion bone is valid
	const FName& theMotionBoneName = InMotionCaptureData.MotionBone.BoneName;
	const int32& theMotionBoneIndex = theReferenceSkeleton.FindBoneIndex(theMotionBoneName);
	const int32& theMotionBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionBoneIndex == INDEX_NONE || theMotionBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Get the motion bone raw track data
	FRawAnimSequenceTrack theMotionBoneRawTrack;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theMotionBoneName, theMotionBoneRawTrack))
	{
		return;
	}

	// Clear the cached motion curves data
	OutMotionCurvesData.Reset();

	// Get the transform data of the initial pose of the motion bone
	FTransform theMotionTransformInitLS;
	const FTransform theMotionReferencePoseTransform = theReferenceSkeleton.GetRefBonePose()[theMotionBoneIndex];
	theMotionTransformInitLS.SetTranslation(FVector(0.f, 0.f, theMotionReferencePoseTransform.GetTranslation().Z));
	theMotionTransformInitLS.SetRotation(theMotionReferencePoseTransform.GetRotation());

	// Each every animation asset track keys
	for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Get the motion bone transform of this frame
		FTransform theMotionBoneTransformLS;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theMotionBoneName, KeyIndex, false, theMotionBoneTransformLS);

		// Get the motion bone delta transform of first key
		FTransform theDeltaMotionTransformLS = UVAT_Library::Subtract_TransformTransform(theMotionBoneTransformLS, theMotionTransformInitLS);

		// Calculate the convert translation
		FVector theConvertTranslation = FVector::ZeroVector;
		if (theMotionBoneRawTrack.PosKeys.IsValidIndex(KeyIndex))
		{
			for (const TPair<TEnumAsByte<EAxis::Type>, float>& theWeightPair : InMotionCaptureData.TranslationWeightsMap)
			{
				// Calculate the convert weight
				const float& theConvertWeight = InMotionCaptureData.GlobalWeight * theWeightPair.Value;

				// Handle convert axis weight
				switch (theWeightPair.Key)
				{
				case EAxis::X:
					theConvertTranslation.X = theDeltaMotionTransformLS.GetTranslation().X * theConvertWeight;
					break;

				case EAxis::Y:
					theConvertTranslation.Y = theDeltaMotionTransformLS.GetTranslation().Y * theConvertWeight;
					break;

				case EAxis::Z:
					theConvertTranslation.Z = theDeltaMotionTransformLS.GetTranslation().Z * theConvertWeight;
					break;
				}
			}
		}

		// Calculate the convert rotation
		FRotator theConvertRotation = FRotator::ZeroRotator;
		if (theMotionBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			for (const TPair<TEnumAsByte<EAxis::Type>, float>& theWeightPair : InMotionCaptureData.RotationWeightsMap)
			{
				// Calculate the convert weight
				const float& theConvertWeight = InMotionCaptureData.GlobalWeight * theWeightPair.Value;

				// Convert the Quat to rotation
				const FRotator theDeltaRotation = theDeltaMotionTransformLS.GetRotation().Rotator();

				// Handle convert axis weight
				switch (theWeightPair.Key)
				{
				case EAxis::X:
					theConvertRotation.Roll = theDeltaRotation.Roll * theConvertWeight;
					break;

				case EAxis::Y:
					theConvertRotation.Pitch = theDeltaRotation.Pitch * theConvertWeight;
					break;

				case EAxis::Z:
					theConvertRotation.Yaw = theDeltaRotation.Yaw * theConvertWeight;
					break;
				}
			}
		}

		// Add the convert motion transform curve data for editor preview
		FTransform theConvertTransform(theConvertRotation, theConvertTranslation);
		OutMotionCurvesData.AddTransformKey(theKeyTime, theConvertTransform);
	}

	// Cleanup the curves data
	//OutMotionCurvesData.Cleanup();

	// Resize the curves data
	OutMotionCurvesData.Resize();

	// Success
	return;
}

void UVAT_Asset::SampleRootMotionCurvesData(UAnimSequence* InAnimSequence, FVirtualBoneCurvesData& OutMotionCurvesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Check the root bone is valid
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (theRootBoneTrackIndex == INDEX_NONE)
	{
		return;
	}	

	// Clear the cached motion curves data
	OutMotionCurvesData.Reset();

	// Each every animation asset track keys
	for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Get the root bone transform of this frame
		FTransform theRootBoneTransformLS;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theRootBoneName, KeyIndex, true, theRootBoneTransformLS);

		// Output the transform key
		OutMotionCurvesData.AddTransformKey(theKeyTime, theRootBoneTransformLS);
	}

	// Cleanup the curves data
	OutMotionCurvesData.Cleanup();

	// Resize the curves data
	OutMotionCurvesData.Resize();

	// Success
	return;
}

void UVAT_Asset::ConvertRootMotionData(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData
	, const FVirtualMotionConvertData& InConvertData, FVirtualBoneCurvesData& InMotionCurvesData, FVirtualCurveAttributes& InCurveAttributes)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InAnimSequence->GetSkeleton()->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theSourceNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Get the root bone track data
	FRawAnimSequenceTrack theSourceRootBoneRawTrack;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theRootBoneName, theSourceRootBoneRawTrack))
	{
		return;
	}

	// Get the motion bone track data
	FRawAnimSequenceTrack theSourceMotionBoneRawTrack;
	const bool bHasMotionBoneData = UVAT_Library::GetBoneTrackData(InAnimSequence, InMotionCaptureData.MotionBone.BoneName, theSourceMotionBoneRawTrack);

	// Define new bone raw track data
	FRawAnimSequenceTrack theNewRootBoneRawTrack;

	// Check the curve data is valid
	if (!InMotionCurvesData.HasCurveData())
	{
		SampleRootMotionCurvesData(InAnimSequence, InMotionCurvesData);
	}

	// Each every track keys
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);
#if 1
		// Define the new root bone transform data
		FTransform theNewRootBoneTransform = FTransform::Identity;

		// Evaluate the transform key value
		const FTransform theSourceRootTransform = InMotionCurvesData.Evaluate(theKeyTime);

		// Add the translation ratio key
#if ENGINE_MAJOR_VERSION > 4
		theNewRootBoneRawTrack.PosKeys.Add(FVector3f(theSourceRootTransform.GetTranslation() * InConvertData.TranslationRatio));
		theNewRootBoneTransform.SetTranslation(FVector(theNewRootBoneRawTrack.PosKeys.Last()));
#else
		theNewRootBoneRawTrack.PosKeys.Add(theSourceRootTransform.GetTranslation() * InConvertData.TranslationRatio);
		theNewRootBoneTransform.SetTranslation(theNewRootBoneRawTrack.PosKeys.Last());
#endif
        // Add the rotation ratio key
		FRotator theAdjustRotation = theSourceRootTransform.Rotator();
		theAdjustRotation.Roll *= InConvertData.RotationRatio.X;
		theAdjustRotation.Pitch *= InConvertData.RotationRatio.Y;
		theAdjustRotation.Yaw *= InConvertData.RotationRatio.Z;
#if ENGINE_MAJOR_VERSION > 4
		theNewRootBoneRawTrack.RotKeys.Add(FQuat4f(theAdjustRotation.Quaternion()));
		theNewRootBoneTransform.SetRotation(FQuat(theNewRootBoneRawTrack.RotKeys.Last()));
#else
		theNewRootBoneRawTrack.RotKeys.Add(theAdjustRotation.Quaternion());
		theNewRootBoneTransform.SetRotation(theNewRootBoneRawTrack.RotKeys.Last());
#endif

		// Add the scale ratio key
		if (theSourceRootBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex))
		{
			// Get source scale key
#if ENGINE_MAJOR_VERSION > 4
			FVector theAdjustScale = FVector(theSourceRootBoneRawTrack.ScaleKeys[KeyIndex]);
#else
			FVector theAdjustScale = theSourceRootBoneRawTrack.ScaleKeys[KeyIndex];
#endif
			// Ratio
			theAdjustScale.X *= InConvertData.ScaleRatio.X;
			theAdjustScale.Y *= InConvertData.ScaleRatio.Y;
			theAdjustScale.Z *= InConvertData.ScaleRatio.Z;

			// Adjust the key
#if ENGINE_MAJOR_VERSION > 4
			theNewRootBoneRawTrack.ScaleKeys.Add(FVector3f(theAdjustScale));
			theNewRootBoneTransform.SetScale3D(FVector(theNewRootBoneRawTrack.ScaleKeys.Last()));
#else
			theNewRootBoneRawTrack.ScaleKeys.Add(theAdjustScale);
			theNewRootBoneTransform.SetScale3D(theNewRootBoneRawTrack.ScaleKeys.Last());
#endif
		}

		// If there is motion bone data, we will remove the relative motion data
		if (bHasMotionBoneData && InConvertData.bRemoveRelativeMotionData
			&& theSourceMotionBoneRawTrack.PosKeys.IsValidIndex(KeyIndex)
			&& theSourceMotionBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			// Calculate the delta motion transform
			FTransform theDeltaMotionTransform = UVAT_Library::Subtract_TransformTransform(theNewRootBoneTransform, theSourceRootTransform);

#if ENGINE_MAJOR_VERSION > 4
			// Convert the motion bone source local space transform to component space
			FTransform theSourceMotionBoneTransformCS(FQuat(theSourceMotionBoneRawTrack.RotKeys[KeyIndex])
				, FVector(theSourceMotionBoneRawTrack.PosKeys[KeyIndex])
				, theSourceMotionBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex) ? FVector(theSourceMotionBoneRawTrack.ScaleKeys[KeyIndex]) : FVector::OneVector);
#else
			// Convert the motion bone source local space transform to component space
			FTransform theSourceMotionBoneTransformCS(theSourceMotionBoneRawTrack.RotKeys[KeyIndex]
				, theSourceMotionBoneRawTrack.PosKeys[KeyIndex]
				, theSourceMotionBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex) ? theSourceMotionBoneRawTrack.ScaleKeys[KeyIndex] : FVector::OneVector);
#endif

			theSourceMotionBoneTransformCS *= theSourceRootTransform;
			theSourceMotionBoneTransformCS.NormalizeRotation();

			// Delta the adjust transform data
#if 1
			theSourceMotionBoneTransformCS.AddToTranslation(theDeltaMotionTransform.GetTranslation());
			FRotator theNewMotionRotation = theSourceMotionBoneTransformCS.Rotator();
			theNewMotionRotation += theDeltaMotionTransform.Rotator();
			theNewMotionRotation.Normalize();
			theSourceMotionBoneTransformCS.SetRotation(theNewMotionRotation.Quaternion());
#else
			theSourceMotionBoneTransformCS = UVAT_Library::Subtract_TransformTransform(theSourceMotionBoneTransformCS, theDeltaMotionTransform);
#endif

			// Convert the motion bone component space transform to new local space
			FTransform theNewMotionBoneTransformLS = theSourceMotionBoneTransformCS;
			theNewMotionBoneTransformLS.SetToRelativeTransform(theNewRootBoneTransform);
			theNewMotionBoneTransformLS.NormalizeRotation();

#if ENGINE_MAJOR_VERSION > 4
			theSourceMotionBoneRawTrack.RotKeys[KeyIndex] = FQuat4f(theNewMotionBoneTransformLS.GetRotation());
			theSourceMotionBoneRawTrack.PosKeys[KeyIndex] = FVector3f(theNewMotionBoneTransformLS.GetTranslation());
#else
			theSourceMotionBoneRawTrack.RotKeys[KeyIndex] = theNewMotionBoneTransformLS.GetRotation();
			theSourceMotionBoneRawTrack.PosKeys[KeyIndex] = theNewMotionBoneTransformLS.GetTranslation();
#endif
		}
#else
		// Convert translation
		if (theSourceRootBoneRawTrack.PosKeys.IsValidIndex(KeyIndex))
		{
			// Get source translation key
			FVector theAdjustTranslation = theSourceRootBoneRawTrack.PosKeys[KeyIndex];

			// Ratio
			theAdjustTranslation.X *= InConvertData.TranslationRatio.X;
			theAdjustTranslation.Y *= InConvertData.TranslationRatio.Y;
			theAdjustTranslation.Z *= InConvertData.TranslationRatio.Z;

			// Adjust the key
			theSourceRootBoneRawTrack.PosKeys[KeyIndex] = theAdjustTranslation;
		}

		// Convert rotation
		if (theSourceRootBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			// Get source rotation key
			FRotator theAdjustRotation = theSourceRootBoneRawTrack.RotKeys[KeyIndex].Rotator();

			// Ratio
			theAdjustRotation.Roll *= InConvertData.RotationRatio.X;
			theAdjustRotation.Pitch *= InConvertData.RotationRatio.Y;
			theAdjustRotation.Yaw *= InConvertData.RotationRatio.Z;

			// Adjust the key
			theSourceRootBoneRawTrack.RotKeys[KeyIndex] = theAdjustRotation.Quaternion();
		}

		// Convert scale
		if (theSourceRootBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex))
		{
			// Get source scale key
			FVector theAdjustScale = theSourceRootBoneRawTrack.ScaleKeys[KeyIndex];

			// Ratio
			theAdjustScale.X *= InConvertData.ScaleRatio.X;
			theAdjustScale.Y *= InConvertData.ScaleRatio.Y;
			theAdjustScale.Z *= InConvertData.ScaleRatio.Z;

			// Adjust the key
			theSourceRootBoneRawTrack.ScaleKeys[KeyIndex] = theAdjustScale;
		}
#endif
	}

	// Apply new bone track data
	UVAT_Library::SetBoneTrackData(InAnimSequence, theRootBoneName, &theNewRootBoneRawTrack);

	// Apply new bone track data
	if (bHasMotionBoneData && InConvertData.bRemoveRelativeMotionData)
	{
		UVAT_Library::SetBoneTrackData(InAnimSequence, InMotionCaptureData.MotionBone.BoneName, &theSourceMotionBoneRawTrack);
	}

	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);

	// Rebuild animation curves
	UVAT_Curve::RebuildAnimationRootMotionCurves(InAnimSequence, InCurveAttributes);
}

void UVAT_Asset::AlignmentRootMotionData(UAnimSequence* InAnimSequence, const FVirtualMotionAlignmentData& InMotionAlignmentData, FVirtualCurveAttributes& InCurveAttributes)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const float theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const int32 theSourceNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Check the root bone data is valid
	FRawAnimSequenceTrack theRootBoneRawTrackData, theAlignmentRootBoneRawTrackData;
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theRootBoneTrackIndex, theRootBoneRawTrackData))
	{
		return;
	}

	// Check the motion bone track is valid
	FRawAnimSequenceTrack theMotionBoneRawTrackData;
	const FName& theMotionBoneName = InMotionAlignmentData.MotionBone.BoneName;
	const int32 theMotionBoneIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionBoneIndex == INDEX_NONE)
	{
		return;
	}
	
	// Calculate line direction
	FVector theLineOrigin = FVector::ZeroVector;
	FVector theLineDirection = FVector::ZeroVector;
	{
		const FTransform pA = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, 0.f, theRootBoneName);
		const FTransform pB = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, theSourceAnimLength, theRootBoneName);
		theLineOrigin = pA.GetLocation();
		theLineDirection = (pB.GetLocation() - pA.GetLocation()).GetSafeNormal();
	}

	// Calculate the motion track trajectory
	bool bIsLineDirection = InMotionAlignmentData.bIsLineTrack;

#if 0
	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Get the root bone transform in the key
		const FTransform theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theRootBoneName);
		const FVector theRootBoneLocation = theRootBoneTransformCS.GetLocation();

		// Calculate the trajectory location
		const FVector theTrajectoryLoc = theLineOrigin + (theLineDirection * ((theRootBoneLocation - theLineOrigin) | theLineDirection));

		// Check the tolerance
		if (!theRootBoneLocation.Equals(theTrajectoryLoc, KINDA_SMALL_NUMBER))
		{
			bIsLineDirection = false;
			break;
		}
	}
#endif

	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Get the motion bone transform in the key
		FTransform theMotionBoneTransformLS = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, KeyIndex, theMotionBoneName);
		FTransform theMotionBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theMotionBoneName);

		// Get the root bone transform in the key
		const FTransform theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theRootBoneName);

		// Ignore key range
		const FVector2D& theIgnoreFrameRange = InMotionAlignmentData.IgnoreFrameRange;
		if (FMath::IsWithinInclusive(float(KeyIndex), theIgnoreFrameRange.X, theIgnoreFrameRange.Y)
			|| (theIgnoreFrameRange.X >= 0.f && KeyIndex >= theIgnoreFrameRange.X && theIgnoreFrameRange.Y < 0.f))
		{
			// Cache the root bone track key
			UVAT_Library::AddBoneTrackKey(theAlignmentRootBoneRawTrackData, theRootBoneTransformCS);

			// Cache the motion bone track key
			UVAT_Library::AddBoneTrackKey(theMotionBoneRawTrackData, theMotionBoneTransformLS);
			continue;
		}

		// Calculate new root bone transform
		FVector theAlignmentRootLocation = FVector(theMotionBoneTransformCS.GetLocation().X, theMotionBoneTransformCS.GetLocation().Y, theRootBoneTransformCS.GetLocation().Z);
		if (bIsLineDirection)
		{
			// Calculate the trajectory location
			theAlignmentRootLocation = theLineOrigin + (theLineDirection * ((theAlignmentRootLocation - theLineOrigin) | theLineDirection));
		}

		// Define the alignment transform data
		const FTransform theAlignmentRootTransformLS(theRootBoneTransformCS.GetRotation(), theAlignmentRootLocation, theRootBoneTransformCS.GetScale3D());

		// Calculate new motion bone transform
		FTransform theAlignmentMotionBoneTransformLS = theMotionBoneTransformCS;
		theAlignmentMotionBoneTransformLS.SetToRelativeTransform(theAlignmentRootTransformLS);

		// Cache the root bone track key
		UVAT_Library::AddBoneTrackKey(theAlignmentRootBoneRawTrackData, theAlignmentRootTransformLS);

		// Cache the motion bone track key
		theMotionBoneTransformLS.SetLocation(theAlignmentMotionBoneTransformLS.GetLocation());
		UVAT_Library::AddBoneTrackKey(theMotionBoneRawTrackData, theMotionBoneTransformLS);
	}

	// Apply the motion bone track changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theMotionBoneName, &theMotionBoneRawTrackData);

	// Apply the motion bone track changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theRootBoneName, &theAlignmentRootBoneRawTrackData);

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);

	// Rebuild animation curves
	UVAT_Curve::RebuildAnimationRootMotionCurves(InAnimSequence, InCurveAttributes);
}

void UVAT_Asset::AlignmentRootMotionCurvesData(UAnimSequence* InAnimSequence, const FVirtualMotionAlignmentData& InMotionAlignmentData, FVirtualBoneCurvesData& OutMotionCurvesData)
{
	// Check the curves data is valid
	if (!OutMotionCurvesData.HasCurveData())
	{
		return;
	}

	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const float theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const int32 theSourceNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Check the root bone data is valid
	FRawAnimSequenceTrack theRootBoneRawTrackData, theAlignmentRootBoneRawTrackData;
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theRootBoneTrackIndex, theRootBoneRawTrackData))
	{
		return;
	}

	// Check the motion bone track is valid
	FRawAnimSequenceTrack theMotionBoneRawTrackData;
	const FName& theMotionBoneName = InMotionAlignmentData.MotionBone.BoneName;
	const int32 theMotionBoneIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionBoneIndex == INDEX_NONE)
	{
		return;
	}

	// Calculate line direction
	FVector theLineOrigin = FVector::ZeroVector;
	FVector theLineDirection = FVector::ZeroVector;
	{
		const FTransform pA = OutMotionCurvesData.Evaluate(0.f);
		const FTransform pB = OutMotionCurvesData.Evaluate(theSourceAnimLength);
		theLineOrigin = pA.GetLocation();
		theLineDirection = (pB.GetLocation() - pA.GetLocation()).GetSafeNormal();
	}

	// Calculate the motion track trajectory
	bool bIsLineDirection = InMotionAlignmentData.bIsLineTrack;

#if 0
	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Get the root bone transform in the key
		const FTransform theRootBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theRootBoneName);
		const FVector theRootBoneLocation = theRootBoneTransformCS.GetLocation();

		// Calculate the trajectory location
		const FVector theTrajectoryLoc = theLineOrigin + (theLineDirection * ((theRootBoneLocation - theLineOrigin) | theLineDirection));

		// Check the tolerance
		if (!theRootBoneLocation.Equals(theTrajectoryLoc, KINDA_SMALL_NUMBER))
		{
			bIsLineDirection = false;
			break;
		}
	}
#endif

	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Get the motion bone transform in the key
		const FTransform theMotionBoneTransformLS = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, KeyIndex, theMotionBoneName);
		const FTransform theMotionBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theMotionBoneName);

		// Get the root bone transform in the key
		const FTransform theRootBoneTransformCS = OutMotionCurvesData.Evaluate(theKeyTime);

		// Ignore key range
		const FVector2D& theIgnoreFrameRange = InMotionAlignmentData.IgnoreFrameRange;
		if (FMath::IsWithinInclusive(float(KeyIndex), theIgnoreFrameRange.X, theIgnoreFrameRange.Y)
			|| (theIgnoreFrameRange.X >= 0.f && KeyIndex >= theIgnoreFrameRange.X && theIgnoreFrameRange.Y < 0.f))
		{
			continue;
		}

		// Calculate new root bone transform
		FVector theAlignmentRootLocation = FVector(theMotionBoneTransformCS.GetLocation().X, theMotionBoneTransformCS.GetLocation().Y, theRootBoneTransformCS.GetLocation().Z);
		if (bIsLineDirection)
		{
			// Calculate closest point in line
			theAlignmentRootLocation = theLineOrigin + (theLineDirection * ((theAlignmentRootLocation - theLineOrigin) | theLineDirection));
		}

		// Define the alignment transform data
		const FTransform theAlignmentRootTransformLS(theRootBoneTransformCS.GetRotation(), theAlignmentRootLocation, theRootBoneTransformCS.GetScale3D());

		// Update the transform key value
		OutMotionCurvesData.AddTransformKey(theKeyTime, theAlignmentRootTransformLS);
	}

	// Cleanup the curves
	OutMotionCurvesData.Cleanup();
}

void UVAT_Asset::ConvertMotionCaptureToReferencePose(UAnimSequence* InAnimSequence, const FVirtualMotionCaptureReferenceData& InMotionCaptureRefData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const float theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const int32 theSourceNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Check the motion bone track is valid
	const FName& theMotionBoneName = InMotionCaptureRefData.MotionBone.BoneName;
	const int32& theMotionRawTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionRawTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Get the motion bone track data
	FRawAnimSequenceTrack theMotionRawTrackData;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theMotionRawTrackIndex, theMotionRawTrackData))
	{
		return;
	}

	// Get the motion first key pose transform in the track data
	FTransform theMotionInitPoseTransform;
	UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theMotionBoneName, 0, false, theMotionInitPoseTransform);

	// Check the motion bone index is valid
	const int32 theMotionBoneIndex = theReferenceSkeleton.FindBoneIndex(theMotionBoneName);
	if (theMotionBoneIndex == INDEX_NONE)
	{
		return;
	}

	// Get the motion reference pose transform in skeleton reference pose
	const FTransform theMotionSourceTransformRef = theReferenceSkeleton.GetRefBonePose()[theMotionBoneIndex];
	FTransform theMotionFinalTransformRef = theMotionSourceTransformRef;

	// Calculate reference bone transform
	if (InMotionCaptureRefData.bUseReferenceSkeleton)
	{
		theMotionFinalTransformRef.SetTranslation(FVector(theMotionFinalTransformRef.GetTranslation().X, theMotionFinalTransformRef.GetTranslation().Y, theMotionInitPoseTransform.GetTranslation().Z));
		FRotator NewRotation(theMotionInitPoseTransform.Rotator().Pitch, theMotionFinalTransformRef.Rotator().GetInverse().Yaw, theMotionInitPoseTransform.Rotator().Roll);
		theMotionFinalTransformRef.SetRotation(NewRotation.Quaternion());
	}
	else if (InMotionCaptureRefData.ReferencePose != nullptr)
	{
		// Get the animation first pose data
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InMotionCaptureRefData.ReferencePose, theMotionBoneName, 0, false, theMotionFinalTransformRef);
	}
	else
	{
		theMotionFinalTransformRef.SetTranslation(FVector(InMotionCaptureRefData.ReferenceLocation.X, InMotionCaptureRefData.ReferenceLocation.Y, theMotionInitPoseTransform.GetTranslation().Z));
		FRotator NewRotation(theMotionInitPoseTransform.Rotator().Pitch, InMotionCaptureRefData.ReferenceRotation.Yaw, theMotionInitPoseTransform.Rotator().Roll);
		theMotionFinalTransformRef.SetRotation(NewRotation.Quaternion());
	}

	// Calculate the difference transform of the initial frame
	const FTransform theDeltaBoneTransform = theMotionInitPoseTransform.GetRelativeTransformReverse(theMotionFinalTransformRef);

	// Define the final motion bone track data
	FRawAnimSequenceTrack theFinalMotionBoneTrackData = FRawAnimSequenceTrack();

	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theSourceNumberOfKeys; KeyIndex++)
	{
		// Get the motion bone transform in the key
		FTransform theMotionBoneTransform;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theMotionBoneName, KeyIndex, false, theMotionBoneTransform);

		// Get the difference between the Motion bone transform of this frame and the first frame
		FTransform NewMotionBoneTransform = theMotionBoneTransform * theDeltaBoneTransform;
		NewMotionBoneTransform.NormalizeRotation();

		// Cache the track key data
		UVAT_Library::AddBoneTrackKey(theFinalMotionBoneTrackData, NewMotionBoneTransform);
	}

	// Apply the motion bone track changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theMotionBoneName, &theFinalMotionBoneTrackData);

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Asset::ConvertRootMotionToMotionCapture(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData, FVirtualBoneCurvesData& OutMotionCurvesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Check the root bone data is valid
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (theRootBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Get the root bone raw track data
	FRawAnimSequenceTrack theRootBoneRawTrack;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theRootBoneTrackIndex, theRootBoneRawTrack))
	{
		return;
	}

	// Calculate the root bone initialize transform
	FTransform theRootInitializeTransform;
#if ENGINE_MAJOR_VERSION > 4
	theRootInitializeTransform.SetTranslation(FVector(theRootBoneRawTrack.PosKeys[0]));
	theRootInitializeTransform.SetRotation(FQuat(theRootBoneRawTrack.RotKeys[0]));
	theRootInitializeTransform.SetScale3D(FVector::OneVector);
#else
	theRootInitializeTransform.SetTranslation(theRootBoneRawTrack.PosKeys[0]);
	theRootInitializeTransform.SetRotation(theRootBoneRawTrack.RotKeys[0]);
	theRootInitializeTransform.SetScale3D(FVector::OneVector);
#endif

	// Check the motion bone data is valid
	const FName& theMotionBoneName = InMotionCaptureData.MotionBone.BoneName;
	const int32& theMotionBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Get the motion bone raw track data
	FRawAnimSequenceTrack theMotionBoneRawTrack;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theMotionBoneTrackIndex, theMotionBoneRawTrack))
	{
		return;
	}

	// Clear cached motion curves data
	OutMotionCurvesData.Reset();

	// Define the local variables
	FRawAnimSequenceTrack theNewMotionTrackData, theNewRootTrackData;
	FTransform theNewRootTransformLS, theSourceRootTransformLS, theResizeRootTransformLS = FTransform::Identity;

	// Each every track keys
	for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Evaluate the pose key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		FTransform theMotionTransformLS;
		UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, KeyIndex, theMotionBoneName, theMotionTransformLS);

		// Cache the motion bone transform data
#if ENGINE_MAJOR_VERSION > 4
		if (theMotionBoneRawTrack.PosKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetTranslation(FVector(theMotionBoneRawTrack.PosKeys[KeyIndex]));
		}
		if (theMotionBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetRotation(FQuat(theMotionBoneRawTrack.RotKeys[KeyIndex]));
		}
		if (theMotionBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetScale3D(FVector(theMotionBoneRawTrack.ScaleKeys[KeyIndex]));
		}
#else
		if (theMotionBoneRawTrack.PosKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetTranslation(theMotionBoneRawTrack.PosKeys[KeyIndex]);
		}
		if (theMotionBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetRotation(theMotionBoneRawTrack.RotKeys[KeyIndex]);
		}
		if (theMotionBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex))
		{
			theMotionTransformLS.SetScale3D(theMotionBoneRawTrack.ScaleKeys[KeyIndex]);
		}
#endif
		// Cache the root bone rotation data
		if (theRootBoneRawTrack.RotKeys.IsValidIndex(KeyIndex))
		{
			// Get the source pose translation
#if ENGINE_MAJOR_VERSION > 4
			FQuat theResizeRotation = FQuat(theRootBoneRawTrack.RotKeys[KeyIndex]);
#else
			FQuat theResizeRotation = theRootBoneRawTrack.RotKeys[KeyIndex];
#endif

			// Check the resize to origin condition
			if (InMotionCaptureData.bResizeToOrigin)
			{
				theResizeRotation = theRootInitializeTransform.GetRotation().Inverse() * theResizeRotation;
			}

			// Set
			theNewRootTransformLS.SetRotation(theResizeRotation);
			theSourceRootTransformLS.SetRotation(theResizeRotation);
			theResizeRootTransformLS.SetRotation(theResizeRotation);
		}

		// Cache the root bone translation data
		if (theRootBoneRawTrack.PosKeys.IsValidIndex(KeyIndex))
		{
			// Get the source pose translation
#if ENGINE_MAJOR_VERSION > 4
			FVector theResizePos = FVector(theRootBoneRawTrack.PosKeys[KeyIndex]);
#else
			FVector theResizePos = theRootBoneRawTrack.PosKeys[KeyIndex];
#endif
			// Check the resize to origin condition
			if (InMotionCaptureData.bResizeToOrigin)
			{
				theResizePos -= theRootInitializeTransform.GetTranslation();
			}

			// Set
			theNewRootTransformLS.SetTranslation(theResizePos);
			theSourceRootTransformLS.SetTranslation(theResizePos);
			theResizeRootTransformLS.SetTranslation(theResizePos);
		}

		// Cache the root bone scale data
		if (theRootBoneRawTrack.ScaleKeys.IsValidIndex(KeyIndex))
		{
#if ENGINE_MAJOR_VERSION > 4
			const FVector theScale = FVector(theRootBoneRawTrack.ScaleKeys[KeyIndex]);
#else
			const FVector theScale = theRootBoneRawTrack.ScaleKeys[KeyIndex];
#endif
			theNewRootTransformLS.SetScale3D(theScale);
			theSourceRootTransformLS.SetScale3D(theScale);
			theResizeRootTransformLS.SetScale3D(theScale);
		}

		// Convert the root motion transform to motion capture
		{
			// Calculate the resize and convert translation
			FVector theResizeTranslation = FVector::ZeroVector;
			FVector theConvertTranslation = theSourceRootTransformLS.GetTranslation();
			for (const TPair<TEnumAsByte<EAxis::Type>, float>& theWeightPair : InMotionCaptureData.TranslationWeightsMap)
			{
				// Calculate the resize weight
				const float& theResizeWeight = InMotionCaptureData.GlobalWeight * theWeightPair.Value;

				// Handle resize axis weight
				switch (theWeightPair.Key)
				{
				case EAxis::X:
					theResizeTranslation.X = (theConvertTranslation.X * theResizeWeight);
					theConvertTranslation.X *= (1.f - theResizeWeight);
					break;

				case EAxis::Y:
					theResizeTranslation.Y = (theConvertTranslation.Y * theResizeWeight);
					theConvertTranslation.Y *= (1.f - theResizeWeight);
					break;

				case EAxis::Z:
					theResizeTranslation.Z = (theConvertTranslation.Z * theResizeWeight);
					theConvertTranslation.Z *= (1.f - theResizeWeight);
					break;
				}
			}

			// Cache the new translation data
			theNewRootTransformLS.SetTranslation(theConvertTranslation);
			theResizeRootTransformLS.SetTranslation(theResizeTranslation);
			
			// Inverse the rotation translation
			if (InMotionCaptureData.bResizeToOrigin)
			{
				const FVector theRotateVector = theRootInitializeTransform.GetRotation().Inverse().RotateVector(theResizeTranslation);
				theResizeRootTransformLS.SetTranslation(theRotateVector);
				theSourceRootTransformLS.SetTranslation(theRootInitializeTransform.GetRotation().Inverse().RotateVector(theSourceRootTransformLS.GetTranslation()));
			}

			// Calculate the resize and convert rotation
			FRotator theResizeRotation = FRotator::ZeroRotator;
			FRotator theConvertRotation = theSourceRootTransformLS.Rotator();
			for (const TPair<TEnumAsByte<EAxis::Type>, float>& theWeightPair : InMotionCaptureData.RotationWeightsMap)
			{
				// Calculate the resize weight
				const float& theResizeWeight = InMotionCaptureData.GlobalWeight * theWeightPair.Value;

				// Handle resize axis weight
				switch (theWeightPair.Key)
				{
				case EAxis::X:
					theResizeRotation.Roll = (theConvertRotation.Roll * theResizeWeight);
					theConvertRotation.Roll *= (1.f - theResizeWeight);
					break;

				case EAxis::Y:
					theResizeRotation.Pitch = (theConvertRotation.Pitch * theResizeWeight);
					theConvertRotation.Pitch *= (1.f - theResizeWeight);
					break;

				case EAxis::Z:
					theResizeRotation.Yaw = (theConvertRotation.Yaw * theResizeWeight);
					theConvertRotation.Yaw *= (1.f - theResizeWeight);
					break;
				}
			}

			// Cache the new rotation data
			theNewRootTransformLS.SetRotation(theConvertRotation.Quaternion());
			theResizeRootTransformLS.SetRotation(theResizeRotation.Quaternion());
		}
		 
		 // Convert the motion bone local space to component space
		FTransform theMotionTransformWS = theMotionTransformLS * theResizeRootTransformLS;
		theMotionTransformWS.NormalizeRotation();

		// Apply new motion bone track data
		UVAT_Library::AddBoneTrackKey(theNewMotionTrackData, theMotionTransformWS);

		// Apply new root bone track data
		UVAT_Library::AddBoneTrackKey(theNewRootTrackData, theNewRootTransformLS);

		// Add the source motion transform curve data for editor preview
		OutMotionCurvesData.AddTransformKey(theKeyTime, theSourceRootTransformLS);
	}

	// Cleanup the curves data
	OutMotionCurvesData.Cleanup();

	// Apply the root bone track data changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theRootBoneName, &theNewRootTrackData);

	// Apply the motion bone track data changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theMotionBoneName, &theNewMotionTrackData);

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Asset::ConvertMotionCaptureToRootMotion(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData
	, const FVirtualBoneCurvesData& InMotionCurvesData, FVirtualCurveAttributes& InCurveAttributes)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theAnimNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& theSourceAnimLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	// Check the root bone is valid
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
	const int32& theRootBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theRootBoneName);
	if (theRootBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Check the motion bone data is valid
	const FName& theMotionBoneName = InMotionCaptureData.MotionBone.BoneName;
	const int32& theMotionBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theMotionBoneName);
	if (theMotionBoneTrackIndex == INDEX_NONE)
	{
		return;
	}

	// Get the animation asset motion bone raw track data
	FRawAnimSequenceTrack theMotionBoneRawTrack;
	if (!UVAT_Library::GetBoneTrackData(InAnimSequence, theMotionBoneName, theMotionBoneRawTrack))
	{
		return;
	}

	// Define the new root bone track data
	FRawAnimSequenceTrack theNewRootBoneTrack = FRawAnimSequenceTrack();

	// Each every animation asset track key
	for (int32 KeyIndex = 0; KeyIndex < theAnimNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Always get the root bone current transform
		FTransform theSourceRootBoneTransform = FTransform::Identity;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, theRootBoneName, KeyIndex, true, theSourceRootBoneTransform);

		// Evaluate the transform key value
		FRotator theRotationKey = FRotator::ZeroRotator;
		FVector theTranslationKey = FVector::ZeroVector;
		InMotionCurvesData.Evaluate(theKeyTime, theRotationKey, theTranslationKey);

		// Calculate convert motion rotation
		FRotator theConvertRotation = theSourceRootBoneTransform.Rotator();
		for (const TPair<TEnumAsByte<EAxis::Type>, float>& theAxisWeightPair : InMotionCaptureData.RotationWeightsMap)
		{
			// Calculate the axis weight
			const float theAxisWeight = InMotionCaptureData.GlobalWeight * theAxisWeightPair.Value;

			// Handle the axis
			switch (theAxisWeightPair.Key)
			{
			case EAxis::X:
				theConvertRotation.Roll = theRotationKey.Roll * theAxisWeight;
				break;

			case EAxis::Y:
				theConvertRotation.Pitch = theRotationKey.Pitch  * theAxisWeight;
				break;

			case EAxis::Z:
				theConvertRotation.Yaw = theRotationKey.Yaw  * theAxisWeight;
				break;
			}
		}

		// Calculate convert motion translation
		FVector theConvertTranslation = theSourceRootBoneTransform.GetTranslation();
		for (const TPair<TEnumAsByte<EAxis::Type>, float>& theAxisWeightPair : InMotionCaptureData.TranslationWeightsMap)
		{
			// Calculate the axis weight
			const float theAxisWeight = InMotionCaptureData.GlobalWeight * theAxisWeightPair.Value;

			// Handle the axis
			switch (theAxisWeightPair.Key)
			{
			case EAxis::X:
				theConvertTranslation.X = theTranslationKey.X * theAxisWeight;
				break;

			case EAxis::Y:
				theConvertTranslation.Y = theTranslationKey.Y * theAxisWeight;
				break;

			case EAxis::Z:
				theConvertTranslation.Z = theTranslationKey.Z * theAxisWeight;
				break;
			}
		}

		// Make the new root bone transform
		FTransform theNewRootBoneTransform(theConvertRotation, theConvertTranslation);
		
		// Sample the new root bone transform to track data
		UVAT_Library::AddBoneTrackKey(theNewRootBoneTrack, theNewRootBoneTransform);

		// Convert the motion bone source local space transform to component space
#if  ENGINE_MAJOR_VERSION > 4 
		FTransform theSourceMotionBoneTransformCS(FQuat(theMotionBoneRawTrack.RotKeys[KeyIndex]), FVector(theMotionBoneRawTrack.PosKeys[KeyIndex]));
#else
		FTransform theSourceMotionBoneTransformCS(theMotionBoneRawTrack.RotKeys[KeyIndex], theMotionBoneRawTrack.PosKeys[KeyIndex]);
#endif
		theSourceMotionBoneTransformCS *= theSourceRootBoneTransform;
		theSourceMotionBoneTransformCS.NormalizeRotation();

		// Convert the motion bone component space transform to new local space
		FTransform theNewMotionBoneTransformLS = theSourceMotionBoneTransformCS;
		theNewMotionBoneTransformLS.SetToRelativeTransform(theNewRootBoneTransform);
		theNewMotionBoneTransformLS.NormalizeRotation();

		// Sample the new motion bone transform to track data
#if  ENGINE_MAJOR_VERSION > 4 
		theMotionBoneRawTrack.RotKeys[KeyIndex] = FQuat4f(theNewMotionBoneTransformLS.GetRotation());
		theMotionBoneRawTrack.PosKeys[KeyIndex] = FVector3f(theNewMotionBoneTransformLS.GetTranslation());
#else
		theMotionBoneRawTrack.RotKeys[KeyIndex] = theNewMotionBoneTransformLS.GetRotation();
		theMotionBoneRawTrack.PosKeys[KeyIndex] = theNewMotionBoneTransformLS.GetTranslation();
#endif
	}

	// Apply the root bone track data changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theRootBoneName, &theNewRootBoneTrack);

	// Apply the motion bone track data changed
	UVAT_Library::SetBoneTrackData(InAnimSequence, theMotionBoneName, &theMotionBoneRawTrack);

	// Rebuild animation curves
	UVAT_Curve::RebuildAnimationRootMotionCurves(InAnimSequence, InCurveAttributes);

	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}
#pragma endregion


#pragma region Compression
void UVAT_Asset::OnApplyCompression(UAnimSequence* InAnimationAsset)
{
	// Check the animation asset is valid
	if (!InAnimationAsset)
	{
		return;
	}

	// Build animation sequences array
	TArray<TWeakObjectPtr<UAnimSequence>> theAnimSequences;
	theAnimSequences.Add(InAnimationAsset);

	FPersonaModule& thePersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
	thePersonaModule.ApplyCompression(theAnimSequences, false);
}
#pragma endregion


#pragma region LOD
void UVAT_Asset::GenerateLODs(USkeletalMesh* InSkeletalMesh, FVirtualAssetGenerateLODsData& InLODsData)
{
#if  ENGINE_MAJOR_VERSION < 5 
	if (!InSkeletalMesh || !InSkeletalMesh->GetImportedModel())
	{
		return;
	}

	// Again Build Morph Target
	bool bShouldRebuild = true;
	USkeletalMesh* ShouldRebuildSkeletalMesh = nullptr;

	int32 LODCount = InLODsData.LODSettings ? InLODsData.LODSettings->GetNumberOfSettings() : InLODsData.DefaultGroups;
	while (bShouldRebuild)
	{
		// Clear Again Build
		if (ShouldRebuildSkeletalMesh != nullptr)
		{
			bShouldRebuild = false;
			InSkeletalMesh = ShouldRebuildSkeletalMesh;
		}

		USkeletalMeshLODSettings* theLODSettings = nullptr;
#if  ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 27 
		theLODSettings = InSkeletalMesh->GetLODSettings();
#else
		theLODSettings = InSkeletalMesh->LODSettings;
#endif

		if (theLODSettings)
		{
			LODCount = theLODSettings->GetNumberOfSettings();
		}
		else if (InLODsData.bOverrideLODSettings)
		{
			InSkeletalMesh->SetLODSettings(InLODsData.LODSettings);
		}

		// Again Set...
		if (!theLODSettings)
		{
			ShouldRebuildSkeletalMesh = InSkeletalMesh;
			TArray<FSkeletalMeshLODInfo>& Local_LODInfo = InSkeletalMesh->GetLODInfoArray();
			for (FSkeletalMeshLODInfo& Local_Info : Local_LODInfo)
			{
				FSkeletalMeshOptimizationSettings& ReductionSettings = Local_Info.ReductionSettings;
				ReductionSettings.bRemapMorphTargets = InLODsData.bRemapMorphTargets;
			}
		}
		else
		{
			bShouldRebuild = false;
		}

		// see if there is 
		bool bRegenerateEvenIfImported = false;
		bool bGenerateBaseLOD = false;
		int32 CurrentNumLODs = InSkeletalMesh->GetLODNum();
		if (CurrentNumLODs == LODCount)
		{
			bool bImportedLODs = false;
			// check if anything is imported and ask if users wants to still regenerate it
			for (int32 LODIdx = 0; LODIdx < LODCount; LODIdx++)
			{
				FSkeletalMeshLODInfo& CurrentLODInfo = *(InSkeletalMesh->GetLODInfo(LODIdx));
				bool bIsReductionActive = InSkeletalMesh->IsReductionActive(LODIdx);
				bool bIsLODModelbuildDataAvailable = InSkeletalMesh->GetImportedModel()->LODModels.IsValidIndex(LODIdx) && InSkeletalMesh->IsLODImportedDataBuildAvailable(LODIdx);
				bool bIsReductionDataPresent = (InSkeletalMesh->GetImportedModel()->OriginalReductionSourceMeshData.IsValidIndex(LODIdx) && !InSkeletalMesh->GetImportedModel()->OriginalReductionSourceMeshData[LODIdx]->IsEmpty());

				if (CurrentLODInfo.bHasBeenSimplified == false && bIsReductionActive)
				{
					if (LODIdx > 0)
					{
						bImportedLODs = true;
					}
					else
					{
						bGenerateBaseLOD = true;
					}
				}
				else if (LODIdx == CurrentLODInfo.ReductionSettings.BaseLOD
					&& CurrentLODInfo.bHasBeenSimplified
					&& !bIsReductionActive
					&& (bIsLODModelbuildDataAvailable || bIsReductionDataPresent))
				{
					//Restore the base LOD data
					CurrentLODInfo.bHasBeenSimplified = false;
					if (!bIsLODModelbuildDataAvailable)
					{
						FLODUtilities::RestoreSkeletalMeshLODImportedData(InSkeletalMesh, LODIdx);
					}
				}
			}

			// if LOD is imported, ask users if they want to regenerate or just leave it
			if (bImportedLODs)
			{
				bRegenerateEvenIfImported = true;
			}
		}

#if  ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 27 
		FLODUtilities::RegenerateLOD(InSkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), bRegenerateEvenIfImported, bGenerateBaseLOD);
#else
		FLODUtilities::RegenerateLOD(InSkeletalMesh, LODCount, bRegenerateEvenIfImported, bGenerateBaseLOD);
#endif
	}
#endif
}

void UVAT_Asset::RemoveLODs(USkeletalMesh* InSkeletalMesh, const TArray<int32>& InRemoveGroups)
{
	FSkeletalMeshUpdateContext UpdateContext;
	UpdateContext.SkeletalMesh = InSkeletalMesh;

	//If we force a regenerate, we want to invalidate the DCC so the render data get rebuilded
	InSkeletalMesh->InvalidateDeriveDataCacheGUID();

	// remove LODs
	int32 CurrentNumLODs = InSkeletalMesh->GetLODNum();
	for (int32 LODIdx = CurrentNumLODs - 1; LODIdx >= 1; LODIdx--)
	{
		FLODUtilities::RemoveLOD(UpdateContext, LODIdx);
	}
}
#pragma endregion


#pragma region Export
void UVAT_Asset::ExportToFBX(const EVirtualExportSourceOption Option, UAnimSequence* InAnimationAsset, FVirtualAssetExportData& InAssetExportData)
{
	UAnimSequence* AnimSequenceToRecord = nullptr;
	if (Option == EVirtualExportSourceOption::AnimData)
	{
		TArray<UObject*> AssetsToExport;
		AssetsToExport.Add(InAnimationAsset);
		OnExportToFBX(AssetsToExport, false, InAssetExportData);
	}
	else if (Option == EVirtualExportSourceOption::PreviewMesh)
	{
		TArray<TWeakObjectPtr<UObject>> Skeletons;
		Skeletons.Add(InAnimationAsset->GetSkeleton());
		CreateAnimationAssets(InAssetExportData, Skeletons, UAnimSequence::StaticClass(), FString("_PreviewMesh"), InAnimationAsset, true);
	}
	else
	{
		ensure(false);
	}
}

bool UVAT_Asset::OnExportToFBX(const TArray<UObject*> AssetsToExport, bool bRecordAnimation, FVirtualAssetExportData& InAssetExportData)
{
	bool AnimSequenceExportResult = false;
	TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
	if (AssetsToExport.Num() > 0)
	{
		UAnimSequence* AnimationToRecord = Cast<UAnimSequence>(AssetsToExport[0]);
		if (AnimationToRecord)
		{
			if (bRecordAnimation)
			{
				RecordMeshToAnimation(InAssetExportData.PreviewMeshComponent, AnimationToRecord);
			}

			AnimSequences.Add(AnimationToRecord);
		}
	}

	if (AnimSequences.Num() > 0)
	{
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		AnimSequenceExportResult = PersonaModule.ExportToFBX(AnimSequences, InAssetExportData.PreviewMeshComponent->SkeletalMesh);
	}
	return AnimSequenceExportResult;
}

void UVAT_Asset::CreateAnimationAssets(FVirtualAssetExportData& InAssetExportData, const TArray<TWeakObjectPtr<UObject>>& SkeletonsOrSkeletalMeshes, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix, UObject* NameBaseObject /*= nullptr*/, bool bDoNotShowNameDialog /*= false*/)
{
	TArray<UObject*> ObjectsToSync;
	for (auto SkelIt = SkeletonsOrSkeletalMeshes.CreateConstIterator(); SkelIt; ++SkelIt)
	{
		USkeletalMesh* SkeletalMesh = nullptr;
		USkeleton* theSkeleton = Cast<USkeleton>(SkelIt->Get());
		if (theSkeleton == nullptr)
		{
			SkeletalMesh = CastChecked<USkeletalMesh>(SkelIt->Get());
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 27
			theSkeleton = SkeletalMesh->GetSkeleton();
#else
			theSkeleton = SkeletalMesh->Skeleton;
#endif
		}

		if (theSkeleton)
		{
			FString Name;
			FString PackageName;
			FString AssetPath = (NameBaseObject) ? NameBaseObject->GetOutermost()->GetName() : theSkeleton->GetOutermost()->GetName();
			// Determine an appropriate name
			CreateUniqueAssetName(AssetPath, InPrefix, PackageName, Name);

			if (bDoNotShowNameDialog == false)
			{
				// set the unique asset as a default name
// 				TSharedRef<SCreateAnimationAssetDlg> NewAnimDlg =
// 					SNew(SCreateAnimationAssetDlg)
// 					.DefaultAssetPath(FText::FromString(PackageName));
// 
// 				// show a dialog to determine a new asset name
// 				if (NewAnimDlg->ShowModal() == EAppReturnType::Cancel)
// 				{
// 					return;
// 				}
// 
// 				PackageName = NewAnimDlg->GetFullAssetPath();
// 				Name = NewAnimDlg->GetAssetName();
			}

			// Create the asset, and assign its skeleton
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			UAnimationAsset* NewAsset = Cast<UAnimationAsset>(AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), AssetClass, NULL));

			if (NewAsset)
			{
				NewAsset->SetSkeleton(theSkeleton);
				if (SkeletalMesh)
				{
					NewAsset->SetPreviewMesh(SkeletalMesh);
				}
				NewAsset->MarkPackageDirty();

				ObjectsToSync.Add(NewAsset);
			}
		}
	}

	OnExportToFBX(ObjectsToSync, true, InAssetExportData);
}

bool UVAT_Asset::RecordMeshToAnimation(USkeletalMeshComponent* PreviewComponent, UAnimSequence* NewAsset)
{
	ISequenceRecorder& RecorderModule = FModuleManager::Get().LoadModuleChecked<ISequenceRecorder>("SequenceRecorder");
	return RecorderModule.RecordSingleNodeInstanceToAnimation(PreviewComponent, NewAsset);
}

/** Creates a unique package and asset name taking the form InBasePackageName+InSuffix */
void UVAT_Asset::CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(InBasePackageName, InSuffix, OutPackageName, OutAssetName);
}
#pragma endregion

#undef LOCTEXT_NAMESPACE