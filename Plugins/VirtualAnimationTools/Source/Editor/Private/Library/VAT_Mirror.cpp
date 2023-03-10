// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Library/VAT_Mirror.h"
#include "Library/VAT_Library.h"
#include "Library/VAT_Bone.h"
#include "Misc/ScopedSlowTask.h"
#include "Kismet/KismetStringLibrary.h"
#include "Library/VAT_Retarget.h"

#define LOCTEXT_NAMESPACE "VAT_Mirror"

static FVector MirrorVector(const FVector& V, EAxis::Type MirrorAxis)
{
	FVector MirrorV(V);

	switch (MirrorAxis)
	{
	case EAxis::X:
		MirrorV.X = -MirrorV.X;
		break;
	case EAxis::Y:
		MirrorV.Y = -MirrorV.Y;
		break;
	case EAxis::Z:
		MirrorV.Z = -MirrorV.Z;
		break;
	}

	return MirrorV;
}

static FQuat MirrorQuat(const FQuat& Q, EAxis::Type MirrorAxis)
{
	FQuat MirrorQ(Q);

	// Given an axis V and an angle A, the corresponding unmirrored quaternion Q = { Q.XYZ, Q.W } is:
	//
	//		Q = { V * sin(A/2), cos(A/2) }
	//
	//  mirror both the axis of rotation and the angle of rotation around that axis.
	// Therefore, the mirrored quaternion Q' for the axis V and angle A is:
	//
	//		Q' = { MirrorVector(V) * sin(-A/2), cos(-A/2) }
	//		Q' = { -MirrorVector(V) * sin(A/2), cos(A/2) }
	//		Q' = { -MirrorVector(V * sin(A/2)), cos(A/2) }
	//		Q' = { -MirrorVector(Q.XYZ), Q.W }
	//
	switch (MirrorAxis)
	{
	case EAxis::X:
		MirrorQ.Y = -MirrorQ.Y;
		MirrorQ.Z = -MirrorQ.Z;
		break;
	case EAxis::Y:
		MirrorQ.X = -MirrorQ.X;
		MirrorQ.Z = -MirrorQ.Z;
		break;
	case EAxis::Z:
		MirrorQ.X = -MirrorQ.X;
		MirrorQ.Y = -MirrorQ.Y;
		break;
	}

	return MirrorQ;
}

void UVAT_Mirror::SamplingMirrorBonesData(UAnimSequence* InAnimSequence, const TArray<FVirtualMirrorBoneData>& InMirrorBonesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the skeleton reference data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Each every mirror bone data
	for (const FVirtualMirrorBoneData& theMirrorBoneData : InMirrorBonesData)
	{
		// Check if the bone exists in the skeleton tree
		if (!theMirrorBoneData.bEnable || theReferenceSkeleton.FindBoneIndex(theMirrorBoneData.Bone.BoneName) == INDEX_NONE)
		{
			continue;
		}

		// Check the bone twin state
		if (theMirrorBoneData.bIsTwinBone)
		{
			// Check if the bone exists in the skeleton tree
			if (theReferenceSkeleton.FindBoneIndex(theMirrorBoneData.TwinBone.BoneName) == INDEX_NONE)
			{
				continue;
			}

			// Sample the source bone mirror data
			FRawAnimSequenceTrack theBoneRawTrack;
			FRawAnimSequenceTrack theMirrorBoneRawTrack;
			UVAT_Library::GetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, theMirrorBoneRawTrack);
			SamplingMirrorBoneTransform(InAnimSequence, theMirrorBoneData, theMirrorBoneRawTrack, theBoneRawTrack);

			// Sample the twin bone mirror data
			FRawAnimSequenceTrack theTwinBoneRawTrack;
			FRawAnimSequenceTrack theTwinMirrorBoneRawTrack;
			UVAT_Library::GetBoneTrackData(InAnimSequence, theMirrorBoneData.TwinBone.BoneName, theTwinMirrorBoneRawTrack);
			SamplingMirrorBoneTransform(InAnimSequence, theMirrorBoneData, theTwinMirrorBoneRawTrack, theTwinBoneRawTrack);

			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, &theTwinBoneRawTrack);
			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.TwinBone.BoneName, &theBoneRawTrack);
		}
		else
		{
			FRawAnimSequenceTrack theBoneRawTrack;
			FRawAnimSequenceTrack theMirrorBoneRawTrack;
			UVAT_Library::GetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, theMirrorBoneRawTrack);
			SamplingMirrorBoneTransform(InAnimSequence, theMirrorBoneData, theMirrorBoneRawTrack, theBoneRawTrack);
			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, &theBoneRawTrack);
		}
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Mirror::SamplingMirrorCSBonesData(UAnimSequence* InAnimSequence, const TArray<FVirtualMirrorBoneData>& InMirrorBonesData)
{
	// Check the animation asset is valid
	if (!InAnimSequence || !InAnimSequence->GetSkeleton())
	{
		return;
	}

	// Get the skeleton reference data
	USkeleton* theSkeleton = InAnimSequence->GetSkeleton();
	const FReferenceSkeleton& theReferenceSkeleton = theSkeleton->GetReferenceSkeleton();

	// Define the mirror transform function
	auto MirrorTransform = [&](EAxis::Type InMirrorAxis, const FTransform& SourceTransform, const FName& SourceParentName, const FName& SourceBoneName, const FName& TargetParentName, const FName& TargetBoneName)->FTransform
	{
		InMirrorAxis = EAxis::X;

		const FQuat TargetParentRefRotation = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, TargetParentName).GetRotation();
		const FQuat TargetBoneRefRotation = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, TargetBoneName).GetRotation();
		const FQuat SourceParentRefRotation = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, SourceParentName).GetRotation();
		const FQuat SourceBoneRefRotation = UVAT_Retarget::GetSkeletonBoneTransformCS(theSkeleton, SourceBoneName).GetRotation();

		// Mirror the translation component:  Rotate the translation into the space of the mirror plane,  mirror across the mirror plane, and rotate into the space of its new parent

		FVector T = SourceTransform.GetTranslation();
		T = SourceParentRefRotation.RotateVector(T);
		T = MirrorVector(T, InMirrorAxis);
		T = TargetParentRefRotation.UnrotateVector(T);

		// Mirror the rotation component:- Rotate into the space of the mirror plane, mirror across the plane, apply corrective rotation to align result with target space's rest orientation, 
		// then rotate into the space of its new parent

		FQuat Q = SourceTransform.GetRotation();
		Q = SourceParentRefRotation * Q;
		Q = MirrorQuat(Q, InMirrorAxis);
		Q *= MirrorQuat(SourceBoneRefRotation, InMirrorAxis).Inverse() * TargetBoneRefRotation;
		Q = TargetParentRefRotation.Inverse() * Q;

		FVector S = SourceTransform.GetScale3D();

		return FTransform(Q, T, S);
	};

	// Each every mirror bone data
	for (const FVirtualMirrorBoneData& theMirrorBoneData : InMirrorBonesData)
	{
		// Check if the bone exists in the skeleton tree
		if (!theMirrorBoneData.bEnable || theReferenceSkeleton.FindBoneIndex(theMirrorBoneData.Bone.BoneName) == INDEX_NONE)
		{
			continue;
		}

		// Get the source bone data
		const FName theSourceBoneName = theMirrorBoneData.Bone.BoneName;
		const int32 theSourceBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theSourceBoneName);
		const int32 theSourceParentBoneIndex = theSourceBoneIndex > 0 ? theReferenceSkeleton.GetRawParentIndex(theSourceBoneIndex) : INDEX_NONE;
		const FName theSourceParentBoneName = theSourceParentBoneIndex >= 0 ? theReferenceSkeleton.GetBoneName(theSourceParentBoneIndex) : NAME_None;

		// Get the twin bone data
		const FName theTwinBoneName = theMirrorBoneData.TwinBone.BoneName;
		const int32 theTwinBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theTwinBoneName);
		const int32 theTwinParentBoneIndex = theTwinBoneIndex > 0 ? theReferenceSkeleton.GetRawParentIndex(theTwinBoneIndex) : INDEX_NONE;
		const FName theTwinParentBoneName = theTwinParentBoneIndex >= 0 ? theReferenceSkeleton.GetBoneName(theTwinParentBoneIndex) : NAME_None;

		// Define the raw track data
		FRawAnimSequenceTrack theTwinRawTrack = FRawAnimSequenceTrack();
		FRawAnimSequenceTrack theSourceRawTrack = FRawAnimSequenceTrack();

		// Get the animation asset number of keys
		const int32 theNumberOfKeys = UVAT_Library::GetNumberOfKeys(InAnimSequence);

		// Each every animation poses
		for (int32 PoseIndex = 0; PoseIndex < theNumberOfKeys; PoseIndex++)
		{
			// Get the source bone local space transform
			FTransform theSourceBoneTransformLS
				= UVAT_Bone::GetBoneTransformLS(InAnimSequence, true, PoseIndex, theSourceBoneName);

			// Mirror root bone
			if (theSourceBoneIndex == 0)
			{
				const FQuat RootBoneRefRotation = UVAT_Retarget::GetSkeletonBoneTransformLS(theSkeleton, theSourceBoneName).GetRotation();

				FVector T = theSourceBoneTransformLS.GetTranslation();
				T = MirrorVector(T, theMirrorBoneData.MirrorAxis);

				FQuat Q = theSourceBoneTransformLS.GetRotation();
				Q = MirrorQuat(Q, theMirrorBoneData.MirrorAxis);
				Q *= MirrorQuat(RootBoneRefRotation, theMirrorBoneData.MirrorAxis).Inverse() * RootBoneRefRotation;

				FVector S = theSourceBoneTransformLS.GetScale3D();

				theSourceBoneTransformLS = FTransform(Q, T, S);

				// Add the local space bone transform to mirror raw track data
				UVAT_Library::AddBoneTrackKey(theSourceRawTrack, theSourceBoneTransformLS);
				continue;
			}

			// Response
			if (theMirrorBoneData.bIsTwinBone)
			{
				// Get the twin bone local space transform
				FTransform theTwinBoneTransformLS
					= UVAT_Bone::GetBoneTransformLS(InAnimSequence, true, PoseIndex, theTwinBoneName);

				const FTransform NewTwinBoneTransform = MirrorTransform(theMirrorBoneData.MirrorAxis, theSourceBoneTransformLS, theTwinParentBoneName, theTwinBoneName, theSourceParentBoneName, theSourceBoneName);
				const FTransform NewSourceBoneTransform = MirrorTransform(theMirrorBoneData.MirrorAxis, theTwinBoneTransformLS, theSourceParentBoneName, theSourceBoneName, theTwinParentBoneName, theTwinBoneName);

				// Add the local space bone transform to mirror raw track data
				UVAT_Library::AddBoneTrackKey(theSourceRawTrack, NewSourceBoneTransform);
				UVAT_Library::AddBoneTrackKey(theTwinRawTrack, NewTwinBoneTransform);
			}
			else
			{
				theSourceBoneTransformLS = MirrorTransform(theMirrorBoneData.MirrorAxis, theSourceBoneTransformLS, theSourceParentBoneName, theSourceBoneName, theSourceParentBoneName, theSourceBoneName);
				
				// Add the local space bone transform to mirror raw track data
				UVAT_Library::AddBoneTrackKey(theSourceRawTrack, theSourceBoneTransformLS);
			}
		}

		// Apply the bone track data changed
		if (theMirrorBoneData.bIsTwinBone)
		{
			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, &theSourceRawTrack);
			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.TwinBone.BoneName, &theTwinRawTrack);
		}
		else
		{
			UVAT_Library::SetBoneTrackData(InAnimSequence, theMirrorBoneData.Bone.BoneName, &theSourceRawTrack);
		}
	}

	// Apply the animation asset changed
	UVAT_Library::ModifyFromAnimSequence(InAnimSequence);
}

void UVAT_Mirror::SamplingMirrorBoneTransform(UAnimSequence* InAnimSequence, const FVirtualMirrorBoneData& InMirrorBoneData, const FRawAnimSequenceTrack& InSourceRawTrack, FRawAnimSequenceTrack& InMirrorRawTrack)
{
	// Check the animation asset is valid
	if (InAnimSequence == nullptr)
	{
		return;
	}

	// If the mirror option is not enabled, we will keep the original data
	if (!InMirrorBoneData.bEnable)
	{
		InMirrorRawTrack = InSourceRawTrack;
		return;
	}

	// Get the animation asset number of keys
	const int32 theNumberOfKeys =  UVAT_Library::GetNumberOfKeys(InAnimSequence);

	// Each every animation poses
	for (int32 PoseIndex = 0; PoseIndex < theNumberOfKeys; PoseIndex++)
	{
		// Get the source bone transform as the pose frame
		FTransform theSourceTransformLS;
#if ENGINE_MAJOR_VERSION > 4
		if (InSourceRawTrack.PosKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetTranslation(FVector(InSourceRawTrack.PosKeys[PoseIndex]));
		}
		if (InSourceRawTrack.RotKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetRotation(FQuat(InSourceRawTrack.RotKeys[PoseIndex]));
		}
		if (InSourceRawTrack.ScaleKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetScale3D(FVector(InSourceRawTrack.ScaleKeys[PoseIndex]));
		}
#else
		if (InSourceRawTrack.PosKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetTranslation(InSourceRawTrack.PosKeys[PoseIndex]);
		}
		if (InSourceRawTrack.RotKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetRotation(InSourceRawTrack.RotKeys[PoseIndex]);
		}
		if (InSourceRawTrack.ScaleKeys.IsValidIndex(PoseIndex))
		{
			theSourceTransformLS.SetScale3D(InSourceRawTrack.ScaleKeys[PoseIndex]);
		}
#endif

		// Sample the bone mirror local space transform
		FTransform theMirrorTransformLS = theSourceTransformLS;
		theMirrorTransformLS.Mirror(InMirrorBoneData.MirrorAxis, InMirrorBoneData.FlipAxis);

		// Additive mirror rotation offset
		if (InMirrorBoneData.RotationOffset != FRotator().ZeroRotator)
		{
			theMirrorTransformLS.SetRotation(InMirrorBoneData.RotationOffset.Quaternion() * theMirrorTransformLS.GetRotation());
			theMirrorTransformLS.NormalizeRotation();
		}

		// Transfer the bone scale
		theMirrorTransformLS.SetScale3D(theMirrorTransformLS.GetScale3D().GetAbs());

		// Add the local space bone transform to mirror raw track data
		UVAT_Library::AddBoneTrackKey(InMirrorRawTrack, theMirrorTransformLS);
	}
}

void UVAT_Mirror::SamplingMirrorBoneTreeAsName(USkeleton* InSkeleton, const EAxis::Type& InMirrorAxis
	, const TMap<FName, FName> InTwinNamesMap, TArray<FVirtualMirrorBoneData>& InMirrorBonesData)
{
	// Check the animation skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Clear cached mirror data
	InMirrorBonesData.Reset();

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Define the mirror bones name
	TArray<FName> theMirrorBonesName;

	// Create the dialog task
	FScopedSlowTask theMirrorToolsTask(theReferenceSkeleton.GetRawRefBoneInfo().Num(), LOCTEXT("MirrorToolsText", "Sampling mirror bone tree"));
	theMirrorToolsTask.MakeDialog(true);

	// Each every mesh bone info
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theMirrorToolsTask.ShouldCancel())
		{
			break;
		}

		// Flag enter progress frame
		theMirrorToolsTask.EnterProgressFrame();

		// Get the mesh bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);

		// Check if and exist, avoid saving the same bone data
		if (theMirrorBonesName.Contains(theBoneName))
		{
			continue;
		}

		// Define the mirror bone data
		FVirtualMirrorBoneData theMirrorBoneData;
		theMirrorBoneData.MirrorAxis = InMirrorAxis;
		theMirrorBoneData.Bone.BoneName = theBoneName;

		// Convert the name to string
		const FString theBoneString = theBoneName.ToString();

		// Define the mirror bone name
		FName theMirrorBoneName = NAME_None;

		// Each every twin params
		for (const TPair<FName, FName>& theTwinPair : InTwinNamesMap)
		{
			// Calculate the twin name model
			const int32& theTwinStringIndex = UKismetStringLibrary::FindSubstring(theBoneString, theTwinPair.Key.ToString(), false, true);
			const bool bIsSuffixName = theTwinStringIndex + theTwinPair.Key.ToString().Len() == theBoneString.Len();

			// Choose the prefix or suffix model
			if (bIsSuffixName && false)
			{
				const int32& theSuffixStringIndex = UKismetStringLibrary::FindSubstring(theBoneString, theTwinPair.Key.ToString(), false, true);
				const int32& theSuffixStringLength = theSuffixStringIndex + theTwinPair.Key.ToString().Len();

				// Check the suffix data is valid
				if (theSuffixStringIndex < 0 && theSuffixStringLength != theBoneString.Len())
				{
					continue;
				}

				// Calculate the mirror bone string
				const FString& thePrefixString = UKismetStringLibrary::GetSubstring(theBoneString, 0, theSuffixStringIndex);
				const FString& theSuffixString = UKismetStringLibrary::GetSubstring(theBoneString, theSuffixStringIndex, theBoneString.Len());
				const FString& theMirrorBoneString = UKismetStringLibrary::Concat_StrStr(thePrefixString, theTwinPair.Value.ToString());

				// Convert the string to name
				theMirrorBoneName = FName(*theMirrorBoneString);
				break;
			}
			else
			{
				const int32& thePrefixStringIndex = UKismetStringLibrary::FindSubstring(theBoneString, theTwinPair.Key.ToString(), false, true);
				const int32& thePrefixStringLength = thePrefixStringIndex + theTwinPair.Key.ToString().Len();

				// Check the suffix data is valid
				if (thePrefixStringIndex < 0 && thePrefixStringLength != theBoneString.Len())
				{
					continue;
				}

				// Calculate the mirror bone string
				const FString& thePrefixString = UKismetStringLibrary::GetSubstring(theBoneString, 0, thePrefixStringIndex);
				const FString& theSuffixString = UKismetStringLibrary::GetSubstring(theBoneString, thePrefixStringLength, theBoneString.Len());
				const FString& theMirrorBoneString = UKismetStringLibrary::Concat_StrStr(thePrefixString, theTwinPair.Value.ToString().Append(theSuffixString));

				// Convert the string to name
				theMirrorBoneName = FName(*theMirrorBoneString);
				break;
			}
		}

		// Check the mirror bone name is valid
		if (theMirrorBoneName == NAME_None)
		{
			// Cache the mirror bone data
			InMirrorBonesData.Add(theMirrorBoneData);
			continue;
		}

		// Each every mesh bone info
		for (const FMeshBoneInfo& theTwinMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
		{
			// Check the bone name is valid
			if (theTwinMeshBoneInfo.Name != theMirrorBoneName)
			{
				continue;
			}

			theMirrorBoneData.bIsTwinBone = true;
			theMirrorBoneData.TwinBone.BoneName = theTwinMeshBoneInfo.Name;
			InMirrorBonesData.Add(theMirrorBoneData);
			theMirrorBonesName.AddUnique(theMirrorBoneData.TwinBone.BoneName);
			break;
		}
	}
}

void UVAT_Mirror::SamplingMirrorBoneTreeAsLength(USkeleton* InSkeleton, const EAxis::Type& InMirrorAxis, const int32& InSampleLength, TArray<FVirtualMirrorBoneData>& InMirrorBonesData)
{
	// Check the animation skeleton is valid
	if (InSkeleton == nullptr)
	{
		return;
	}

	// Clear cached mirror data
	InMirrorBonesData.Reset();

	// Get the reference skeleton
	const FReferenceSkeleton& theReferenceSkeleton = InSkeleton->GetReferenceSkeleton();

	// Define the mirror bones name
	TArray<FName> theMirrorBonesName;

	// Create the dialog task
	FScopedSlowTask theMirrorToolsTask(theReferenceSkeleton.GetRawRefBoneInfo().Num(), LOCTEXT("MirrorToolsText", "Sampling mirror bone tree"));
	theMirrorToolsTask.MakeDialog(true);

	// Each every mesh bone info
	for (const FMeshBoneInfo& theMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theMirrorToolsTask.ShouldCancel())
		{
			break;
		}

		// Flag enter progress frame
		theMirrorToolsTask.EnterProgressFrame();

		// Get the mesh bone data
		const FName& theBoneName = theMeshBoneInfo.Name;
		const int32& theBoneIndex = theReferenceSkeleton.FindRawBoneIndex(theBoneName);

		// Check if and exist, avoid saving the same bone data
		if (theMirrorBonesName.Contains(theBoneName))
		{
			continue;
		}

		// Define the mirror bone data
		FVirtualMirrorBoneData theMirrorBoneData;
		theMirrorBoneData.MirrorAxis = InMirrorAxis;
		theMirrorBoneData.Bone.BoneName = theBoneName;

		// Convert the name to string
		const FString& theBoneString = theBoneName.ToString();

		// Each every mesh bone info, find twin bone name
		for (const FMeshBoneInfo& theTwinMeshBoneInfo : theReferenceSkeleton.GetRawRefBoneInfo())
		{
			// Convert the name to string
			const FString& theTwinBoneString = theTwinMeshBoneInfo.Name.ToString();

			// Check the bone name is valid
			if (theTwinMeshBoneInfo.Name == NAME_None || theBoneName == theTwinMeshBoneInfo.Name
				|| theTwinBoneString.Len() < InSampleLength || theTwinBoneString.Len() != theBoneString.Len())
			{
				continue;
			}

			// We must find the same and consecutive strings to determine the twin bone
			TArray<int32> theBoneStringFeatureIndices;

			// Each every bone string data
			for (int32 StringIndex = 0; StringIndex < theBoneString.Len(); StringIndex++)
			{
				// Get the bone one string data
				const FString theBoneData = theBoneString.Mid(StringIndex, 1);

				// Define the feature data
				bool bFindData = false;
				int32 theStartSearchIndex = theBoneStringFeatureIndices.Num() > 0 ? theBoneStringFeatureIndices.Last() : 0;

				// Each every twin bone string data
				for (int32 TwinStringIndex = theStartSearchIndex; TwinStringIndex < theTwinBoneString.Len(); TwinStringIndex++)
				{
					// Get the bone one string data
					const FString theTwinData = theTwinBoneString.Mid(TwinStringIndex, 1);

					// Check the data is equal
					if (theBoneData == theTwinData)
					{
						bFindData = true;
						theBoneStringFeatureIndices.AddUnique(TwinStringIndex);
						break;
					}
				}

				// Check the find state
				if (bFindData)
				{
					if (theBoneStringFeatureIndices.Num() >= InSampleLength)
					{
						theMirrorBoneData.bIsTwinBone = true;
						theMirrorBoneData.TwinBone.BoneName = theTwinMeshBoneInfo.Name;
						theMirrorBonesName.AddUnique(theMirrorBoneData.TwinBone.BoneName);
						break;
					}
				}
				else
				{
					theBoneStringFeatureIndices.Reset();
				}
			}

			// Always check the twin data is valid
			if (theMirrorBoneData.bIsTwinBone == true)
			{
				break;
			}
		}

		// Cache the mirror bone data
		InMirrorBonesData.Add(theMirrorBoneData);
	}
}
#undef LOCTEXT_NAMESPACE