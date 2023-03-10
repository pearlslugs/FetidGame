// Copyright 2021 Phyronnaz

#include "VoxelFoliageRayHandler.h"
#include "VoxelFoliageProxy.h"

bool FVoxelFoliageRayHandler::TraceRay(const FVector& Start, const FVector& Direction, FVector& HitNormal, FVector& HitPosition) const
{
	VOXEL_SCOPED_STAT(Stats.TraceRay_Ray, 1);
	
	ensure(Direction.IsNormalized());

	const bool bHit = TraceRayInternal(Start, Direction, HitNormal, HitPosition);
	if (bStoreDebugRays)
	{
		FVoxelFoliageDebugRay Ray;
		Ray.Start = Start;
		Ray.Direction = Direction;
		Ray.bHit = bHit;
		Ray.HitNormal = HitNormal;
		Ray.HitPosition = HitPosition;
		DebugRays.Add(Ray);
	}
	return bHit;
}
