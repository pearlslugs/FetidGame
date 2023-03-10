// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Retarget.h"
#include "Library/VAT_Library.h"
#include "EditorAnimUtils.h"
#include "Animation/AnimSequence.h"

#if ENGINE_MAJOR_VERSION > 4
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimationDataController/Public/AnimDataController.h"
#include "AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#else
#include "AnimationModifiers/Public/AnimationBlueprintLibrary.h"
#endif

#pragma region Get Skeleton Bone Transform
FTransform UVAT_Retarget::GetSkeletonBoneTransformLS(USkeleton* InSkeleton, const FName& InBoneName)
{
	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return FTransform();
	}

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Get the bone index
	const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(InBoneName);
	if (theBoneIndex == INDEX_NONE)
	{
		return FTransform();
	}

	// Return the bone pose
	return theReferenceSkeleton.GetRawRefBonePose()[theBoneIndex];
}

FTransform UVAT_Retarget::GetSkeletonBoneTransformLS(USkeleton* InSkeleton, const FName& InBoneName, const FTransform& InBoneTransformCS)
{
	//Define the new bone transform
	FTransform theBoneTransformLS = InBoneTransformCS;

	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return theBoneTransformLS;
	}

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	FTransform theParentBoneTransformCS;
	FName theParentBoneName = InBoneName;
	while (theParentBoneName != theRootBoneName)
	{
		// Get the parent bone name
		theParentBoneName = theReferenceSkeleton.GetBoneName(theReferenceSkeleton.GetParentIndex(theReferenceSkeleton.FindBoneIndex(theParentBoneName)));

		// Get the parent bone transform
		FTransform theParentBoneTransformLS = GetSkeletonBoneTransformLS(InSkeleton, theParentBoneName);

		// Convert to component space
		theParentBoneTransformCS *= theParentBoneTransformLS;
	}

	// Convert to parent bone space
	theBoneTransformLS.SetToRelativeTransform(theParentBoneTransformCS);

	// Return the result
	return theBoneTransformLS;
}

FTransform UVAT_Retarget::GetSkeletonBoneTransformCS(USkeleton* InSkeleton, const FName& InBoneName)
{
	//Define the new bone transform
	FTransform theBoneTransformCS = FTransform();

	// Check the skeleton is valid
	if (InSkeleton == nullptr)
	{
		return theBoneTransformCS;
	}

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Get the root bone name
	const FName& theRootBoneName = theReferenceSkeleton.GetBoneName(0);

	// Calculate the component space
	FName theParentBoneName = InBoneName;
	while (theParentBoneName != theRootBoneName && theParentBoneName != NAME_None)
	{
		// Get the parent bone transform
		FTransform ParentBoneTransformLS = GetSkeletonBoneTransformLS(InSkeleton, theParentBoneName);

		// Convert to parent space
		theBoneTransformCS *= ParentBoneTransformLS;

		// Get the parent bone name
		theParentBoneName = theReferenceSkeleton.GetBoneName(theReferenceSkeleton.GetParentIndex(theReferenceSkeleton.FindBoneIndex(theParentBoneName)));
	}

	// Convert to component space
	FTransform theRootBoneTransformCS = GetSkeletonBoneTransformLS(InSkeleton, theRootBoneName);
	theBoneTransformCS *= theRootBoneTransformCS;

	// Return the result
	return theBoneTransformCS;
}
#pragma endregion

void UVAT_Retarget::OnRetargetPoseFromSkeleton(USkeleton* InSkeleton, USkeleton* InReferenceSkeleton, UAnimSequence* InAnimSequence, const FVirtualBonesData& InBonesData)
{
	if (!InSkeleton || !InReferenceSkeleton || !InAnimSequence)
	{
		return;
	}

	const FReferenceSkeleton& RetargetSkeleton = InSkeleton->GetReferenceSkeleton();
	const FReferenceSkeleton& ReferenceSkeleton = InReferenceSkeleton->GetReferenceSkeleton();

	// Each every mesh bone info, find all children bones name
// 	for (const FMeshBoneInfo& theMeshBoneInfo : InSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo())
// 	{
// 		// Get the bone data
// 		const FName& theBoneName = theMeshBoneInfo.Name;
// 		const int32& theBoneIndex = InSkeleton->GetReferenceSkeleton().FindRawBoneIndex(theBoneName);
// 
// 		// Reference bone data
// 		const int32& ReferenceBoneIndex = ReferenceSkeleton.FindRawBoneIndex(theBoneName);
// 		if (ReferenceBoneIndex < 0)
// 		{
// 			continue;
// 		}
// 		const FTransform& ReferenceBoneTransformLS = ReferenceSkeleton.GetRawRefBonePose()[ReferenceBoneIndex];
// 
// 		// Retarget bone data
// 		const int32& RetargetBoneIndex = RetargetSkeleton.FindRawBoneIndex(theBoneName);
// 		if (RetargetBoneIndex < 0)
// 		{
// 			continue;
// 		}
// 		const FTransform& RetargetSkeletonBoneTransform = GetSkeletonBoneTransformLS(InSkeleton, theBoneName);
// 
// 		// Calculate relative reference bone transform
// 		FTransform RetargetBoneTransform = GetSkeletonBoneTransformCS(InReferenceSkeleton, theBoneName);
// 		//FTransform RetargetBoneTransform = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, 0, theBoneName);
// 
// 		RetargetBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, 0, theBoneName, RetargetBoneTransform);
// 		RetargetBoneTransform.NormalizeRotation();
// 
// 		// Set new bone track
// 		FRawAnimSequenceTrack ToRawAnimSequenceTrack;
// 		ToRawAnimSequenceTrack.PosKeys.Add(BoneData.bIncludeTranslation ? RetargetBoneTransform.GetTranslation() : RetargetSkeletonBoneTransform.GetTranslation());
// 		ToRawAnimSequenceTrack.RotKeys.Add(BoneData.bIncludeRotation ? RetargetBoneTransform.GetRotation() : FQuat::Identity);
// 		ToRawAnimSequenceTrack.ScaleKeys.Add(BoneData.bIncludeScale ? RetargetBoneTransform.GetScale3D() : RetargetSkeletonBoneTransform.GetScale3D());
// 
// 		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &ToRawAnimSequenceTrack);
// 	}

	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Retarget::OnRetargetPoseFromAnimation(USkeleton* InSkeleton, UAnimSequence* InAnimSequence, UAnimSequence* InReferenceAnimSequence, const FVirtualBonesData& InBonesData)
{
	if (!InSkeleton || !InAnimSequence || !InReferenceAnimSequence || !InReferenceAnimSequence->GetSkeleton())
	{
		return;
	}

	const FReferenceSkeleton& RetargetSkeleton = InSkeleton->GetReferenceSkeleton();
	const FReferenceSkeleton& ReferenceSkeleton = InReferenceAnimSequence->GetSkeleton()->GetReferenceSkeleton();

	for (const FVirtualBoneData& BoneData : InBonesData.BonesData)
	{
		if (BoneData.Bone.BoneName == "None")
			continue;

		// Reference bone data
		const int32& ReferenceBoneIndex = ReferenceSkeleton.FindRawBoneIndex(BoneData.Bone.BoneName);
		if (ReferenceBoneIndex < 0)
		{
			continue;
		}

		// Retarget bone data
		const int32& RetargetBoneIndex = RetargetSkeleton.FindRawBoneIndex(BoneData.Bone.BoneName);
		if (RetargetBoneIndex < 0)
		{
			continue;
		}
		const FTransform& RetargetSkeletonBoneTransform = GetSkeletonBoneTransformLS(InSkeleton, BoneData.Bone.BoneName);

		// Calculate relative reference bone transform
		FTransform RetargetBoneTransform = UVAT_Bone::GetBoneTransformCS(InReferenceAnimSequence, false, 0, BoneData.Bone.BoneName);
		RetargetBoneTransform = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, 0, BoneData.Bone.BoneName, RetargetBoneTransform);
		RetargetBoneTransform.NormalizeRotation();

		// Set new bone track
		FRawAnimSequenceTrack ToRawAnimSequenceTrack;
#if ENGINE_MAJOR_VERSION > 4
		ToRawAnimSequenceTrack.PosKeys.Add(FVector3f(BoneData.bIncludeTranslation ? RetargetBoneTransform.GetTranslation() : RetargetSkeletonBoneTransform.GetTranslation()));
		ToRawAnimSequenceTrack.RotKeys.Add(BoneData.bIncludeRotation ? FQuat4f(RetargetBoneTransform.GetRotation()) : FQuat4f::Identity);
		ToRawAnimSequenceTrack.ScaleKeys.Add(FVector3f(BoneData.bIncludeScale ? RetargetBoneTransform.GetScale3D() : RetargetSkeletonBoneTransform.GetScale3D()));
#else
		ToRawAnimSequenceTrack.PosKeys.Add(BoneData.bIncludeTranslation ? RetargetBoneTransform.GetTranslation() : RetargetSkeletonBoneTransform.GetTranslation());
		ToRawAnimSequenceTrack.RotKeys.Add(BoneData.bIncludeRotation ? RetargetBoneTransform.GetRotation() : FQuat::Identity);
		ToRawAnimSequenceTrack.ScaleKeys.Add(BoneData.bIncludeScale ? RetargetBoneTransform.GetScale3D() : RetargetSkeletonBoneTransform.GetScale3D());
#endif
		UVAT_Library::SetBoneTrackData(InAnimSequence, BoneData.Bone.BoneName, &ToRawAnimSequenceTrack);
	}

	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Retarget::OnRetargetAnimations(UAnimSequence* InAnimSequence, FVirtualRetargetAnimationsData& InRetargetData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData)
{
	// Check the target skeleton is valid
	if (InRetargetData.TargetSkeleton == nullptr)
	{
		return;
	}

	// Check the Retarget assets is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Check the Retarget skeleton is valid
	USkeleton* theRetargetSkeleton = InAnimSequence->GetSkeleton();
	if (!theRetargetSkeleton || !theRetargetSkeleton->GetPreviewMesh(false))
	{
		return;
	}

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const FReferenceSkeleton& theTargetReferenceSkeleton = InRetargetData.TargetSkeleton->GetReferenceSkeleton();
	const FReferenceSkeleton& theRetargetReferenceSkeleton = InRetargetData.TargetSkeleton->GetReferenceSkeleton();

	// Convert to weak object reference
	TArray<TWeakObjectPtr<UObject>> theWeakObjects{ InAnimSequence };

	// Cache every bone transform data
	TMap<FName, FVirtualBoneCurvesData> theRetargetBoneLSDataMap;
	TMap<FName, FVirtualBoneCurvesData> theRetargetBoneCSDataMap;
	for (const FMeshBoneInfo& theMeshBoneInfo : theRetargetReferenceSkeleton.GetRawRefBoneInfo())
	{
		// Get the bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theRetargetReferenceSkeleton.FindRawBoneIndex(theBoneName);

		// Check the bone names is valid
		if (theBoneName == NAME_None)
		{
			continue;
		}

		// Find the rig bone name
		const FName theRigBoneName = theRetargetSkeleton->GetRigBoneMapping(theBoneName);

		// Check the rig bone names is valid
		if (theRigBoneName == NAME_None)
		{
			continue;
		}

		// Cache the target bone transform data
		FVirtualBoneCurvesData& theBoneLSData = theRetargetBoneLSDataMap.FindOrAdd(theRigBoneName);
		FVirtualBoneCurvesData& theBoneCSData = theRetargetBoneCSDataMap.FindOrAdd(theRigBoneName);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the Retarget bone component space transform
			FTransform theRetargetTransformLS = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, theKeyTime, theBoneName);

			// Cache to bone curves data
			theBoneLSData.AddTransformKey(theKeyTime, theRetargetTransformLS);

			// Get the Retarget bone component space transform
			FTransform theRetargetTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theBoneName);

			// Cache to bone curves data
			theBoneCSData.AddTransformKey(theKeyTime, theRetargetTransformCS);
		}
	}

	// Cache the constraint bone transform data
	TMap<FName, FVirtualBoneCurvesData> theConstraintBoneDataMap;
	for (TPair<FName, FBoneReference>& theConstraintPair : InRetargetData.ContraintBonesMap)
	{
		const FName& theTargetBoneName = theConstraintPair.Key;
		const FName& theRetargetBoneName = theConstraintPair.Value.BoneName;

		// Check the bone names is valid
		if (theTargetBoneName == NAME_None || theRetargetBoneName == NAME_None)
		{
			continue;
		}

		// Cache the target bone transform data
		FVirtualBoneCurvesData& theBoneData = theConstraintBoneDataMap.FindOrAdd(theTargetBoneName);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the Retarget bone component space transform
			FTransform theRetargetTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theRetargetBoneName);

			// Cache to bone curves data
			theBoneData.AddTransformKey(theKeyTime, theRetargetTransformCS);
		}
	}

	// Call engine source Retarget
	EditorAnimUtils::RetargetAnimations(theRetargetSkeleton, InRetargetData.TargetSkeleton, theWeakObjects, InRetargetData.bRemapReferencedAssets, nullptr, InRetargetData.bConvertSpaces);

	// Apply default constraint
	UVAT_Bone::SampleConstraintBones(InAnimSequence, InConstraintBonesData);

	// Apply cached constraint bones data
	for (TPair<FName, FVirtualBoneCurvesData>& theConstraintPair : theConstraintBoneDataMap)
	{
		const FName& theTargetBoneName = theConstraintPair.Key;
		const FVirtualBoneCurvesData& theBoneCurvesData = theConstraintPair.Value;

		// Get the parent bone transform
		const int32& theTargetBoneIndex = theTargetReferenceSkeleton.FindBoneIndex(theTargetBoneName);
		const int32 theParentBoneIndex = theTargetReferenceSkeleton.GetParentIndex(theTargetBoneIndex);
		const FName theParentBoneName = theParentBoneIndex >= 0 ? theTargetReferenceSkeleton.GetBoneName(theParentBoneIndex) : NAME_None;

		// Each every pose keys
		FRawAnimSequenceTrack theTargetBoneTrackData;
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the cached target bone component space transform
			FTransform theTargetTransformCS = theBoneCurvesData.Evaluate(theKeyTime);

			// Convert to parent bone space
			if (theParentBoneName != NAME_None)
			{
				const FTransform theParentBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theParentBoneName);
				theTargetTransformCS.SetToRelativeTransform(theParentBoneTransformCS);
			}

			// Cache to bone track data
			UVAT_Library::AddBoneTrackKey(theTargetBoneTrackData, theTargetTransformCS);
		}

		// Apply the new bone track
		UVAT_Library::SetBoneTrackData(InAnimSequence, theTargetBoneName, &theTargetBoneTrackData);
	}

#if 0 // Component space convert
	for (TPair<FName, FVirtualBoneCurvesData>& theConstraintPair : theRetargetBoneCSDataMap)
	{
		// Get the right bone name
		const FName& theRigBoneName = theConstraintPair.Key;
		const FVirtualBoneCurvesData& theBoneCurvesData = theConstraintPair.Value;

		// Find the rig bone name
		const FName theTargetBoneName = theRetargetSkeleton->GetRigBoneMapping(theRigBoneName);

		// Check the target bone names is valid
		if (theTargetBoneName == NAME_None)
		{
			continue;
		}

		// Get the parent bone transform
		const int32& theTargetBoneIndex = theTargetReferenceSkeleton.FindBoneIndex(theTargetBoneName);
		const int32 theParentBoneIndex = theTargetReferenceSkeleton.GetParentIndex(theTargetBoneIndex);
		const FName theParentBoneName = theParentBoneIndex >= 0 ? theTargetReferenceSkeleton.GetBoneName(theParentBoneIndex) : NAME_None;

		// Each every pose keys
		FRawAnimSequenceTrack theTargetBoneTrackData;
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the cached target bone component space transform
			FTransform theTargetTransformCS = theBoneCurvesData.Evaluate(theKeyTime);

			// Convert to parent bone space
			if (theParentBoneName != NAME_None)
			{
				const FTransform theParentBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theParentBoneName);
				theTargetTransformCS.SetToRelativeTransform(theParentBoneTransformCS);
			}

			// Cache to bone track data
			theTargetBoneTrackData.PosKeys.Add(theTargetTransformCS.GetLocation());
			theTargetBoneTrackData.RotKeys.Add(theTargetTransformCS.GetRotation());
			theTargetBoneTrackData.ScaleKeys.Add(theTargetTransformCS.GetScale3D());
		}

		// Apply the new bone track
		UVAT_Library::SetBoneTrackData(InAnimSequence, theTargetBoneName, &theTargetBoneTrackData);
	}
#endif

#if 0 // Local space convert
	for (TPair<FName, FVirtualBoneCurvesData>& theConstraintPair : theRetargetBoneLSDataMap)
	{
		// Get the right bone name
		const FName& theRigBoneName = theConstraintPair.Key;
		const FVirtualBoneCurvesData& theBoneCurvesData = theConstraintPair.Value;

		// Find the rig bone name
		const FName theTargetBoneName = theRetargetSkeleton->GetRigNodeNameFromBoneName(theRigBoneName);

		// Check the target bone names is valid
		if (theTargetBoneName == NAME_None)
		{
			continue;
		}

		// Each every pose keys
		FRawAnimSequenceTrack theTargetBoneTrackData;
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the cached target bone component space transform
			FTransform theTargetTransformCS = theBoneCurvesData.Evaluate(theKeyTime);

			// Cache to bone track data
			theTargetBoneTrackData.PosKeys.Add(theTargetTransformCS.GetLocation());
			theTargetBoneTrackData.RotKeys.Add(theTargetTransformCS.GetRotation());
			theTargetBoneTrackData.ScaleKeys.Add(theTargetTransformCS.GetScale3D());
		}

		// Apply the new bone track
		UVAT_Library::SetBoneTrackData(InAnimSequence, theTargetBoneName, &theTargetBoneTrackData);
	}
#endif

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Retarget::OnRebuildAnimations(UAnimSequence* InAnimSequence)
{
	// Check the assets is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// Check the skeleton is valid
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	if (!theSkeleton || !theSkeleton->GetPreviewMesh(false))
	{
		return;
	}

	// Get the animation asset data
	const int32& theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Cache every bone transform data
	TMap<FName, FVirtualBoneCurvesData> theBoneLSDataMap;
	TMap<FName, FVirtualBoneCurvesData> theBoneCSDataMap;
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// Get the bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);

		// Check the bone names is valid
		if (theBoneName == NAME_None)
		{
			continue;
		}

		// Cache the target bone transform data
		FVirtualBoneCurvesData& theBoneLSData = theBoneLSDataMap.FindOrAdd(theBoneName);
		FVirtualBoneCurvesData& theBoneCSData = theBoneCSDataMap.FindOrAdd(theBoneName);

		// Each every pose keys
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the  bone component space transform
			FTransform theTransformLS = UVAT_Bone::GetBoneTransformLS(InAnimSequence, false, theKeyTime, theBoneName);

			// Cache to bone curves data
			theBoneLSData.AddTransformKey(theKeyTime, theTransformLS);

			// Get the  bone component space transform
			FTransform theTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theBoneName);

			// Cache to bone curves data
			theBoneCSData.AddTransformKey(theKeyTime, theTransformCS);
		}
	}

	// Copy source data
	const float theLength = UVAT_Library::GetSequenceLength(InAnimSequence);
	const int32 theRawNumberOfFrame = UVAT_Library::GetNumberOfFrames(InAnimSequence);
	const FRawCurveTracks theCurveTracks = InAnimSequence->RawCurveData;
	TArray<FAnimNotifyTrack> theNotifyTrack = InAnimSequence->AnimNotifyTracks;

	// Set the motion animation length
	InAnimSequence->CreateAnimation(theSkeleton->GetPreviewMesh(false));
#if ENGINE_MAJOR_VERSION > 4
	TheAnimController.SetPlayLength(FrameTime);
	TheAnimController.Resize(theLength, 0, theLength);

	const int32 FrameRate = 1.f / FrameTime;
	FFrameRate TargetFrameRate(FrameRate, 1);
	TheAnimController.SetFrameRate(TargetFrameRate);
	TheAnimController.NotifyPopulated();

#else
	InAnimSequence->SequenceLength = theLength;
	InAnimSequence->SetRawNumberOfFrame(theRawNumberOfFrame);
	InAnimSequence->ResizeSequence(theLength, theRawNumberOfFrame, false, 0, theRawNumberOfFrame);
#endif
	InAnimSequence->RawCurveData = theCurveTracks;
	InAnimSequence->AnimNotifyTracks = theNotifyTrack;

#if 0 // Component space convert
	for (TPair<FName, FVirtualBoneCurvesData>& theTrackPair : theBoneCSDataMap)
	{
		// Get the bone name
		const FName& theBoneName = theTrackPair.Key;
		const FVirtualBoneCurvesData& theBoneCurvesData = theTrackPair.Value;

		// Get the parent bone transform
		const int32& theBoneIndex = theReferenceSkeleton.FindBoneIndex(theBoneName);
		const int32 theParentBoneIndex = theReferenceSkeleton.GetParentIndex(theBoneIndex);
		const FName theParentBoneName = theParentBoneIndex >= 0 ? theReferenceSkeleton.GetBoneName(theParentBoneIndex) : NAME_None;

		// Each every pose keys
		FRawAnimSequenceTrack theBoneTrackData;
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the cached target bone component space transform
			FTransform theTransformCS = theBoneCurvesData.Evaluate(theKeyTime);

			// Convert to parent bone space
			if (theParentBoneName != NAME_None)
			{
				const FTransform theParentBoneTransformCS = UVAT_Bone::GetBoneTransformCS(InAnimSequence, false, theKeyTime, theParentBoneName);
				theTransformCS.SetToRelativeTransform(theParentBoneTransformCS);
			}

			// Cache to bone track data
			theBoneTrackData.PosKeys.Add(theTransformCS.GetLocation());
			theBoneTrackData.RotKeys.Add(theTransformCS.GetRotation());
			theBoneTrackData.ScaleKeys.Add(theTransformCS.GetScale3D());
		}

		// Apply the new bone track
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theBoneTrackData);
	}
#else
	for (TPair<FName, FVirtualBoneCurvesData>& theTrackPair : theBoneLSDataMap)
	{
		// Get the bone name
		const FName& theBoneName = theTrackPair.Key;
		const FVirtualBoneCurvesData& theBoneCurvesData = theTrackPair.Value;

		// Each every pose keys
		FRawAnimSequenceTrack theBoneTrackData;
		for (int32 KeyIndex = 0; KeyIndex < theNumberOfKeys; KeyIndex++)
		{
			// Evaluate the key time
			const float theKeyTime = InAnimSequence->GetTimeAtFrame(KeyIndex);

			// Get the cached target bone component space transform
			FTransform theTargetTransformCS = theBoneCurvesData.Evaluate(theKeyTime);

			// Cache to bone track data
			theBoneTrackData.PosKeys.Add(theTargetTransformCS.GetLocation());
			theBoneTrackData.RotKeys.Add(theTargetTransformCS.GetRotation());
			theBoneTrackData.ScaleKeys.Add(theTargetTransformCS.GetScale3D());
		}

		// Apply the new bone track
		UVAT_Library::SetBoneTrackData(InAnimSequence, theBoneName, &theBoneTrackData);
	}
#endif

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}
