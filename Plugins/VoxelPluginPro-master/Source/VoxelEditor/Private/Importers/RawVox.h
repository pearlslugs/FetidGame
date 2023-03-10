// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

struct FVoxelDataAssetData;

namespace RawVox
{
	bool ImportToAsset(const FString& File, FVoxelDataAssetData& Asset);
}

