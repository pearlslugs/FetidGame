// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Types.h"
#include "AlphaBlend.h"
#include "VAT_GameFramework.generated.h"

class UAnimSequence;

/** Struct of animation asset foot weight data */
USTRUCT()
struct FVirtualFootWeightData
{
	GENERATED_USTRUCT_BODY()

	/** If true, we will exchange foot position */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bInverseFootPosition : 1;

	/** Compare first pose motion distance frame, it can avoid the error of the first frame judgment */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params", meta = (UIMin = "0", ClampMin = "0"))
	int32 CompareFrame;

	/** Compare poses tolerance */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	float CompareTolerance;

	/** Apex weight offset range pose time */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FVector2D OffsetRange;

	/** Foot weight curve sample data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FVirtualCurveSampleData CurveSampleData;

	/** Sampled animation poses, we will evaluate and find the most similar poses to sample as Apex */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<UAnimSequence*> ComparePosesAsset;

	/** Sampled animation poses time */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<float> ComparePosesTime;

public:

	/** Return the compare pose assets is valid */
	bool IsValidComparePosesAsset() const;

	FVirtualFootWeightData()
		: bInverseFootPosition(false)
		, CompareFrame(10)
		, CompareTolerance(10.f)
		, OffsetRange(FVector2D(-0.1f, 0.1f))
		, CurveSampleData(FVirtualCurveSampleData())
	{
		ComparePosesAsset.AddDefaulted(2);
	}
};

/** Struct of animation asset foot lock data */
USTRUCT()
struct FVirtualFootLockSampleData
{
	GENERATED_USTRUCT_BODY()

	/** If true, we always set it to the apex value once the value is valid */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bMakeFullWeight : 1;

	/** If it true, we should avoid curve inversion */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bAvoidInverseCurve : 1;

	/** If it true, We calculate landed min value in tip and heel */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bUseMinValueInLanded : 1;

	/** Blend in locking curves */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FAlphaBlend BlendIn;

	/** Blend in offset time, two key time blend ratio, or offset blend length */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float BlendInOffsetRatio;

	/** Blend out locked curves */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FAlphaBlend BlendOut;

	/** Blend out offset time, two key time blend ratio, or offset blend length */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float BlendOutOffsetRatio;

	/** If it true, We will unlock the animation without any root motion, if there is no motion data, we will keep the last clip is blended out */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bUnLockInNoneMotion : 1;

	/** None motion blend out locking curves */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params", meta = (EditCondition = "bUnLockInNoneMotion"))
	FAlphaBlend NoneMotionBlendOut;

	/** None motion blend out offset time, two key time blend ratio, or offset blend length */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params", meta = (EditCondition = "bUnLockInNoneMotion", UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float NoneMotionBlendOutOffsetRatio;

	/** Foot weight curve sample data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FVirtualCurveSampleData CurveSampleData;

public:

	FVirtualFootLockSampleData()
		: bMakeFullWeight(true)
		, bAvoidInverseCurve(true)
		, bUseMinValueInLanded(false)
		, BlendIn(FAlphaBlend(0.2f))
		, BlendInOffsetRatio(1.f)
		, BlendOut(FAlphaBlend(0.2f))
		, BlendOutOffsetRatio(1.f)
		, bUnLockInNoneMotion(true)
		, NoneMotionBlendOut(FAlphaBlend(0.2f))
		, NoneMotionBlendOutOffsetRatio(0.f)
		, CurveSampleData(FVirtualCurveSampleData())
	{
		BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		BlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		NoneMotionBlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
	}
};

/** Struct of animation asset foot offset data */
USTRUCT()
struct FVirtualFootOffsetData
{
	GENERATED_USTRUCT_BODY()

	/** If it is true, when the footstep is displaced, it is considered to be offset, otherwise it is only considered to be offset when it is off the ground.*/
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bUseMaxValue : 1;

	/** If is true, we will convert the offset alpha curve */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bMakeAlphaCurve : 1;

	/** If is true, we will output apex value */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bMakeFullWeight : 1;

	/** If it true, we should avoid curve inversion */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bAvoidInverseCurve : 1;

	/** If is true, we will use tip relative transform data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bUseTipRelativeTransform : 1;

	/** If is true, we will output children curve data to animation */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bOutputChildCurve : 1;

	/** If is true, we will output merger curve data to animation */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bOutputMergeCurve : 1;

	/** Output merge all sample curve data to desired curve */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FName OutputCurveName;

	/** Output runtime curve data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FRuntimeFloatCurve OutputCurveData;

	/** Foot weight curve sample data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FVirtualCurveSampleData CurveSampleData;

public:

	FVirtualFootOffsetData()
		: bUseMaxValue(false)
		, bMakeAlphaCurve(true)
		, bMakeFullWeight(false)
		, bAvoidInverseCurve(true)
		, bUseTipRelativeTransform(false)
		, bOutputChildCurve(true)
		, bOutputMergeCurve(false)
		, OutputCurveName("FootOffset")
		, CurveSampleData(FVirtualCurveSampleData())
	{}
};

/** Struct of animation asset merge aim offset data */
USTRUCT()
struct FVirtualMergeAimOffsetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	class UBlendSpace* AimOffsetBlendSpace;

	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<float> SampleValues;

	/** Only Y */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<float> OutputSampleValues;

// 	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
// 	float PoseInterval;

	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	float OutputLength;

	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FName OutputAssetName;

	FVirtualMergeAimOffsetData()
		: AimOffsetBlendSpace(nullptr)
		, OutputLength(1.f)
		, OutputAssetName(NAME_None)
	{}
};

/** Struct of animation pose asset data */
USTRUCT()
struct FVirtualPoseAssetData
{
	GENERATED_USTRUCT_BODY()

	/** Animation pose asset. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	UAnimSequence* PoseAsset;

	/** Animation pose evaluate position. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	float EvaluatePos;

	/** Animation pose evaluate frame, is the frame is valid, we always use it. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	int32 EvaluateFrame;

	/** Animation output curve value. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	float OutputCurveValue;

public:

	FVirtualPoseAssetData(float InCurveValue)
		: PoseAsset(nullptr)
		, EvaluatePos(0.f)
		, EvaluateFrame(INDEX_NONE)
		, OutputCurveValue(InCurveValue)
	{}

	FVirtualPoseAssetData()
		: PoseAsset(nullptr)
		, EvaluatePos(0.f)
		, EvaluateFrame(INDEX_NONE)
		, OutputCurveValue(0.f)
	{}
};

/** Struct of animation poses compare data */
USTRUCT()
struct FVirtualPosesCompareData
{
	GENERATED_USTRUCT_BODY()

	/** Compare poses tolerance */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	float CompareTolerance;

	/** Compare blend poses bone reference */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<FBoneReference> BlendBones;
	
	/** Animation poses output curve value data. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	TArray<FVirtualPoseAssetData> PosesAssetData;

	/** If is true, we always output Lower - Higher curve */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	uint8 bOutputUnityValue : 1;

	/** Animation poses output curve name. */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FName CurveName;

	/** Output runtime curve data */
	UPROPERTY(EditAnywhere, Category = "GameFramework Params")
	FRuntimeFloatCurve CurveData;

public:

	FVirtualPosesCompareData()
		: CompareTolerance(10.f)
		, bOutputUnityValue(true)
		, CurveName(NAME_None)
	{
		PosesAssetData.Add(FVirtualPoseAssetData(0.f));
		PosesAssetData.Add(FVirtualPoseAssetData(1.f));
	}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_GameFramework : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Lock range of the sampled footsteps is automatically generated as a specific curve
	*
	* @param InAnimSequence		Animation pose asset
	* @param InFootLockData		Foot lock sample data
	* @param InLegsBaseData		Skeleton legs bone data
	*
	*/
	static void SamplingFootLock(UAnimSequence* InAnimSequence, FVirtualFootLockSampleData& InFootLockData, const TArray<FVirtualLegBaseData>& InLegsBaseData);

	/**
	* Weight range of the sampled footsteps is automatically generated as a specific curve
	*
	* @param InAnimSequence		Animation pose asset
	* @param InFootWeightData		Foot weight sample data
	* @param InPoseSampleData		Pose sample data
	*
	*/
	static void SamplingFootWeight(UAnimSequence* InAnimSequence
		, FVirtualFootWeightData& InFootWeightData, const FVirtualPoseSearchSampleData& InPoseSampleData);

	/**
	* Landed range of the sampled footsteps is automatically generated as a specific curve
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveSampleData		Curve sample config data
	* @param InLegsBaseData		Skeleton legs bone data
	*
	*/
	static void SamplingFootLanded(UAnimSequence* InAnimSequence, FVirtualCurveSampleData& InCurveSampleData, const TArray<FVirtualLegBaseData>& InLegsBaseData);

	/**
	* Position range of the sampled footsteps is automatically generated as a specific curve
	*
	* @param InAnimSequence		Animation pose asset
	* @param InFootWeightData		Foot weight sample data
	* @param InLegsBaseData		Skeleton legs bone data
	*
	*/
	static void SamplingFootPosition(UAnimSequence* InAnimSequence, FVirtualFootWeightData& InFootWeightData, const TArray<FVirtualLegBaseData>& InLegsBaseData);

	/**
	* Offset range of the sampled footsteps is automatically generated as a specific curve
	*
	* @param InAnimSequence		Animation pose asset
	* @param InFootOffsetData		Foot offset sample data
	* @param InLegsBaseData		Skeleton legs bone data
	*
	*/
	static void ExportFootOffset(const int32& InIndex, UAnimSequence* InAnimSequence, FVirtualFootOffsetData& InFootOffsetData, const FVirtualLegBaseData& InLegBaseData);
	static void SamplingFootOffset(UAnimSequence* InAnimSequence, FVirtualFootOffsetData& InFootOffsetData, const TArray<FVirtualLegBaseData>& InLegsBaseData);

	/**
	* Output blend curves for multiple poses
	*
	* @param InAnimSequence		Animation pose asset
	* @param InPoseCompareData		Animation poses blend curve data
	*
	*/
	static void SamplingPosesBlendData(UAnimSequence* InAnimSequence, FVirtualPosesCompareData& InPoseCompareData);


	/////////////////////////////////////////////////////////////////////////////////////
	// BEGIN DEPRECATED, just some experimental approach.
	/////////////////////////////////////////////////////////////////////////////////////

public:

#if 0
	template <typename DataType, typename CurveClass>
	static void BakeFootLockCurve(UAnimSequence* InAnimSequence, FVirtualSmartFeetLandNotifyData& InFeetData, FVirtualCurveAttributes& InCurveAttributes);

	template <typename DataType, typename CurveClass>
	static void BakeFootIKCurve(UAnimSequence* InAnimSequence, FVirtualSmartFeetLandNotifyData& InFeetData, FVirtualCurveAttributes& InCurveAttributes);

	static void MergeAimOffset(const FVirtualMergeAimOffsetData& InMergeAimOffsetData);
#endif

};
