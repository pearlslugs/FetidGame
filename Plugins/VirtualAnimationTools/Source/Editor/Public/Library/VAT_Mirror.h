// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Mirror.generated.h"

/** Struct of animation asset mirror bone data */
USTRUCT(BlueprintType)
struct FVirtualMirrorBoneData
{
	GENERATED_BODY()

	/** Flag whether to enable the mirroring option for this bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params")
	bool bEnable;

	/** Whether the flag is a twin bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	bool bIsTwinBone;

	/** Mirror bone reference */
	UPROPERTY(EditAnywhere, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	FBoneReference Bone;

	/** Mirror twin bone reference */
	UPROPERTY(EditAnywhere, Category = "Mirror Params", meta = (editcondition = "bEnable && bIsTwinBone"))
	FBoneReference TwinBone;

	/** Mirror bone axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	TEnumAsByte<EAxis::Type> MirrorAxis;

	/** Mirror bone flip axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	TEnumAsByte<EAxis::Type> FlipAxis;

	/** Flag whether to mirror translation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	bool bMirrorTranslation;

	/** Mirror bone rotation offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Params", meta = (editcondition = "bEnable"))
	FRotator RotationOffset;

	FVirtualMirrorBoneData()
		: bEnable(true)
		, bIsTwinBone(false)
		, MirrorAxis(EAxis::X)
		, FlipAxis(EAxis::None)
		, bMirrorTranslation(false)
		, RotationOffset(FRotator().ZeroRotator)
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Mirror : public UObject
{
	GENERATED_BODY()

public:

	/**
	*  Sampling bone mirror data and bake to specified animation asset
	*
	* @param InAnimSequence		Animation asset
	* @param InMirrorBonesData		Animation skeleton mirror bones data
	*
	*/
	static void SamplingMirrorBonesData(UAnimSequence* InAnimSequence, const TArray<FVirtualMirrorBoneData>& InMirrorBonesData);

	/**
	*  Sampling component space bone mirror data and bake to specified animation asset
	*
	* @param InAnimSequence		Animation asset
	* @param InMirrorBonesData		Animation skeleton mirror bones data
	*
	*/
	static void SamplingMirrorCSBonesData(UAnimSequence* InAnimSequence, const TArray<FVirtualMirrorBoneData>& InMirrorBonesData);

	/**
	*  Sampling bone mirror data for specified animation asset
	*
	* @param InAnimSequence		Animation asset
	* @param InMirrorBoneData		Animation skeleton mirror bone data
	* @param InSourceRawTrack		Animation asset source raw track data
	* @param InMirrorRawTrack		Animation asset mirror raw track data
	*
	*/
	static void SamplingMirrorBoneTransform(UAnimSequence* InAnimSequence, const FVirtualMirrorBoneData& InMirrorBoneData, const FRawAnimSequenceTrack& InSourceRawTrack, FRawAnimSequenceTrack& InMirrorRawTrack);


	/**
	*  Sampling the mirror bone tree based on the same prefix or suffix name
	*
	* @param InSkeleton		Animation skeleton
	* @param InMirrorAxis		Global mirror axis
	* @param InTwinNamesMap		Sample suffix string as map
	* @param InMirrorBonesData		Sample bone tree data
	*
	*/
	static void SamplingMirrorBoneTreeAsName(USkeleton* InSkeleton, const EAxis::Type& InMirrorAxis
		, const TMap<FName, FName> InTwinNamesMap, TArray<FVirtualMirrorBoneData>& InMirrorBonesData);

	/**
	*  Sampling a mirrored bone tree, will automatically compare the same bone names
	*
	* @param InSkeleton		Animation skeleton
	* @param InMirrorAxis		Global mirror axis
	* @param InSampleLength		Sample the same string length, if it is invalid, we will use suffix model
	* @param InMirrorBonesData		Sample bone tree data
	* @param InSuffixL		Sample suffix string as left
	* @param InSuffixR		Sample suffix string as right
	*
	*/
	static void SamplingMirrorBoneTreeAsLength(USkeleton* InSkeleton, const EAxis::Type& InMirrorAxis, const int32& InSampleLength, TArray<FVirtualMirrorBoneData>& InMirrorBonesData);
};
