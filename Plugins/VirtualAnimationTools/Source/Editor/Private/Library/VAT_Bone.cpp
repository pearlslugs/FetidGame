// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Bone.h"
#include "Library/VAT_Notify.h"
#include "Library/VAT_Retarget.h"
#include "Library/VAT_Library.h"

#include "Kismet/KismetMathLibrary.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#if ENGINE_MAJOR_VERSION > 4
#include "Animation/AnimData/CurveIdentifier.h"
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#include "ReferenceSkeleton.h"
#include "Misc/ScopedSlowTask.h"

#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "../Public/Library/VAT_Asset.h"
#include "EditorLevelLibrary.h"

#define LOCTEXT_NAMESPACE "VAT_Bone"

/*-----------------------------------------------------------------------------
	FVirtualBoneData implementation.
-----------------------------------------------------------------------------*/

void FVirtualBoneData::GetRawBoneTrackData(const int32& InNumberOfKeys, FRawAnimSequenceTrack& OutTrackData) const
{
	// Define the default transform key
	FTransform theLastTransformKey = FTransform::Identity;

	// Clear previous cached data
	OutTrackData.PosKeys.Reset();
	OutTrackData.RotKeys.Reset();
	OutTrackData.ScaleKeys.Reset();

	// Make every transforms data
	for (const FVirtualTransformData& theTransform : TracksData)
	{
		theLastTransformKey = FTransform(theTransform.Rotation, theTransform.Location, theTransform.Scale);
		UVAT_Library::AddBoneTrackKey(OutTrackData, theLastTransformKey);
	}

	// Add other default keys
	if (InNumberOfKeys != TracksData.Num())
	{
		for (int32 KeyIndex = TracksData.Num(); KeyIndex < InNumberOfKeys; KeyIndex++)
		{
			UVAT_Library::AddBoneTrackKey(OutTrackData, theLastTransformKey);
		}
	}
}

/*-----------------------------------------------------------------------------
	FVirtualBoneBranchFilter implementation.
-----------------------------------------------------------------------------*/

TArray<FName> FVirtualBoneBranchFilter::GetBoneTreeNames(const USkeleton* InSkeleton) const
{
	TArray<FName> theBoneTreeNames;

	// Check the source bone is valid
	if (SourceBone.BoneName == NAME_None)
	{
		return theBoneTreeNames;
	}

	// Get the reference skeleton data
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Check the source bone index is valid
	const int32& theSourceBoneIndex = theReferenceSkeleton.FindRawBoneIndex(SourceBone.BoneName);
	if (theSourceBoneIndex == INDEX_NONE)
	{
		return theBoneTreeNames;
	}

	// Check and add the source bone name
	if (bIncludeSourceBone)
	{
		theBoneTreeNames.AddUnique(SourceBone.BoneName);
	}

	// Get the interrupt bone index
	const int32& theInterruptBoneIndex = InSkeleton->GetReferenceSkeleton().FindRawBoneIndex(InterruptBone.BoneName);

	// Each every mesh bone info, find all children bones name
	for (const FMeshBoneInfo& theMeshBoneInfo : InSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo())
	{
		// Get the bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = InSkeleton->GetReferenceSkeleton().FindRawBoneIndex(theBoneName);

		// Check both child of condition
		if (theReferenceSkeleton.BoneIsChildOf(theBoneIndex, theSourceBoneIndex))
		{
			// Check the interrupt bone
			if (theBoneName == InterruptBone.BoneName)
			{
				if (bIncludeInterruptBone)
				{
					theBoneTreeNames.AddUnique(theBoneName);
				}
				continue;
			}

			// Check the bone is interrupt bone children condition
			if (theInterruptBoneIndex >= 0 && theReferenceSkeleton.BoneIsChildOf(theBoneIndex, theInterruptBoneIndex))
			{
				continue;
			}

			// Cache the bone name
			theBoneTreeNames.AddUnique(theBoneName);
		}
	}

	return theBoneTreeNames;
}

#pragma region Bone Name
bool UVAT_Bone::BoneIsChildOf(USkeleton* InSkeleton, const FName& InBoneName, const FName& InChildBoneName)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return false;
	}

	// Get the reference skeleton data
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Get the bone index
	const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(InBoneName);

	// Get the child bone index 
	const int32& theChildBoneIndex = InSkeleton->GetReferenceSkeleton().FindRawBoneIndex(InChildBoneName);

	// Return the result
	return theReferenceSkeleton.BoneIsChildOf(theChildBoneIndex, theBoneIndex);
}

bool UVAT_Bone::GetBonesName(USkeleton* InSkeleton, TArray<FName>* OutBonesName)
{
	if (InSkeleton && OutBonesName)
	{
		OutBonesName->Reset();
		const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();
		for (int i = 0; i < theReferenceSkeleton.GetRawRefBoneInfo().Num(); i++)
		{
			OutBonesName->AddUnique(theReferenceSkeleton.GetRawRefBoneInfo()[i].Name);
		}
	}

	return OutBonesName ? true : false;
}
#pragma endregion


#pragma region Bone Transform
bool UVAT_Bone::HasMotionData(UAnimSequence* InAnimSequence, const float& InLastTime, const float& InNextTime, const float& InSampleRate, bool InLastTrackKey)
{
	// Check the animation sequence is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return false;
	}

	// Get the skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Find last track time
	if (InLastTrackKey)
	{
		const float theSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);
		const FTransform theLastMotionTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, theSequenceLength, theRootBoneName);
		
		// Check the motion data is equal
		{
			const FTransform thePoseTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, InLastTime, theRootBoneName);
			if (thePoseTransform.Equals(theLastMotionTransform))
			{
				return false;
			}
		}

		// Check the motion data is equal
		{
			const FTransform thePoseTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, InNextTime, theRootBoneName);
			if (thePoseTransform.Equals(theLastMotionTransform))
			{
				return false;
			}
		}

		// Sample the pose features
		float thePoseSampleTime = 0.f;
		while (thePoseSampleTime <= (theSequenceLength + 0.0001f))
		{
			// Clamp the pose time
			thePoseSampleTime = FMath::Clamp(thePoseSampleTime, 0.f, theSequenceLength);

			// Check the motion data is equal
			const FTransform thePoseTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, thePoseSampleTime, theRootBoneName);
			if (thePoseTransform.Equals(theLastMotionTransform))
			{
				if (InNextTime >= thePoseSampleTime)
				{
					return false;
				}
				else
				{
					break;
				}
			}

			// Iterate
			thePoseSampleTime += InSampleRate;
		}

		// Always return true
		return true;
	}

	// Calculate the delta motion transform data
	const FTransform thePreviousMotionTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, InLastTime, theRootBoneName);
	const FTransform theCurrentMotionTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, InNextTime, theRootBoneName);
	const FTransform theDeltaMotionTransform = UVAT_Library::Subtract_TransformTransform(theCurrentMotionTransform, thePreviousMotionTransform);

	// Return the result
	return !theDeltaMotionTransform.GetLocation().IsNearlyZero(KINDA_SMALL_NUMBER) || !theDeltaMotionTransform.GetRotation().IsIdentity(KINDA_SMALL_NUMBER);
}

bool UVAT_Bone::HasAnyMotionData(UAnimSequence* InAnimSequence)
{
	// Check the animation sequence is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return false;
	}

	// Get the skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Each every pose keys
	for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
	{
		// Evaluate the key time
		const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

		// Get the pos key transform data
		const FTransform theTrackBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, theKeyTime, theRootBoneName);

		// Check the motion data is valid
		if (!theTrackBoneTransform.GetLocation().IsNearlyZero(KINDA_SMALL_NUMBER) || !theTrackBoneTransform.GetRotation().IsIdentity(KINDA_SMALL_NUMBER))
		{
			return true;
		}
	}

	// Return the result
	return false;
}

FTransform UVAT_Bone::GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName)
{
	FTransform theBoneTransformLS;
#if WITH_EDITOR
	UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, InBoneName, InFrame, InExtractRootMotion, theBoneTransformLS);
#endif // WITH_EDITOR
	return theBoneTransformLS;
}

FTransform UVAT_Bone::GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const float& InTime, const FName& InBoneName)
{
	FTransform theBoneTransformLS;
#if WITH_EDITOR
	UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, InBoneName, InTime, InExtractRootMotion, theBoneTransformLS);
#endif // WITH_EDITOR
	return theBoneTransformLS;
}

FTransform UVAT_Bone::GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName, const FTransform& InBoneTransformCS)
{
	// Always use default bone transform
	FTransform theBoneTransformLS = InBoneTransformCS;

	// Check the animation sequence is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return theBoneTransformLS;
	}

	// Get the skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Check the bone index is valid
	const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(InBoneName);
	if (theBoneIndex == INDEX_NONE)
	{
		return theBoneTransformLS;
	}

	// Check the parent bone index is valid
	const int32& theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
	if (theParentBoneIndex == INDEX_NONE)
	{
		return theBoneTransformLS;
	}

	// Get the parent bone name
	const FName& theParentBoneName = theReferenceSkeleton.GetBoneName(theParentBoneIndex);
	if (theParentBoneName == NAME_None)
	{
		return theBoneTransformLS;
	}

	// Get parent bone component space transform
	const FTransform& theParentBoneTransformCS = GetBoneTransformCS(InAnimSequence, InExtractRootMotion, InFrame, theParentBoneName);
	theBoneTransformLS.SetToRelativeTransform(theParentBoneTransformCS);
	theBoneTransformLS.NormalizeRotation();
	return theBoneTransformLS;
}

FTransform UVAT_Bone::GetBoneTransformCS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName)
{
	return GetBoneTransformCS(InAnimSequence, InExtractRootMotion, InAnimSequence ? InAnimSequence->GetTimeAtFrame(InFrame) : 0, InBoneName);
}

FTransform UVAT_Bone::GetBoneTransformCS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const float& InTime, const FName& InBoneName)
{
	FTransform theBoneTransformCS;

	// Check the animation sequence is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return theBoneTransformCS;
	}

	// Get the skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Get the root bone transform
	FTransform theRootBoneTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theRootBoneName);
	if (InExtractRootMotion)
	{
		UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, theRootBoneName, InTime, InExtractRootMotion, theRootBoneTransformCS);
	}

	// Calculate parent space
	FName ParentBoneName = InBoneName;
	while (ParentBoneName != theRootBoneName)
	{
		// Get the parent bone local space transform
		FTransform theParentBoneTransformLS;
#if WITH_EDITOR
		UAnimationBlueprintLibrary::GetBonePoseForTime(InAnimSequence, ParentBoneName, InTime, false, theParentBoneTransformLS);
#endif // WITH_EDITOR

		// Convert to parent space
		theBoneTransformCS *= theParentBoneTransformLS;

		// Find the bone index
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(ParentBoneName);
		if (theBoneIndex == INDEX_NONE)
		{
			break;
		}

		// Find the parent bone index
		const int32& theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
		if (theParentBoneIndex == INDEX_NONE)
		{
			break;
		}

		// Get the parent bone name
		ParentBoneName = theReferenceSkeleton.GetBoneName(theParentBoneIndex);
	}

	// Convert to component space transform
	theBoneTransformCS *= theRootBoneTransformCS;

	// Normalize the transform
	theBoneTransformCS.NormalizeRotation();

	// Success
	return theBoneTransformCS;
}
#pragma endregion

#if 0
#pragma region Set Bone Transform
void UVAT_Bone::OnSetBoneTransform(UAnimSequence* InAnimSequence, UAnimSequence* InReferenceAnimSequence, const FVector2D& InFrameRange, const FVector2D& InReferenceFrameRange, const FVirtualBonesData& InBonesData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData)
{
	if (!InAnimSequence || !InReferenceAnimSequence)
	{
		return;
	}

	for (const FVirtualBoneData& BoneData : InBonesData.BonesData)
	{
		if (UVAT_Library::GetBoneTrackIndex(InReferenceAnimSequence, BoneData.Bone.BoneName) == INDEX_NONE)
		{
			continue;
		}

		// Get from animation track
		FRawAnimSequenceTrack FromRawAnimSequenceTrack;
		if (!UVAT_Library::GetBoneTrackData(InAnimSequence, BoneData.Bone.BoneName, FromRawAnimSequenceTrack))
		{
			continue;
		}

		// Get reference animation track
		FRawAnimSequenceTrack ReferenceAnimationBoneTrack;
		if (!UVAT_Library::GetBoneTrackData(InReferenceAnimSequence, BoneData.Bone.BoneName, ReferenceAnimationBoneTrack))
		{
			continue;
		}

		// Get from animation first frame bone track
		FTransform FromBoneTransformBegin = FTransform().Identity;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InAnimSequence, BoneData.Bone.BoneName, 0, false, FromBoneTransformBegin);

		// Get reference first frame bone track
		FTransform ReferenceBoneTransform = FTransform().Identity;
		FTransform ReferenceBoneTransformBegin = FTransform().Identity;
		UAnimationBlueprintLibrary::GetBonePoseForFrame(InReferenceAnimSequence, BoneData.Bone.BoneName, 0, false, ReferenceBoneTransformBegin);

		// Calculate delta transform
		const FTransform DeltaTransform = UVAT_Library::Subtract_TransformTransform(ReferenceBoneTransformBegin, FromBoneTransformBegin);

		// Calculate new bone track
		const int32& ToAnimNumFrames = UVAT_Library::GetNumberOfKeys(InAnimSequence);
		for (int i = 0; i < ToAnimNumFrames; i++)
		{
			// Copy reference animation bone transform
			if (ReferenceAnimationBoneTrack.PosKeys.IsValidIndex(i))
			{
				ReferenceBoneTransform.SetLocation(ReferenceAnimationBoneTrack.PosKeys[i]);
			}
			if (ReferenceAnimationBoneTrack.RotKeys.IsValidIndex(i))
			{
				ReferenceBoneTransform.SetRotation(ReferenceAnimationBoneTrack.RotKeys[i]);
			}
			if (ReferenceAnimationBoneTrack.ScaleKeys.IsValidIndex(i))
			{
				ReferenceBoneTransform.SetScale3D(ReferenceAnimationBoneTrack.ScaleKeys[i]);
			}

			if (UKismetMathLibrary::InRange_IntInt(i, InReferenceFrameRange.X, InReferenceFrameRange.Y) || InReferenceFrameRange.Y < 0)
			{
				// Location
				if (BoneData.bIncludeTranslation)
				{
					switch (BoneData.BoneModifyType)
					{
					case EVirtualBoneModifyType::VBT_Delta:
						if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.PosKeys[i] = (FromRawAnimSequenceTrack.PosKeys[i]
								+ DeltaTransform.GetLocation());
						}
						break;

					case EVirtualBoneModifyType::VBT_Replace:
						if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.PosKeys[i] = ReferenceBoneTransform.GetLocation();
						}
						break;

					case EVirtualBoneModifyType::VBT_Additive:
						if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.PosKeys[i] = (FromRawAnimSequenceTrack.PosKeys[i]
								+ ReferenceBoneTransform.GetLocation());
						}
						break;
					}
				}

				// Rotation
				if (BoneData.bIncludeRotation)
				{
					switch (BoneData.BoneModifyType)
					{
					case EVirtualBoneModifyType::VBT_Delta:
						if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.RotKeys[i] = (DeltaTransform.GetRotation()
								* FromRawAnimSequenceTrack.RotKeys[i]);
						}
						break;

					case EVirtualBoneModifyType::VBT_Replace:
						if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.RotKeys[i] = ReferenceBoneTransform.GetRotation();
						}
						break;

					case EVirtualBoneModifyType::VBT_Additive:
						if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.RotKeys[i] = (ReferenceBoneTransform.GetRotation()
								* FromRawAnimSequenceTrack.RotKeys[i]);
						}
						break;
					}
				}

				// Scale
				if (BoneData.bIncludeScale)
				{
					switch (BoneData.BoneModifyType)
					{
					case EVirtualBoneModifyType::VBT_Delta:
						if (FromRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.ScaleKeys[i] = (DeltaTransform.GetScale3D()
								+ FromRawAnimSequenceTrack.ScaleKeys[i]);
						}
						break;

					case EVirtualBoneModifyType::VBT_Replace:
						if (FromRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.ScaleKeys[i] = ReferenceBoneTransform.GetScale3D();
						}
						break;

					case EVirtualBoneModifyType::VBT_Additive:
						if (FromRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
						{
							FromRawAnimSequenceTrack.ScaleKeys[i] = (ReferenceBoneTransform.GetScale3D()
								+ FromRawAnimSequenceTrack.ScaleKeys[i]);
						}
						break;
					}
				}
			}
		}

		UVAT_Library::SetBoneTrackData(InAnimSequence, BoneData.Bone.BoneName, &FromRawAnimSequenceTrack);
	}

	if (!SampleConstraintBones(InAnimSequence, InConstraintBonesData))
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_Bone::OnSetBoneTransform(UAnimSequence* InAnimSequence, const FVector2D& InFrameRange, const FVector2D& InReferenceFrameRange, const FVirtualBonesData& InBonesData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData)
{
	if (!InAnimSequence)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* AnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theDataController = InAnimSequence->GetController();
	theDataController.OpenBracket(LOCTEXT("SetBoneTransform_Description", "Set bone transform."), false);
#endif

	for (const FVirtualBoneData& BoneData : InBonesData.BonesData)
	{
		const int32& theBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, BoneData.Bone.BoneName);
		if (theBoneTrackIndex == INDEX_NONE)
		{
			UVAT_Library::AddBoneTrack(InAnimSequence, BoneData.Bone.BoneName);
		}

		FRawAnimSequenceTrack ToRawAnimSequenceTrack;
		FRawAnimSequenceTrack FromRawAnimSequenceTrack;
		if (!UVAT_Library::GetBoneTrackData(InAnimSequence, BoneData.Bone.BoneName, FromRawAnimSequenceTrack))
		{
			continue;
		}

		const int32& ToAnimNumFrames = UVAT_Library::GetNumberOfKeys(InAnimSequence);
		for (int i = 0; i < ToAnimNumFrames; i++)
		{
			if (UKismetMathLibrary::InRange_IntInt(i, InFrameRange.X, InFrameRange.Y) || InFrameRange.Y < 0)
			{
				if (BoneData.bIncludeTranslation)
				{
					switch (BoneData.BoneModifyType)
					{
					case EVirtualBoneModifyType::VBT_Replace:
						ToRawAnimSequenceTrack.PosKeys.Add(BoneData.BoneLocation);
						break;

					case EVirtualBoneModifyType::VBT_Additive:
						if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
						{
							ToRawAnimSequenceTrack.PosKeys.Add(BoneData.BoneLocation + FromRawAnimSequenceTrack.PosKeys[i]);
						}
						else
						{
							ToRawAnimSequenceTrack.PosKeys.Add(BoneData.BoneLocation);
						}
						break;
					}
				}
				else if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
				{
					ToRawAnimSequenceTrack.PosKeys.Add(FromRawAnimSequenceTrack.PosKeys[i]);
				}

				if (BoneData.bIncludeRotation)
				{
					switch (BoneData.BoneModifyType)
					{
					case EVirtualBoneModifyType::VBT_Replace:
						ToRawAnimSequenceTrack.RotKeys.Add(BoneData.BoneRotation.Quaternion());
						break;

					case EVirtualBoneModifyType::VBT_Additive:
						if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
						{
							ToRawAnimSequenceTrack.RotKeys.Add(FromRawAnimSequenceTrack.RotKeys[i] * BoneData.BoneRotation.Quaternion());
						}
						else
						{
							ToRawAnimSequenceTrack.RotKeys.Add(BoneData.BoneRotation.Quaternion());
						}
						break;
					}
				}
				else if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
				{
					ToRawAnimSequenceTrack.RotKeys.Add(FromRawAnimSequenceTrack.RotKeys[i]);
				}

				if (ToRawAnimSequenceTrack.RotKeys.Num() > 0)
				{
					ToRawAnimSequenceTrack.RotKeys.Last().Normalize();
				}

				if (BoneData.bIncludeTranslation)
				{
					ToRawAnimSequenceTrack.ScaleKeys.Add(BoneData.BoneScale);
				}
				else if (FromRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
				{
					ToRawAnimSequenceTrack.ScaleKeys.Add(FromRawAnimSequenceTrack.ScaleKeys[i]);
				}
			}
			else
			{
				if (FromRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
					ToRawAnimSequenceTrack.PosKeys.Add(FromRawAnimSequenceTrack.PosKeys[i]);

				if (FromRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
				{
					ToRawAnimSequenceTrack.RotKeys.Add(FromRawAnimSequenceTrack.RotKeys[i]);
					ToRawAnimSequenceTrack.RotKeys.Last().Normalize();
				}

				if (FromRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
					ToRawAnimSequenceTrack.ScaleKeys.Add(FromRawAnimSequenceTrack.ScaleKeys[i]);
			}
		}

		UVAT_Library::SetBoneTrackData(InAnimSequence, BoneData.Bone.BoneName, &ToRawAnimSequenceTrack);
	}

#if ENGINE_MAJOR_VERSION > 4
	theDataController.CloseBracket();
#endif

	if (!SampleConstraintBones(InAnimSequence, InConstraintBonesData))
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}

void UVAT_Bone::OnBakeBoneTransforms(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData)
{
	if (!InAnimSequence)
	{
		return;
	}

	const int32& AnimFrames = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const float& AnimSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);

	const bool IsValidRange = InBakeBoneData.IsValidRange(AnimFrames);
	const float BeginTime = IsValidRange ? (AnimSequenceLength / AnimFrames) * InBakeBoneData.BakeFrameRange.X : 0.f;
	const float EndTime = IsValidRange ? (AnimSequenceLength / AnimFrames) * InBakeBoneData.BakeFrameRange.Y : AnimSequenceLength;

	OnBakeBoneTransformsFromRange(InAnimSequence, InBakeBoneData, FVector2D(BeginTime, EndTime));
}

bool UVAT_Bone::OnBakeBoneTransformsFromRange(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData, FVector2D InRange)
{
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return false;
	}

	bool IsValid = false;
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(InAnimSequence))
	{
		for (FVirtualBoneTransforms& BoneTransforms : InBakeBoneData.BakeBoneTransforms)
		{
			const int32& BoneIndex = theReferenceSkeleton.FindBoneIndex(BoneTransforms.Bone.BoneName);
			if (BoneIndex != INDEX_NONE && InBakeBoneData.IsValid())
			{
				IsValid = true;
				const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);
				FTransform theRootBoneTransformCS = theReferenceSkeleton.GetRawRefBonePose()[0];

				const int32& AnimFrames = UVAT_Library::GetNumberOfKeys(AnimSequence);
				const float& AnimSequenceLength = UVAT_Library::GetSequenceLength(AnimSequence);

				const float FrameTime = AnimSequenceLength / AnimFrames;
				const float DeltaTime = InBakeBoneData.BakeFPS > 0 ? 1.f / InBakeBoneData.BakeFPS : 1 / 30.f;

				const bool IsValidRange = FMath::IsWithinInclusive(InRange.Y - InRange.X, 0.f, AnimSequenceLength);
				const float BeginTime = IsValidRange ? InRange.X : 0.f;
				const float EndTime = IsValidRange ? InRange.Y : AnimSequenceLength;

				int32 Count = 0;
				float Position = BeginTime;
				BoneTransforms.BoneTransforms.Reset();

				while (Position < (EndTime + KINDA_SMALL_NUMBER))
				{
					FTransform theBoneTransformCS;
					FName ParentBoneName = BoneTransforms.Bone.BoneName;
					while (ParentBoneName != theRootBoneName)
					{
						FTransform theParentBoneTransformLS;
						UAnimationBlueprintLibrary::GetBonePoseForTime(AnimSequence, ParentBoneName, Position, false, theParentBoneTransformLS);

						theBoneTransformCS *= theParentBoneTransformLS;

						ParentBoneName = theReferenceSkeleton.GetBoneName(theReferenceSkeleton.GetParentIndex(theReferenceSkeleton.FindBoneIndex(ParentBoneName)));
					}

					theBoneTransformCS *= theRootBoneTransformCS;

					FTransform SocketTransform = FTransform::Identity;
					SocketTransform.SetLocation(InBakeBoneData.BakeSocketLocation);
					SocketTransform.SetRotation(InBakeBoneData.BakeSocketRotation.Quaternion());
					theBoneTransformCS = SocketTransform * theBoneTransformCS;

					BoneTransforms.BoneTransforms.Add(theBoneTransformCS);

					++Count;
					Position += DeltaTime;
				}
			}
		}
	}

	return IsValid;
}

UAnimNotify* UVAT_Bone::OnBakeBoneTransformsToNotify(UAnimSequence* InAnimSequenceBase, FVirtualBakeBoneData& InBakeBoneData)
{
	if (!InAnimSequenceBase || !InAnimSequenceBase->GetSkeleton())
	{
		return nullptr;
	}

	USkeleton* theSkeleton = InAnimSequenceBase->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	const bool IsInvalidName = InBakeBoneData.BakeNotifyTrackName == InvalidName;
	if (!IsInvalidName && !UVAT_Notify::IsValidAnimNotifyTrackName(InAnimSequenceBase, InBakeBoneData.BakeNotifyTrackName))
	{
		return nullptr;
	}

	TArray<struct FAnimNotifyEvent> Notifies = InAnimSequenceBase->Notifies;
	const int32& TrackIndex = IsInvalidName ? INDEX_NONE : UVAT_Notify::GetTrackIndexForAnimNotifyTrackName(InAnimSequenceBase, InBakeBoneData.BakeNotifyTrackName);

	for (int i = 0; i < Notifies.Num(); i++)
	{
		FAnimNotifyEvent& AnimNotifyEvent = Notifies[i];
		if (AnimNotifyEvent.Notify && AnimNotifyEvent.Notify->GetClass() == InBakeBoneData.BakeNotifyClass && (IsInvalidName || AnimNotifyEvent.TrackIndex == TrackIndex))
		{
			const float& NotifyDuration = AnimNotifyEvent.GetDuration();
			const float& NotifyPosition = FMath::Abs(AnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : AnimNotifyEvent.GetTriggerTime();

			// Bake notify state
			if (OnBakeBoneTransformsFromRange(InAnimSequenceBase, InBakeBoneData, FVector2D(NotifyPosition, InBakeBoneData.GetFrameNumber())))
			{
				return AnimNotifyEvent.Notify;
			}
		}
	}

	return nullptr;
}

UAnimNotifyState* UVAT_Bone::OnBakeBoneTransformsToNotifyState(UAnimSequence* InAnimSequenceBase, FVirtualBakeBoneData& InBakeBoneData)
{
	if (!InAnimSequenceBase || !InAnimSequenceBase->GetSkeleton())
	{
		return nullptr;
	}

	USkeleton* theSkeleton = InAnimSequenceBase->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	const bool IsInvalidName = InBakeBoneData.BakeNotifyTrackName == InvalidName;
	if (!IsInvalidName && !UVAT_Notify::IsValidAnimNotifyTrackName(InAnimSequenceBase, InBakeBoneData.BakeNotifyTrackName))
	{
		return nullptr;
	}

	TArray<struct FAnimNotifyEvent> Notifies = InAnimSequenceBase->Notifies;
	const int32& TrackIndex = IsInvalidName ? INDEX_NONE : UVAT_Notify::GetTrackIndexForAnimNotifyTrackName(InAnimSequenceBase, InBakeBoneData.BakeNotifyTrackName);

	for (int i = 0; i < Notifies.Num(); i++)
	{
		FAnimNotifyEvent& AnimNotifyEvent = Notifies[i];
		if (AnimNotifyEvent.NotifyStateClass && AnimNotifyEvent.NotifyStateClass->GetClass() == InBakeBoneData.BakeNotifyStateClass && (IsInvalidName || AnimNotifyEvent.TrackIndex == TrackIndex))
		{
			const float& NotifyDuration = AnimNotifyEvent.GetDuration();
			const float& NotifyPosition = FMath::Abs(AnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : AnimNotifyEvent.GetTriggerTime();

			// Bake notify state
			if (OnBakeBoneTransformsFromRange(InAnimSequenceBase, InBakeBoneData, FVector2D(NotifyPosition, NotifyPosition + NotifyDuration)))
			{
				return AnimNotifyEvent.NotifyStateClass;
			}
		}
	}

	return nullptr;
}

bool UVAT_Bone::OnBakeBoneTransformsToNotifyEvent(UAnimSequence* InAnimSequence, const FAnimNotifyEvent& InAnimNotifyEvent, FVirtualBakeBoneData& InBakeBoneData)
{
	const float& NotifyDuration = InAnimNotifyEvent.GetDuration();
	const float& NotifyPosition = FMath::Abs(InAnimNotifyEvent.GetTriggerTime()) == KINDA_SMALL_NUMBER ? 0.f : InAnimNotifyEvent.GetTriggerTime();

	if (InAnimNotifyEvent.Notify)
	{
		if (InAnimSequence)
		{
			const int32& AnimFrames = UVAT_Library::GetNumberOfKeys(InAnimSequence);
			const float& AnimSequenceLength = UVAT_Library::GetSequenceLength(InAnimSequence);
			const float FrameTime = AnimSequenceLength / AnimFrames;

			InBakeBoneData.BakeFrameRange = FVector2D(NotifyPosition, NotifyPosition + (InBakeBoneData.BakeFrameRange.Y *FrameTime));
			return OnBakeBoneTransformsFromRange(InAnimSequence, InBakeBoneData, InBakeBoneData.BakeFrameRange);
		}
	}
	else if (InAnimNotifyEvent.NotifyStateClass)
	{
		InBakeBoneData.BakeFrameRange = FVector2D(NotifyPosition, NotifyPosition + NotifyDuration);
		return OnBakeBoneTransformsFromRange(InAnimSequence, InBakeBoneData, InBakeBoneData.BakeFrameRange);
	}

	return false;
}
#pragma endregion
#endif

#pragma region Bone Data
void UVAT_Bone::AddBonesData(USkeletalMesh* InSkeletalMesh, const TArray<FVirtualAddBoneData>& InBonesData)
{
	// Check the skeletal mesh is valid
	if (InSkeletalMesh == nullptr)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	USkeleton* theSkeleton = InSkeletalMesh->GetSkeleton();
#else
	USkeleton* theSkeleton = InSkeletalMesh->Skeleton;
#endif
	// Check the skeleton is valid
	if (theSkeleton == nullptr)
	{
		return;
	}

	// Get the reference skeleton modifier
#if  ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 27 
	FReferenceSkeleton& theReferenceSkeleton = InSkeletalMesh->GetRefSkeleton();
#else
	FReferenceSkeleton& theReferenceSkeleton = InSkeletalMesh->RefSkeleton;
#endif
	FReferenceSkeletonModifier theReferenceSkeletonModifier(theReferenceSkeleton, theSkeleton);

	// Each every bones data
	for (const FVirtualAddBoneData& theBoneData : InBonesData)
	{
		// Check the bone index is valid
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneData.BoneName);
		if (theBoneIndex != INDEX_NONE)
		{
			continue;
		}

		// Check the parent bone index is valid
		const int32& theParentBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneData.ParentBone.BoneName);
		if (theParentBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Calculate the new bone local space transform in skeleton
		FTransform theBoneTransformLS(theBoneData.BoneRotation, theBoneData.BoneLocation, theBoneData.BoneScale);
		if (theBoneData.ConstraintBone.BoneName != NAME_None)
		{
			FTransform theParentBoneTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theBoneData.ParentBone.BoneName);
			FTransform theConstraintBoneTransformCS = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, theBoneData.ConstraintBone.BoneName);
			theBoneTransformLS = theConstraintBoneTransformCS;
			theBoneTransformLS.SetToRelativeTransform(theParentBoneTransformCS);
			theBoneTransformLS.NormalizeRotation();
		}

		// Make the new bone mesh info
		FMeshBoneInfo theNewMeshBoneInfo(theBoneData.BoneName, theBoneData.BoneName.ToString(), theParentBoneIndex);

		// Add the bone to reference skeleton modifier
		theReferenceSkeletonModifier.Add(theNewMeshBoneInfo, theBoneTransformLS);

		// Add the bone in last index
		const int32 theNewBoneIndex = theReferenceSkeleton.GetRawRefBoneInfo().Num() - 1;

#if 1
		// Add the bone to every LODs mesh data
		for (int32 LODIndex = 0; LODIndex < InSkeletalMesh->GetImportedModel()->LODModels.Num(); LODIndex++)
		{
			FSkeletalMeshLODModel* theLodRenderData = &InSkeletalMesh->GetImportedModel()->LODModels[LODIndex];
			theLodRenderData->ActiveBoneIndices.Add(theNewBoneIndex);
			theLodRenderData->RequiredBones.Add(theNewBoneIndex);
		}
#else
		// Add the bone to every LODs mesh data
		if (FSkeletalMeshRenderData* theSkeletalMeshRenderData = InSkeletalMesh->GetResourceForRendering())
		{
			for (FSkeletalMeshLODRenderData& theLODData : theSkeletalMeshRenderData->LODRenderData)
			{
				theLODData.ActiveBoneIndices.Add(theNewBoneIndex);
				theLODData.RequiredBones.Add(theNewBoneIndex);
			}
		}
#endif
	}

	// Flag the skeleton and 
	theSkeleton->Modify();
	InSkeletalMesh->Modify();

#if 0
	// there is no way to call a protected method
	theSkeleton->HandleSkeletonHierarchyChange();
#else
	// In order to call the protected method, we need to add the virtual bone first, and then delete it.
	FName theVirtualBoneName = "VAT_Test";
	theSkeleton->AddNewVirtualBone(theReferenceSkeleton.GetBoneName(0), theReferenceSkeleton.GetBoneName(0), theVirtualBoneName);

	TArray<FName> theRemoveVirtualBonesName{ theVirtualBoneName };
	theSkeleton->RemoveVirtualBones(theRemoveVirtualBonesName);
#endif
	// Rebuild the reference skeleton
	theReferenceSkeleton.RebuildRefSkeleton(theSkeleton, true);
	
	// Merge all bones to bone tree
	//theSkeleton->MergeAllBonesToBoneTree(InSkeletalMesh);

	// Create skeleton object and merge all bones
	InSkeletalMesh->InvalidateDeriveDataCacheGUID();
	InSkeletalMesh->PostEditChange();

	// Merge all bones to bone tree
	theSkeleton->MergeAllBonesToBoneTree(InSkeletalMesh);

	// Flag post load
	InSkeletalMesh->PostLoad();

	// Apply the skeletal mesh changed
	UVAT_Library::ModifyFromObject(InSkeletalMesh);
}

void UVAT_Bone::RemoveBonesData(USkeleton* InSkeleton, const TArray<FBoneReference>& InBonesReference, bool InRemoveChildBones)
{
	// If the specified skeleton is valid, we delete the bones on the specified bone and clean up the skeletal mesh using that skeleton
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Get the remove bones name
	TArray<FName> theRemoveBonesName;
	for (const FBoneReference& theBone : InBonesReference)
	{
		theRemoveBonesName.AddUnique(theBone.BoneName);
	}

	// Check the remove bones number is valid
	if (theRemoveBonesName.Num() == 0)
	{
		return;
	}

	// Get the skeleton using skeletal mesh
	TArray<USkeletalMesh*> theSkeletalMeshes;
	for (TObjectIterator<USkeletalMesh> SkeletalMeshIterator; SkeletalMeshIterator; ++SkeletalMeshIterator)
	{
		USkeletalMesh* theSkeletalMesh = *SkeletalMeshIterator;
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 27
		if (theSkeletalMesh != nullptr && theSkeletalMesh->Skeleton == InSkeleton)
		{
			theSkeletalMeshes.Add(theSkeletalMesh);
			RemoveBonesData(theSkeletalMesh, InBonesReference, InRemoveChildBones);
		}
#else
		if (theSkeletalMesh != nullptr && theSkeletalMesh->GetSkeleton() == InSkeleton)
		{
			theSkeletalMeshes.Add(theSkeletalMesh);
			RemoveBonesData(theSkeletalMesh, InBonesReference, InRemoveChildBones);
		}
#endif
	}

	// Apply the skeleton changed
	UVAT_Library::ModifyFromObject(InSkeleton);
	return;
}

void UVAT_Bone::RemoveBonesData(USkeletalMesh* InSkeletalMesh, const TArray<FBoneReference>& InBonesReference, bool InRemoveChildBones)
{
	// Check the skeletal mesh is valid
	if (InSkeletalMesh == nullptr)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	USkeleton* theSkeleton = InSkeletalMesh->GetSkeleton();
#else
	USkeleton* theSkeleton = InSkeletalMesh->Skeleton;
#endif
	// Check the skeleton is valid
	if (theSkeleton == nullptr)
	{
		return;
	}

	// Get the reference skeleton modifier
#if  ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 27 
	FReferenceSkeleton& theReferenceSkeleton = InSkeletalMesh->GetRefSkeleton();
#else
	FReferenceSkeleton& theReferenceSkeleton = InSkeletalMesh->RefSkeleton;
#endif
	FReferenceSkeletonModifier theReferenceSkeletonModifier(theReferenceSkeleton, theSkeleton);

	// Flag the skeleton and 
	theSkeleton->Modify();
	InSkeletalMesh->Modify();

	// Get the remove bones name
	TArray<FName> theRemoveBonesName;
	TArray<int32> theRemoveBoneIdices;
	for (const FBoneReference& theBone : InBonesReference)
	{
		// Check the bone name is valid
		if (theBone.BoneName == NAME_None)
		{
			continue;
		}

		theRemoveBonesName.AddUnique(theBone.BoneName);
		theRemoveBoneIdices.AddUnique(theReferenceSkeleton.FindBoneIndex(theBone.BoneName));
	}

	// Check the remove bones number is valid
	if (theRemoveBonesName.Num() == 0 || theRemoveBonesName.Num() != theRemoveBoneIdices.Num())
	{
		return;
	}

	// Remove the bones in mesh reference skeleton
	theRemoveBoneIdices = theReferenceSkeleton.RemoveBonesByName(theSkeleton, theRemoveBonesName);

#if 1
	int32 theRemoveCount = theRemoveBoneIdices.Num();
	while (theRemoveCount > 0)
	{
		--theRemoveCount;
		// remove the bone to source mesh
		for (int32 LODIndex = 0; LODIndex < InSkeletalMesh->GetImportedModel()->LODModels.Num(); LODIndex++)
		{
			FSkeletalMeshLODModel* theLodRenderData = &InSkeletalMesh->GetImportedModel()->LODModels[LODIndex];
			//theLodRenderData->ActiveBoneIndices.RemoveAt(theLodRenderData->RequiredBones.Num() - 1);
			theLodRenderData->RequiredBones.RemoveAt(theLodRenderData->RequiredBones.Num() - 1);
		}

		// remove the bone to every LODs mesh data
		if (FSkeletalMeshRenderData* theSkeletalMeshRenderData = InSkeletalMesh->GetResourceForRendering())
		{
			for (FSkeletalMeshLODRenderData& theLODData : theSkeletalMeshRenderData->LODRenderData)
			{
				//theLODData.ActiveBoneIndices.RemoveAt(theLODData.ActiveBoneIndices.Num() - 1);
				theLODData.RequiredBones.RemoveAt(theLODData.RequiredBones.Num() - 1);
			}
		}
	}
#else
	// remove the bone to every LODs mesh data
	if (FSkeletalMeshRenderData* theSkeletalMeshRenderData = InSkeletalMesh->GetResourceForRendering())
	{
		for (FSkeletalMeshLODRenderData& theLODData : theSkeletalMeshRenderData->LODRenderData)
		{
			break;
			int32 theRemoveCount = theRemoveBoneIdices.Num();
			while (theRemoveCount > 0)
			{
				--theRemoveCount;
				//theLODData.ActiveBoneIndices.RemoveAt(theLODData.ActiveBoneIndices.Num() - 1);
				theLODData.RequiredBones.RemoveAt(theLODData.RequiredBones.Num() - 1);
			}
		}
	}
#endif

	// In order to call the protected method, we need to add the virtual bone first, and then delete it.
	FName theVirtualBoneName = "VAT_Test";
	theSkeleton->AddNewVirtualBone(theReferenceSkeleton.GetBoneName(0), theReferenceSkeleton.GetBoneName(0), theVirtualBoneName);

	TArray<FName> theRemoveVirtualBonesName{ theVirtualBoneName };
	theSkeleton->RemoveVirtualBones(theRemoveVirtualBonesName);

	// Cleanup the skeletal mesh
	InSkeletalMesh->InvalidateDeriveDataCacheGUID();
	InSkeletalMesh->PostEditChange();

	// Flag the skeleton is modify changed
	theSkeleton->Modify();

	// Remove the bones from skeleton
	theSkeleton->RemoveBonesFromSkeleton(theRemoveBonesName, InRemoveChildBones);

	// Flag post load
	InSkeletalMesh->PostLoad();

	// Apply the skeletal mesh changed
	UVAT_Library::ModifyFromObject(InSkeletalMesh);
}
#pragma endregion


#pragma region Bone Track
void UVAT_Bone::AddBonesTrack(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneData>& InBonesData)
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
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Each every bone data
	for (const FVirtualBoneData& theBoneData : InBonesData)
	{
		// Check the track data is valid
		if (theBoneData.TracksData.Num() == 0)
		{
			continue;
		}

		// Get the bone data
		const FName& theBoneName = theBoneData.Bone.BoneName;
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);

		// Check the bone data is valid
		if (theBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Define the new bone track data
		FRawAnimSequenceTrack theNewBoneTrackData;

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Define the bone data
			FTransform theBoneTransform = theBoneData.TracksData[0].GetTransform();

			// Get the pos key transform data
			const FTransform theTrackBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, theKeyTime, theBoneName);

			// Choose modify type
			switch (theBoneData.ModifyType)
			{
			case EVirtualBoneModifyType::VBT_Delta:
				theBoneTransform = UVAT_Library::Subtract_TransformTransform(theTrackBoneTransform, theBoneTransform);
				break;

			case EVirtualBoneModifyType::VBT_Additive:
				theBoneTransform = UVAT_Library::Add_TransformTransform(theTrackBoneTransform, theBoneTransform);
				break;

			case EVirtualBoneModifyType::VBT_Replace:
				theBoneTransform = theTrackBoneTransform;
				break;
			}

			// Normalize rotation
			theBoneTransform.NormalizeRotation();

			// Cache the pose key to track data
			UVAT_Library::AddBoneTrackKey(theNewBoneTrackData, theBoneTransform);
		}

		// Apply the bone track data changed
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theNewBoneTrackData);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Bone::ModifyBonesTrack(UAnimSequence* InAnimSequence, FVirtualModifyBoneData& InModifyBoneData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Each every bone data
	for (const FVirtualBoneData& theBoneData : InModifyBoneData.BonesData)
	{
		// Get the bone data
		const FName& theBoneName = theBoneData.Bone.BoneName;
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);

		// Check the bone data is valid
		if (theBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Check the bone track index is valid
		const int32& theBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theBoneName);
		if (theBoneTrackIndex == INDEX_NONE)
		{
			continue;
		}

#if 0
		// Get the source bone track data
		FRawAnimSequenceTrack theSourceBoneTrackData;
		UVAT_Library::GetBoneTrackData(InAnimSequence, theBoneTrackIndex, theSourceBoneTrackData);
#endif

		// Define the new bone track data
		FRawAnimSequenceTrack theNewBoneTrackData;
		if (!InModifyBoneData.bMakeFisrtFrameIsZero)
		{
			theBoneData.GetRawBoneTrackData(theNumberOfKeys, theNewBoneTrackData);
		}

		// Check the bone custom track is valid
		if (theNewBoneTrackData.PosKeys.Num() == 0 || theNewBoneTrackData.RotKeys.Num() == 0)
		{
			UVAT_Library::GetBoneTrackData(InAnimSequence, theBoneName, theNewBoneTrackData, true);
		}

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Define the bone data
			FTransform theBoneTransform;
#if ENGINE_MAJOR_VERSION > 4
			FTransform theTrackBoneTransform(FQuat(theNewBoneTrackData.RotKeys[KeyIndex]), FVector(theNewBoneTrackData.PosKeys[KeyIndex]), FVector(theNewBoneTrackData.ScaleKeys[KeyIndex]));
#else
			FTransform theTrackBoneTransform(theNewBoneTrackData.RotKeys[KeyIndex], theNewBoneTrackData.PosKeys[KeyIndex], theNewBoneTrackData.ScaleKeys[KeyIndex]);
#endif
			// Choose the bone space data
			if (InModifyBoneData.bMakeFisrtFrameIsZero)
			{
				InModifyBoneData.ModifyType = EVirtualBoneModifyType::VBT_Delta;
				if (InModifyBoneData.BoneSpace == EBoneControlSpace::BCS_BoneSpace)
				{
					theBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, 0, theBoneName);
				}
				else
				{
					theBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, 0, theBoneName);
				}
			}
			else if (InModifyBoneData.BoneSpace == EBoneControlSpace::BCS_BoneSpace)
			{
				theBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, KeyIndex, theBoneName);
			}
			else
			{
				theBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theBoneName);
			}

			// Scale the transform data
			if (InModifyBoneData.ModifyType != EVirtualBoneModifyType::VBT_Blend)
			{
				// Convert translation
				FVector theAdjustTranslation = theBoneTransform.GetLocation();
				theAdjustTranslation.X *= InModifyBoneData.TranslationRatio.X;
				theAdjustTranslation.Y *= InModifyBoneData.TranslationRatio.Y;
				theAdjustTranslation.Z *= InModifyBoneData.TranslationRatio.Z;
				theBoneTransform.SetTranslation(theAdjustTranslation);

				// Convert rotation
				FRotator theAdjustRotation = theBoneTransform.Rotator();
				theAdjustRotation.Roll *= InModifyBoneData.RotationRatio.X;
				theAdjustRotation.Pitch *= InModifyBoneData.RotationRatio.Y;
				theAdjustRotation.Yaw *= InModifyBoneData.RotationRatio.Z;
				theBoneTransform.SetRotation(theAdjustRotation.Quaternion());

				// Convert scale
				FVector theAdjustScale = theBoneTransform.GetScale3D();
				theAdjustScale.X *= InModifyBoneData.ScaleRatio.X;
				theAdjustScale.Y *= InModifyBoneData.ScaleRatio.Y;
				theAdjustScale.Z *= InModifyBoneData.ScaleRatio.Z;
				theBoneTransform.SetScale3D(theAdjustScale);
			}

			// Choose modify type
			switch (InModifyBoneData.ModifyType)
			{
			case EVirtualBoneModifyType::VBT_Delta:
				theBoneTransform = UVAT_Library::Subtract_TransformTransform(theTrackBoneTransform, theBoneTransform);
				break;

			case EVirtualBoneModifyType::VBT_Blend:
				theBoneTransform = UVAT_Library::BlendTransform(theBoneTransform, theTrackBoneTransform, InModifyBoneData.TranslationRatio
					, InModifyBoneData.RotationRatio, InModifyBoneData.ScaleRatio);
				break;

			case EVirtualBoneModifyType::VBT_Additive:
				theBoneTransform = UVAT_Library::Add_TransformTransform(theTrackBoneTransform, theBoneTransform);
				break;

			case EVirtualBoneModifyType::VBT_Replace:
				theBoneTransform = theTrackBoneTransform;
				break;
			}

			// Convert the bone to local space
			if (InModifyBoneData.BoneSpace != EBoneControlSpace::BCS_BoneSpace)
			{
				const int32 theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
				if (theParentBoneIndex != INDEX_NONE)
				{
					const FName theParentBoneName = theReferenceSkeleton.GetBoneName(theParentBoneIndex);
					if (theParentBoneName != NAME_None)
					{
						const FTransform theParentBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theParentBoneName);
						theBoneTransform.SetToRelativeTransform(theParentBoneTransformCS);
					}
				}
			}

			// Normalize rotation
			theBoneTransform.NormalizeRotation();

			// Cache the pose key to track data
			UVAT_Library::ModifyBoneTrackKey(theNewBoneTrackData, theBoneTransform, KeyIndex);
		}

		// Apply the bone track data changed
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theNewBoneTrackData);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Bone::RemoveBonesTrack(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneData>& InBonesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Each every bone data
	for (const FVirtualBoneData& theBoneData : InBonesData)
	{
		// Get the bone data
		const FName& theBoneName = theBoneData.Bone.BoneName;
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);

		// Check the bone data is valid
		if (theBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Insert the bone track
		const int32& InsertBoneIndex = UVAT_Library::InsertBoneTrack(InAnimSequence, theBoneName, theBoneIndex);

		// Check the insert bone track is valid
		if (InsertBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Remove the bone track
		UVAT_Library::RemoveBoneTrack(InAnimSequence, theBoneData.Bone.BoneName, theBoneData.bIncludeTranslation, theBoneData.bIncludeRotation, theBoneData.bIncludeScale);
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Bone::SampleBonesTrack(const EBoneControlSpace& InBoneSpace, UAnimSequence* InAnimSequence, TArray<FVirtualBoneData>& InBonesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Create the dialog task
	FScopedSlowTask theBoneToolsTask(InBonesData.Num(), LOCTEXT("BoneToolsText", "Sampling animation assets bones track data"));
	theBoneToolsTask.MakeDialog(true);

	// Each every bone data
	for (FVirtualBoneData& theBoneData : InBonesData)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theBoneToolsTask.ShouldCancel())
		{
			break;
		}

		// Flag enter progress frame
		theBoneToolsTask.EnterProgressFrame();

		// Get the bone data
		const FName& theBoneName = theBoneData.Bone.BoneName;
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);

		// Check the bone data is valid
		if (theBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// Check the bone track index is valid
		const int32& theBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theBoneName);
		if (theBoneTrackIndex == INDEX_NONE)
		{
			continue;
		}

		// Reset the output bone track
		theBoneData.TracksData.Init(FVirtualTransformData(), theNumberOfKeys);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the source bone transform
			FTransform theBoneTransform = FTransform::Identity;

			// Output desired bone space transform
			switch (InBoneSpace)
			{
			case EBoneControlSpace::BCS_BoneSpace:
				theBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, theKeyTime, theBoneName);
				break;

			default:
				theBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, theKeyTime, theBoneName);
				break;
			}

			// Cache the pose key to track data
			theBoneData.TracksData[KeyIndex].Location = theBoneTransform.GetLocation();
			theBoneData.TracksData[KeyIndex].Rotation = theBoneTransform.Rotator();
			theBoneData.TracksData[KeyIndex].Scale = theBoneTransform.GetScale3D();
		}
	}
}
#pragma endregion


#pragma region Bone Layer
void UVAT_Bone::LayeredBoneBlend(UAnimSequence* InAnimSequence, FVirtualBonesLayerData& InBoneLayerData
	, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData)
{
	// Get the layer animation sequence asset
	UAnimSequence* theLayerAnimSequence = InBoneLayerData.LayerAnimSequence;

	// Check the animation asset is valid
	if (!InAnimSequence || !theLayerAnimSequence)
	{
		return;
	}

	// Check if both sides are the same skeleton
	if (InAnimSequence->GetSkeleton() != theLayerAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	int32 theSourcePoseNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const int32& theLayerPoseNumberOfKeys = UVAT_Library::GetNumberOfKeys(theLayerAnimSequence);

	// Each every local space blend bone
	TArray<FName> theLocalSpaceBonesName;
	for (const FBoneReference& theLocalSpaceBone : InBoneLayerData.LocalSpaceBones)
	{
		// Check the local space bone is valid
		if (theLocalSpaceBone.BoneName != NAME_None)
		{
			continue;
		}

#if 0 // Merge
		// Get layer pose local space bone track
		FRawAnimSequenceTrack theLayerPoseTrack;
		UVAT_Library::GetBoneTrackData(theLayerAnimSequence, theLocalSpaceBone.BoneName, theLayerPoseTrack);

		// Apply the bone track data changed
		UVAT_Library::SetBoneTrackData(InAnimSequence, theLocalSpaceBone.BoneName, &theLayerPoseTrack);
#endif
		// Cache the local space name to array
		theLocalSpaceBonesName.AddUnique(theLocalSpaceBone.BoneName);
	}

	// Check the matching both animation length condition
	if (InBoneLayerData.bResizeSourcePoseLength)
	{
		FVirtualAssetResizeData theResizeData = FVirtualAssetResizeData();
		const int32 theFrameRate = 1.f / UVAT_Library::GetFrameTime(InAnimSequence);
		theResizeData.FrameRate = FFrameRate(theFrameRate, 1);
		theResizeData.Length = theLayerAnimSequence->GetPlayLength();
		UVAT_Asset::ResizeAnimationAsset(InAnimSequence, theResizeData);
		theSourcePoseNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	}

	// Each every branch filter
	for (const FVirtualBoneBranchFilter& theBranchFilter : InBoneLayerData.BonesBranchFilter)
	{
		// Get all modifier bones name
		const TArray<FName> theBoneTreeNames = theBranchFilter.GetBoneTreeNames(theSkeleton);

		// Each every bone name
		for (const FName& theBoneName : theBoneTreeNames)
		{
			// Check the bone index is valid
			const int32 theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);
			if (theBoneIndex == INDEX_NONE)
			{
				continue;
			}

			// Check the parent bone index is valid 
			const int32 theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
			if (theParentBoneIndex == INDEX_NONE)
			{
				continue;
			}
			
			// Check the parent bone name is valid
			const FName theParentBoneName = theReferenceSkeleton.GetBoneName(theParentBoneIndex);
			if (theParentBoneName == NAME_None)
			{
				continue;
			}

			// Define the raw track data
			FRawAnimSequenceTrack theNewTrackData = FRawAnimSequenceTrack();

			// Cache the bone data of the layer pose and scale the length
			FVirtualBoneCurvesData theLayerPoseBoneCurvesData = FVirtualBoneCurvesData();

			// Check the matching both animation length condition
			if (InBoneLayerData.bResizeLayerPoseLength)
			{
				// Each every layer pose keys
				for (int32 KeyIndex = 0; KeyIndex < theLayerPoseNumberOfKeys; KeyIndex++)
				{
					// Evaluate the key time
					const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

					// Choose bone space
					if (InBoneLayerData.bMeshSpaceRotationBlend && !theLocalSpaceBonesName.Contains(theBoneName))
					{
						// Get the layer pose bone component space transform
						const FTransform theLayerSourceBoneTransformCS = UVAT_Bone::GetBoneTransformCS(theLayerAnimSequence, false, KeyIndex, theBoneName);
						theLayerPoseBoneCurvesData.AddTransformKey(theKeyTime, theLayerSourceBoneTransformCS);
					}
					else
					{
						// Get the layer pose bone component local transform
						const FTransform theLayerSourceBoneTransformLS = UVAT_Bone::GetBoneTransformLS(theLayerAnimSequence, false, KeyIndex, theBoneName);
						theLayerPoseBoneCurvesData.AddTransformKey(theKeyTime, theLayerSourceBoneTransformLS);
					}
				}

				// Resize the layer pose length to matching source pose length
				FVector2D theSourcePoseTimeRange(0.f, InAnimSequence->GetPlayLength());
				theLayerPoseBoneCurvesData.Resize(&theSourcePoseTimeRange);
			}

			// Each every source pose keys
			for (int32 KeyIndex = 0; KeyIndex < theSourcePoseNumberOfKeys; KeyIndex++)
			{
				// Evaluate the key time
				const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

				// Choose local space or mesh space blend
				if (InBoneLayerData.bMeshSpaceRotationBlend && !theLocalSpaceBonesName.Contains(theBoneName))
				{
					// Get the source pose parent bone component space transform
					const FTransform theSourceParentBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, true, KeyIndex, theParentBoneName);
					
					// Get the layer pose bone component space transform
					FTransform theLayerSourceBoneTransformCS = UVAT_Bone::GetBoneTransformCS(theLayerAnimSequence, false, KeyIndex, theBoneName);

					// If is resize layer pose, we get previous cached pose data
					if (InBoneLayerData.bResizeLayerPoseLength)
					{
						theLayerSourceBoneTransformCS = theLayerPoseBoneCurvesData.Evaluate(theKeyTime);
					}

					// Convert layer pose to source pose local space
					FTransform theNewSourceBoneTransformLS = theLayerSourceBoneTransformCS;
					theNewSourceBoneTransformLS.SetToRelativeTransform(theSourceParentBoneTransformCS);

					// Cache the pose key
					UVAT_Library::AddBoneTrackKey(theNewTrackData, theNewSourceBoneTransformLS);	
				}
				else
				{
					// Get the layer pose bone component local transform
					FTransform theLayerSourceBoneTransformLS = UVAT_Bone::GetBoneTransformLS(theLayerAnimSequence, false, KeyIndex, theBoneName);

					// If is resize layer pose, we get previous cached pose data
					if (InBoneLayerData.bResizeLayerPoseLength)
					{
						theLayerSourceBoneTransformLS = theLayerPoseBoneCurvesData.Evaluate(theKeyTime);
					}

					// Cache the pose key
					UVAT_Library::AddBoneTrackKey(theNewTrackData, theLayerSourceBoneTransformLS);
				}
			}

			// Apply the bone track data changed
			UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theNewTrackData);
		}
	}

	// Constraint default config bones
	if (!SampleConstraintBones(InAnimSequence, InConstraintBonesData))
	{
		UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	}
}
#pragma endregion


#pragma region Bone Filter
void UVAT_Bone::FilterBoneTracks(UAnimSequence* InAnimSequence, const FVirtualBonesFilterData& InBoneFilterData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the animation asset skeleton data
	const USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Each every branch filter
	for (const FVirtualBoneBranchFilter& theBranchFilter : InBoneFilterData.BonesBranchFilter)
	{
		// Get all modifier bones name
		const TArray<FName> theBoneTreeNames = theBranchFilter.GetBoneTreeNames(theSkeleton);

		// Each every bone name
		for (const FName& theBoneName : theBoneTreeNames)
		{
			// Check the bone index is valid
			const int32 theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);
			if (theBoneIndex == INDEX_NONE)
			{
				continue;
			}

			// Check the parent bone index is valid 
			const int32 theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
			if (theParentBoneIndex == INDEX_NONE)
			{
				continue;
			}

			// Check the parent bone name is valid
			const FName theParentBoneName = theReferenceSkeleton.GetBoneName(theParentBoneIndex);
			if (theParentBoneName == NAME_None)
			{
				continue;
			}

			// Cache the bone curves data
			FVirtualBoneCurvesData theBoneCurvesData = FVirtualBoneCurvesData();

			// Each every pose keys
			for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
			{
				// Evaluate the key time
				const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

				// Get the bone local space transform
				const FTransform theBoneTransformLS = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, KeyIndex, theBoneName);

				// Cache the transform key
				theBoneCurvesData.AddTransformKey(theKeyTime, theBoneTransformLS);
			}

			// Apply the bone track filter
			theBoneCurvesData.Filter(InBoneFilterData.FilterType, InBoneFilterData.Tolerance);

			// Define the raw track data
			FRawAnimSequenceTrack theNewTrackData = FRawAnimSequenceTrack();

			// Each every pose keys
			for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
			{
				// Evaluate the key time
				const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

				// Get the bone local space transform
				const FTransform theBoneTransformLS = theBoneCurvesData.Evaluate(theKeyTime);

				// Cache the bone track data
				UVAT_Library::AddBoneTrackKey(theNewTrackData, theBoneTransformLS);
			}

			// Apply the bone track data changed
			UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theNewTrackData);
		}
	}

	// Apply animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}
#pragma endregion


#pragma region Bone Constraint
bool UVAT_Bone::SampleConstraintBones(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData)
{
	// Check the constraint bones data is valid
	if (InConstraintBonesData.Num() == 0)
	{
		return false;
	}

	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return false;
	}

#if 0
	InAnimSequence->Modify(true);
#endif

	// Get the animation asset skeleton data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

#if ENGINE_MAJOR_VERSION > 4
	UAnimDataModel* theAnimDataModel = InAnimSequence->GetDataModel();
	IAnimationDataController& theDataController = InAnimSequence->GetController();
	theDataController.OpenBracket(LOCTEXT("ConstraintBone_Description", "Constraint bone."), false);
#endif

#if 0
	InAnimSequence->MarkRawDataAsModified();
	InAnimSequence->PostEditChange();
	InAnimSequence->MarkPackageDirty();
#endif

#if ENGINE_MAJOR_VERSION < 5
	// Transform keys data for baked bones
	if (InAnimSequence->HasBakedTransformCurves())
	{
		InAnimSequence->BakeTrackCurvesToRawAnimation();
		InAnimSequence->ClearBakedTransformData();
	}
#endif

	// Each every constraint bone data
	for (const FVirtualBoneConstraintData& theConstraintBoneData : InConstraintBonesData)
	{
		// Check the bone data is valid
		if (theConstraintBoneData.SourceBone.BoneName == NAME_Name
			|| theConstraintBoneData.TargetBone.BoneName == NAME_Name)
		{
			continue;
		}

		// Get the source bone track data
		FRawAnimSequenceTrack theSourceBoneTrackData, theNewSourceBoneTrackData;
		const int32& theSourceBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theConstraintBoneData.SourceBone.BoneName);
		if (theSourceBoneTrackIndex == INDEX_NONE)
		{
			UVAT_Library::AddBoneTrack(InAnimSequence, theConstraintBoneData.SourceBone.BoneName);
		}
		UVAT_Library::GetBoneTrackData(InAnimSequence, theConstraintBoneData.SourceBone.BoneName, theSourceBoneTrackData);

		// Get the target bone track data
		FRawAnimSequenceTrack theTargetBoneTrackData;
		const int32& theTargetBoneTrackIndex = UVAT_Library::GetBoneTrackIndex(InAnimSequence, theConstraintBoneData.TargetBone.BoneName);
		if (theTargetBoneTrackIndex == INDEX_NONE)
		{
			UVAT_Library::AddBoneTrack(InAnimSequence, theConstraintBoneData.TargetBone.BoneName);
		}
		UVAT_Library::GetBoneTrackData(InAnimSequence, theConstraintBoneData.TargetBone.BoneName, theTargetBoneTrackData);

		// Define the relative transform
		FTransform theRelativeTransform = FTransform::Identity;
		theRelativeTransform.SetLocation(theConstraintBoneData.RelativeLocation);
		theRelativeTransform.SetRotation(theConstraintBoneData.RelativeRotation.Quaternion());

		// Get the animation asset number of keys
		const int32 theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

		// Each every keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Check the frame range
			if (UKismetMathLibrary::InRange_IntInt(KeyIndex, theConstraintBoneData.FrameRange.X, theConstraintBoneData.FrameRange.Y)
				|| (KeyIndex > theConstraintBoneData.FrameRange.X && theConstraintBoneData.FrameRange.Y < 0))
			{
				// Calculate the target bone component space transform
				FTransform theTargetBoneTransformCS = GetBoneTransformCS(InAnimSequence, theConstraintBoneData.bExportRootMotion, KeyIndex, theConstraintBoneData.TargetBone.BoneName);
				theTargetBoneTransformCS = theRelativeTransform * theTargetBoneTransformCS;

				// Convert the target transform to local space transform
				FTransform theBoneTransformLS = GetBoneTransformLS(InAnimSequence, false, KeyIndex, theConstraintBoneData.SourceBone.BoneName, theTargetBoneTransformCS);
				theBoneTransformLS.NormalizeRotation();

				// Cache the bone data
				UVAT_Library::AddBoneTrackKey(theNewSourceBoneTrackData, theBoneTransformLS);
			}
			else
			{
				UVAT_Library::TransferBoneTrackKey(theSourceBoneTrackData, theNewSourceBoneTrackData, KeyIndex);
			}
		}

		// Apply the bone track data changed
		UVAT_Library::SetBoneTrackData(InAnimSequence, theConstraintBoneData.SourceBone.BoneName, &theNewSourceBoneTrackData);
	}

#if ENGINE_MAJOR_VERSION > 4
	theDataController.CloseBracket();
#endif

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
	return true;
}

#if 0 // DEPRECATED
void UVAT_Bone::OnConstraintVirtualBone(UAnimSequence* InAnimSequence)
{
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Save all virtual bone
	TArray<FVirtualBone> VirtualBonesData = theSkeleton->GetVirtualBones();

	// Remove all virtual bone
	UAnimationBlueprintLibrary::RemoveAllVirtualBones(InAnimSequence);

	for (FVirtualBone& VirtualBoneData : VirtualBonesData)
	{
		UAnimationBlueprintLibrary::AddVirtualBone(InAnimSequence, VirtualBoneData.SourceBoneName, VirtualBoneData.TargetBoneName, VirtualBoneData.VirtualBoneName);
	}
}

void UVAT_Bone::OnResetConstraintBone(UAnimSequence* InAnimSequence, const FVirtualBoneConstraintData& InConstraintBoneData)
{
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	FRawAnimSequenceTrack theBoneRawAnimSequenceTrack;
	UVAT_Library::GetBoneTrackData(InAnimSequence, InConstraintBoneData.Bone.BoneName, theBoneRawAnimSequenceTrack);
	FRawAnimSequenceTrack ConstraintRawAnimSequenceTrack;
	UVAT_Library::GetBoneTrackData(InAnimSequence, InConstraintBoneData.ConstraintBone.BoneName, ConstraintRawAnimSequenceTrack);

	FRawAnimSequenceTrack ToRawAnimSequenceTrack;
	const int32& ToAnimNumFrames = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	for (int i = 0; i < ToAnimNumFrames; i++)
	{
		if (UKismetMathLibrary::InRange_IntInt(i, InConstraintBoneData.ConstraintAnimFrameRange.X, InConstraintBoneData.ConstraintAnimFrameRange.Y) || InConstraintBoneData.ConstraintAnimFrameRange.Y < 0)
		{
			// Calculate the Bone Transform LS
			const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(InConstraintBoneData.Bone.BoneName);
			if (theBoneIndex == INDEX_NONE)
				return;

			FTransform theBoneTransformLS;
			if (theReferenceSkeleton.GetRawRefBonePose().IsValidIndex(theBoneIndex))
			{
				theBoneTransformLS = theReferenceSkeleton.GetRawRefBonePose()[theBoneIndex];
			}

			if (ConstraintRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.PosKeys.Add(theBoneTransformLS.GetLocation());

			if (ConstraintRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.RotKeys.Add(theBoneTransformLS.GetRotation());

			if (ConstraintRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.ScaleKeys.Add(theBoneTransformLS.GetScale3D());
		}
		else
		{
			if (theBoneRawAnimSequenceTrack.PosKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.PosKeys.Add(theBoneRawAnimSequenceTrack.PosKeys[i]);

			if (theBoneRawAnimSequenceTrack.RotKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.RotKeys.Add(theBoneRawAnimSequenceTrack.RotKeys[i]);

			if (theBoneRawAnimSequenceTrack.ScaleKeys.IsValidIndex(i))
				ToRawAnimSequenceTrack.ScaleKeys.Add(theBoneRawAnimSequenceTrack.ScaleKeys[i]);
		}
	}

	UVAT_Library::SetBoneTrackData(InAnimSequence, InConstraintBoneData.Bone.BoneName, &ToRawAnimSequenceTrack);
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}
#endif // DEPRECATED
#pragma endregion


#pragma region Bone Space
void UVAT_Bone::TransformToBoneSpace(UAnimSequence* InSequence, FName InBoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation)
{
	FTransform theBoneTransformCS = GetBoneTransformCS(InSequence, false, 0.f, InBoneName);
	FMatrix BoneToWorldTM = theBoneTransformCS.ToMatrixWithScale();

	FMatrix WorldTM = FRotationTranslationMatrix(InRotation, InPosition);
	FMatrix LocalTM = WorldTM * BoneToWorldTM.Inverse();

	OutPosition = LocalTM.GetOrigin();
	OutRotation = LocalTM.Rotator();
}
#pragma endregion

#undef LOCTEXT_NAMESPACE
