// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "VAT_DetailsViewData.h"
#include "Library/VAT_Library.h"
#include "Animation/AnimTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorUtilityLibrary.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

#define LOCTEXT_NAMESPACE "VAT_DetailsViewData"

UVAT_DetailsViewData::UVAT_DetailsViewData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DataClass = GetClass();

	// **************************************** Bone **********************************************//
	SampleBonsData.AddDefaulted();

	// **************************************** Curve **********************************************/
	CurvePresetName = "All";
	FVirtualCurvePresetData& theAllCurvePresetData = CurvePresetsMap.FindOrAdd("All");
	// Each every curve types
	for (int32 i = 0; i < int32(EVirtualCurveType::MAX); i++)
	{
		theAllCurvePresetData.CurvesType.Add(EVirtualCurveType(i));
		//TMap_MaxCurveValue.FindOrAdd(EVirtualCurveType(i), 0.f);
	}

	// **************************************** Notify **********************************************//
	NotifiesData.AddDefaulted();
	NotifiesTrackData.AddDefaulted();

	// **************************************** Montage **********************************************//
	bSortMontageSlots = true;
	bClearInvalidSlots = true;

	// **************************************** Asset **********************************************//
	AssetCompositesData.AddDefaulted();

	// **************************************** Mirorr **********************************************//
	MirrorAssetType = EVirtualToolsRebuildAssetType::VTAT_DuplicateAsset;
	bSaveMirrorBoneTree = false;
	MirrorAxis = EAxis::X;
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("L_");
		theValue = "R_";
	}
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("L-");
		theValue = "R-";
	}
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("_L");
		theValue = "_R";
	}
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("-L");
		theValue = "-R";
	}
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("LF");
		theValue = "RF";
	}
	{
		FName& theValue = MirrorTwinNamesMap.FindOrAdd("LB");
		theValue = "RB";
	}
	MirrorSampleStringLength = 0;
	MirrorAssetSuffixName = "_Mirror";
}

void UVAT_DetailsViewData::OnSelectionChanged()
{
	// Check basic animations number is valid
	if (Animations.Num() > 0)
	{
		return;
	}

	// Show in selected animations
	TArray<UAnimSequenceBase*> theSelectedAnimations;
	if (!IsSelectedChanged(theSelectedAnimations))
	{
		return;
	}

	// Copy the selected animations 
	SelectedAnimations = theSelectedAnimations;

	// Response animations property changed
	OnAnimationsEditChangeProperty(nullptr);
}

void UVAT_DetailsViewData::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);

	// Check the property is valid
	if (!InPropertyChangedEvent.Property)
	{
		return;
	}

	// Get the property name
	const FName thePropertyName = InPropertyChangedEvent.Property->GetFName();

	// Handle event
	if (thePropertyName == GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, DataClass))
	{
		OnDataClassEditChangeProperty(InPropertyChangedEvent);
	}
	else if (thePropertyName == GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, Skeleton))
	{
		OnAnimationsEditChangeProperty(&InPropertyChangedEvent);
	}
	else if (thePropertyName == GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, Animations))
	{
		OnAnimationsEditChangeProperty(&InPropertyChangedEvent);
	}
}

#pragma region Animation
bool UVAT_DetailsViewData::IsSelectedChanged(TArray<UAnimSequenceBase*>& OutAsset)
{
	// Fast path check selected number change state
	TArray<UObject*> theSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
#if 0
	if (theSelectedAssets.Num() == SelectedAnimations.Num())
	{
		return false;
	}
#endif

	// Check first selected asset is skeletal mesh
	USkeletalMesh* theSkeletalMesh = nullptr;

	// Check the asset is changed
	for (auto theSelectedAsset : theSelectedAssets)
	{
		// Check the selected asset is valid
		if (theSelectedAsset == nullptr)
		{
			continue;
		}

		// Check the class is animation sequence base
		if (!theSelectedAsset->IsA(UAnimSequenceBase::StaticClass()))
		{
			if (theSelectedAsset->IsA(USkeletalMesh::StaticClass()))
			{
				theSkeletalMesh = Cast<USkeletalMesh>(theSelectedAsset);
			}
			continue;
		}

		// Cache the animation asset
		OutAsset.Add(Cast<UAnimSequenceBase>(theSelectedAsset));
	}

	// Fast path check selected number change state
	if (OutAsset.Num() != SelectedAnimations.Num())
	{
		return true;
	}

	// Each every asset
	for (int32 AssetIndex = 0; AssetIndex < SelectedAnimations.Num(); AssetIndex++)
	{
		if (OutAsset[AssetIndex] != SelectedAnimations[AssetIndex])
		{
			return true;
		}
	}

	// Check skeleton is valid
	if (Skeleton == nullptr)
	{
		return theSkeletalMesh != nullptr;
	}

	// Failed
	return false;
}

USkeleton* UVAT_DetailsViewData::GetSelectedSkeleton()
{
	// Check the default skeleton is valid
	if (Skeleton != nullptr)
	{
		Skeleton;
	}

	// If we do not have an animation asset that needs to be modified, we will query the currently selected animation asset
	if (Animations.Num() == 0)
	{
		TArray<UObject*> theSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
		for (auto theSelectedAsset : theSelectedAssets)
		{
			// Check the selected asset is valid
			if (theSelectedAsset == nullptr)
			{
				continue;
			}

			// Check the class is animation sequence base
			if (!theSelectedAsset->IsA(UAnimSequenceBase::StaticClass()))
			{
				if (theSelectedAsset->IsA(USkeletalMesh::StaticClass()))
				{
					// Return the animation asset skeleton
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 27
					return Cast<USkeletalMesh>(theSelectedAsset)->Skeleton;
#else
					return Cast<USkeletalMesh>(theSelectedAsset)->GetSkeleton();
#endif
				}
				continue;
			}

			// Return the animation asset skeleton
			return Cast<UAnimSequenceBase>(theSelectedAsset)->GetSkeleton();
		}

		// Return the result
		return Skeleton;
	}

	// Return default animation skeleton
	return Skeleton;
}

TArray<UAnimSequenceBase*> UVAT_DetailsViewData::GetSelectedAnimAssets()
{
	// If we do not have an animation asset that needs to be modified, we will query the currently selected animation asset
	if (Animations.Num() == 0)
	{
		TArray<UAnimSequenceBase*> theAnimAssets;
		TArray<UObject*> theSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
		for (auto theSelectedAsset : theSelectedAssets)
		{
			// Check the selected asset is valid
			if (theSelectedAsset == nullptr)
			{
				continue;
			}

			// Check the class is animation sequence base
			if (!theSelectedAsset->IsA(UAnimSequenceBase::StaticClass()))
			{
				continue;
			}

			// Cache the animation asset
			theAnimAssets.Add(Cast<UAnimSequenceBase>(theSelectedAsset));
		}

		// Return the result
		return theAnimAssets;
	}

	// Return default animation assets
	return Animations;
}

TArray<UAnimSequence*> UVAT_DetailsViewData::GetSelectedAnimSequences()
{
	// Define the variables
	TArray<UAnimSequence*> theAnimSequences;

	// Each every selected asset
	for (auto theSelectedAsset : GetSelectedAnimAssets())
	{
		// Check the selected asset is valid
		if (theSelectedAsset == nullptr)
		{
			continue;
		}

		// Check the class is animation sequence
		if (!theSelectedAsset->IsA(UAnimSequence::StaticClass()))
		{
			continue;
		}

		// Cache the animation asset
		theAnimSequences.Add(Cast<UAnimSequence>(theSelectedAsset));
	}

	// Return the result
	return theAnimSequences;
}

TArray<UAnimMontage*> UVAT_DetailsViewData::GetSelectedAnimMontages()
{
	// Define the variables
	TArray<UAnimMontage*> theAnimMontages;

	// Each every selected asset
	for (auto theSelectedAsset : GetSelectedAnimAssets())
	{
		// Check the selected asset is valid
		if (theSelectedAsset == nullptr)
		{
			continue;
		}

		// Check the class is animation montage
		if (!theSelectedAsset->IsA(UAnimMontage::StaticClass()))
		{
			continue;
		}

		// Cache the animation asset
		theAnimMontages.Add(Cast<UAnimMontage>(theSelectedAsset));
	}

	// Return the result
	return theAnimMontages;
}

void UVAT_DetailsViewData::SetDataClass(const TSubclassOf<UVAT_DetailsViewData>& InDataClass)
{
	DataClass = InDataClass;
}

void UVAT_DetailsViewData::SetVirtualToolsType(const EVirtualToolsType& InToolsType)
{
	VirtualToolsType = InToolsType;
}

void UVAT_DetailsViewData::OnDataClassEditChangeProperty(struct FPropertyChangedEvent& InPropertyChangedEvent)
{
	if (OnDataClassChangingPropertiesDelegate.IsBound())
	{
		OnDataClassChangingPropertiesDelegate.Broadcast(DataClass);
	}
}

void UVAT_DetailsViewData::OnAnimationsEditChangeProperty(struct FPropertyChangedEvent* InPropertyChangedEvent)
{
	// Cache the skeleton reference
	Skeleton = GetSelectedSkeleton();

	// Initialize montages property
	switch (VirtualToolsType)
	{
	case EVirtualToolsType::Montage:
		InitializeMontageSlotsData();
		break;
	}

	// Response the tools changed
	if (OnVirtualToolsChangingPropertiesDelegate.IsBound())
	{
		OnVirtualToolsChangingPropertiesDelegate.Broadcast(InPropertyChangedEvent);
	}
}
#pragma endregion


#pragma region Bone
void UVAT_DetailsViewData::OnClearBoneTools(const EVirtualBoneToolsType& InToolsType)
{
}

void UVAT_DetailsViewData::OnSampleBoneTracks(const EVirtualBoneToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Always sample first animation asset
	if (theAnimSequences.Num() > 0)
	{
#if 0
		// Create the dialog task
		FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Sampling animation assets bones track data"));
		theBoneToolsTask.MakeDialog(true);

		// Each every animation asset
		for (UAnimSequence* theAnimSequence : theAnimSequences)
		{
			// We check if the cancel button has been pressed, if so we break the execution of the loop
			if (theBoneToolsTask.ShouldCancel())
			{
				break;
			}

			// Flag enter progress frame
			theBoneToolsTask.EnterProgressFrame();

			// Handle tools type
			switch (InToolsType)
			{
			case EVirtualBoneToolsType::ModifyBoneTrack:
				UVAT_Bone::SampleBonesTrack(ModifyBonesData.BoneSpace, theAnimSequence, ModifyBonesData.BonesData);
				break;
			}
			break;
		}
#else
		// Handle tools type
		switch (InToolsType)
		{
		case EVirtualBoneToolsType::ModifyBoneTrack:
			UVAT_Bone::SampleBonesTrack(ModifyBonesData.BoneSpace, theAnimSequences[0], ModifyBonesData.BonesData);
			break;
		}
#endif
	}
}

void UVAT_DetailsViewData::OnResponseBoneTools(const EVirtualBoneToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Handle tools type
	switch (InToolsType)
	{
	case EVirtualBoneToolsType::AddBone:
		if (AddBonesData.SkeletalMeshs.Num() > 0 && AddBonesData.BonesData.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(AddBonesData.SkeletalMeshs.Num(), LOCTEXT("BoneToolsText", "Add skeletal mesh bones data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every skeletal mesh assets
			for (USkeletalMesh* theSkeletalMesh : AddBonesData.SkeletalMeshs)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::AddBonesData(theSkeletalMesh, AddBonesData.BonesData);
			}
		}
		break;

	case EVirtualBoneToolsType::RemoveBone:
		if (RemoveBonesData.Bones.Num() > 0)
		{
			if (RemoveBonesData.Skeletons.Num() >0 && RemoveBonesData.Skeletons[0] != nullptr)
			{
				// Create the dialog task
				FScopedSlowTask theBoneToolsTask(RemoveBonesData.Skeletons.Num(), LOCTEXT("BoneToolsText", "Remove skeletal mesh bones data"));
				theBoneToolsTask.MakeDialog(true);

				// Each every skeleton assets
				for (USkeleton* theSkeleton : RemoveBonesData.Skeletons)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theBoneToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theBoneToolsTask.EnterProgressFrame();
					UVAT_Bone::RemoveBonesData(theSkeleton, RemoveBonesData.Bones, RemoveBonesData.bRemoveChildBones);
				}
			}
			else if (RemoveBonesData.SkeletalMeshs.Num() > 0 && RemoveBonesData.SkeletalMeshs[0] != nullptr)
			{
				// Create the dialog task
				FScopedSlowTask theBoneToolsTask(RemoveBonesData.SkeletalMeshs.Num(), LOCTEXT("BoneToolsText", "Remove skeletal mesh bones data"));
				theBoneToolsTask.MakeDialog(true);

				// Each every skeletal mesh assets
				for (USkeletalMesh* theSkeletalMesh : RemoveBonesData.SkeletalMeshs)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theBoneToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theBoneToolsTask.EnterProgressFrame();
					UVAT_Bone::RemoveBonesData(theSkeletalMesh, RemoveBonesData.Bones, RemoveBonesData.bRemoveChildBones);
				}
			}
		}
		break;

	case EVirtualBoneToolsType::AddBoneTrack:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Add animation assets bones track data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::AddBonesTrack(theAnimSequence, SampleBonsData);
			}
		}
		break;

	case EVirtualBoneToolsType::ModifyBoneTrack:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Modify animation assets bones track data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::ModifyBonesTrack(theAnimSequence, ModifyBonesData);
			}
		}
		break;

	case EVirtualBoneToolsType::RemoveBoneTrack:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Remove animation assets bones track data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::RemoveBonesTrack(theAnimSequence, SampleBonsData);
			}
		}
		break;

	case EVirtualBoneToolsType::ReplaceAnimPose:
		//OnBakeReplaceAnimPoseBone();
		break;

	case EVirtualBoneToolsType::LayeredBoneBlend:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Sampling animation assets blended data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::LayeredBoneBlend(theAnimSequence, BoneLayerData, BonesConstraintData);
			}
		}
		break;

	case EVirtualBoneToolsType::FilterBone:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Filter animation assets bones data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::FilterBoneTracks(theAnimSequence, BoneFilterData);
			}
		}
		break;

	case EVirtualBoneToolsType::ConstraintBone:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theBoneToolsTask(theAnimSequences.Num(), LOCTEXT("BoneToolsText", "Sampling animation assets constraint data"));
			theBoneToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theBoneToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theBoneToolsTask.EnterProgressFrame();
				UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
			}
		}
		break;
	}
}

#if 0
void UVAT_DetailsViewData::OnBakeReplaceAnimPoseBone()
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	FScopedSlowTask ReplaceAnimPoseTask(theAnimSequences.Num(), LOCTEXT("ReplaceAnimPoseText", "Replace Animation Pose"));
	ReplaceAnimPoseTask.MakeDialog(true);

	for (int i = 0; i < theAnimSequences.Num(); i++)
	{		
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (ReplaceAnimPoseTask.ShouldCancel())
			break;

		ReplaceAnimPoseTask.EnterProgressFrame();

		if (UAnimSequence* AnimSequence = theAnimSequences[i])
		{
			if (ReferenceAnimSequences_Bone.IsValidIndex(0))
			{
				if (UAnimSequence* ReferenceAnimSequence = ReferenceAnimSequences_Bone[0])
				{
					FVirtualBonesData BonesData;
					UVAT_Bone::GetBonesNameAsBoneTree(AnimSequence->GetSkeleton(), BakeBoneTree, &BonesData);

					UVAT_Bone::OnSetBoneTransform(AnimSequence, ReferenceAnimSequence, ReferenceAnimFrameRange_Bone, ChangeAnimFrameRange_Bone, BonesData, BonesConstraintData);
				}
			}
		}
	}
}

void UVAT_DetailsViewData::OnBakeReplaceAnimTransformBone()
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	FScopedSlowTask ReplaceAnimTransformTask(theAnimSequences.Num(), LOCTEXT("ReplaceAnimTransformText", "Replace Animation Transform"));
	ReplaceAnimTransformTask.MakeDialog(true);

	for (int i = 0; i < theAnimSequences.Num(); i++)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (ReplaceAnimTransformTask.ShouldCancel())
			break;

		ReplaceAnimTransformTask.EnterProgressFrame();

		if (UAnimSequence* AnimSequence = theAnimSequences[i])
		{
			FVirtualBonesData BonesData;
			UVAT_Bone::GetBonesNameAsBoneTree(AnimSequence->GetSkeleton(), BakeBoneTree, &BonesData);

			UVAT_Bone::OnSetBoneTransform(AnimSequence, ReferenceAnimFrameRange_Bone, ChangeAnimFrameRange_Bone, BonesData, BonesConstraintData);
		}
	}
}

void UVAT_DetailsViewData::OnBakeReplaceAnimLayerPoseBone()
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	FScopedSlowTask ReplaceAnimLayerPoseTask(theAnimSequences.Num(), LOCTEXT("ReplaceAnimLayerPoseText", "Replace Animation Layer Pose"));
	ReplaceAnimLayerPoseTask.MakeDialog(true);

	for (int i = 0; i < theAnimSequences.Num(); i++)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (ReplaceAnimLayerPoseTask.ShouldCancel())
			break;

		ReplaceAnimLayerPoseTask.EnterProgressFrame();

		if (UAnimSequence* AnimSequence = theAnimSequences[i])
		{
			if (ReferenceAnimSequences_Bone.IsValidIndex(0))
			{
				if (UAnimSequence* ReferenceAnimSequence = ReferenceAnimSequences_Bone[0])
				{
					FVirtualBonesData BonesData;
					UVAT_Bone::GetBonesNameAsBoneTree(AnimSequence->GetSkeleton(), BakeBoneTree, &BonesData);
					UVAT_Bone::LayeredBoneBlend(AnimSequence, ReferenceAnimSequence, BakeBoneTree, BonesData, BoneLayerData, BonesConstraintData);
				}
			}
		}
	}
}

void UVAT_DetailsViewData::OnBakeBoneTransform()
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	FScopedSlowTask BakeAnimBoneTransformTask(theAnimSequences.Num(), LOCTEXT("BakeAnimBoneTransformText", "Baking Animation Transform"));
	BakeAnimBoneTransformTask.MakeDialog(true);

	for (int i = 0; i < theAnimSequences.Num(); i++)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (BakeAnimBoneTransformTask.ShouldCancel())
			break;

		BakeAnimBoneTransformTask.EnterProgressFrame();

		if (UAnimSequence* AnimSequence = theAnimSequences[i])
		{
			for (FVirtualBakeBoneData& BakeBoneData : BakeBonesData)
			{
				TArray<FAnimNotifyEvent> AnimNotifiesEvent = UVAT_Notify::GetNotifiesEventAsTrackName(AnimSequence, BakeBoneData.BakeNotifyTrackName);
				for (FAnimNotifyEvent& AnimNotifyEvent : AnimNotifiesEvent)
				{
					// Notify
					if (AnimNotifyEvent.Notify && AnimNotifyEvent.Notify->GetClass() == BakeBoneData.BakeNotifyClass)
					{
						if (UVAT_Bone::OnBakeBoneTransformsToNotifyEvent(AnimSequence, AnimNotifyEvent, BakeBoneData))
						{
							//K2_BakeBoneTransformsToNotify(AnimNotifyEvent.Notify, BakeBoneData);
						}
					}

					// Notify State
					else if (AnimNotifyEvent.NotifyStateClass && AnimNotifyEvent.NotifyStateClass->GetClass() == BakeBoneData.BakeNotifyStateClass)
					{
						if (UVAT_Bone::OnBakeBoneTransformsToNotifyEvent(AnimSequence, AnimNotifyEvent, BakeBoneData))
						{
							//K2_BakeBoneTransformsToNotifyState(AnimNotifyEvent.NotifyStateClass, BakeBoneData);
						}
					}
				}

				UVAT_Library::ModifyFromAnimSequence(AnimSequence);
			}
		}
	}
}
#endif
#pragma endregion


#pragma region Curve
void UVAT_DetailsViewData::SetAccumulateCurve(const bool InState)
{
	SampleCurveAttributes.bSampleAccumulateCurve = InState;
}

void UVAT_DetailsViewData::OnResponseCurveTools(const EVirtualCurveToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Handle tools type
	switch (InToolsType)
	{
	case EVirtualCurveToolsType::Motion:
		if (theAnimSequences.Num() > 0)
		{
			// Sampling data
			FVirtualCurveAttributes theSampleCurveAttributes = SampleCurveAttributes;
			if (FVirtualCurvePresetData* theCurvePresetData = CurvePresetsMap.Find(CurvePresetName))
			{
				// Rebuild curve filter types
				theSampleCurveAttributes.RebuildCurveFilterMap(*theCurvePresetData);

				// Create the dialog task
				FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Sampling animation assets motion curves data"));
				theCurveToolsTask.MakeDialog(true);

				// Each every animation asset
				for (UAnimSequence* theAnimSequence : theAnimSequences)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theCurveToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theCurveToolsTask.EnterProgressFrame();
					UVAT_Curve::SampleAnimationRootMotionCurves(theAnimSequence, theSampleCurveAttributes, theCurvePresetData->CurvesType);
				}
			}
		}
		break;

	case EVirtualCurveToolsType::Bone:
		if (theAnimSequences.Num() > 0)
		{
			if (FVirtualCurvePresetData* theCurvePresetData = CurvePresetsMap.Find(CurvePresetName))
			{
				// Create the dialog task
				FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Sampling animation assets bones curve data"));
				theCurveToolsTask.MakeDialog(true);

				// Each every animation asset
				for (UAnimSequence* theAnimSequence : theAnimSequences)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theCurveToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theCurveToolsTask.EnterProgressFrame();
					for (FVirtualBoneCurveData& theBoneCurveData : SampleBonesCurve)
					{
						UVAT_Curve::SampleBoneCurve(theAnimSequence, theBoneCurveData, SampleCurveAttributes);
					}
				}
			}
		}
		break;

	case EVirtualCurveToolsType::Copy:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Copy animation assets curves data"));
			theCurveToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theCurveToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theCurveToolsTask.EnterProgressFrame();
				UVAT_Curve::CopyAnimationCurvesData(theAnimSequence, CopyCurvesData);
			}
		}
		break;

	case EVirtualCurveToolsType::Transfer:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Transfer animation assets curves data"));
			theCurveToolsTask.MakeDialog(true);

			// Each every animation asset
			for (int32 Index = 0; Index < theAnimSequences.Num(); Index++)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theCurveToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theCurveToolsTask.EnterProgressFrame();

				if (TransferCurvesData.IsValidIndex(Index))
				{
					UVAT_Curve::TransferAnimationCurvesData(theAnimSequences[Index], TransferCurvesData[Index]);
				}
				else if(TransferCurvesData.Num() > 0)
				{
					UVAT_Curve::TransferAnimationCurvesData(theAnimSequences[Index], TransferCurvesData.Last());
				}
			}
		}
		break;

	case EVirtualCurveToolsType::Sort:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Sorting animation assets curves data"));
			theCurveToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theCurveToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theCurveToolsTask.EnterProgressFrame();
				UVAT_Curve::SortAnimationAssetCurves(theAnimSequence, SampleCurveAttributes);
			}
		}
		break;

	case EVirtualCurveToolsType::Scale:
		break;

	case EVirtualCurveToolsType::Remove:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Remove animation assets curves data"));
			theCurveToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theCurveToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theCurveToolsTask.EnterProgressFrame();

				// Choose remove mode
				if (bClearAllCurves)
				{
					UVAT_Curve::ClearAnimationAssetCurve(theAnimSequence);
				}
				else
				{
					// Append all remove curve names
					TArray<FName> theCurvesName;
					if (FVirtualCurvePresetData* theCurvePresetData = CurvePresetsMap.Find(CurvePresetName))
					{
						theCurvesName.AddUnique(CurvePresetName);
						theCurvesName.Append(theCurvePresetData->CurvesName);
						for (EVirtualCurveType& theCurveType : theCurvePresetData->CurvesType)
						{
							if (FName* theCurveName = SampleCurveAttributes.AnimCurvesNameMap.Find(theCurveType))
							{
								theCurvesName.AddUnique(*theCurveName);
							}
						}
					}
					else
					{
						theCurvesName.Add(CurvePresetName);
					}
					UVAT_Curve::RemoveAnimationAssetCurve(theAnimSequence, theCurvesName);
				}
			}
		}
		break;

	case EVirtualCurveToolsType::Output:
		if (theAnimSequences.Num() > 0 && OutputCurveAssets.Num() > 0)
		{
			// Sort curves
			TArray<UCurveFloat*> theCurvesFloatAsset;
			TArray<UCurveVector*> theCurvesVectorAsset;
			for (UCurveBase* theCurveBase : OutputCurveAssets)
			{
				// Check the curve asset is valid
				if (theCurveBase == nullptr)
				{
					continue;
				}

				// Sort
				if (theCurveBase->IsA(UCurveFloat::StaticClass()))
				{
					theCurvesFloatAsset.Add(Cast<UCurveFloat>(theCurveBase));
				}
				else if (theCurveBase->IsA(UCurveVector::StaticClass()))
				{
					theCurvesVectorAsset.Add(Cast<UCurveVector>(theCurveBase));
				}
			}

			// Sampling data
			FVirtualCurveAttributes theSampleCurveAttributes = SampleCurveAttributes;
			if (FVirtualCurvePresetData* theCurvePresetData = CurvePresetsMap.Find(CurvePresetName))
			{
				// Rebuild curve filter types
				theSampleCurveAttributes.RebuildCurveFilterMap(*theCurvePresetData);

				// Create the dialog task
				FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Sampling animation assets motion curves data"));
				theCurveToolsTask.MakeDialog(true);

				// Each every animation asset
				for (UAnimSequence* theAnimSequence : theAnimSequences)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theCurveToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theCurveToolsTask.EnterProgressFrame();

					// Generate the curves data map
					TMap<EVirtualCurveType, FRuntimeFloatCurve> theCurvesDataMap;
					for (const EVirtualCurveType& theCurveType : theCurvePresetData->CurvesType)
					{
						theCurvesDataMap.FindOrAdd(theCurveType);
					}

					// Sample the motion curves
					UVAT_Curve::SampleRootMotionCurves(theAnimSequence, theSampleCurveAttributes, theCurvesDataMap);
					TArray<FRuntimeFloatCurve> theFloatCurves;
					theCurvesDataMap.GenerateValueArray(theFloatCurves);
					UVAT_Curve::SampleRootMotionCurvesToCurvesFloat(theCurvesFloatAsset, theSampleCurveAttributes, theFloatCurves);
					UVAT_Curve::SampleRootMotionCurvesToCurvesVector(theCurvesVectorAsset, theSampleCurveAttributes, theFloatCurves);
				}
			}
			else if (CurvePresetName != NAME_None)
			{
				// Create the dialog task
				FScopedSlowTask theCurveToolsTask(theAnimSequences.Num(), LOCTEXT("CurveToolsText", "Sampling animation assets curves data"));
				theCurveToolsTask.MakeDialog(true);

				// Each every animation asset
				for (UAnimSequence* theAnimSequence : theAnimSequences)
				{
					// We check if the cancel button has been pressed, if so we break the execution of the loop
					if (theCurveToolsTask.ShouldCancel())
					{
						break;
					}

					// Flag enter progress frame
					theCurveToolsTask.EnterProgressFrame();

					// Sample the desired curves
					TArray<FRuntimeFloatCurve> theFloatCurves;
					UVAT_Curve::SampleAnimationAssetCurves(theAnimSequence, CurvePresetName, theFloatCurves);
					UVAT_Curve::SampleRootMotionCurvesToCurvesFloat(theCurvesFloatAsset, SampleCurveAttributes, theFloatCurves);
					UVAT_Curve::SampleRootMotionCurvesToCurvesVector(theCurvesVectorAsset, SampleCurveAttributes, theFloatCurves);
				}
			}
		}
		break;
	}
}
#pragma endregion


#pragma region Notify
void UVAT_DetailsViewData::OnSampleNotifiesData(const EVirtualNotifyToolsType& InToolsType)
{
	// Get current selected animation assets
	TArray<UAnimSequenceBase*> theAnimAssets = GetSelectedAnimAssets();
	if (theAnimAssets.Num() == 0)
	{
		return;
	}

	// Create the dialog task
	FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Sample animation assets notifies data"));
	theNotifyToolsTask.MakeDialog(true);

	// Clear cached data
	ModifyNotifiesData.Reset();
	ModifyNotifiesTrackMap.Reset();

	// Each every animation asset
	for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theNotifyToolsTask.ShouldCancel())
		{
			break;
		}

		// Flag enter progress frame
		theNotifyToolsTask.EnterProgressFrame();

		// Check the animation asset is valid
		if (theAnimAsset == nullptr)
		{
			continue;
		}

		// Handle tools type
		switch (InToolsType)
		{
		case EVirtualNotifyToolsType::AddNotifies:
			break;

		case EVirtualNotifyToolsType::ModifyNotifies:
			if (true)
			{
				FVirtualMultiNotifiesData& theMultiNotifiesData = ModifyNotifiesData.FindOrAdd(theAnimAsset);
				UVAT_Notify::GetNotifiesData(theAnimAsset, theMultiNotifiesData.NotifiesData);
			}
			break;

		case EVirtualNotifyToolsType::RemoveNotifies:
			UVAT_Notify::GetNotifiesData(theAnimAsset, NotifiesData);
			break;

		case EVirtualNotifyToolsType::AddNotifiesTrack:
			break;

		case EVirtualNotifyToolsType::ModifyNotifiesTrack:
			if (true)
			{
				FVirtualNotifyModifyTrackData& theModifyTrackData = ModifyNotifiesTrackMap.FindOrAdd(theAnimAsset);
				UVAT_Notify::GetNotifiesTrackName(theAnimAsset, theModifyTrackData.TrackNameMap);
				for (TPair<UAnimSequenceBase*, FVirtualNotifyModifyTrackData>& thePair : ModifyNotifiesTrackMap)
				{
					for (TPair<FName, FName>& theChildPair : thePair.Value.TrackNameMap)
					{
						theChildPair.Value = theChildPair.Key;
					}
				}
			}
			break;

		case EVirtualNotifyToolsType::RemoveNotifiesTrack:
			UVAT_Notify::GetNotifiesTrackName(theAnimAsset, NotifiesTrackData);
			break;
		}
	}
}

void UVAT_DetailsViewData::OnResponseNotifyTools(const EVirtualNotifyToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequenceBase*> theAnimAssets = GetSelectedAnimAssets();

	// Handle tools type
	switch (InToolsType)
	{
	case EVirtualNotifyToolsType::AddNotifies:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Add animation assets notifies"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();
				UVAT_Notify::AddNotifiesEvent(theAnimAsset, NotifiesData);
			}
		}
		break;

	case EVirtualNotifyToolsType::ModifyNotifies:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Modify animation assets notifies"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();
				if (FVirtualMultiNotifiesData* theMultiNotifiesData = ModifyNotifiesData.Find(theAnimAsset))
				{
					UVAT_Notify::ModifyNotifies(theAnimAsset, theMultiNotifiesData->NotifiesData);
				}
			}
		}
		break;

	case EVirtualNotifyToolsType::RemoveNotifies:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Remove animation assets notifies"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();
				UVAT_Notify::RemoveNotifies(theAnimAsset, NotifiesData);
			}
		}
		break;

	case EVirtualNotifyToolsType::AddNotifiesTrack:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Add animation assets notifies track"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();
				
				// Each every notify track data
				for (const FVirtualNotifyTrackData& theNotifyTrackData : NotifiesTrackData)
				{
					UVAT_Notify::AddNotifyTrack(theAnimAsset, theNotifyTrackData.TrackName, theNotifyTrackData.TrackColor);
				}

				// Apply the animation asset changed
				UVAT_Library::ModifyFromAnimSequence(theAnimAsset);
			}
		}
		break;

	case EVirtualNotifyToolsType::ModifyNotifiesTrack:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Modify animation assets notifies track"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();

				// Each every track data
				if (FVirtualNotifyModifyTrackData* theModifyTrackData = ModifyNotifiesTrackMap.Find(theAnimAsset))
				{
					for (const TPair<FName, FName>& thePair : theModifyTrackData->TrackNameMap)
					{
						UVAT_Notify::ModifyNotifyTrack(theAnimAsset, thePair.Value, thePair.Key);
					}
				}

				// Apply the animation asset changed
				UVAT_Library::ModifyFromAnimSequence(theAnimAsset);
			}
		}
		break;

	case EVirtualNotifyToolsType::RemoveNotifiesTrack:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Remove animation assets notifies track"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();

				// Each every notify track data
				for (const FVirtualNotifyTrackData& theNotifyTrackData : NotifiesTrackData)
				{
					UVAT_Notify::RemoveNotifyTrack(theAnimAsset, theNotifyTrackData.TrackName);
				}

				// Apply the animation asset changed
				UVAT_Library::ModifyFromAnimSequence(theAnimAsset);
			}
		}
		break;

		case EVirtualNotifyToolsType::FootStep:
		if (theAnimAssets.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theNotifyToolsTask(theAnimAssets.Num(), LOCTEXT("NotifyToolsText", "Sampling foot step notifies"));
			theNotifyToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequenceBase* theAnimAsset : theAnimAssets)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theNotifyToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theNotifyToolsTask.EnterProgressFrame();

				// Only support animation sequence now
				if (UAnimSequence* theAnimSequence = Cast<UAnimSequence>(theAnimAsset))
				{
					// Sample foot step notifies
					UVAT_Notify::SampleFootStepNotifies(theAnimSequence, FootStepSampleData, LegsBaseData, NotifiesData);

					// Apply the animation asset changed
					UVAT_Library::ModifyFromAnimSequence(theAnimAsset);
				}
			}
		}
		break;
	}
}
#pragma endregion


#pragma region Montage
void UVAT_DetailsViewData::InitializeMontageSlotsData()
{
	// Reset the montage reference data
	MontageGroupsMap.Reset();

	// Check the selected skeleton is valid
	USkeleton* theSkeleton = GetSelectedSkeleton();
	if (theSkeleton == nullptr)
	{
		return;
	}

	// Get current selected animation montages
	TArray<UAnimMontage*> theAnimMontages = GetSelectedAnimMontages();

	// Initialize current montage slots data
	if (theAnimMontages.Num() == 0)
	{
		return;
	}

	// Reset the montage slots data
	MontageSlotsData.Reset();

	// Create the dialog task
	FScopedSlowTask theMontageToolsTask(theAnimMontages.Num(), LOCTEXT("MontageToolsText", "Sampling montages slots data"));
	theMontageToolsTask.MakeDialog(true);

	// Each every animation asset
	for (UAnimMontage* theAnimMontage : theAnimMontages)
	{
		// We check if the cancel button has been pressed, if so we break the execution of the loop
		if (theMontageToolsTask.ShouldCancel())
		{
			break;
		}

		// Flag enter progress frame
		theMontageToolsTask.EnterProgressFrame();

		// Check the montage asset is valid
		if (theAnimMontage == nullptr)
		{
			continue;
		}

		// Get the animation montage slots name
		TArray<FName> theMontageSlotsName;
		UVAT_Montage::GetMontageSlotNames(theAnimMontage, theMontageSlotsName);

		// Define the add montage slot data event
		auto OnAddMontageSlotData = [&](UAnimMontage* InMontage, const TArray<FName>& InSlotsName)
		{
			FVirtualMontageSlotData& theMontageSlotData = MontageSlotsData.AddDefaulted_GetRef();
			theMontageSlotData.Montages.Add(InMontage);
			theMontageSlotData.SlotsName = InSlotsName;
		};

		// Check the montage slots data is valid
		if (MontageSlotsData.Num() == 0)
		{
			OnAddMontageSlotData(theAnimMontage, theMontageSlotsName);
			continue;
		}

		// Check if the same data exists
		for (FVirtualMontageSlotData& theMontageSlotData : MontageSlotsData)
		{
			// Quickly check if the number of montage slots is the same
			if (theMontageSlotData.SlotsName.Num() != theMontageSlotsName.Num())
			{
				OnAddMontageSlotData(theAnimMontage, theMontageSlotsName);
				break;
			}

			// Define the data is valid
			bool bCreateData = false;

			// Each every montage slots name
			for (int32 SlotIndex = 0; SlotIndex < theMontageSlotData.SlotsName.Num(); SlotIndex++)
			{
				if (theMontageSlotsName[SlotIndex] != theMontageSlotData.SlotsName[SlotIndex])
				{
					bCreateData = true;
					break;
				}
			}

			// Create montage slot data
			if (bCreateData)
			{
				OnAddMontageSlotData(theAnimMontage, theMontageSlotsName);
				break;
			}
			else
			{
				theMontageSlotData.Montages.Add(theAnimMontage);
			}
		}
	}
}

void UVAT_DetailsViewData::OnResponseMontageTools(const EVirtualMontageToolsType& InToolsType)
{
#if 0
	// Get current selected animation montages
	TArray<UAnimMontage*> theAnimMontages = GetSelectedAnimMontages();
#endif

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualMontageToolsType::Modifier:
		if (MontageSlotsData.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theMontageToolsTask(MontageSlotsData.Num(), LOCTEXT("MontageToolsText", "Modifier montage slots name"));
			theMontageToolsTask.MakeDialog(true);

			// Each every montage slot data
			for (FVirtualMontageSlotData& theMontageSlotData : MontageSlotsData)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theMontageToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theMontageToolsTask.EnterProgressFrame();

				// Each every animation montages
				for (UAnimMontage* theAnimMontage : theMontageSlotData.Montages)
				{
					// Check the animation montage is valid
					if (theAnimMontage == nullptr)
					{
						continue;
					}

					// Sort the montage slots index
					if (bSortMontageSlots)
					{
						UVAT_Montage::SortMontageSlots(theAnimMontage->GetSkeleton(), theMontageSlotData.SlotsName);
					}

					// Replace current montage slots name
					UVAT_Montage::OnReplaceMontageSlots(theAnimMontage, theMontageSlotData.SlotsName);
				}
			}
		}
		break;

	case EVirtualMontageToolsType::Loop:
		if (MontageSlotsData.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theMontageToolsTask(MontageSlotsData.Num(), LOCTEXT("MontageToolsText", "Set montage slots track is looping"));
			theMontageToolsTask.MakeDialog(true);

			// Each every montage slot data
			for (FVirtualMontageSlotData& theMontageSlotData : MontageSlotsData)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theMontageToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theMontageToolsTask.EnterProgressFrame();

				// Each every animation montages
				for (UAnimMontage* theAnimMontage : theMontageSlotData.Montages)
				{
					// Check the animation montage is valid
					if (theAnimMontage == nullptr)
					{
						continue;
					}

					// Set the montage is loop section
					UVAT_Montage::SetMontageLoopSection(theAnimMontage);
				}
			}
		}
		break;

	case EVirtualMontageToolsType::Transfer:
		if (TargetSkeletons_Montage.Num() > 0)
		{
			// Check the selected skeleton is valid
			USkeleton* theSkeleton = GetSelectedSkeleton();

			// Create the dialog task
			FScopedSlowTask theMontageToolsTask(TargetSkeletons_Montage.Num(), LOCTEXT("MontageToolsText", "Transfer skeleton groups and slots"));
			theMontageToolsTask.MakeDialog(true);

			// Each every transfer skeleton
			for (USkeleton* theTargetSkeleton : TargetSkeletons_Montage)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theMontageToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theMontageToolsTask.EnterProgressFrame();

				// Transfer data
				UVAT_Montage::OnTransferMontageSlots(theSkeleton, theTargetSkeleton, bClearInvalidSlots);
			}
		}
		break;
	}
}
#pragma endregion



#pragma region Asset
void UVAT_DetailsViewData::SetRootMotionToolsType(const EVirtualRootMotionToolsType& InTypes)
{
	MotionSampleData.MotionToolsType = InTypes;
}

void UVAT_DetailsViewData::OnResponseAssetTools(const EVirtualAssetToolsType& InToolsType, const EVirtualRootMotionProcessType& InMotionProcessType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualAssetToolsType::Crop:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Cropping animation assets"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::CropAnimationAsset(theAnimSequence, AssetCropData);
			}
		}
		break;

	case EVirtualAssetToolsType::Insert:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Inserting animation assets"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::InsertAnimationAsset(theAnimSequence, AssetInsertData);
			}
		}
		break;

	case EVirtualAssetToolsType::Resize:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Resize animation assets"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::ResizeAnimationAsset(theAnimSequence, AssetResizeData);
			}
		}
		break;

	case EVirtualAssetToolsType::Replace:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Replace animation assets"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::ReplaceAnimationAsset(theAnimSequence, AssetReplaceData);
			}
		}
		break;

	case EVirtualAssetToolsType::Composite:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Compositing animation assets"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::OnCompositesAnimationAsset(theAnimSequence, AssetCompositesData);
			}
		}
		break;


	case EVirtualAssetToolsType::SampleRootMotion:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Sampling animation assets root motion data"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
#if 1
				UVAT_Asset::SampleMotionCurvesData(theAnimSequence, MotionSampleData, MotionCurvesData);
#else
				switch (MotionSampleData.MotionToolsType)
				{
				case EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture:
					UVAT_Asset::SampleMotionCurvesData(theAnimSequence, MotionSampleData, MotionCurvesData);
					break;

				case EVirtualRootMotionToolsType::ConvertMotionCaptureToRootMotion:
					UVAT_Asset::SampleRootMotionCurvesData(theAnimSequence, MotionCurvesData);
					break;
				}
#endif
			}
		}
		break;

	case EVirtualAssetToolsType::ResizeRootMotion:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Resizing animation assets root motion data"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				switch (InMotionProcessType)
				{
				case EVirtualRootMotionProcessType::SamplingCurves:
					switch (MotionSampleData.MotionToolsType)
					{
					case EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture:
						UVAT_Asset::SampleRootMotionCurvesData(theAnimSequence, MotionCurvesData);
						break;

					case EVirtualRootMotionToolsType::ConvertMotionCaptureToRootMotion:
						UVAT_Asset::SampleMotionCurvesData(theAnimSequence, MotionSampleData, MotionCurvesData);
						break;
					}
					break;

				case EVirtualRootMotionProcessType::ConvertingAsset:
					switch (MotionSampleData.MotionToolsType)
					{
					case EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture:
						UVAT_Asset::ConvertRootMotionToMotionCapture(theAnimSequence, MotionSampleData, MotionCurvesData);
						UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
						MotionSampleData.MotionToolsType = EVirtualRootMotionToolsType::ConvertMotionCaptureToRootMotion;
						break;

					case EVirtualRootMotionToolsType::ConvertMotionCaptureToRootMotion:
						UVAT_Asset::ConvertMotionCaptureToRootMotion(theAnimSequence, MotionSampleData, MotionCurvesData, SampleCurveAttributes);
						UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
						MotionSampleData.MotionToolsType = EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture;
						break;
					}
					break;
				}
			}
		}
		break;

	case EVirtualAssetToolsType::ConvertRootMotion:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Converting animation assets root motion data"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				switch (InMotionProcessType)
				{
				case EVirtualRootMotionProcessType::SamplingCurves:
					UVAT_Asset::SampleRootMotionCurvesData(theAnimSequence, MotionCurvesData);
					break;

				case EVirtualRootMotionProcessType::ConvertingAsset:
					UVAT_Asset::ConvertRootMotionData(theAnimSequence, MotionSampleData, MotionConvertData, MotionCurvesData, SampleCurveAttributes);
					UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
				}
				break;
			}
		}
		break;

	case EVirtualAssetToolsType::AlignmentRootMotion:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Alignment animation assets root motion data"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::AlignmentRootMotionData(theAnimSequence, MotionAlignmentData, SampleCurveAttributes);
				UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
			}
		}
		break;

	case EVirtualAssetToolsType::ConvertMotionToReferencePose:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theAssetToolsTask(theAnimSequences.Num(), LOCTEXT("AssetToolsText", "Converting motion capture asset to reference pose"));
			theAssetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theAssetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theAssetToolsTask.EnterProgressFrame();
				UVAT_Asset::ConvertMotionCaptureToReferencePose(theAnimSequence, MotionCaptureReferenceData);
				UVAT_Bone::SampleConstraintBones(theAnimSequence, BonesConstraintData);
			}
		}
		break;

	case EVirtualAssetToolsType::GenerateLOD:
 		for (USkeletalMesh* theSkeletalMesh : SkeletalMeshs)
 		{
 			UVAT_Asset::GenerateLODs(theSkeletalMesh, LODsData);
 		}
		break;

	case EVirtualAssetToolsType::RemoveLOD:
		for (USkeletalMesh* theSkeletalMesh : SkeletalMeshs)
		{
			UVAT_Asset::RemoveLODs(theSkeletalMesh, RemoveLODGroups);
		}
		break;

	case EVirtualAssetToolsType::Export:
		for (UAnimSequence* AnimSequence : theAnimSequences)
		{
			UVAT_Asset::ExportToFBX(EVirtualExportSourceOption::PreviewMesh, AnimSequence, AssetExportData);
		}
		break;
	}
}

void UVAT_DetailsViewData::OnResponseAlignmentMotionCurves()
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Response
	if (theAnimSequences.Num() > 0)
	{
		UVAT_Asset::AlignmentRootMotionCurvesData(theAnimSequences[0], MotionAlignmentData, MotionCurvesData);
	}
}
#pragma endregion


#pragma region Mirror
void UVAT_DetailsViewData::OnResponseMirrorTools(const EVirtualMirrorToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualMirrorToolsType::Mirror:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theMirrorToolsTask(theAnimSequences.Num(), LOCTEXT("MirrorToolsText", "Sampling mirror animation assets"));
			theMirrorToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theMirrorToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theMirrorToolsTask.EnterProgressFrame();

				// Get the mirror animation sequence reference
				UAnimSequence* theMirrorAnimSequence = UVAT_Asset::RebuildAnimationAsset(theAnimSequence, MirrorAssetType, &MirrorAssetSuffixName);

				// Response the mirror animation asset
				if (theMirrorAnimSequence != nullptr)
				{
					//UVAT_Mirror::SamplingMirrorBonesData(theMirrorAnimSequence, MirrorBoneTree);
					UVAT_Mirror::SamplingMirrorCSBonesData(theMirrorAnimSequence, MirrorBoneTree);
				}
			}
		}
		break;

	case EVirtualMirrorToolsType::MirrorBoneTree:
		if (GetSelectedSkeleton() != nullptr)
		{
			// Response sample mirror bone tree
			if (MirrorSampleStringLength > 0)
			{
				UVAT_Mirror::SamplingMirrorBoneTreeAsLength(GetSelectedSkeleton(), MirrorAxis, MirrorSampleStringLength, MirrorBoneTree);
			}
			else
			{
				UVAT_Mirror::SamplingMirrorBoneTreeAsName(GetSelectedSkeleton(), MirrorAxis, MirrorTwinNamesMap, MirrorBoneTree);
			}

			// If the save option is checked, we should save the currently sampled bone tree data
			if (bSaveMirrorBoneTree)
			{
				if (UVAT_DetailsViewData* theDataCDO = GetClass()->GetDefaultObject<UVAT_DetailsViewData>())
				{
					theDataCDO->MirrorBoneTree = MirrorBoneTree;
					UVAT_Library::ModifyFromObject(theDataCDO);
				}
			}
		}
		break;
	}
}

void UVAT_DetailsViewData::OnMirrorPropertyChanged(struct FPropertyChangedEvent* InPropertyChangedEvent)
{
	if (OnVirtualToolsChangingPropertiesDelegate.IsBound())
	{
		OnVirtualToolsChangingPropertiesDelegate.Broadcast(InPropertyChangedEvent);
	}
}
#pragma endregion


#pragma region Retarget
void UVAT_DetailsViewData::OnResponseRetargetTools(const EVirtualRetargetToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	FVirtualBonesData BonesData;
	//UVAT_Bone::GetBonesNameAsBoneTree(GetSelectedSkeleton(), RetargetBoneTree, &BonesData);

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualRetargetToolsType::Pose:
		if (Skeleton && theAnimSequences.Num() > 0)
		{
			if (ReferenceSkeleton_Retarget == nullptr)
			{
				UVAT_Retarget::OnRetargetPoseFromAnimation(GetSelectedSkeleton(), theAnimSequences[0], ReferenceAnimSequence_Retarget, BonesData);
			}
			else
			{
				UVAT_Retarget::OnRetargetPoseFromSkeleton(GetSelectedSkeleton(), ReferenceSkeleton_Retarget, theAnimSequences[0], BonesData);
			}
		}
		break;

	case EVirtualRetargetToolsType::Animation:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theRetargetToolsTask(theAnimSequences.Num(), LOCTEXT("RetargetToolsText", "Sampling retarget animation assets"));
			theRetargetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theRetargetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theRetargetToolsTask.EnterProgressFrame();
				UVAT_Retarget::OnRetargetAnimations(theAnimSequence, RetargetAnimationsData, BonesConstraintData);
			}
		}
		break;

	case EVirtualRetargetToolsType::Rebuild:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theRetargetToolsTask(theAnimSequences.Num(), LOCTEXT("RetargetToolsText", "Rebuilding animation assets"));
			theRetargetToolsTask.MakeDialog(true);

			// Each every animation asset
			for (UAnimSequence* theAnimSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theRetargetToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theRetargetToolsTask.EnterProgressFrame();
				UVAT_Retarget::OnRebuildAnimations(theAnimSequence);
			}
		}
		break;
	}
}
#pragma endregion


#pragma region PoseSearch
void UVAT_DetailsViewData::OnResponsePoseSearchTools(const EVirtualPoseSearchToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualPoseSearchToolsType::Distance:
		if (theAnimSequences.Num() > 0)
		{			
			// Create the dialog task
			FScopedSlowTask thePoseSearchToolsTask(theAnimSequences.Num(), LOCTEXT("PoseSearchToolsText", "Sampling pose distance for animation assets"));
			thePoseSearchToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (thePoseSearchToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				thePoseSearchToolsTask.EnterProgressFrame();
				UVAT_PoseSearch::SamplePoseFeatures<float, FFloatCurve>(theSequence, PoseSearchSampleData);
			}
		}
		break;

	case EVirtualPoseSearchToolsType::TimeSync:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask thePoseSearchToolsTask(theAnimSequences.Num(), LOCTEXT("PoseSearchToolsText", "Sampling pose time sync data for animation assets"));
			thePoseSearchToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (thePoseSearchToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				thePoseSearchToolsTask.EnterProgressFrame();
				UVAT_PoseSearch::SamplePoseTimeSyncFeatures<float, FFloatCurve>(theSequence, PoseSearchSampleData, PoseSearchTimeSyncData);
			}
		}
		break;

	case EVirtualPoseSearchToolsType::Animation:
		if (ToAnimSequencesPosesMap.Num() > 0 && theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask thePoseSearchToolsTask(ToAnimSequencesPosesMap.Num(), LOCTEXT("PoseSearchToolsText", "Sampling pose data for animation assets"));
			thePoseSearchToolsTask.MakeDialog(true);

			// Each every one data
			for (TPair<UAnimSequence*, FRuntimeFloatCurve>& thePair : ToAnimSequencesPosesMap)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (thePoseSearchToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				thePoseSearchToolsTask.EnterProgressFrame();
				UVAT_PoseSearch::SampleTwoPoseFeatures(theAnimSequences[0], thePair.Key, PoseSearchSampleData, thePair.Value);
			}
		}
		break;
	}
}
#pragma endregion


#pragma region GameFramework
void UVAT_DetailsViewData::OnResponseGamerFrameworkTools(const EVirtualGameFrameworkToolsType& InToolsType)
{
	// Get current selected animation sequences
	TArray<UAnimSequence*> theAnimSequences = GetSelectedAnimSequences();

	// Response the tools type
	switch (InToolsType)
	{
	case EVirtualGameFrameworkToolsType::FootIK:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling foot ik curves"));
			theGameFrameworkToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				UVAT_GameFramework::SamplingFootLanded(theSequence, FootIKSampleData, LegsBaseData);
			}
		}
		break;

	case EVirtualGameFrameworkToolsType::FootLock:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling foot lock curves"));
			theGameFrameworkToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				FootLockSampleData.CurveSampleData.GroundedHeights.Reset();
				UVAT_GameFramework::SamplingFootLock(theSequence, FootLockSampleData, LegsBaseData);

				// If the curves data is invalid, we can try enter first pose bone height
				int32 thePairIndex = -1;
				bool bShouldAgainSample = false;
				FootLockSampleData.CurveSampleData.GroundedHeights.Init(VAT_MIN, FootLockSampleData.CurveSampleData.RuntimeCurvesMap.Num());
				for (const TPair<FName, FRuntimeFloatCurve>& thePair : FootLockSampleData.CurveSampleData.RuntimeCurvesMap)
				{
					++thePairIndex;

					// Get the apex range
					const FVector2D theApexRange = FootLockSampleData.CurveSampleData.CurvesApexRanges.IsValidIndex(thePairIndex)
						? FootLockSampleData.CurveSampleData.CurvesApexRanges[thePairIndex] : FVector2D::ZeroVector;

					// Check the curves not all zero
					bool bIsValidCurve = false;
					for (const FRichCurveKey& theCurveKey : thePair.Value.GetRichCurveConst()->Keys)
					{
						if (theCurveKey.Value != theApexRange.X)
						{
							bIsValidCurve = true;
							break;
						}
					}

					// Again check the curve is valid
					if (bIsValidCurve)
					{
						continue;
					}

					// Check the leg base data is valid
					if (!LegsBaseData.IsValidIndex(thePairIndex))
					{
						continue;
					}

					// Get the tip and heel first pose height
					float theTipHeight = 0.f;
					if (const FRuntimeFloatCurve* theCurvePtr = FootLockSampleData.CurveSampleData.TipBoneHeightsCurvesMap.Find(LegsBaseData[thePairIndex].TipBone.BoneName))
					{
						if (theCurvePtr->GetRichCurveConst()->Keys.Num() > 0)
						{
							theTipHeight = theCurvePtr->GetRichCurveConst()->GetFirstKey().Value;
						}
					}
					float theHeelHeight = 0.f;
					if (const FRuntimeFloatCurve* theCurvePtr = FootLockSampleData.CurveSampleData.HeelBoneHeightsCurvesMap.Find(LegsBaseData[thePairIndex].FootBone.BoneName))
					{
						if (theCurvePtr->GetRichCurveConst()->Keys.Num() > 0)
						{
							theHeelHeight = theCurvePtr->GetRichCurveConst()->GetFirstKey().Value;
						}
					}

					// Try rebuild the curves
					bShouldAgainSample = true;
					FootLockSampleData.CurveSampleData.GroundedHeights[thePairIndex] = FootLockSampleData.bUseMinValueInLanded
						? FMath::Min(theTipHeight, theHeelHeight) : FMath::Max(theTipHeight, theHeelHeight);
				}

				// Again sample
				if (bShouldAgainSample)
				{
					UVAT_GameFramework::SamplingFootLock(theSequence, FootLockSampleData, LegsBaseData);
				}
			}
		}
		break;

	case EVirtualGameFrameworkToolsType::FootOffset:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling foot offset curves"));
			theGameFrameworkToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				UVAT_GameFramework::SamplingFootOffset(theSequence, FootOffsetSampleData, LegsBaseData);
			}
		}
		break;

	case EVirtualGameFrameworkToolsType::FootWeight:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling foot weight curves"));
			theGameFrameworkToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				UVAT_GameFramework::SamplingFootWeight(theSequence, FootWeightSampleData, PoseSearchSampleData);
			}
		}
		break;

	case EVirtualGameFrameworkToolsType::FootPosition:
 		if (theAnimSequences.Num() > 0)
 		{
			// Create the dialog task
 			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling foot position curves"));
			theGameFrameworkToolsTask.MakeDialog(true);
 
			// Each every one data
 			for (UAnimSequence* theSequence : theAnimSequences)
 			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				UVAT_GameFramework::SamplingFootPosition(theSequence, FootPositionSampleData, LegsBaseData);
 			}
 		}
		break;

	case EVirtualGameFrameworkToolsType::PosesCurve:
		if (theAnimSequences.Num() > 0)
		{
			// Create the dialog task
			FScopedSlowTask theGameFrameworkToolsTask(theAnimSequences.Num(), LOCTEXT("GameFrameworkToolsText", "Sampling poses blend curves"));
			theGameFrameworkToolsTask.MakeDialog(true);

			// Each every one data
			for (UAnimSequence* theSequence : theAnimSequences)
			{
				// We check if the cancel button has been pressed, if so we break the execution of the loop
				if (theGameFrameworkToolsTask.ShouldCancel())
				{
					break;
				}

				// Flag enter progress frame
				theGameFrameworkToolsTask.EnterProgressFrame();
				UVAT_GameFramework::SamplingPosesBlendData(theSequence, PosesBlendSampleData);
			}
		}
		break;
	}
}
#pragma endregion

#undef LOCTEXT_NAMESPACE