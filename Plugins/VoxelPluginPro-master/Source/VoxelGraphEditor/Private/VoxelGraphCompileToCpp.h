// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

class UVoxelGraphGenerator;
class SWidget;

namespace FVoxelGraphCompileToCpp
{
	void Compile(UVoxelGraphGenerator* Generator, bool bIsAutomaticCompile);
}
