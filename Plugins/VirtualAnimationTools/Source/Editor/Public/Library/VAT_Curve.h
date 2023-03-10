// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimCurveTypes.h"
#include "VAT_Types.h"
#include "VAT_Curve.generated.h"

class USkeleton;
class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;
class UCurveFloat;
class UCurveVector;
struct FRuntimeFloatCurve;

/** Types of virtual curve tools */
UENUM()
enum class EVirtualCurveType : uint8
{
	Speed,
	SpeedAlpha,
	AnimPosition,
	ForwardMovement,
	ForwardMovementAlpha,
	RightMovement,
	RightMovementAlpha,
	UpMovement,
	UpMovementAlpha,
	TranslationAlpha,
	PitchRotation,
	PitchRotationAlpha,
	YawRotation,
	YawRotationAlpha,
	RollRotation,
	RollRotationAlpha,
	RotationAlpha,
	MAX UMETA(Hidden)
};

/** Types of virtual curve absolute */
UENUM()
enum class EVirtualCurveAbsoluteType : uint8
{
	Absolute,
	AbsoluteInFirst,
	AbsoluteInLast,
	MAX UMETA(Hidden)
};


/** Struct of animation asset curve preset data */
USTRUCT()
struct FVirtualCurvePresetData
{
	GENERATED_USTRUCT_BODY()

	/** Tag of the curve preset */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	FString Tag;

	/** Names of animation asset custom curve */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<FName> CurvesName;

	/** Types of animation asset motion curve */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<EVirtualCurveType> CurvesType;

	/** Types of animation asset motion curve */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<EVirtualCurveFilterType> CurvesFilterType;

	FVirtualCurvePresetData()
		: Tag(FString())
	{}
};

/** Struct of animation asset bone curve data */
USTRUCT()
struct FVirtualBoneCurveData
{
	GENERATED_USTRUCT_BODY()

	/** Bone reference */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	FBoneReference Bone;

	/** Type of Sample bone control space. */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TEnumAsByte<EBoneControlSpace> BoneControlSpace;

	/** Output curve name */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	FName OutputName;

	FVirtualBoneCurveData()
		: BoneControlSpace(EBoneControlSpace::BCS_ComponentSpace)
		, OutputName(NAME_None)
	{}
};

/** Struct of animation asset curve attributes data */
USTRUCT()
struct FVirtualCurveAttributes
{
	GENERATED_USTRUCT_BODY()

	/** If it is a motion loop animation asset, we will copy the last frame of the speed curve to the first frame, otherwise the data of the second frame will be taken by default */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	uint8 bIsLoopMotionAsset : 1;

	/** If it is true, we will clean up the curve. No matter whether the new data is valid or not, the default is to overwrite the old data after detecting the new data, rather than delete the data */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	uint8 bAlwaysClearCurves : 1;

	/** Whether to correct the alpha curve to a positive value */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	EVirtualCurveAbsoluteType CurveAbsType;

	/** Flag sample accumulate Curves */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	uint8 bSampleAccumulateCurve : 1;

	/** Flag sample accumulate line curves, we allow rotation > 180 angle */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	uint8 bSampleAccumulateLineCurve : 1;

	/** Keep the final decimal place to what we expect, if it is < 0, it will not be optimized, if it is 0, then keep it to an integer */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	int32 DecimalPlaces;

	/** Curve filter tolerance value */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	float FilterTolerance;

	/** Curve filter tolerance value */
	UPROPERTY(EditAnywhere, Category = "Curve Params", meta = (EditCondition = "bIsLoopMotionAsset"))
	float LoopMotionAssetTolerance;

	/** Animation curves name reference map */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Curve Params")
	TMap<EVirtualCurveType, FName> AnimCurvesNameMap;

	/** Animation curves color reference map */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Curve Params")
	TMap<EVirtualCurveType, FColor> AnimCurvesColorMap;

	/** Animation curves filter reference map */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Curve Params")
	TMap<EVirtualCurveType, EVirtualCurveFilterType> AnimCurvesFilterMap;

public:

	/** Rebuild curve filter map from curve preset data */
	void RebuildCurveFilterMap(const FVirtualCurvePresetData& InPresetData);

	FVirtualCurveAttributes()
		: bIsLoopMotionAsset(false)
		, bAlwaysClearCurves(true)
		, CurveAbsType(EVirtualCurveAbsoluteType::Absolute)
		, bSampleAccumulateCurve(false)
		, bSampleAccumulateLineCurve(false)
		, DecimalPlaces(3)
		, FilterTolerance(0.0001f)
		, LoopMotionAssetTolerance(1.f)
	{
		for (int32 i = 0; i < int32(EVirtualCurveType::MAX); i++)
		{
			UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EVirtualCurveType"), true);
			FName EnumName(*EnumPtr->GetNameStringByIndex(i));
			AnimCurvesNameMap.FindOrAdd(EVirtualCurveType(i), EnumName);
			AnimCurvesColorMap.FindOrAdd(EVirtualCurveType(i), FColor::Transparent);
			if (false && i != int32(EVirtualCurveType::AnimPosition))
			{
				AnimCurvesFilterMap.FindOrAdd(EVirtualCurveType(i), EVirtualCurveFilterType::Reduce);
			}
			else
			{
				AnimCurvesFilterMap.FindOrAdd(EVirtualCurveType(i), EVirtualCurveFilterType::Reduce);
			}
		}
	}
};

/** Struct of copy curve data */
USTRUCT()
struct FVirtualCopyCurveData
{
	GENERATED_USTRUCT_BODY()

	/** Flag remove source curve data */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	uint8 bRemoveSourceData : 1;

	/** Source curve name */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	FName SourceName;

	/** Output curve name */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	FName OutputName;

	/** If the animation sequence is not empty, we copy the curve to the target asset instead of the selected animation asset */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<UAnimSequence*> Assets;

public:

	/** Return the data is valid */
	bool IsValid() const { return SourceName != NAME_None && OutputName != NAME_None; }

	FVirtualCopyCurveData()
		: bRemoveSourceData(true)
		, SourceName(NAME_None)
		, OutputName(NAME_None)
	{}
};

/** Struct of transfer curve data */
USTRUCT()
struct FVirtualTransferCurveData
{
	GENERATED_USTRUCT_BODY()

	/** If the curves name is invalid, we will transfer all curves data */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<FName> CurvesName;

	/** If the animation sequence is not empty, we copy the curve to the target asset instead of the selected animation asset */
	UPROPERTY(EditAnywhere, Category = "Curve Params")
	TArray<UAnimSequence*> Assets;

	FVirtualTransferCurveData()
	{
		Assets.AddDefaulted();
	}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Curve : public UObject
{
	GENERATED_BODY()

public:

	/** Get the frame number for the provided time */
	static int32 GetFrameAtTime(UAnimSequence* InAnimSequence, const float Time);

	/** Get the time at the given frame */
	static float GetTimeAtFrame(UAnimSequence* InAnimSequence, const int32 Frame);

	/** Return the curve smart name from animation sequence */
	static FSmartName GetCurveSmartName(UAnimSequence* InAnimSequence, const FName& InCurveName);

	/** Return desired curve class */
	template <typename DataType, typename CurveClass>
	static CurveClass* GetCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName, const ERawCurveTrackTypes& InCurveType);

	/** Return float curve class */
	template <typename DataType, typename CurveClass>
	static FFloatCurve* GetFloatCurveClass(UAnimSequence* InAnimSequence, const FName& InCurveName);

public:

#pragma region Curve 

	// Returns whether the curve type is alpha curve
	static bool IsAlphaType(const EVirtualCurveType& InVirtualCurveType);

	// Return the curve value from the input transform according to the curve type
	static float GetCurveValueFromType(const EVirtualCurveType& InCurveType, const FTransform& InRootMotionTransform, float InDeltaTime = 0.1667f);

	// Returns the type of curve that exists
	static void GetAnimationCurvesType(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, TArray<EVirtualCurveType>& OutCurvesType);
	
#pragma endregion


#pragma region Sample Curves

	/**
	* Sample specifies animation asset bone curve data
	*
	* @param InAnimSequence		Animation pose asset
	* @param InBoneCurveData		Bone curve data
	* @param InCurveAttributes		Sample curves attributes
	*
	*/
	static void SampleBoneCurve(UAnimSequence* InAnimSequence, const FVirtualBoneCurveData& InBoneCurveData, const FVirtualCurveAttributes& InCurveAttributes);

	/**
	* Sample specifies animation asset cached curves
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveName		Sample curve name
	* @param OutAnimCurves		Return the curves asset that needs to be output
	*
	*/
	static void SampleAnimationAssetCurves(UAnimSequence* InAnimSequence, const FName& InCurveName, TArray<FRuntimeFloatCurve>& OutAnimCurves);

	/**
	* Sample specifies animation asset root motion curves
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveAttributes		Sample curves attributes
	* @param OutAnimCurvesMap		Return the curve asset that needs to be output
	*
	*/
	static void SampleRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, TMap<EVirtualCurveType, FRuntimeFloatCurve>& OutAnimCurvesMap);

	/**
	* Sample specifies animation asset root motion curves to animation sequence.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveAttributes		Sample curves attributes
	* @param InAnimCurvesMap		Sample animation asset curves data
	*
	*/
	static void SampleRootMotionCurvesToAnimSequence(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, const TMap<EVirtualCurveType, FRuntimeFloatCurve>& InAnimCurvesMap);

	/**
	* Sample specifies animation asset root motion curves to CurveFloat asset.
	*
	* @param InCurvesFloat		Curves float asset
	* @param InCurveAttributes		Sample curves attributes
	* @param InAnimCurves		Sample animation asset curves data
	*
	*/
	static void SampleRootMotionCurvesToCurvesFloat(TArray<UCurveFloat*> InCurvesFloat, const FVirtualCurveAttributes& InCurveAttributes, const TArray<FRuntimeFloatCurve>& InAnimCurvesMap);

	/**
	* Sample specifies animation asset root motion curves to CurveVector asset.
	* Every 3 curves is stored in a corresponding Curve Vector asset for an iteration number
	*
	* @param InCurvesVector		Curves vector asset
	* @param InCurveAttributes		Sample curves attributes
	* @param InAnimCurves		Sample animation asset curves data
	*
	*/
	static void SampleRootMotionCurvesToCurvesVector(TArray<UCurveVector*> InCurvesVector, const FVirtualCurveAttributes& InCurveAttributes, const TArray<FRuntimeFloatCurve>& InAnimCurves);

public:

	/**
	* Rebuild specifies animation asset root motion curves to animation sequence.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveAttributes		Sample curves attributes
=	*
	*/
	static void RebuildAnimationRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, bool InAccumulateCurve = true);

	/**
	* Sample specifies animation asset root motion curves to animation sequence.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveAttributes		Sample curves attributes
	* @param InAnimCurvesType		Sample animation asset curves type
	*
	*/
	static void SampleAnimationRootMotionCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes, const TArray<EVirtualCurveType>& InAnimCurvesType);

#pragma endregion


#pragma region Curve Event

	/**
	* Copy source curve data to desired curve.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurvesData		Copy curves data
	*
	*/
	static void CopyAnimationCurvesData(UAnimSequence* InAnimSequence, const TArray<FVirtualCopyCurveData>& InCurvesData);

	/**
	* Transfer source curve data to desired curve.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InTransferCurveData		Transfer curves data
	*
	*/
	static void TransferAnimationCurvesData(UAnimSequence* InAnimSequence, FVirtualTransferCurveData& InTransferCurveData);

	/**
	* Sort the curves in the animation assets according to the configured curve names.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurveAttributes		Sample curves attributes
	*
	*/
	static void SortAnimationAssetCurves(UAnimSequence* InAnimSequence, const FVirtualCurveAttributes& InCurveAttributes);

	/**
	* Delete the specified curves data.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurvesName		Remove curves name
	*
	*/
	static void RemoveAnimationAssetCurve(UAnimSequence* InAnimSequence, const TArray<FName>& InCurvesName);

	/**
	* Clear all curves data.
	*
	* @param InAnimSequence		Animation pose asset
	*
	*/
	static void ClearAnimationAssetCurve(UAnimSequence* InAnimSequence);

	/**
	* Resize the specified curves data.
	*
	* @param InAnimSequence		Animation pose asset
	* @param InCurvesName		Remove curves name
	*
	*/
	static void ResizeAnimationAssetCurves(UAnimSequence* InAnimSequence);

#pragma endregion


#pragma region Curve Filter

	/** Apply curve filter */
	static void ApplyCurveFilter(FRuntimeFloatCurve& InFloatCurve, const EVirtualCurveFilterType& InFilterType, const float InTolerance = 0.1f);

	/** Apply Euler curve filter */
	static void ApplyCurveEulerFilter(FRuntimeFloatCurve& InFloatCurve, const float InTolerance = 0.1f);

	/** Apply Simplify curve filter */
	static void ApplyCurveReduceFilter(FRuntimeFloatCurve& InFloatCurve, const float InTolerance = 0.1f);

#pragma endregion


	/**
	 * Convert a animation curve to specified runtime curve.
	 * @param	InAnimSequence	The specified animation
	 * @param	InCurve			The specified runtime float curve
	 * @param	InCurveName			The specified animation curve name
	 * @return  The curve data valid
	 */
	UFUNCTION(BlueprintCallable, Category = "VAT| Curve", meta = (WorldContext = "InWorldContextObject", DisplayName = "K2_ResizeRuntimeCurve"))
	static void K2_ResizeRuntimeCurve(UObject* InWorldContextObject, UPARAM(ref) FVirtualResizeCurveData& InCurve);


#pragma region Convert

	/**
	 * Convert a animation curve to specified runtime curve.
	 * @param	InAnimSequence	The specified animation
	 * @param	InCurve			The specified runtime float curve
	 * @param	InCurveName			The specified animation curve name
	 * @return  The curve data valid
	 */
	UFUNCTION(BlueprintCallable, Category = "VMM| Curve", meta = (WorldContext = "InWorldContextObject", DisplayName = "Convert Anim Curve To Runtime Curve"))
	static bool K2_ConvertAnimCurveToRuntimeCurve(UObject* InWorldContextObject, UAnimSequence* InAnimSequence, UPARAM(ref) FRuntimeFloatCurve& InCurve, FName InCurveName);
	static bool ConvertAnimCurveToRuntimeCurve(UAnimSequence* InAnimSequence, FRuntimeFloatCurve& InCurve, const FName& InCurveName);

	/**
	 * Convert a runtime float curve to specified animation curve.
	 * @param	InAnimSequence	The specified animation
	 * @param	InCurve			The specified runtime float curve
	 * @param	InCurveName			The specified animation curve name
	 * @return  The curve data valid
	 */
	UFUNCTION(BlueprintCallable, Category = "VMM| Curve", meta = (WorldContext = "InWorldContextObject", DisplayName = "Convert Runtime Curve To Anim Curve"))
	static bool K2_ConvertRuntimeCurveToAnimCurve(UObject* InWorldContextObject, UAnimSequence* InAnimSequence, const FRuntimeFloatCurve& InCurve
			, FName InCurveName, TEnumAsByte<ERichCurveInterpMode> InCurveInterpMode = ERichCurveInterpMode::RCIM_Linear);
	template <typename DataType, typename CurveClass>
	static bool ConvertRuntimeCurveToAnimCurve(UAnimSequence* InAnimSequence, const FRuntimeFloatCurve& InCurve
		, const FName& InCurveName, const TEnumAsByte<ERichCurveInterpMode> InCurveInterpMode = ERichCurveInterpMode::RCIM_None);

#pragma endregion
};
