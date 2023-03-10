// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Bone.h"
#include "VAT_Retarget.generated.h"

class USkeleton;
class UAnimSequenceBase;
struct FBoneReference;

USTRUCT()
struct FVirtualRetargetAnimationsData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Retarget Params")
	USkeleton* TargetSkeleton;

	UPROPERTY(EditAnywhere, Category = "Retarget Params")
	TMap<FName, FBoneReference> ContraintBonesMap;

	UPROPERTY(EditAnywhere, Category = "Retarget Params")
	bool bRemapReferencedAssets;

	UPROPERTY(EditAnywhere, Category = "Retarget Params")
	bool bAllowRemapToExisting;

	UPROPERTY(EditAnywhere, Category = "Retarget Params")
	bool bConvertSpaces;

	FVirtualRetargetAnimationsData()
		: TargetSkeleton(nullptr)
		, bRemapReferencedAssets(true)
		, bAllowRemapToExisting(true)
		, bConvertSpaces(true)
	{}
};

/**
 *
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Retarget : public UObject
{
	GENERATED_BODY()

public:

#pragma region Get Skeleton Bone Transform

	// Returns the local space transform of the current skeleton reference bone
	static FTransform GetSkeletonBoneTransformLS(USkeleton* InSkeleton, const FName& InBoneName);

	// Returns the current skeleton reference bone transform from component space to local space
	static FTransform GetSkeletonBoneTransformLS(USkeleton* InSkeleton, const FName& InBoneName, const FTransform& InBoneTransformCS);

	// Returns the component space transform of the current skeleton reference bone 
	static FTransform GetSkeletonBoneTransformCS(USkeleton* InSkeleton, const FName& InBoneName);

#pragma endregion

public:

	// Replace the current animation with the specified frame animation of the reference animation
	static void OnRetargetPoseFromSkeleton(USkeleton* InSkeleton, USkeleton* InReferenceSkeleton, UAnimSequence* InAnimSequence, const FVirtualBonesData& InBonesData);

	// Replace the current animation with the specified frame animation of the reference animation
	static void OnRetargetPoseFromAnimation(USkeleton* InSkeleton, UAnimSequence* InAnimSequence, UAnimSequence* InReferenceAnimSequence, const FVirtualBonesData& InBonesData);

	static void OnRetargetAnimations(UAnimSequence* InAnimSequence, FVirtualRetargetAnimationsData& InRetargetData, const TArray<FVirtualBoneConstraintData>& InConstraintBonesData);

	static void OnRebuildAnimations(UAnimSequence* InAnimSequence);

};
