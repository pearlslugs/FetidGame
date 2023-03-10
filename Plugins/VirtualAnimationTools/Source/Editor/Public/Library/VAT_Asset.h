// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Math/Axis.h"
#include "VAT_Curve.h"
#include "VAT_Types.h"
#include "AlphaBlend.h"
#include "BoneContainer.h"
#include "VAT_Asset.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class USkeletalMeshLODSettings;

/** Types of root motion tools type */
UENUM()
enum class EVirtualRootMotionToolsType : uint8
{
	ConvertRootMotionToMotionCapture,
	ConvertMotionCaptureToRootMotion,
	MAX UMETA(Hidden)
};

/** Types of root motion process type */
enum class EVirtualRootMotionProcessType : uint8
{
	SamplingCurves,
	ConvertingAsset,
	MAX UMETA(Hidden)
};

/** Options for asset export */
enum class EVirtualExportSourceOption : uint8
{
	AnimData,
	PreviewMesh,
	Max
};

/** Struct of animation asset track data */
struct FVirtualAssetTrackData
{
	/** Animation asset track data name */
	FName TrackName;

	/** Animation asset track bone poses time data */
	TArray<float> PosesTime;

	/** Animation asset track bone poses transform data */
	TArray<FTransform> PosesTransform;

	FVirtualAssetTrackData()
		: TrackName(NAME_None)
	{}
};

/** Struct of animation asset crop data */
USTRUCT()
struct FVirtualAssetCropData
{
	GENERATED_USTRUCT_BODY()

	/** Cropped start frame in animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 StartFrame;

	/** Cropped end frame in animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 EndFrame;

	FVirtualAssetCropData()
		: StartFrame(0)
		, EndFrame(0)
	{}
};

/** Struct of animation asset insert data */
USTRUCT()
struct FVirtualAssetInsertData
{
	GENERATED_USTRUCT_BODY()

	/** Insert start frame in animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 StartFrame;

	/** Insert end frame in animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 EndFrame;

	/** Copy the specified pose of an animation asset in pose frame */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 CopyFrame;

	/** Count to copy the pose */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "1", ClampMin = "1"))
	int32 CopyCount;

	FVirtualAssetInsertData()
		: StartFrame(0)
		, EndFrame(1)
		, CopyFrame(0)
		, CopyCount(1)
	{}
};

/** Struct of animation asset resize data */
USTRUCT()
struct FVirtualAssetResizeData
{
	GENERATED_USTRUCT_BODY()

	/** Flag enable frame rate modifier condition */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bEnableFrameRateModifier : 1;

	/** Resize frame rate, always use 30 frames to sync with the engine by default */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (EditCondition = "bEnableFrameRateModifier"))
	FFrameRate FrameRate;

	/** Flag enable animation length modifier condition */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bEnableLengthModifier : 1;

	/** Desired animation length, if it is invalid, will keep animation asset length unchanged */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (EditCondition = "bEnableLengthModifier", UIMin = "0", ClampMin = "0"))
	float Length;

	/** Flag enable animation play rate modifier condition */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bEnablePlayRateModifier : 1;

	/** Desired animation length, if it is invalid, will keep animation asset length unchanged */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (EditCondition = "bEnablePlayRateModifier"))
	float PlayRate;

	FVirtualAssetResizeData()
		: bEnableFrameRateModifier(false)
		, FrameRate(FFrameRate(30, 1))
		, bEnableLengthModifier(true)
		, Length(0.f)
		, bEnablePlayRateModifier(false)
		, PlayRate(1.f)
	{}
};

/** Struct of animation asset replace data */
USTRUCT()
struct FVirtualAssetReplaceData
{
	GENERATED_USTRUCT_BODY()

	/** For the referenced animation asset, we will copy its animation pose data. If it is empty, we will copy the source animation pose data by default */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	UAnimSequence* ReferenceAnimSequence;

	/** Source pose frame that needs to be replaced */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	TArray<int32> SourceFrames;

	/** Pose frame of the target animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	TArray<int32> CopyFrames;

	/** Ignore replace bones data */
	UPROPERTY(EditAnywhere, Category = "Asset Params", meta = (UIMin = "0", ClampMin = "0"))
	TArray<FBoneReference> IgnoreBones;

	FVirtualAssetReplaceData()
		: ReferenceAnimSequence(nullptr)
	{
		SourceFrames.Add(0);
		CopyFrames.Add(0);
	}
};

/** Struct of animation asset composite data */
USTRUCT()
struct FVirtualAssetCompositeData
{
	GENERATED_USTRUCT_BODY()

	/**	Whether the flag is inserted into the previous animation asset, the default is always composite in the last frame */
	//UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bInsert : 1;

	/**	If true, we replace A with blend data instead of B's ​​blend duration */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bBlendLastAsset : 1;

	/** Frame range to be composite */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector2D FrameRange;

	/** Last animation asset blend in the composite asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FAlphaBlend BlendIn;

	/** Composite Animation asset */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	UAnimSequence* AnimSequence;

	FVirtualAssetCompositeData()
		: bInsert(false)
		, bBlendLastAsset(false)
		, FrameRange(FVector2D(1.f, -1.f))
		, BlendIn(FAlphaBlend(0.f))
		, AnimSequence(nullptr)
	{
		BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
	}
};

/** Struct of animation asset convert root motion data */
USTRUCT()
struct FVirtualMotionConvertData
{
	GENERATED_USTRUCT_BODY()

	/** If is true, we adjust motion bone data. */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bRemoveRelativeMotionData : 1;

	/** Translation ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector TranslationRatio;

	/** Rotation ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector RotationRatio;

	/** Scale ratio of the root bone, if it is 0, it will be cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector ScaleRatio;

	FVirtualMotionConvertData()
		: bRemoveRelativeMotionData(false)
		, TranslationRatio(FVector::OneVector)
		, RotationRatio(FVector(0.f, 0.f, 1.f))
		, ScaleRatio(FVector::OneVector)
	{}
};

/** Struct of animation asset alignment motion capture to reference pose data */
USTRUCT()
struct FVirtualMotionAlignmentData
{
	GENERATED_USTRUCT_BODY()

	/** If true, we convert the motion trajectory to a linear trajectory */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bIsLineTrack : 1;

	/** Ignore calculated pose frame range */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector2D IgnoreFrameRange;

	/** Motion bone reference for real movement (Usually Pelvis Bone) */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FBoneReference MotionBone;

	FVirtualMotionAlignmentData()
		: bIsLineTrack(false)
		, IgnoreFrameRange(FVector2D(0.f, 0.f))
	{}
};

/** Struct of animation asset convert motion capture to reference pose data */
USTRUCT()
struct FVirtualMotionCaptureReferenceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bUseReferenceSkeleton : 1;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	UAnimSequence* ReferencePose;

	/** Motion bone reference for real movement (Usually Pelvis Bone) */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FBoneReference MotionBone;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector ReferenceLocation;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FRotator ReferenceRotation;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FVector ReferenceScale;

	FVirtualMotionCaptureReferenceData()
		: bUseReferenceSkeleton(false)
		, ReferencePose(nullptr)
		, ReferenceLocation(FVector(0.f, -1.056153f, 96.750603f))
		, ReferenceRotation(FRotator(89.790634f, 89.999062f, 89.999062f))
		, ReferenceScale(FVector::OneVector)
	{}
};

/** Struct of animation asset motion capture data */
USTRUCT()
struct FVirtualMotionSampleData
{
	GENERATED_USTRUCT_BODY()

	/** Whether to resize the root bone to the origin for zeroing */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	EVirtualRootMotionToolsType MotionToolsType;

	/** Whether to resize the root bone to the origin for zeroing */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bResizeToOrigin: 1;

	/** Motion bone reference for real movement (Usually Pelvis Bone) */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	FBoneReference MotionBone;

	/** Global weight by which all data will be multiplied */
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	float GlobalWeight;

	/** Independent weights for each axis of rotation.
	  * X = Roll weight, Y = Pitch weight, Z = Yaw weight */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Asset Params")
	TMap<TEnumAsByte<EAxis::Type>, float> RotationWeightsMap;

	/** Independent weights for each axis of translation. */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Asset Params")
	TMap<TEnumAsByte<EAxis::Type>, float> TranslationWeightsMap;

	FVirtualMotionSampleData()
		: MotionToolsType(EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture)
		, bResizeToOrigin(true)
		, GlobalWeight(1.f)
	{
		RotationWeightsMap.FindOrAdd(EAxis::X, 0.f);
		RotationWeightsMap.FindOrAdd(EAxis::Y, 0.f);
		RotationWeightsMap.FindOrAdd(EAxis::Z, 1.f);
		TranslationWeightsMap.FindOrAdd(EAxis::X, 1.f);
		TranslationWeightsMap.FindOrAdd(EAxis::Y, 1.f);
		TranslationWeightsMap.FindOrAdd(EAxis::Z, 1.f);
	}
};

USTRUCT()
struct FVirtualAssetGenerateLODsData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "10"), Category = "Asset Params")
	int32 DefaultGroups;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bRemapMorphTargets : 1;
	
	UPROPERTY(EditAnywhere, Category = "Asset Params")
	uint8 bOverrideLODSettings : 1;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	USkeletalMeshLODSettings* LODSettings;

	//UPROPERTY(EditAnywhere, Category = "Asset Params")
	//class FTargetPlatform TargetPlatform;

	FVirtualAssetGenerateLODsData()
		: DefaultGroups(4)
		, bRemapMorphTargets(true)
		, bOverrideLODSettings(true)
		, LODSettings(nullptr)
	{}
};

USTRUCT()
struct FVirtualAssetExportData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "120"), Category = "Asset Params")
	int32 Rate;

	UPROPERTY(EditAnywhere, Category = "Asset Params")
	USkeletalMeshComponent* PreviewMeshComponent;

	FVirtualAssetExportData()
		: Rate(60)
		, PreviewMeshComponent(nullptr)
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Asset : public UObject
{
	GENERATED_BODY()

public:

#pragma region Animation Asset

	/**
	 * Crops the raw animation data either from Start to CurrentTime or CurrentTime to End depending on.
	 *
	 * @param	InAnimSequence		Reference animation sequence
	 * @param	InAssetCropFrameData		Asset crop frame data
	 */
	static void CropAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetCropData& InAssetCropFrameData);

	/**
	 * Insert the raw animation data from start to end frame
	 *
	 * @param	InAnimSequence		Reference animation sequence
	 * @param	InAssetInsertData		Asset insert data
	 */
	static void InsertAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetInsertData& InAssetInsertData);

	/**
	 * Readjust the frame rate and length of animation assets
	 *
	 * @param	InAnimSequence		Reference animation sequence
	 * @param	InAssetResizeData		Asset resize data
	 */
	static void ResizeAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetResizeData& InAssetResizeData);

	/**
	 * Replace target pose frame
	 *
	 * @param	InAnimSequence		Reference animation sequence
	 * @param	InAssetReplaceData		Asset replace data
	 */
	static void ReplaceAnimationAsset(UAnimSequence* InAnimSequence, const FVirtualAssetReplaceData& InAssetReplaceData);

	/**
	 * Composite the raw animation data from start to end frame
	 *
	 * @param	InAnimSequence		Reference animation sequence
	 * @param	InAssetCompositesData		Asset composites data
	 */
	static void OnCompositesAnimationAsset(UAnimSequence* InAnimSequence, TArray<FVirtualAssetCompositeData>& InAssetCompositesData);

	/**
	 * Rebuild new animation assets, override, duplicated etc.
	 *
	 * @param	InAnimSequence		Reference animation asset
	 * @param	InRebuildAssetType		Asset rebuild model
	 * @param	InSuffixName		New animation asset suffix name
	 * @return					New animation asset
	 */
	static UAnimSequence* RebuildAnimationAsset(UAnimSequence* InAnimSequence, EVirtualToolsRebuildAssetType InRebuildAssetType = EVirtualToolsRebuildAssetType::VTAT_DuplicateAsset, const FName* InSuffixName = nullptr);

#pragma endregion


#pragma region Root Motion

	/**
	 * Returns the upper limit of the current root bone motion data.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	OutMaxSpeed		Out max motion speed
	 */
	static FTransform GetRootMotionApexTransform(UAnimSequence* InAnimSequence, float& OutMaxSpeed);

	/**
	 * Extract root motion transform from the animation.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InStartTime		Start time
	 * @param	InDeltaTime		Delta time
	 * @param	InLooping		Flag the animation asset is looping
	 */
	static FTransform OnExtractRootMotion(UAnimSequence* InAnimSequence, const float& InStartTime, const float& InDeltaTime, const bool& InLooping);

	/**
	 * Extract root motion transform from a contiguous position range (no looping).
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InStartTime		Extract start time
	 * @param	InEndTrackPosition		Extract next time
	 */
	static FTransform OnExtractRootMotionFromRange(UAnimSequence* InAnimSequence, const float& InStartTrackPosition, const float& InEndTrackPosition);

#pragma endregion


#pragma region Motion Capture

	/**
	 * Sample motion data and convert it into curve data.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionCaptureData		Motion capture config data
	 * @param	OutMotionCurvesData		Motion curves data
	 */
	static void SampleMotionCurvesData(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData, FVirtualBoneCurvesData& OutMotionCurvesData);

	/**
	 * Sample root motion data and convert it into curve data.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	OutMotionCurvesData		Motion curves data
	 */
	static void SampleRootMotionCurvesData(UAnimSequence* InAnimSequence, FVirtualBoneCurvesData& OutMotionCurvesData);

	/**
	 * Convert the data magnification of the root bone, if 0, cleanup will be performed, otherwise scaling will be performed.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionCaptureData		Motion capture config data
	 * @param	InConvertData		Root motion convert data
	 * @param	InMotionCurvesData		Motion capture curves data
	 * @param	InCurveAttributes		Skeleton curve attributes
	 */
	static void ConvertRootMotionData(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData
		, const FVirtualMotionConvertData& InConvertData, FVirtualBoneCurvesData& InMotionCurvesData, FVirtualCurveAttributes& InCurveAttributes);

	/**
	 * Align the XY of root motion with the pelvis so that they are always in line.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionAlignmentData		Motion capture alignment data
	 * @param	InCurveAttributes		Skeleton curve attributes
	 */
	static void AlignmentRootMotionData(UAnimSequence* InAnimSequence, const FVirtualMotionAlignmentData& InMotionAlignmentData, FVirtualCurveAttributes& InCurveAttributes);


	/**
	 * Align the XY of root motion with the pelvis so that they are always in line.
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionAlignmentData		Motion capture alignment data
	 * @param	OutMotionCurvesData		Output root motion curves data
	 */
	static void AlignmentRootMotionCurvesData(UAnimSequence* InAnimSequence, const FVirtualMotionAlignmentData& InMotionAlignmentData, FVirtualBoneCurvesData& OutMotionCurvesData);

	/**
	 * Convert motion capture animation asset to reference pose transform (Zeroing the initial frame transform).
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionCaptureRefData		Motion capture reference pose data
	 */
	static void ConvertMotionCaptureToReferencePose(UAnimSequence* InAnimSequence, const FVirtualMotionCaptureReferenceData& InMotionCaptureRefData);

	/**
	 * Convert root motion animation asset to motion capture (Transfer the root motion bone track data to motion bone).
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionCaptureData		Motion capture config data
	 */
	static void ConvertRootMotionToMotionCapture(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData, FVirtualBoneCurvesData& OutMotionCurvesData);

	/**
	 * Convert motion capture animation asset to root motion (Transfer the motion bone track data to root bone).
	 *
	 * @param	InAnimSequence		Animation asset
	 * @param	InMotionCaptureData		Motion capture config data
	 * @param	InMotionCurvesData		Motion capture curves data
	 * @param	InCurveAttributes		Skeleton curve attributes
	 */
	static void ConvertMotionCaptureToRootMotion(UAnimSequence* InAnimSequence, const FVirtualMotionSampleData& InMotionCaptureData, const FVirtualBoneCurvesData& InMotionCurvesData, FVirtualCurveAttributes& InCurveAttributes);

#pragma endregion


#pragma region Compression

	/** Apply Compression to list of animations and optionally asks to pick an overrides to the bone compression settings */
	static void OnApplyCompression(UAnimSequence* InAnimationAsset);

#pragma endregion


#pragma region LOD

	/**
	 * Automatically generate LODs and optimize the generation of morph LODs on the basis of the engine
	 *
	 * @param	InSkeletalMesh		Modifier skeletal mesh
	 * @param	InLODsData		Generate LODs data
	 */
	static void GenerateLODs(USkeletalMesh* InSkeletalMesh, FVirtualAssetGenerateLODsData& InLODsData);

	/**
	 * Automatically remove LODs on the basis of the engine
	 *
	 * @param	InSkeletalMesh		Modifier skeletal mesh
	 * @param	InRemoveGroups		Remove LODs group indices
	 */
	static void RemoveLODs(USkeletalMesh* InSkeletalMesh, const TArray<int32>& InRemoveGroups);

#pragma endregion


#pragma region Export

	/////////////////////////////////////////////////////////////////////////////////////
	// Experimental, just some experimental approach.
	/////////////////////////////////////////////////////////////////////////////////////

	static void ExportToFBX(const EVirtualExportSourceOption Option, UAnimSequence* InAnimationAsset, FVirtualAssetExportData& InAssetExportData);

	static bool OnExportToFBX(const TArray<UObject*> NewAssets, bool bRecordAnimation, FVirtualAssetExportData& InAssetExportData);

	static void CreateAnimationAssets(FVirtualAssetExportData& InAssetExportData, const TArray<TWeakObjectPtr<UObject>>& SkeletonsOrSkeletalMeshes, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix, UObject* NameBaseObject = nullptr, bool bDoNotShowNameDialog = false);

	static bool RecordMeshToAnimation(USkeletalMeshComponent* PreviewComponent, UAnimSequence* NewAsset);

	static void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName);

#pragma endregion

};
