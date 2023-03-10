// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelFoliageQueryCache.h"

struct FVoxelFoliageDensity;

class FVoxelFoliageDensityQuery : public FVoxelFoliageQueryBase
{
public:
	using FVoxelFoliageQueryBase::FVoxelFoliageQueryBase;

	v_flt GetDensity() const;
	float GetDensity(const FVoxelFoliageDensity& FoliageDensity) const;
	
private:
	float GetBaseDensity(const FVoxelFoliageDensity& FoliageDensity) const;
};
