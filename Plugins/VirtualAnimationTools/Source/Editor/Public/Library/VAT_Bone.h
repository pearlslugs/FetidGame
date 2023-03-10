// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Types.h"
#include "VAT_Bone.generated.h"

class USkeleton;
class USkeletalMesh;

class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;

class UAnimNotify;
class UAnimNotifyState;

/** Types of virtual bone modifier tools */
UENUM()
enum class EVirtualBoneModifyType : uint8
{
	VBT_Delta		UMETA(DisplayName = "Delta"),    // We only take the difference value of the first frame of the animation to superimpose
	VBT_Blend		UMETA(DisplayName = "Blend"),
	VBT_Replace		UMETA(DisplayName = "Replace"),
	VBT_Additive		UMETA(DisplayName = "Additive"),
	VBT_MAX UMETA(Hidden)
};

/** Struct of animation asset bone data */
USTRUCT()
struct FVirtualBoneData
{
	GENERATED_USTRUCT_BODY()

#if 1
	/** Flag whether the bone translation track is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeTranslation : 1;

	/** Flag whether the bone rotation track is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeRotation : 1;

	/** Flag whether the bone scale track is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeScale : 1;
#endif
	/** Source bone reference */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference Bone;

	/** Modifier bone data type */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	EVirtualBoneModifyType ModifyType;

	/** Bone track data in animation asset every key. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualTransformData> TracksData;

public:

	/** Return the bone track data */
	void GetRawBoneTrackData(const int32& InNumberOfKeys, FRawAnimSequenceTrack& OutTrackData) const;

	FVirtualBoneData()
#if 1
		: bIncludeTranslation(true)
		, bIncludeRotation(true)
		, bIncludeScale(true)
#endif
		, ModifyType(EVirtualBoneModifyType::VBT_Additive)
	{
		//TracksData.AddDefaulted();
	}
};

/** Struct of bone add data */
USTRUCT()
struct FVirtualAddBoneData
{
	GENERATED_USTRUCT_BODY()

	/** Added bone name will automatically detect whether there is the same bone name */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FName BoneName;

	/** Name of the parent bone of the new bone, the new bone will be the child bone of this bone */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference ParentBone;

	/** If the bone is not empty, the BoneTransform is automatically set, such as the constraints of the IK bone */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference ConstraintBone;

	/** Bone default local space location in skeleton */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector BoneLocation;

	/** Bone default local space rotation in skeleton */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FRotator BoneRotation;

	/** Bone default local space scale in skeleton */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector BoneScale;

	FVirtualAddBoneData()
		: BoneName(NAME_None)
		, BoneLocation(FVector::ZeroVector)
		, BoneRotation(FRotator::ZeroRotator)
		, BoneScale(FVector::OneVector)
	{}
};

/** Struct of add bones data in skeletal mesh */
USTRUCT()
struct FVirtualAddBonesData
{
	GENERATED_USTRUCT_BODY()

	/** Skeletal mesh model that needs to add bones */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<USkeletalMesh*> SkeletalMeshs;

	/** Add bones data */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualAddBoneData> BonesData;

	FVirtualAddBonesData()
	{
		SkeletalMeshs.AddDefaulted();
		BonesData.AddDefaulted();
	}
};

/** Struct of remove bones data in skeletal mesh or skeleton */
USTRUCT()
struct FVirtualRemoveBonesData
{
	GENERATED_USTRUCT_BODY()

	/** Flag whether the bone of the subclass needs to be deleted */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bRemoveChildBones : 1;

	/** Flags whether to delete this bone on the skeletons, it will query all skeletal meshes that use this bone, and clean it up */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<USkeleton*> Skeletons;

	/** Skeletal mesh model that needs to remove bones */
	UPROPERTY(EditAnywhere, Category = "Bone Params", meta = (EditCondition = "!Skeleton"))
	TArray<USkeletalMesh*> SkeletalMeshs;

	/** Remove bones reference */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FBoneReference> Bones;

	FVirtualRemoveBonesData()
		: bRemoveChildBones(true)
	{
		Skeletons.AddDefaulted();
		SkeletalMeshs.AddDefaulted();
		Bones.AddDefaulted();
	}
};

/** Struct of animation asset layer bone branch filter data */
USTRUCT()
struct FVirtualBoneBranchFilter
{
	GENERATED_USTRUCT_BODY()

	/** Flag whether the source bone is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeSourceBone : 1;

	/** Flag whether the children bone is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeChildrenBone : 1;

	/** Flag whether the interrupt bone is involved in the calculation */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bIncludeInterruptBone : 1;

	/** Blend depth **/
	UPROPERTY(EditAnywhere, Category = "Bone Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 BlendDepth;

	/** Source bone reference to filter **/
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference SourceBone;

	/** Source bone to this bone for interrupt calculation **/
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference InterruptBone;

public:

	// Return the names of all children bones of the source bone to the interrupt bone
	TArray<FName> GetBoneTreeNames(const USkeleton* InSkeleton) const;

	FVirtualBoneBranchFilter()
		: bIncludeSourceBone(true)
		, bIncludeChildrenBone(true)
		, bIncludeInterruptBone(false)
		, BlendDepth(0)
	{}
};

/** Struct of animation asset bone filter data */
USTRUCT()
struct FVirtualBonesFilterData
{
	GENERATED_USTRUCT_BODY()

	/** Value of animation asset bone filter tolerance */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	float Tolerance;

	/** Types of animation asset bone filter */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	EVirtualCurveFilterType FilterType;

	/** Bones branch filter as layered blend per bone, blend target poses will be performed in sequence */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualBoneBranchFilter> BonesBranchFilter;

public:

	FVirtualBonesFilterData()
		: Tolerance(0.1f)
		, FilterType(EVirtualCurveFilterType::Euler)
	{
		FVirtualBoneBranchFilter& theBoneBrachFilter = BonesBranchFilter.AddDefaulted_GetRef();
		theBoneBrachFilter.SourceBone.BoneName = "Root";
	}
};

/** Struct of animation asset layer bone data */
USTRUCT()
struct FVirtualBonesLayerData
{
	GENERATED_USTRUCT_BODY()

	/** Whether to rescale the source animation to the same length as the target pose. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bResizeSourcePoseLength : 1;

	/** Whether to rescale the target layer animation to the same length as the source pose. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bResizeLayerPoseLength : 1;

	/** Whether to blend bone rotations in mesh space or in local space */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bMeshSpaceRotationBlend : 1;

	/** Whether to blend bone scales in mesh space or in local space */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bMeshSpaceScaleBlend : 1;

	/** Layer animation sequence asset */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	UAnimSequence* LayerAnimSequence;

	/** Bones branch filter as layered blend per bone, blend target poses will be performed in sequence */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualBoneBranchFilter> BonesBranchFilter;

	/** When component space blending is selected, the local space data of the specified bone can be retained without secondary transformation */
	UPROPERTY(EditAnywhere, Category = "Bone Params", meta = (EditCondition = "bMeshSpaceRotationBlend"))
	TArray<FBoneReference> LocalSpaceBones;

public:

	FVirtualBonesLayerData()
		: bResizeSourcePoseLength(true)
		, bResizeLayerPoseLength(true)
		, bMeshSpaceRotationBlend(false)
		, bMeshSpaceScaleBlend(false)
		, LayerAnimSequence(nullptr)
	{
		BonesBranchFilter.AddDefaulted();
	}
};

/** Struct of animation asset bone modifier data */
USTRUCT()
struct FVirtualModifyBoneData
{
	GENERATED_USTRUCT_BODY()

	/** If is true, we adjust first frame is zero, additive all track. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bMakeFisrtFrameIsZero : 1;

	/** Bone control space type */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TEnumAsByte<EBoneControlSpace> BoneSpace;

	/** Modifier bone data type */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	EVirtualBoneModifyType ModifyType;

	/** Modify bones data */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualBoneData> BonesData;

	/** Translation ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector TranslationRatio;

	/** Rotation ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector RotationRatio;

	/** Scale ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector ScaleRatio;

	FVirtualModifyBoneData()
		: bMakeFisrtFrameIsZero(false)
		, BoneSpace(EBoneControlSpace::BCS_BoneSpace)
		, ModifyType(EVirtualBoneModifyType::VBT_Replace)
		, TranslationRatio(FVector::OneVector)
		, RotationRatio(FVector::OneVector)
		, ScaleRatio(FVector::OneVector)
	{
		BonesData.AddDefaulted();
	}
};


/** Struct of bone constraint data */
USTRUCT()
struct FVirtualBoneConstraintData
{
	GENERATED_USTRUCT_BODY()

	/** For the source bone that needs to be constrained, we will modify the data of the bone */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference SourceBone;

	/** Reference target bone, we will copy the bone data to the source bone */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FBoneReference TargetBone;

	/** Use root motion space */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	uint8 bExportRootMotion : 1;

	/** If the frame is invalid, default constraint all animation pose frame */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector2D FrameRange;

	/** Relative location offset of target bone space */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector RelativeLocation;

	/** Relative rotation offset of target bone space */
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FRotator RelativeRotation;

	FVirtualBoneConstraintData()
		: bExportRootMotion(false)
		, FrameRange(FVector2D(-1.f))
		, RelativeLocation(ForceInitToZero)
		, RelativeRotation(ForceInitToZero)
	{}
};

USTRUCT()
struct FVirtualBonesData
{
	GENERATED_USTRUCT_BODY()

	// Smart change bone name.
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualBoneData> BonesData;

public:

	int32 AddUniqueData(const FVirtualBoneData& InBoneData)
	{
		bool bContain = false;
		for (FVirtualBoneData& BoneData : BonesData)
		{
			if (BoneData.Bone.BoneName == InBoneData.Bone.BoneName)
			{
				bContain = true;
				break;
			}
		}

		if (!bContain)
			return BonesData.Add(InBoneData);

		return INDEX_NONE;
	}

	FVirtualBonesData()
	{}
};

USTRUCT()
struct FVirtualBoneTransforms
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, Category = "Bone Params")
		FBoneReference Bone;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
		FBoneReference RootBone;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
		TArray<FTransform> BoneTransforms;

	FVirtualBoneTransforms()
	{}
};

USTRUCT()
struct FVirtualBakeBoneData
{
	GENERATED_USTRUCT_BODY()

	// From animations array index.
	UPROPERTY(VisibleAnywhere, Category = "Bone Params")
	int32 BakeAnimIndex;

	UPROPERTY(VisibleAnywhere, Category = "Bone Params")
	UAnimSequenceBase* BakeAnimSequenceBase;

	// Bake animation Frames Per Second
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	int32 BakeFPS;

	/*
	 * If the frame is invalid, default constraint all animation frame
	 * If bake to notify, auto check frame range.
	*/
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector2D BakeFrameRange;

	// If the track name is invalid, auto bake all notify.
	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FName BakeNotifyTrackName;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TSubclassOf<UAnimNotify> BakeNotifyClass;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TSubclassOf<UAnimNotifyState> BakeNotifyStateClass;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FVector BakeSocketLocation;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
	FRotator BakeSocketRotation;

	UPROPERTY(EditAnywhere, Category = "Bone Params")
	TArray<FVirtualBoneTransforms> BakeBoneTransforms;

public:

	bool IsValid() { return true/*GetFrameNumber() > 0*/; }

	bool IsValidRange(const int32& InAnimFrameRange) { return GetFrameNumber() > 0 && GetFrameNumber() <= InAnimFrameRange; }

	int32 GetFrameNumber() { return BakeFrameRange.X + BakeFrameRange.Y; }

	FVirtualBakeBoneData()
		: BakeAnimIndex(-1)
		, BakeAnimSequenceBase(nullptr)
		, BakeFPS(60)
		, BakeFrameRange(FVector2D(-1.f))
		, BakeNotifyTrackName("None")
		, BakeNotifyClass(NULL)
		, BakeNotifyStateClass(NULL)
		, BakeSocketLocation(FVector().ZeroVector)
		, BakeSocketRotation(FRotator().ZeroRotator)
	{
		BakeBoneTransforms.AddDefaulted();
	}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Bone : public UObject
{
	GENERATED_BODY()
	
public:

#pragma region Bone

	// Return the desired bone is child of bone
	static bool BoneIsChildOf(USkeleton* InSkeleton, const FName& InBoneName, const FName& InChildBoneName);

	 // Return the names of all bones of the current skeleton
	static bool GetBonesName(USkeleton* InSkeleton, TArray<FName>* OutBonesName);

#pragma endregion


#pragma region Get Bone Transform

	/** Return the motion data is valid */
	static bool HasMotionData(UAnimSequence* InAnimSequence, const float& InLastTime, const float& InNextTime, const float& InSampleRate = 0.333f, const bool InLastTrackKey = false);

	/** Return the animation has any motion data condition */
	static bool HasAnyMotionData(UAnimSequence* InAnimSequence);

	/**
	* Retrieves Bone Pose data for the given Bone Name at the specified Frame from the given Animation Sequence.
	*
	* @param	InAnimSequence			Animation sequence asset
	* @param	InExtractRootMotion		Whether or not any undo-redo changes should be generated
	*
	* @return	Bone local space transform
	*/
	static FTransform GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName);
	
	// Retrieves Bone Pose data for the given Bone Name at the specified Frame from the given Animation Sequence.
	static FTransform GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const float& InTime, const FName& InBoneName);

	// Returns the current bone transform from component space to local space
	static FTransform GetBoneTransformLS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName, const FTransform& InBoneTransformCS);

	// Returns the component space transform of the current bone 
	UFUNCTION(BlueprintCallable, Category = "VAT| Bone", meta = (DisplayName = "GetBoneTransformCSForFrame"))
	static FTransform GetBoneTransformCS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const int32& InFrame, const FName& InBoneName);

	// Returns the component space transform of the current bone 
	//UFUNCTION(BlueprintCallable, Category = "VAT| Bone", meta = (DisplayName = "GetBoneTransformCSForTime"))
	static FTransform GetBoneTransformCS(UAnimSequence* InAnimSequence, const bool& InExtractRootMotion, const float& InTime, const FName& InBoneName);
	
#pragma endregion

#if 0
#pragma region Set Bone Transform

	// Replace the current animation with the specified frame animation of the reference animation
	static void OnSetBoneTransform(UAnimSequence* InAnimSequence, UAnimSequence* InReferenceAnimSequence, const FVector2D& InFrameRange, const FVector2D& InReferenceFrameRange, const FVirtualBonesData& InBonesData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);
	
	// Replace the current animation with the specified frame animation of the reference transform
	static void OnSetBoneTransform(UAnimSequence* InAnimSequence, const FVector2D& InFrameRange, const FVector2D& InReferenceFrameRange, const FVirtualBonesData& InBonesData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);
	
	// Intelligently modify the animation bone data according to the input parameters
	static void OnBakeBoneTransforms(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData);

	// Intelligently modify the animation bone data according to the input parameters range
	static bool OnBakeBoneTransformsFromRange(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData, FVector2D InRange);

	// Returns AnimNotify, Intelligently modify the animation bone data according to the input parameters range
	static UAnimNotify* OnBakeBoneTransformsToNotify(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData);

	// Returns AnimNotifyState, Intelligently modify the animation bone data according to the input parameters range
	static UAnimNotifyState* OnBakeBoneTransformsToNotifyState(UAnimSequence* InAnimSequence, FVirtualBakeBoneData& InBakeBoneData);

	// Intelligently modify the animation bone data according to the AnimNotifyEvent
	static bool OnBakeBoneTransformsToNotifyEvent(UAnimSequence* InAnimSequence, const FAnimNotifyEvent& InAnimNotifyEvent, FVirtualBakeBoneData& InBakeBoneData);

#pragma endregion
#endif

#pragma region Bone Data

	/**
	* Adds a new bone animation track for the provided name. Broadcasts a EAnimDataModelNotifyType::TrackAdded notify if successful.
	*
	* @param	InSkeletalMesh			Desired skeletal mesh asset
	* @param	InBonesData			Add bones data
	*
	*/
	static void AddBonesData(USkeletalMesh* InSkeletalMesh, const TArray<FVirtualAddBoneData>& InBonesData);

	/**
	* Removes an existing bone animation  with the provided name. Broadcasts a EAnimDataModelNotifyType::Removed notify if successful.
	*
	* @param	InSkeleton			Desired skeleton
	* @param	InBonesReference			Remove bones reference
	* @param	InRemoveChildBones			Flag remove children bones
	*
	*/
	static void RemoveBonesData(USkeleton* InSkeleton, const TArray<FBoneReference>& InBonesReference, bool InRemoveChildBones = true);

	/**
	* Removes an existing bone animation  with the provided name. Broadcasts a EAnimDataModelNotifyType::Removed notify if successful.
	*
	* @param	InSkeletalMesh			Desired skeletal mesh asset
	* @param	InBonesReference			Remove bones reference
	* @param	InRemoveChildBones			Flag remove children bones
	*
	*/
	static void RemoveBonesData(USkeletalMesh* InSkeletalMesh, const TArray<FBoneReference>& InBonesReference, bool InRemoveChildBones = true);

#pragma endregion


#pragma region Bone Track

	/**
	* Adds a new bone animation track for the provided name.
	*
	* @param	InAnimSequence			Animation sequence asset
	* @param	InBonesData		Bones track data
	*
	*/
	static void AddBonesTrack(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneData>& InBonesData);

	/**
	* Modify a bone animation track for the provided name.
	*
	* @param	InAnimSequence			Animation sequence asset
	* @param	InModifyBoneData		Bones track modify data
	*
	*/
	static void ModifyBonesTrack(UAnimSequence* InAnimSequence, FVirtualModifyBoneData& InModifyBoneData);

	/**
	* Removes an existing bone animation track with the provided name.
	*
	* @param	InAnimSequence			Animation sequence asset
	* @param	InBonesData		Bones track data
	*
	*/
	static void RemoveBonesTrack(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneData>& InBonesData);

	/**
	* Sample an existing bone animation track with the provided name.
	*
	* @param	InBoneSpace			Bone control space type
	* @param	InAnimSequence			Animation sequence asset
	* @param	OutBonesData		Return bones track data in desired control space
	*
	*/
	static void SampleBonesTrack(const EBoneControlSpace& InBoneSpace, UAnimSequence* InAnimSequence, TArray<FVirtualBoneData>& OutBonesData);

#pragma endregion


#pragma region Bone Layer


	/**
	*  Layered blend (per bone); has dynamic number of blendposes that can blend per different bone sets.
	*
	* @param	InAnimSequence			Animation sequence asset
	* @param	InBoneLayerData		Layer bone data
	* @param	InConstraintBonesData		Constraint bones data
	*
	*/
	static void LayeredBoneBlend(UAnimSequence* InAnimSequence, FVirtualBonesLayerData& InBoneLayerData
		, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);

#pragma endregion


#pragma region Bone Filter

	/**
	* Filter incorrect animation data
	*
	* @param InAnimSequence		Animation pose asset
	* @param InBoneFilterData		Bone filter data
	*
	*/
	static void FilterBoneTracks(UAnimSequence* InAnimSequence, const FVirtualBonesFilterData& InBoneFilterData);

#pragma endregion


#pragma region Bone Constraint

	/**
	* Constrain the source bone to the target bone, copy and paste the data of the target bone
	*
	* @param InAnimSequence		Animation pose asset
	* @param InConstraintBonesData		Constraint bones data
	*
	*/
	static bool SampleConstraintBones(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);

#if 0 // DEPRECATED
	//	Constrain virtual bones based on input parameters
	static void OnConstraintVirtualBone(UAnimSequence* InAnimSequence);

	//	Reset Reference Bone Transform
	static void OnResetConstraintBone(UAnimSequence* InAnimSequence, const FVirtualBoneConstraintData& InConstraintBoneData);
#endif
#pragma endregion


#pragma region Bone Delta

	/**
	* Constrain the source bone to the target bone, copy and paste the data of the target bone
	*
	* @param InAnimSequence		Animation pose asset
	* @param InConstraintBonesData		Constraint bones data
	*
	*/
	//static bool SampleDeltaBones(UAnimSequence* InAnimSequence, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);

#pragma endregion


#pragma region Bone Space

	/**
	 *	Transform a location/rotation from world space to bone relative space.
	 *	This is handy if you know the location in world space for a bone attachment, as AttachComponent takes location/rotation in bone-relative space.
	 *
	 * @param BoneName Name of bone
	 * @param InPosition Input position
	 * @param InRotation Input rotation
	 * @param OutPosition (out) Transformed position
	 * @param OutRotation (out) Transformed rotation
	 */
	UFUNCTION(BlueprintCallable, Category = "VAT| BoneSpace")
	static void TransformToBoneSpace(UAnimSequence* InSequence, FName InBoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation);

#pragma endregion
};
