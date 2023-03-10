// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelHitGenerator.h"

class FVoxelFoliageRayHandler;

class FVoxelRayHitGenerator : public FVoxelHitGenerator
{
public:
	FVoxelFoliageRayHandler& RayHandler;
	
	explicit FVoxelRayHitGenerator(const FParameters& Parameters, FVoxelFoliageRayHandler& RayHandler)
		: FVoxelHitGenerator(Parameters)
		, RayHandler(RayHandler)
	{
	}
	
protected:
	virtual TArray<FVoxelFoliageHit> GenerateImpl(int32 NumRays) override;
};
