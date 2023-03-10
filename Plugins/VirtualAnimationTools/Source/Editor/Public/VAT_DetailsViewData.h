// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorActionUtility.h"

#include "Library/VAT_Bone.h"
#include "Library/VAT_Curve.h"
#include "Library/VAT_Notify.h"
#include "Library/VAT_Montage.h"
#include "Library/VAT_Asset.h"
#include "Library/VAT_Mirror.h"
#include "Library/VAT_Retarget.h"
#include "Library/VAT_PoseSearch.h"
#include "Library/VAT_GameFramework.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"

#include "VAT_DetailsViewData.generated.h"

class USkeleton;
class UAnimMontage;
class UAnimSequence;
class UAnimComposite;
class UAnimSequenceBase;
class UAnimNotify;
class UAnimNotifyState;

class USkeletalMesh;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDataClassChangingProperties, const TSubclassOf<UVAT_DetailsViewData>&);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVirtualToolsChangingProperties, const FPropertyChangedEvent*);


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_DetailsViewData : public UActorActionUtility, public IBoneReferenceSkeletonProvider
{
	GENERATED_UCLASS_BODY()

public:
	mutable FOnDataClassChangingProperties OnDataClassChangingPropertiesDelegate;
	mutable FOnVirtualToolsChangingProperties OnVirtualToolsChangingPropertiesDelegate;

public: // IBoneReferenceSkeletonProvider
	class USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError) override { bInvalidSkeletonIsError = false; return Skeleton; }

public:

	/** Response handle event in selected changed */
	void OnSelectionChanged();

	/**
	 * This alternate version of PostEditChange is called when properties inside structs are modified.  The property that was actually modified
	 * is located at the tail of the list.  The head of the list of the FStructProperty member variable that contains the property that was modified.
	 */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent) override;

#pragma region Animation

public:

	/** Cached current virtual tools type */
	UPROPERTY(Transient)
	EVirtualToolsType VirtualToolsType;

	/** Template class used by the virtual animation tools */
	UPROPERTY(EditAnywhere, Category = Animation, meta = (DisplayName = "Class"))
	TSubclassOf<UVAT_DetailsViewData> DataClass;

	/** Skeleton used by the first animation asset */
	UPROPERTY(EditAnywhere, Category = Animation)
	USkeleton* Skeleton;

	/** Animation assets that need to be modifier */
	UPROPERTY(EditAnywhere, Category = Animation)
	TArray<UAnimSequenceBase*> Animations;

	/** Selected animation assets in editor preview */
	UPROPERTY(VisibleAnywhere, Category = Animation)
	TArray<UAnimSequenceBase*> SelectedAnimations;

public:

	/** Return whether a selection change has occurred */
	bool IsSelectedChanged(TArray<UAnimSequenceBase*>& OutAsset);

	/** Return the set of currently selected skeleton */
	USkeleton* GetSelectedSkeleton();

	/** Return the set of currently selected animation assets */
	TArray<UAnimSequenceBase*> GetSelectedAnimAssets();

	/** Return the set of currently selected animation sequences */
	TArray<UAnimSequence*> GetSelectedAnimSequences();

	/** Return the set of currently selected animation montages */
	TArray<UAnimMontage*> GetSelectedAnimMontages();

	/** Set the new data class */
	void SetDataClass(const TSubclassOf<UVAT_DetailsViewData>& InDataClass);

	/** Set current virtual tools type */
	void SetVirtualToolsType(const EVirtualToolsType& InToolsType);

	/** response data class property changed */
	void OnDataClassEditChangeProperty(struct FPropertyChangedEvent& InPropertyChangedEvent);

	/** response animations property changed */
	void OnAnimationsEditChangeProperty(struct FPropertyChangedEvent* InPropertyChangedEvent);

#pragma endregion


#pragma region Skeleton

	/** Base leg data for this animation skeleton */
	UPROPERTY(EditAnywhere, Category = "Skeleton Params")
	TArray<FVirtualLegBaseData> LegsBaseData;

#pragma endregion


#pragma region Bone

	/** Add bones data in animation assets */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	FVirtualAddBonesData AddBonesData;

	/** Remove bones data in animation assets */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	FVirtualRemoveBonesData RemoveBonesData;

	/** Sample bones data */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	TArray<FVirtualBoneData> SampleBonsData;

	/** Modify bones data */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	FVirtualModifyBoneData ModifyBonesData;

	/** Layered blend (per bone); has dynamic number of blendposes that can blend per different bone sets */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	FVirtualBonesLayerData BoneLayerData;

	/** Skeleton data filtering data, can correct wrong animation data */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	FVirtualBonesFilterData BoneFilterData;

	/** Constraint bones data */
	UPROPERTY(EditAnywhere, Category = "Bone Attributes")
	TArray<FVirtualBoneConstraintData> BonesConstraintData;

public:

	/** Clear bone cached data event */
	void OnClearBoneTools(const EVirtualBoneToolsType& InToolsType);

	/** Sample bone tracks tools event */
	void OnSampleBoneTracks(const EVirtualBoneToolsType& InToolsType);

	/** Response bone tools event */
	void OnResponseBoneTools(const EVirtualBoneToolsType& InToolsType);

#pragma endregion


#pragma region Curve

protected:

	/** Flag whether to clean up all curves for animation assets */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	uint8 bClearAllCurves : 1;

	/** Name of the currently used curve preset, it will find the corresponding curve preset
	  * If the corresponding preset configuration is not found, the curve name will be directly queried */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	FName CurvePresetName;
	
	/** Pre-configured curve name presets */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	TMap<FName, FVirtualCurvePresetData> CurvePresetsMap;

	/** Sample bones curve data */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	TArray<FVirtualBoneCurveData> SampleBonesCurve;

	/** Converts the current animation curves data to the specified curve asset */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	TArray<UCurveBase*> OutputCurveAssets;

	/** Copy data from source curve to target curve */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	TArray<FVirtualCopyCurveData> CopyCurvesData;

	/** Transfer data from source curve to target curve */
	UPROPERTY(EditAnywhere, Category = "Curve Attributes")
	TArray<FVirtualTransferCurveData> TransferCurvesData;

	/** Sample curve attributes data */
	UPROPERTY(EditAnywhere, Category = "Curve Reference")
	FVirtualCurveAttributes SampleCurveAttributes;

public:

	/** Set accumulate curve state */
	void SetAccumulateCurve(const bool InState);

	/** Response curve tools event */
	void OnResponseCurveTools(const EVirtualCurveToolsType& InToolsType);

#pragma endregion


#pragma region Notify

protected:

	/** Animation notifies class data */
	UPROPERTY(EditAnywhere, Category = "Notify Attributes")
	TArray<FVirtualNotifiesData> NotifiesData;

	/** Animation modify notifies track data */
	UPROPERTY(EditAnywhere, Category = "Notify Attributes")
	TMap<UAnimSequenceBase*, FVirtualMultiNotifiesData> ModifyNotifiesData;

	/** Animation notifies track data */
	UPROPERTY(EditAnywhere, Category = "Notify Attributes")
	TArray<FVirtualNotifyTrackData> NotifiesTrackData;

	/** Animation modify notifies track data */
	UPROPERTY(EditAnywhere, Category = "Notify Attributes")
	TMap<UAnimSequenceBase*, FVirtualNotifyModifyTrackData> ModifyNotifiesTrackMap;

	/** Sampling configuration data for foot step */
	UPROPERTY(EditAnywhere, Category = "Notify Attributes")
	FVirtualCurveSampleData FootStepSampleData;

public:

	/** Sample notifies data */
	void OnSampleNotifiesData(const EVirtualNotifyToolsType& InToolsType);

	/** Response notify tools event */
	void OnResponseNotifyTools(const EVirtualNotifyToolsType& InToolsType);

#pragma endregion


#pragma region Montage

public:

	/** Slots data of the current selected montages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Slots")
	TArray<FVirtualMontageSlotData> MontageSlotsData;

	/** If is true, we will clear other invalid groups and slots */
	UPROPERTY(EditAnywhere, Category = "Montage")
	uint8 bClearInvalidSlots : 1;

	/** Slots data of the current selected montages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (DisplayName = "Target Skeletons"))
	TArray<USkeleton*> TargetSkeletons_Montage;

	/** Skeleton are sorted in the initial order of the montage slots */
	UPROPERTY(EditAnywhere, Category = "Montage Reference")
	uint8 bSortMontageSlots : 1;

	/** Montages groups and slots name reference */
	UPROPERTY(VisibleAnywhere, Category = "Montage Reference")
	TMap<FName, FVirtualMontageGroupData> MontageGroupsMap;

public:

	/** Initialize selected montage slots data */
	void InitializeMontageSlotsData();

	/** Response montage tools event */
	void OnResponseMontageTools(const EVirtualMontageToolsType& InToolsType);

#pragma endregion


#pragma region Asset

protected:

	/** Animation asset crop data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualAssetCropData AssetCropData;

	/** Animation asset insert data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualAssetInsertData AssetInsertData;

	/** Animation asset resize data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualAssetResizeData AssetResizeData;

	/** Animation asset replace data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualAssetReplaceData AssetReplaceData;

	/** Animation asset composites data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	TArray<FVirtualAssetCompositeData> AssetCompositesData;

	/** Animation asset motion curves data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualBoneCurvesData MotionCurvesData;

	/** Animation asset motion sample data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualMotionSampleData MotionSampleData;
	
	/** Animation asset root motion convert data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualMotionConvertData MotionConvertData;

	/** Animation asset root motion alignment data */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualMotionAlignmentData MotionAlignmentData;
	
	/** Convert motion capture asset to reference pose data (Zeroing the initial frame transform) */
	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualMotionCaptureReferenceData MotionCaptureReferenceData;

	/** Wait Modifier */
	/** Wait Modifier */

	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	TArray<USkeletalMesh*> SkeletalMeshs;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Asset Attributes")
	FVirtualAssetGenerateLODsData LODsData;

	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	TArray<int32> RemoveLODGroups;

	UPROPERTY(EditAnywhere, Category = "Asset Attributes")
	FVirtualAssetExportData AssetExportData;

public:

	/** Response set root motion tools type */
	void SetRootMotionToolsType(const EVirtualRootMotionToolsType& InTypes);

	/** Response asset tools event */
	void OnResponseAssetTools(const EVirtualAssetToolsType& InToolsType, const EVirtualRootMotionProcessType& InMotionProcessType);

	/** Response alignment motion curves event */
	void OnResponseAlignmentMotionCurves();

#pragma endregion


#pragma region Mirror

public:

	/** Types of mirror animation asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	EVirtualToolsRebuildAssetType MirrorAssetType;

	/** Flag whether to save the current mirror data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	uint8 bSaveMirrorBoneTree : 1;

	/** Reference skeleton global mirror bone axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	TEnumAsByte<EAxis::Type> MirrorAxis;

	/** Twin bones direction names map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	TMap<FName, FName> MirrorTwinNamesMap;

	/** Create a new mirror animation asset suffix name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	FName MirrorAssetSuffixName;

	/** <Experimental> String length of the mirror sample, will compare whether each twin bone has the same string length */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Attributes")
	int32 MirrorSampleStringLength;

	/** Mirror skeleton bone tree  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror BoneTree")
	TArray<FVirtualMirrorBoneData> MirrorBoneTree;

public:

	/** Response mirror tools event */
	void OnResponseMirrorTools(const EVirtualMirrorToolsType& InToolsType);

	/** Response mirror property changed event */
	void OnMirrorPropertyChanged(struct FPropertyChangedEvent* InPropertyChangedEvent);

#pragma endregion


#pragma region Retarget

protected:

	UPROPERTY(EditAnywhere, Category = Retarget, meta = (DisplayName = "Reference Skeleton"))
	USkeleton* ReferenceSkeleton_Retarget;

	UPROPERTY(EditAnywhere, Category = Retarget, meta = (DisplayName = "Reference Anim Sequence"))
	UAnimSequence* ReferenceAnimSequence_Retarget;

	// Bake bone tree
	UPROPERTY(EditAnywhere, Category = Retarget)
	TArray<FVirtualBoneBranchFilter> RetargetBoneTree;

	UPROPERTY(EditAnywhere, Category = Retarget)
	FVirtualRetargetAnimationsData RetargetAnimationsData;

public:

	/** Response retarget tools event */
	void OnResponseRetargetTools(const EVirtualRetargetToolsType& InToolsType);

#pragma endregion


#pragma region PoseSearch

protected:

	/** Data of animation pose search, corresponding to animation assets - start position curve */
	UPROPERTY(EditAnywhere, Category = PoseSearch, meta = (DisplayName = "To Anim Sequences"))
	TMap<UAnimSequence*, FRuntimeFloatCurve> ToAnimSequencesPosesMap;

	/** Pose search sampled config data */
	UPROPERTY(EditAnywhere, Category = PoseSearch)
	FVirtualPoseSearchSampleData PoseSearchSampleData;

	/** Pose search time sync data */
	UPROPERTY(EditAnywhere, Category = PoseSearch)
	FVirtualPoseSearchTimeSyncData PoseSearchTimeSyncData;
	
public:

	/** Response pose search tools event */
	virtual void OnResponsePoseSearchTools(const EVirtualPoseSearchToolsType& InToolsType);

#pragma endregion


#pragma region GameFramework

protected:

	/** Sampling configuration data for footstep IK curves */
 	UPROPERTY(EditAnywhere, Category = GameFramework, meta = (DisplayName = "Foot IK Sample Data"))
	FVirtualCurveSampleData FootIKSampleData;

	/** Sampling configuration data for footstep lock curves */
	UPROPERTY(EditAnywhere, Category = GameFramework)
	FVirtualFootLockSampleData FootLockSampleData;

	/** Sampling configuration data for footstep offset curves */
	UPROPERTY(EditAnywhere, Category = GameFramework)
	FVirtualFootOffsetData FootOffsetSampleData;

	/** Sampling configuration data for footstep weight data */
	UPROPERTY(EditAnywhere, Category = GameFramework)
	FVirtualFootWeightData FootWeightSampleData;

	/** Sampling configuration data for footstep position curves */
	UPROPERTY(EditAnywhere, Category = GameFramework)
	FVirtualFootWeightData FootPositionSampleData;

	/** Sampling configuration data for poses blend curves */
	UPROPERTY(EditAnywhere, Category = GameFramework)
	FVirtualPosesCompareData PosesBlendSampleData;

public:

	/** Response game framework tools event */
	virtual void OnResponseGamerFrameworkTools(const EVirtualGameFrameworkToolsType& InToolsType);

#pragma endregion
};
