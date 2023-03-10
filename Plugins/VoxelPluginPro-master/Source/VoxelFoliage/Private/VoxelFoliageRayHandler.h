// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

struct FVoxelFoliageStats;

struct FVoxelFoliageDebugRay
{
	FVector Start;
	FVector Direction;

	bool bHit;
	FVector HitNormal;
	FVector HitPosition;
};

class FVoxelFoliageRayHandler
{
public:
	FVoxelFoliageStats& Stats;

	explicit FVoxelFoliageRayHandler(FVoxelFoliageStats& Stats)
		: Stats(Stats)
	{
	}
	virtual ~FVoxelFoliageRayHandler() = default;

	virtual bool HasError() const = 0;
	bool TraceRay(const FVector& Start, const FVector& Direction, FVector& HitNormal, FVector& HitPosition) const;
	
protected:
	virtual bool TraceRayInternal(const FVector& Start, const FVector& Direction, FVector& HitNormal, FVector& HitPosition) const = 0;

public:
	bool bStoreDebugRays = false;
	mutable TArray<FVoxelFoliageDebugRay> DebugRays;
};