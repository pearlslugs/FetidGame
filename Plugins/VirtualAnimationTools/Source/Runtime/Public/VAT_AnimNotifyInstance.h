// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once
#include "Animation/AnimTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "VAT_AnimNotifyInstance.generated.h"

/** Struct of animation asset notify event data */
USTRUCT()
struct FVirtualNotifyRuntimeData
{
	GENERATED_USTRUCT_BODY()

	/** Trigger weight threshold */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	float TriggerWeightThreshold;

	/** Montage notify tick type */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TEnumAsByte<EMontageNotifyTickType::Type> MontageTickType;

	/** Defines the chance of of this notify triggering, 0 = No Chance, 1 = Always triggers */
	UPROPERTY(EditAnywhere, Category = "Notify Params", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float NotifyTriggerChance;

	/** LOD to start filtering this notify from.*/
	UPROPERTY(EditAnywhere, Category = "Notify Params", meta = (ClampMin = "0", UIMin = "0"))
	int32 NotifyFilterLOD;

	/** If enabled this notify will trigger when the animation is a follower in a sync group (by default only the sync group leaders notifies trigger */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	bool bTriggerOnFollower;

	/** If disabled this notify will be skipped on dedicated servers */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	bool bTriggerOnDedicatedServer;

	/** Defines a method for filtering notifies (stopping them triggering) e.g. by looking at the meshes current LOD */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TEnumAsByte<ENotifyFilterType::Type> NotifyFilterType;

	FVirtualNotifyRuntimeData()
		: TriggerWeightThreshold(ZERO_ANIMWEIGHT_THRESH)
		, MontageTickType(EMontageNotifyTickType::Queued)
		, NotifyTriggerChance(1.f)
		, NotifyFilterLOD(0)
		, bTriggerOnFollower(false)
		, bTriggerOnDedicatedServer(true)
		, NotifyFilterType(ENotifyFilterType::NoFiltering)
	{}
};

//////////////////////////////////////////////////////////////////////////
// UVAT_AnimNotifyInstance
//////////////////////////////////////////////////////////////////////////

UCLASS(editinlinenew, const, hidecategories = Object, collapsecategories)
class VIRTUALANIMATIONTOOLS_API UVAT_AnimNotifyInstance : public UAnimNotify
{
	GENERATED_UCLASS_BODY()

protected:

	/** Animation notify event reference */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FVirtualNotifyRuntimeData NotifyEventData;

public:

	/**
	 * Transfer notify event data to notify event instance
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyInstance		Animation notify instance
	 *
	 */
	virtual void TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent);

};


//////////////////////////////////////////////////////////////////////////
// UVAT_AnimNotifyStateInstance
//////////////////////////////////////////////////////////////////////////

UCLASS(editinlinenew, const, hidecategories = Object, collapsecategories)
class VIRTUALANIMATIONTOOLS_API UVAT_AnimNotifyStateInstance : public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()

protected:

	/** Animation notify event reference */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FVirtualNotifyRuntimeData NotifyEventData;

public:

	/**
	 * Transfer notify event data to notify event instance
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyInstance		Animation notify instance
	 *
	 */
	virtual void TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent);

};