// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelFoliageRayHandler.h"

// Defined in UE5
#ifndef USE_EMBREE
#define USE_EMBREE 0
#endif

#define USE_EMBREE_VOXEL_FOLIAGE (USE_EMBREE_VOXEL || (USE_EMBREE && ENGINE_MAJOR_VERSION >= 5))

#if USE_EMBREE_VOXEL_FOLIAGE
#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

struct FVoxelChunkMesh;

class FVoxelFoliageEmbreeRayHandler : public FVoxelFoliageRayHandler
{
public:
	FVoxelFoliageEmbreeRayHandler(
		FVoxelFoliageStats& Stats,
		TArray<uint32>&& Indices,
		TArray<FVector>&& Vertices);
	~FVoxelFoliageEmbreeRayHandler();
	
	virtual bool HasError() const override { return !EmbreeScene || !EmbreeDevice; }
	
protected:
	virtual bool TraceRayInternal(const FVector& Start, const FVector& Direction, FVector& HitNormal, FVector& HitPosition) const override;

private:
	RTCDevice EmbreeDevice = nullptr;
	RTCScene EmbreeScene = nullptr;
	RTCGeometry Geometry = nullptr;

	TArray<uint32> Indices;
	TArray<FVector> Vertices;
};

#endif