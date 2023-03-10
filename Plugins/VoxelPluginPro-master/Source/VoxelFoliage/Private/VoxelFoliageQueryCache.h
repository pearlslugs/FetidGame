// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelValue.h"
#include "VoxelMaterial.h"

struct FVoxelFoliageStats;
class FVoxelFoliageProxy;
class FVoxelRuntimeSettings;
class FVoxelGeneratorInstance;
class FVoxelConstDataAccelerator;

class FVoxelFoliageQueryCache
{
public:
	FVoxelFoliageStats& Stats;
	const FVoxelConstDataAccelerator& Accelerator;

	FVoxelFoliageQueryCache(FVoxelFoliageStats& Stats, const FVoxelConstDataAccelerator& Accelerator)
		: Stats(Stats)
		, Accelerator(Accelerator)
	{
	}

	FVoxelValue GetValue(const FIntVector& Position) const;
	FVoxelMaterial GetMaterial(const FIntVector& Position) const;

private:
	mutable TMap<FIntVector, FVoxelValue> CachedValues;
	mutable TMap<FIntVector, FVoxelMaterial> CachedMaterials;
};

class FVoxelFoliageQueryBase
{
public:
	const FVoxelFoliageQueryCache& Cache;
	const FVoxelVector Position;
	const FVoxelFoliageProxy& Foliage;
	const FVoxelRuntimeSettings& Settings;

	FVoxelFoliageQueryBase(
		const FVoxelFoliageQueryCache& Cache,
		const FVoxelVector& Position,
		const FVoxelFoliageProxy& Foliage,
		const FVoxelRuntimeSettings& Settings)
		: Cache(Cache)
		, Position(Position)
		, Foliage(Foliage)
		, Settings(Settings)
	{
	}

protected:
	FVoxelMaterial GetMaterial() const;

	v_flt GetFloatCustomOutput(FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const;
	FColor GetColorCustomOutput(FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const;

private:
	mutable TOptional<FVoxelMaterial> CachedMaterial;

	template<typename T>
	T GetCustomOutput(T Default, FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const;
};