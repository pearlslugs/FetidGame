// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelCompilationPass.h"

struct FVoxelGetRangeAnalysisPass
{
	VOXEL_PASS_BODY(FVoxelGetRangeAnalysisPass);
	
	static void Apply(FVoxelGraphCompiler& Compiler);
};
