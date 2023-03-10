// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelBasis.h"
#include "VoxelHitGenerator.h"

class FVoxelHeightHitGenerator : public FVoxelHitGenerator
{
public:
	using FVoxelHitGenerator::FVoxelHitGenerator;

protected:
	virtual TArray<FVoxelFoliageHit> GenerateImpl(int32 NumRays) override;
};