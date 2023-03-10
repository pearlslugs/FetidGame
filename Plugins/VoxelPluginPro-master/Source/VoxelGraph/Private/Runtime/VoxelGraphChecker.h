// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

class FVoxelGraphErrorReporter;
class FVoxelGraph;

namespace FVoxelGraphChecker
{
	void CheckGraph(FVoxelGraphErrorReporter& ErrorReporter, const FVoxelGraph& Graph);
}
