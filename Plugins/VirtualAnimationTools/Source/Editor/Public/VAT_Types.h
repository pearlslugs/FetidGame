// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "UObject/NoExportTypes.h"
#include "Curves/RealCurve.h"
#include "Curves/CurveFloat.h"
#include "VAT_Types.generated.h"

#define VAT_MIN          -314159265.f        // max value
#define VAT_MAX          314159265.f        // max value

class UAnimSequence;

/** Types of virtual animation tools */
UENUM()
enum class EVirtualToolsType : uint8
{
	Bone,
	Curve,
	Notify,
	Montage,
	Asset,
	Mirror,
	Retarget,
	PoseSearch,
	GameFramework,
	MAX UMETA(Hidden)
};

/** Types of virtual bone tools type */
UENUM()
enum class EVirtualBoneToolsType : uint8
{
	AddBone,
	RemoveBone,
	FilterBone,
	AddBoneTrack,
	BlendBoneTrack,
	ModifyBoneTrack,
	RemoveBoneTrack,
	ReplaceAnimPose,
	LayeredBoneBlend,
	BakeBoneTransform,
	ConstraintBone,
	MAX UMETA(Hidden)
};

/** Types of virtual curve tools type */
UENUM()
enum class EVirtualCurveToolsType : uint8
{
	Motion,
	Bone,
	Copy,
	Transfer,
	Sort,
	Scale,
	Remove,
	Output,
	MAX UMETA(Hidden)
};

/** Types of virtual notify tools type */
UENUM()
enum class EVirtualNotifyToolsType : uint8
{
	AddNotifies,
	ModifyNotifies,
	RemoveNotifies,
	AddNotifiesTrack,
	ModifyNotifiesTrack,
	RemoveNotifiesTrack,
	FootStep,
	MAX UMETA(Hidden)
};

/** Types of virtual montage tools type */
UENUM()
enum class EVirtualMontageToolsType : uint8
{
	Modifier,
	Loop,
	Sort,
	Transfer,
	MAX UMETA(Hidden)
};

/** Types of virtual asset tools type */
UENUM()
enum class EVirtualAssetToolsType : uint8
{
	Crop,
	Insert,
	Resize,
	Replace,
	Composite,
	// Sample root bone animation dat.
	SampleRootMotion,
	// Resize root bone animation data, it includes transformation of MoCap data, etc.
	ResizeRootMotion,
	// Convert root bone animation data
	ConvertRootMotion,
	AlignmentRootMotion,
	// Convert motion capture asset to reference pose(Zeroing the initial frame transform)
	ConvertMotionToReferencePose,
	GenerateLOD,
	RemoveLOD,
	Export,
	MAX UMETA(Hidden)
};

/** Types of virtual mirror tools type */
UENUM()
enum class EVirtualMirrorToolsType : uint8
{
	Mirror,
	MirrorBoneTree,
	MAX UMETA(Hidden)
};

UENUM()
enum class EVirtualRetargetToolsType : uint8
{
	Pose,
	Animation,
	Rebuild,
	MAX UMETA(Hidden)
};

/** Tool type for game framework */
UENUM()
enum class EVirtualGameFrameworkToolsType : uint8
{
	FootIK,
	FootLock,
	FootOffset,
	FootWeight,
	FootPosition,
	PosesCurve,
	MAX UMETA(Hidden)
};

/** Types of rebuild animation asset type */
UENUM()
enum class EVirtualToolsRebuildAssetType : uint8
{
	// Overwrite the original asset and only process data modifications on the bones
	VTAT_Override		UMETA(DisplayName = "Override"),
	// Create a new asset without animation data
	VTAT_CreateAsset		UMETA(DisplayName = "CreateAsset"),
	// Duplicate a new asset, including the original animation data
	VTAT_DuplicateAsset		UMETA(DisplayName = "DuplicateAsset"),
};

/** Types of virtual curve filter tools */
UENUM()
enum class EVirtualCurveFilterType : uint8
{
	Bake								UMETA(DisplayName = "Bake"),
	Euler								UMETA(DisplayName = "Euler"),
	FourierTransform					UMETA(DisplayName = "FFT"),
	Reduce								UMETA(DisplayName = "Reduce"),
	Default								UMETA(DisplayName = "Default"),
	MAX UMETA(Hidden)
};

/** Struct of bone transform data */
USTRUCT()
struct FVirtualTransformData
{
	GENERATED_USTRUCT_BODY()

	/** We can't offset bones children bones, children bones are usually rotated to change position */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector Location;

	/** Editor instead of rotation for quaternions */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FRotator Rotation;

	/** Bone scale */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector Scale;

public:

	FTransform GetTransform() const { return FTransform(Rotation, Location, Scale); }

	FVirtualTransformData()
		: Location(ForceInitToZero)
		, Rotation(ForceInitToZero)
		, Scale(FVector::OneVector)
	{}
};

/** Struct of resize curve data */
USTRUCT(BlueprintType)
struct FVirtualResizeCurveData
{
	GENERATED_USTRUCT_BODY()

	/** Curve tag */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FName Tag;

	/** Resize curve time range, X = Min Time, Y = Max Time */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector2D TimeRange;

	/** Resize curve value range, X = Min Value, Y = Max Value */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector2D ValueRange;

	/** Runtime float curve */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FRuntimeFloatCurve FloatCurve;

public:

	/** Resize the curve data */
	void Resize(FVector2D* InTimeRange = nullptr, FVector2D* InValueRange = nullptr);

	/** Cleanup the invalid curve data */
	void Cleanup();

	FVirtualResizeCurveData()
		: TimeRange(FVector2D(-1.f))
		, ValueRange(FVector2D(FLT_MAX))
	{}
};

/** Struct of animation asset bone runtime float curve data */
USTRUCT()
struct FVirtualBoneCurvesData
{
	GENERATED_USTRUCT_BODY()

	/** Resize curves data of rotation, 0 = Roll, 1 = Pitch, 2 = Yaw */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Global Params")
	TArray<FVirtualResizeCurveData> RotationResizeCurvesData;

	/** Resize curves data of translation, 0 = X, 1 = Y, 2 = Z */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Global Params")
	TArray<FVirtualResizeCurveData> TranslationResizeCurvesData;

public:

	/** return the transform key of key time */
	FTransform Evaluate(const float& InKeyTime) const;

	/** return the transform key of key time */
	void Evaluate(const float& InKeyTime, FRotator& OutRotation, FVector& OutTranslation) const;

	/** Add the transform key to curve */
	void AddTransformKey(const float& InKeyTime, const FTransform& InTransform);
	
	/** Return has curve data */
	bool HasCurveData();

	/** Resize curves */
	void Resize(FVector2D* InTimeRange = nullptr, FVector2D* InValueRange = nullptr);

	/** Filter the transform curve */
	void Filter(const EVirtualCurveFilterType& InFilterType, const float InTolerance = 0.1f);

	/** Cleanup invalid curves */
	void Cleanup();

	/** Reset the runtime float curves data */
	void Reset();

	FVirtualBoneCurvesData()
	{
		RotationResizeCurvesData.Init(FVirtualResizeCurveData(), 3);
		RotationResizeCurvesData[0].Tag = "Roll";
		RotationResizeCurvesData[1].Tag = "Pitch";
		RotationResizeCurvesData[2].Tag = "Yaw";
		TranslationResizeCurvesData.Init(FVirtualResizeCurveData(), 3);
		TranslationResizeCurvesData[0].Tag = "X";
		TranslationResizeCurvesData[1].Tag = "Y";
		TranslationResizeCurvesData[2].Tag = "Z";
	}
};

/** Bake the curve runtime data of the foot position */
struct FVirtualLegPoseRuntimeData
{
	/** Flag the leg pose is in landed state */
	bool bIsLanded;

	/** Foot tip pose lower height */
	float TipLowerHeight;

	/** Foot tip pose time lower height */
	float TipLowerHeightPoseTime;

	/** Foot tip pose index lower height */
	int32 TipLowerHeightPoseIndex;

	/** Foot heel pose lower height */
	float HeelLowerHeight;

	/** Foot heel pose time lower height */
	float HeelLowerHeightPoseTime;

	/** Foot heel pose index lower height */
	int32 HeelLowerHeightPoseIndex;

	/** Last foot tip poses lower height */
	TArray<float> TipLowerHeights;

	/** Last foot heel poses lower height */
	TArray<float> HeelLowerHeights;

	/** Foot tip poses component space transform */
	TArray<FTransform> TipTransformCS;

	/** Foot heel poses component space transform */
	TArray<FTransform> HeelTransformCS;

	/** Foot tip idle pose transform */
	TOptional<FTransform> TipIdlePoseTransform;

	/** Foot heel idle pose transform */
	TOptional<FTransform> HeelIdlePoseTransform;

public:

	/** Initialize the leg runtime data, return the poses number */
	int32 Initialize(UAnimSequence* InAnimSequence, const float& InSampleRate, const FVirtualLegBaseData& InLegBaseData
	, bool InUseTipRelativeTM = true, FVirtualCurveSampleData* InCurveSampleData = nullptr);
	
	/** Return the pose min height in desired index */
	float GetMinPoseHeight(const int32& InPoseIndex);

	/** Return the pose max height in desired index */
	float GetMaxPoseHeight(const int32& InPoseIndex);

	/** Return the pose min value in desired index */
	float GetMinPoseValue(const int32& InPoseIndex);

	/** Return the pose max value in desired index */
	float GetMaxPoseValue(const int32& InPoseIndex);

	/** Return whether the leg of the current pose has landed */
	bool IsLandedPose(const int32& InPoseIndex, const float InTolerance = 0.f
	, const float InGroundedHeight = FLT_MIN, const float InHorizontalTolerance = FLT_MIN
	, const bool InCalculateMinVale = true);

	FVirtualLegPoseRuntimeData()
		: bIsLanded(false)
		, TipLowerHeight(0.f)
		, TipLowerHeightPoseTime(0.f)
		, TipLowerHeightPoseIndex(INDEX_NONE)
		, HeelLowerHeight(0.f)
		, HeelLowerHeightPoseTime(0.f)
		, HeelLowerHeightPoseIndex(INDEX_NONE)
	{}
};

/** Leg base bone data of animation skeleton */
USTRUCT()
struct FVirtualLegBaseData
{
	GENERATED_USTRUCT_BODY()

	/** Foot bone reference */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FBoneReference FootBone;

	/** Foot tip bone reference as ball */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FBoneReference TipBone;

	/** Foot tip socket translation offset */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector TipSocketLocation;

	/** Foot heel socket translation offset */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector HeelSocketLocation;

	/*
	 * X is enter ground value, Y is left ground value.
	 * If only one value, all foot bone use first index.
	*/
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FVector2D FootHeightToleranceRange;

	/** Calculated error value, which depends on whether the animation is aligned with the ground */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float Tolerance;

	/** If output the curve value it would be the upper bound of the curve value */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float CurveApexValue;

	// Look bone
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float NewKeyOffset;

	/** Smoothing mode for curves */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TEnumAsByte<ERichCurveInterpMode> InterpMode;

	/*
	 * If only one value, all notify position use first index.
	 * Can set every notify position offset.
	*/
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TArray<float> PositionOffset;

public:

	/** Return the data is valid */
	bool IsValid() const;

	FVirtualLegBaseData()
		: TipSocketLocation(ForceInitToZero)
		, HeelSocketLocation(ForceInitToZero)
		, FootHeightToleranceRange(FVector2D(0.f, 1.f))
		, Tolerance(1.f)
		, CurveApexValue(0.f)
		, NewKeyOffset(0.f)
		, InterpMode(ERichCurveInterpMode::RCIM_Cubic)
	{}
};

/** Sampled curve config data */
USTRUCT()
struct FVirtualCurveSampleData
{
	GENERATED_USTRUCT_BODY()

	/** The larger the frame rate, the more curve keys are generated, the higher the accuracy, and the higher the performance. */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	FFrameRate SampleRate;

	/** Output curve Interp mode */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TEnumAsByte<ERichCurveInterpMode> CurveInterpMode;

	/** Curve filter type */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	EVirtualCurveFilterType CurveFilterType;

	/** Curve filter tolerance value */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	float CurveFilterTolerance;

	/** The interval of baking frames, we can avoid errors caused by the animation itself being too close to the ground */
	UPROPERTY(EditAnywhere, Category = "Global Params", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Interval;

	/** Tolerance from minimum when entering or leaving the ground : Height */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float AirTolerance;

	/** Tolerance from minimum when entering or leaving the ground : Height  */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float LandedTolerance;

	/** Pose frame when Idle : Horizontal */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	int32 IdlePoseFrame;

	/** Tolerance from minimum when Idle : Horizontal */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float HorizontalTolerance;

	/** The height of the evaluation plane is usually 0, which is to avoid the error caused by walking through the ground */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float GroundedHeight;

	/** Pose offset time of the curve key */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	float OffsetTime;

	/** Pose offset frame of the curve key, time = frame * sample rate */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	int32 OffsetFrame;

	/** The height of the evaluation plane is usually 0, which is to avoid the error caused by walking through the ground */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TArray<float> GroundedHeights;

	/** Output curves value range, X is invalid value, Y is desired value
	  * if it is multiple data, we will correspond to the target array index one-to-one, otherwise the first data is used by default */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TArray<FVector2D> CurvesApexRanges;

	/** Output curve name and runtime curve, if it is multiple data, we will correspond to the target array index one-to-one, otherwise the first data is used by default */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TMap<FName, FRuntimeFloatCurve> RuntimeCurvesMap;

	/** Provides reference sampled bone height data */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TMap<FName, FRuntimeFloatCurve> TipBoneHeightsCurvesMap;

	/** Provides reference sampled bone height data */
	UPROPERTY(EditAnywhere, Category = "Global Params")
	TMap<FName, FRuntimeFloatCurve> HeelBoneHeightsCurvesMap;

	FVirtualCurveSampleData()
		: SampleRate(FFrameRate(60, 1))
		, CurveInterpMode(RCIM_Constant)
		, CurveFilterType(EVirtualCurveFilterType::Default)
		, CurveFilterTolerance(0.0001f)
		, Interval(0.05f)
		, AirTolerance(1.f)
		, LandedTolerance(1.f)
		, IdlePoseFrame(INDEX_NONE)
		, HorizontalTolerance(5.f)
		, GroundedHeight(VAT_MIN)
		, OffsetTime(0.f)
		, OffsetFrame(0)
	{}
};