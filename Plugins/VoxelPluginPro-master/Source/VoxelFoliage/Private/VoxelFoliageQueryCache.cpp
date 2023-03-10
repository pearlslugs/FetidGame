// Copyright 2021 Phyronnaz

#include "VoxelFoliageQueryCache.h"
#include "VoxelFoliageProxy.h"
#include "VoxelData/VoxelDataIncludes.h"

FVoxelValue FVoxelFoliageQueryCache::GetValue(const FIntVector& Position) const
{
	if (const FVoxelValue* Value = CachedValues.Find(Position))
	{
		return *Value;
	}

	const FVoxelValue Value = VOXEL_INLINE_STAT(Stats.GetValue, 1, Accelerator.Get<FVoxelValue>(Position, 0));
	CachedValues.Add(Position, Value);
	return Value;
}

FVoxelMaterial FVoxelFoliageQueryCache::GetMaterial(const FIntVector& Position) const
{
	if (const FVoxelMaterial* Material = CachedMaterials.Find(Position))
	{
		return *Material;
	}

	const FVoxelMaterial Material = VOXEL_INLINE_STAT(Stats.GetMaterial, 1, Accelerator.Get<FVoxelMaterial>(Position, 0));
	CachedMaterials.Add(Position, Material);
	return Material;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMaterial FVoxelFoliageQueryBase::GetMaterial() const
{
	if (CachedMaterial)
	{
		return CachedMaterial.GetValue();
	}
	
	FIntVector ClosestPoint;
	v_flt Distance = MAX_vflt;
	for (auto& Neighbor : FVoxelUtilities::GetNeighbors(Position))
	{
		if (!Cache.GetValue(Neighbor).IsEmpty())
		{
			const v_flt PointDistance = (FVoxelVector(Neighbor) - Position).SizeSquared();
			if (PointDistance < Distance)
			{
				Distance = PointDistance;
				ClosestPoint = Neighbor;
			}
		}
	}
	if (Distance > 100)
	{
		// Happens
		ClosestPoint = FVoxelUtilities::RoundToInt(Position);
	}
	
	CachedMaterial = Cache.GetMaterial(ClosestPoint);
	return CachedMaterial.GetValue();
}

v_flt FVoxelFoliageQueryBase::GetFloatCustomOutput(FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const
{
	return GetCustomOutput<v_flt>(0.f, OutputName, GeneratorOverride);
}

FColor FVoxelFoliageQueryBase::GetColorCustomOutput(FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const
{
	return GetCustomOutput<FColor>(FColor(ForceInit), OutputName, GeneratorOverride);
}

template<typename T>
T FVoxelFoliageQueryBase::GetCustomOutput(T Default, FName OutputName, const FVoxelGeneratorInstance* GeneratorOverride) const
{
	// Need to get the right ItemHolder. No need to use closest not empty position.

	FVoxelGeneratorQueryData QueryData;
	QueryData.GetMaterial = [&]() { return GetMaterial(); };
	
	return Cache.Accelerator.GetCustomOutput<T>(
		Default,
		OutputName,
		Position.X, Position.Y, Position.Z,
		0,
		QueryData,
		GeneratorOverride);
}