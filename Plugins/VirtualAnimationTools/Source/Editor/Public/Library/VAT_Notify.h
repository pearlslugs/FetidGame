// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "VAT_Types.h"
#include "VAT_Notify.generated.h"

class USkeleton;
class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;
class UAnimNotify;
class UAnimNotifyState;

UENUM()
enum class EVirtualSmartNotifyType : uint8
{
	FootLand,
	MultiplePosition,
	MAX UMETA(Hidden)
};

/** Struct of animation asset notify event data */
USTRUCT()
struct FVirtualNotifyEventData
{
	GENERATED_USTRUCT_BODY()

	/** Color of Notify in editor */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FColor NotifyColor;

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

	FVirtualNotifyEventData()
		: NotifyColor(FColor(0.f))
		, TriggerWeightThreshold(ZERO_ANIMWEIGHT_THRESH)
		, MontageTickType(EMontageNotifyTickType::Queued)
		, NotifyTriggerChance(1.f)
		, NotifyFilterLOD(0)
		, bTriggerOnFollower(false)
		, bTriggerOnDedicatedServer(true)
		, NotifyFilterType(ENotifyFilterType::NoFiltering)
	{}
};

/** Struct of animation asset notify track data */
USTRUCT()
struct FVirtualNotifyTrackData
{
	GENERATED_USTRUCT_BODY()

	/** Animation notify track name */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FName TrackName;

	/** Animation notify track color */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FLinearColor TrackColor;

	FVirtualNotifyTrackData()
		: TrackName(NAME_None)
		, TrackColor(FLinearColor::White)
	{}
};

/** Struct of animation asset notify modify track data */
USTRUCT()
struct FVirtualNotifyModifyTrackData
{
	GENERATED_USTRUCT_BODY()

	/** Key is old track name, Value is new track name */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TMap<FName, FName> TrackNameMap;

	FVirtualNotifyModifyTrackData()
	{}
};

/** Struct of animation asset notify data */
USTRUCT()
struct FVirtualNotifyData
{
	GENERATED_USTRUCT_BODY()

	/** Desired animation position, We will limit to what is valid */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	float Position;

	/** Animation notify instance reference */
	UPROPERTY(EditAnywhere, Instanced, Category = "Notify Params")
	UAnimNotify* NotifyInstance;

public:

	/** Return the notify instance is valid */
	bool IsValid() const { return NotifyInstance != NULL; }

	FVirtualNotifyData()
		: Position(0.f)
		, NotifyInstance(NULL)
	{}
};

/** Struct of animation asset notify state data */
USTRUCT()
struct FVirtualNotifyStateData
{
	GENERATED_USTRUCT_BODY()

	/** Desired animation position and duration, We will limit to what is valid
	  * X = Position, Y = Duration */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FVector2D Range;

	/** Animation notify state instance reference */
	UPROPERTY(EditAnywhere, Instanced, Category = "Notify Params")
	UAnimNotifyState* NotifyStateInstance;

public:

	/** Return the notify state instance is valid */
	bool IsValid() const { return NotifyStateInstance != NULL; }

	/** Return the notify state start position in animation asset */
	float GetStartPosition() const { return Range.X; }

	/** Return the notify state duration in animation asset */
	float GetDuration() const { return  Range.X >= 0.f ? Range.Y - Range.X : Range.Y; }

	FVirtualNotifyStateData()
		: Range(FVector2D(0.f))
		, NotifyStateInstance(NULL)
	{}
};

/** Struct of animation asset notifies data */
USTRUCT()
struct FVirtualNotifiesData
{
	GENERATED_USTRUCT_BODY()

	/** Desired animation notify track name, If the target track is invalid, we will add a default track */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	FName TrackName;

	/** Animation notifies data */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TArray<FVirtualNotifyData> Notifies;

	/** Animation notifies state data */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TArray<FVirtualNotifyStateData> NotifiesState;

	FVirtualNotifiesData()
		: TrackName(NAME_None)
	{}
};

/** Struct of animation asset multi notifies data */
USTRUCT()
struct FVirtualMultiNotifiesData
{
	GENERATED_USTRUCT_BODY()

	/** Notifies data */
	UPROPERTY(EditAnywhere, Category = "Notify Params")
	TArray<FVirtualNotifiesData> NotifiesData;

	FVirtualMultiNotifiesData()
	{}
};

/**
 * 
 */
UCLASS()
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Notify : public UObject
{
	GENERATED_BODY()
	
public:

#pragma region Notify Range

	/**
	 * Limit the position of the AnimNotify within the effective animation length
	 *
	 * @param	InSequenceLength		Animation asset sequence length
	 * @param	InPosition		Animation notify start position
	 */
	static void LimitPositionRange(const float& InSequenceLength, float& InPosition);

	/**
	 * Limit the position and length of the AnimNotifyState to be within the effective animation length
	 *
	 * @param	InAnimSequenceBase		Animation sequence base asset
	 * @param	InPosition		Animation notify state start position
	 * @param	InDuration		Animation notify state duration
	 */
	static void LimitPositionRange(UAnimSequenceBase* InAnimSequenceBase, float& InPosition, float& InDuration);

#pragma endregion


#pragma region Notify Event

	/**
	 * Add notifies event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	OutNotifiesData		Return animation notifies data
	 *
	 */
	static void GetNotifiesData(UAnimSequenceBase* InAnimSequenceBase, TArray<FVirtualNotifiesData>& OutNotifiesData);

	/**
	 * Transfer notify event instance to notify event data
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesData		Animation notifies data
	 *
	 */
	static void TransferNotifyEventToData(const FAnimNotifyEvent& InNotifyEvent, FVirtualNotifyEventData& InNotifyEventData);

	/**
	 * Transfer notify event data to notify event instance
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesData		Animation notifies data
	 *
	 */
	static void TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent, const FVirtualNotifyEventData& InNotifyEventData);

#pragma endregion


#pragma region Add Notify

	/**
	 * Add notifies event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesData		Animation notifies data
	 * 
	 */
	static void AddNotifiesEvent(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData);

#if 0
	/**
	 * Add notifies event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InAddNotifiesData		Animation notifies data
	 * @param	OutAnimNotifies		Return animation notifies object
	 * @param	OutAnimNotifiesState		Return animation notifies state object
	 *
	 */
	static void AddNotifiesEvent(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData, TArray<UAnimNotify>& OutAnimNotifies, TArray<UAnimNotifyState>& OutAnimNotifiesState);
#endif

	/**
	 * Add notify event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyData		Animation notify data
	 * @param	InNotifyTrackName		Desired add to notify track name
	 * @return   Return animation notifies event
	 */
	static TArray<UAnimNotify*> AddNotifyEvent(UAnimSequenceBase* InAnimSequenceBase, const FVirtualNotifyData& InNotifyData, const FName& InNotifyTrackName);

	/**
	 * Add notify state event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyStateData		Animation notify state data
	 * @param	InNotifyTrackName		Desired add to notify track name
	 * @return   Return animation notifies event
	 */
	static TArray<UAnimNotifyState*> AddNotifyStateEvent(UAnimSequenceBase* InAnimSequenceBase, const FVirtualNotifyStateData& InNotifyStateData, const FName& InNotifyTrackName);

#pragma endregion


#pragma region Modify Notify

	/**
	 * Modify notifies event to animation asset
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesData		Animation notifies data
	 *
	 */
	static void ModifyNotifies(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData);

#pragma endregion


#pragma region Remove Notify


	/**
	 * Remove notifies by notify class and check desired position
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesData		Animation notifies data
	 *
	 */
	static void RemoveNotifies(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifiesData>& InNotifiesData);

	/**
	 * Remove notifies by notify index
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifiesIndex		Remove notifies index
	 *
	 */
	static bool RemoveNotifiesByIndex(UAnimSequenceBase* InAnimSequenceBase, const TArray<int32>& InNotifiesIndex);

	/**
	 * Remove notifies by notify class and check desired position
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyData		Animation notify data
	 * @param	InNotifyTrackName		Animation notify track name
	 *
	 */
	static bool RemoveNotifiesByClass(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifyData>& InNotifiesData, const FName* InNotifyTrackName = nullptr);

	/**
	 * Remove notifies state by notify state class and check desired position
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyStateData		Animation notify state data
	 * @param	InNotifyTrackName		Animation notify track name
	 *
	 */
	static bool RemoveNotifiesStateByClass(UAnimSequenceBase* InAnimSequenceBase, const TArray<FVirtualNotifyStateData>& InNotifiesStateData, const FName* InNotifyTrackName = nullptr);

#pragma endregion


#pragma region Notify Track

	/**
	 * Checks whether or not the given Track Name is a valid Animation Notify Track in the Animation Sequence
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyTrackName		Animation notify track name
	 * @return   Return animation asset has the desired notify track name result
	 */
	static bool HasNotifyTrack(const UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName);

	/**
	 * Get the animation notify track index by animation notify track name
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyTrackName		Animation notify track name
	 * @return   Return the animation notify track index
	 */
	static int32 GetNotifyTrackIndex(const UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName);

	/**
	 * Get the animation notify tracks name
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InTrackNameMap		Animation notify track name map
	 * @return   Return the animation notify track index
	 */
	static void GetNotifiesTrackName(const UAnimSequenceBase* InAnimSequenceBase, TMap<FName, FName>& InTrackNameMap);

	/**
	 * Get the animation notify tracks name
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InTrackData		Animation notify track name data
	 * @return   Return the animation notify track index
	 */
	static void GetNotifiesTrackName(const UAnimSequenceBase* InAnimSequenceBase, TArray<FVirtualNotifyTrackData>& InTrackData);

	/**
	 * Add an Animation Notify Track to the Animation Sequence
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyTrackName		Animation notify track name
	 * @param	InTrackColor		Animation notify track color
	 * @return   Return animation asset add notify track result
	 */
	static bool AddNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName, const FLinearColor InTrackColor = FLinearColor::White);

	/**
	 * Modify animation notify track data
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyTrackName		Animation notify track name
	 * @param	InLastNotifyTrackName		Animation old notify track name
	 * @return   Return animation asset modify notify track result
	 */
	static bool ModifyNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName, const FName& InLastNotifyTrackName);

	/**
	 * Remove animation notify track data
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InNotifyTrackName		Animation notify track name
	 * @param	InPreviousNotifyTrackName		Animation old notify track name
	 * @return   Return animation asset remove notify track result
	 */
	static bool RemoveNotifyTrack(UAnimSequenceBase* InAnimSequenceBase, const FName& InNotifyTrackName);

#pragma endregion


#pragma region Foot Step

	/**
	 * Sample and add foot step notifies
	 *
	 * @param	InAnimSequenceBase		Animation asset sequence base
	 * @param	InCurveSampleData		Animation asset curve sample data
	 * @param	InLegsBaseData		Animation skeleton legs base data
	 * @param	& InNotifiesData		Foot step notifies data
	 *
	 */
	static void SampleFootStepNotifies(UAnimSequence* InAnimSequenceBase, const FVirtualCurveSampleData& InCurveSampleData
		, const TArray<FVirtualLegBaseData>& InLegsBaseData, TArray<FVirtualNotifiesData>& InNotifiesData);

#pragma endregion

};
