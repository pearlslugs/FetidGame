// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_PoseSearch.generated.h"

class USkeleton;
class UAnimSequence;

/** Types of virtual pose search tools */
UENUM()
enum class EVirtualPoseSearchToolsType : uint8
{
	Distance,
	TimeSync,
	Animation,
	MAX UMETA(Hidden)
};

/** Struct of animation asset pose sample curve data */
USTRUCT()
struct FVirtualPoseSampleCurveData
{
	GENERATED_USTRUCT_BODY()

	/** If the curve name is valid, we will output the corresponding curve */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FName OutputCurveName;

	/** If the reference curve is valid, we will sample the current curve for reference */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FName ReferenceCurveName;

	/** If the reference curve is valid, we will sample the current curve for reference */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	float ReferenceCurveValue;

	/** Runtime preview of curve data */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FRuntimeFloatCurve OutputCurve;

public:

	/** Returns whether sampling poses are possible */
	bool CanSamplePose(UAnimSequence* InAnimSequence, const float& InSamplePoseTime, int32 InSampleToleranceFrame = 0, float InSampleRate = 0.33f) const;
	 
	FVirtualPoseSampleCurveData()
		: OutputCurveName(NAME_None)
		, ReferenceCurveName(NAME_None)
		, ReferenceCurveValue(0.f)
	{}
};

/** Struct of animation asset pose sample bone data */
USTRUCT()
struct FVirtualPoseSampleBoneData
{
	GENERATED_USTRUCT_BODY()

	/** Sample bone reference */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FBoneReference Bone;

	/** Sample reference bone reference, used for bone orientation comparisons */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FBoneReference ReferenceBone;

	/** If we want to compare the front and back, left and right, up and down of the bones, we need to fill in 1 */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FVector SampleBoneAxis;

	/** After sampling, the corresponding curve will be baked */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	TArray<FVirtualPoseSampleCurveData> SampleCurvesData;

	FVirtualPoseSampleBoneData()
		: SampleBoneAxis(FVector(0.f, 1.f, 1.f))
	{}
};

/** Struct of animation asset pose search sample data */
USTRUCT()
struct FVirtualPoseSearchSampleData
{
	GENERATED_USTRUCT_BODY()

	/** The larger the frame rate, the more curve keys are generated, the higher the accuracy, and the higher the performance. */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	FFrameRate SampleRate;

	/** Now you must configure the axis of motion for sampling Forward and backward movements use y, and left right uses X.. */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	TEnumAsByte<EAxis::Type> SampleAxis;

	/** The error accuracy of sampling. If it is 0, we will sample all data.
	  * If it is 1, we will offset 1 frame to the left, and the number of sampling frames */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams", meta = (UIMin = "-1", ClampMin = "-1"))
	int32 SampleToleranceFrame;

	/** Compare the difference based on the position of the bone to the root bone and output the compressed distance */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	bool bUseBonePositions;

	/** The motion speed of the bones will be compared, if it is a static curve, please use the speed of root motion to search */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	bool bUseBoneVelocities;

	/** Sample bones data */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	TArray<FVirtualPoseSampleBoneData> BonesData;

	FVirtualPoseSearchSampleData()
		: SampleRate(FFrameRate(60, 1))
		, SampleAxis(EAxis::Y)
		, SampleToleranceFrame(0)
		, bUseBonePositions(true)
		, bUseBoneVelocities(true)
	{}
};


/** Struct of animation asset pose search time sync data */
USTRUCT()
struct FVirtualPoseSearchTimeSyncData
{
	GENERATED_USTRUCT_BODY()

	/** Transfer pose search curves data */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	TArray<FName> CurvesName;

	/** If the animation sequence is not empty, we copy the curve to the target asset instead of the selected animation asset */
	UPROPERTY(EditAnywhere, Category = "PoseSearchParams")
	TArray<UAnimSequence*> FollowAssets;

	FVirtualPoseSearchTimeSyncData()
	{
		CurvesName.AddUnique(TEXT("FootPosition"));
		CurvesName.AddUnique(TEXT("Foot_L"));
		CurvesName.AddUnique(TEXT("Foot_R"));
		FollowAssets.AddDefaulted();
	}
};

/**
 * Current search mode is only root - foot distance
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_PoseSearch : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Get valid pose distance, value is not zero.
	*
	* @param InEvaluatePos		Evaluate animation pose time
	* @param InPoseSampleCurveData		Animation asset pose sample curve data
	* @return  Pose distance value
	*
	*/
	static float GetPoseDistance(const float& InEvaluatePos, const FVirtualPoseSampleCurveData& InPoseSampleCurveData);

	/**
	* Get valid pose distance, value is not zero.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InEvaluatePos		Evaluate animation pose time
	* @param InPoseSampleData		Animation asset pose sample data
	* @return  Pose distance value
	*
	*/
	static float GetPoseDistance(UAnimSequence* InAnimSequence, const float& InEvaluatePos, const FVirtualPoseSearchSampleData& InPoseSampleData);

	/**
	* Get can sample curve index in array
	*
	* @param InAnimSequence		Animation pose asset
	* @param InEvaluatePos		Evaluate animation pose time
	* @param InPoseSampleData		Animation asset pose sample data
	* @return  Sample bone and curve index
	*
	*/
	static FVector2D GetCanSampleIndex(UAnimSequence* InAnimSequence, const float& InEvaluatePos, const FVirtualPoseSearchSampleData& InPoseSampleData);

	/**
	* Sample specifies animation pose features is looping condition
	*
	* @param InAnimSequence		Animation pose asset
	* @param InTolerance		Tolerance value of the comparison posture is usually 0, it is inevitable that the head and tail postures are not exactly the same.
	* @return  Is loop animation asset.
	*
	*/
	static bool IsLoopAnimationAsset(UAnimSequence* InAnimSequence, const float InTolerance = 1.f);

	/**
	* Sample specifies animation pose features is same pose condition
	*
	* @param InEvaluatePos		Evaluate source animation pose time
	* @param InComparePos		Evaluate compare animation pose time
	* @param InAnimSequence		Animation pose asset
	* @param InCompareAnimSequence		Compare animation pose asset
	* @param InTolerance		Tolerance value of the comparison posture is usually 0, it is inevitable that the head and tail postures are not exactly the same.
	* @param OutTolerance		Return two poses tolerance value
	* @return  Is sample animation pose.
	*
	*/
	static bool IsSampePose(const float& InEvaluatePos, const float& InComparePos
		, UAnimSequence* InAnimSequence, UAnimSequence* InCompareAnimSequence
		, const float InTolerance = 1.f, float* OutTolerance = nullptr);

	/**
	* Sample specifies animation pose features
	*
	* @param InAnimSequence		Animation pose asset
	* @param InPoseSampleData		Sampled configuration data
	*
	*/
	template <typename DataType, typename CurveClass>
	static void SamplePoseFeatures(UAnimSequence* InAnimSequence, FVirtualPoseSearchSampleData& InPoseSampleData, bool InBuildCurve = true);

	/**
	* Sample specifies animation pose features
	*
	* @param InAnimSequence		Animation pose asset
	* @param InNextSequence		Next animation pose asset
	* @param InPoseSampleData		Sampled configuration data
	* @param OutPositionCurve		Output a curve graph of two animation pose jump position
	*
	*/
	static void SampleTwoPoseFeatures(UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence, FVirtualPoseSearchSampleData& InPoseSampleData, FRuntimeFloatCurve& OutPositionCurve);

	/**
	* Sample specifies animation pose time sync features
	*
	* @param InAnimSequence		Animation pose asset
	* @param InPoseSampleData		Sampled configuration data
	*
	*/
	template <typename DataType, typename CurveClass>
	static void SamplePoseTimeSyncFeatures(UAnimSequence* InAnimSequence, FVirtualPoseSearchSampleData& InPoseSampleData, FVirtualPoseSearchTimeSyncData& InPoseTimeSyncData, bool InBuildCurve = true);

	/////////////////////////////////////////////////////////////////////////////////////
	// BEGIN DEPRECATED, just some experimental approach.
	/////////////////////////////////////////////////////////////////////////////////////

public:

#if 0
	UFUNCTION(BlueprintCallable)
	static float PoseBakedMatching(const TArray<FName>& InCurvesName, UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence);

	UFUNCTION(BlueprintCallable)
	static float PoseMergeMatching(const float InDesiredPos, const FName& InCurveName, UAnimSequence* InAnimSequence, UAnimSequence* InNextSequence);

	UFUNCTION(BlueprintCallable)
	static void PoseBaked(UPARAM(ref) FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InAnimSequence, bool InBuildCurve = true);
	
	UFUNCTION(BlueprintCallable)
	static void PoseMerge(UPARAM(ref) FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InAnimSequence, bool InBuildCurve = true);

	static void FeetPoseSearch(FVirtualPoseSearchSampleData& InPoseSampleData, FVirtualSmartFeetLandNotifyData& InFeetData, UAnimSequence* InAnimSequence, bool InBuildCurve = true);

	UFUNCTION(BlueprintCallable)
	static void PoseSearch(UPARAM(ref) FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InCurrentSequence, UAnimSequence* InNextSequence, bool InBuildCurve = true);

	UFUNCTION(BlueprintCallable)
	static float PoseSimpleSearch(UPARAM(ref) FVirtualPoseSearchSampleData& InPoseSampleData, UAnimSequence* InCurrentSequence, UAnimSequence* InNextSequence, float InCurrentPos);
#endif

};

