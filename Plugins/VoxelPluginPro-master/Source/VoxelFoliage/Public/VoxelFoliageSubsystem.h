// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelIntBox.h"
#include "VoxelTickable.h"
#include "Containers/Queue.h"
#include "VoxelFoliageInterface.h"
#include "VoxelFoliageBucket.h"
#include "VoxelFoliageSubsystem.generated.h"

class UVoxelFoliage;
class UVoxelFoliageBiome;
class UVoxelFoliageCollection;
struct FVoxelFoliageSaveImpl;
struct FVoxelFoliageResultData;

namespace FVoxelFoliageSaveVersion
{
	enum Type : int32;
}

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageSubsystemProxy : public UVoxelFoliageInterfaceSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelFoliageSubsystem);
};

class VOXELFOLIAGE_API FVoxelFoliageSubsystem : public IVoxelFoliageInterface, public FVoxelTickable
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelFoliageSubsystemProxy);

	void Serialize(FArchive& Ar, FVoxelFoliageSaveVersion::Type Version);

	const auto& GetBuckets() const { return Buckets; }
	
	//~ Begin IVoxelSubsystem Interface
	virtual void PostCreate(const IVoxelSubsystem* OldSubsystem) override;
	virtual void Destroy() override;
	//~ End IVoxelSubsystem Interface

	void SaveTo(FVoxelFoliageSaveImpl& Save);
	void LoadFrom(const FVoxelFoliageSaveImpl& Save);

	bool GetFoliageData(const FGuid& FoliageGuid, TArray<TVoxelSharedPtr<FVoxelFoliageResultData>>& OutData) const;

	//~ Begin IVoxelFoliageInterface Interface
	virtual int32 RegisterBiome(UVoxelFoliageBiomeBase* Biome) override;

	virtual void Regenerate(const FVoxelIntBox& Bounds) override;
	virtual void MarkDirty(const FVoxelIntBox& Bounds) override;
	
#if WITH_EDITOR
	virtual bool NeedsToRebuild(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) const override;
#endif
	//~ End IVoxelFoliageInterface Interface

protected:
	//~ Begin FVoxelTickable Interface
	virtual void Tick(float DeltaTime) override;
	//~ End FVoxelTickable Interface

private:
	template<typename T>
	void IterateChunksInBounds(const FVoxelIntBox& Bounds, T Lambda);
	
	TMap<FVoxelFoliageBucketKey, TVoxelSharedPtr<FVoxelFoliageBucket>> Buckets;
	
	struct FFoliageProxyInfo
	{
		// Biome or collection
		TWeakObjectPtr<const UObject> Container;
		int32 FoliageIndex = 0;

		TVoxelWeakPtr<FVoxelFoliageBucket> Bucket;
		TVoxelWeakPtr<FVoxelFoliageProxy> FoliageProxy;
	};
	TMap<FGuid, FFoliageProxyInfo> GuidToProxies;

	TMap<TWeakObjectPtr<UVoxelFoliageBiomeType>, TMap<int32, TWeakObjectPtr<UVoxelFoliageBiome>>> BiomeTypes;

public:
	struct FTaskCallback
	{
		TVoxelWeakPtr<FVoxelFoliageBucket> Bucket;
		FVoxelFoliageProxyId FoliageId;
		FVoxelFoliageTaskId TaskId;
		FVoxelIntBox Bounds;
		TVoxelSharedPtr<FVoxelFoliageResultData> Data;
	};
	TQueue<FTaskCallback, EQueueMode::Mpsc> TaskCallbacks;

	void ProcessTaskCallbacks();
};