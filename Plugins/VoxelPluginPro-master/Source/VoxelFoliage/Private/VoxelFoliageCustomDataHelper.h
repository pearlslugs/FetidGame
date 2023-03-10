// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelFoliageQueryCache.h"

struct FVoxelFoliageCustomData;

class FVoxelFoliageCustomDataQuery : public FVoxelFoliageQueryBase
{
public:
	using FVoxelFoliageQueryBase::FVoxelFoliageQueryBase;

	float GetCustomData(const FVoxelFoliageCustomData& FoliageCustomData) const;

private:
	static float ColorToFloat(FColor Color);
};