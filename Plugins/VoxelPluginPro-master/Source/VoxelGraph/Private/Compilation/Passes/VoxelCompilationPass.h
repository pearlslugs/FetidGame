// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

class FVoxelGraphCompiler;

#define VOXEL_PASS_BODY(Class) static const TCHAR* GetName() { sizeof(Class); return TEXT(#Class); }

