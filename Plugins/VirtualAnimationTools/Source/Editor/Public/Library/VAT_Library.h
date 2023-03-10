 // Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VAT_Library.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Library : public UObject
{
	GENERATED_BODY()

public:	// **************************************** Math **********************************************//

	/** Normalize the transformation */
	static void NormailizeTransform(FTransform& InTransform);

	/** Blend two transform */
	static FTransform BlendTransform(const FTransform& InTransform, const FTransform& InOtherTransform
		, FVector InTranslationWeight = FVector(1.f), FVector InRotationWeight = FVector(1.f), FVector InScaleWeight = FVector(1.f));

	/** Additive two transform */
	static FTransform Add_TransformTransform(const FTransform& InTransform, const FTransform& InOtherTransform);

	/** Subtract two transform */
	static FTransform Subtract_TransformTransform(const FTransform& InTransform, const FTransform& InOtherTransform);

public:	// **************************************** Anim **********************************************//

	/** Get the time at the frame */
	static float GetFrameTime(UAnimSequenceBase* InAnimSequence);

	/** Get the keys number at the animation asset */
	static int32 GetNumberOfKeys(UAnimSequence* InAnimSequence);

	/** Get the frames number at the animation asset */
	static int32 GetNumberOfFrames(UAnimSequence* InAnimSequence);

	/** Get the sequence length at the animation asset */
	static float GetSequenceLength(UAnimSequenceBase* InAnimSequence);

public:	// **************************************** Bone **********************************************//

	/** Return the bone track data from bone name */
	static bool GetBoneTrackData(UAnimSequence* InAnimSequence, const FName& InBoneName, FRawAnimSequenceTrack& OutBoneTack, const bool InFillTrack = false);

	/** Return the bone track data from bone track index */
	static bool GetBoneTrackData(UAnimSequence* InAnimSequence, const int32& InBoneTrackIndex, FRawAnimSequenceTrack& OutBoneTack, const bool InFillTrack = false);

	/** Return the bone track index from bone name */
	static int32 GetBoneTrackIndex(UAnimSequence* InAnimSequence, const FName& InBoneName, bool InAddDefaultTrack = true);

	/** Add default bone track at the animation asset */
	static void AddBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName);

	/** Insert default bone track at the animation asset */
	static int32 InsertBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName, const int32& InInsertBoneIndex);

	/** Remove desired bone track at the animation asset */
	static int32 RemoveBoneTrack(UAnimSequence* InAnimSequence, const FName& InBoneName, const bool InRemoveTranslation = true, const bool InRemoveRotation = true, const bool InRemoveScale = false);

	/** Modify desired bone track at the animation asset */
	static bool SetBoneTrackData(UAnimSequence* InAnimSequence, const FName& InBoneName, FRawAnimSequenceTrack* InTrackData, bool InShouldTransact = false);

	/** Add bone track key data from transform key */
	static void AddBoneTrackKey(FRawAnimSequenceTrack& InTrackData, const FTransform& InTransformKey);

	/** Modify bone track key data at the key index */
	static void ModifyBoneTrackKey(FRawAnimSequenceTrack& InTrackData, const FTransform& InTransformKey, const int32& InKeyIndex);

	/** Transfer bone track key data from key index */
	static void TransferBoneTrackKey(const FRawAnimSequenceTrack& InSourceTrackData, FRawAnimSequenceTrack& InNewTrackData, const int32& InKeyIndex);

public:

	/** Make object dirt state */
	UFUNCTION(BlueprintCallable, Category = "VAT| Library")
	static void ModifyFromObject(UObject* InObject);

	/** Make animation sequence dirt state */
	UFUNCTION(BlueprintCallable, Category = "VAT| Library")
	static void ModifyFromAnimSequence(UAnimSequenceBase* InAnimSequence);

	/** Make animation montage dirt state */
	UFUNCTION(BlueprintCallable, Category = "VAT| Library")
	static void ModifyFromAnimMontage(UAnimMontage* InAnimMontage);

	/** Checks if the animation data has to be re-baked / compressed and does so */
	UFUNCTION(BlueprintCallable, Category = "VAT| Library")
	static void UpdateCompressedAnimationDataAsSequence(UAnimSequence* InAnimSequnence);
};
