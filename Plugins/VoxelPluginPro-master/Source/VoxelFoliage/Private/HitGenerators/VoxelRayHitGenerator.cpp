// Copyright 2021 Phyronnaz

#include "HitGenerators/VoxelRayHitGenerator.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageUtilities.h"
#include "VoxelFoliageRayHandler.h"
#include "VoxelFoliageRandomGenerator.h"

#include "VoxelCancelCounter.h"
#include "VoxelData/VoxelDataIncludes.h"

TArray<FVoxelFoliageHit> FVoxelRayHitGenerator::GenerateImpl(int32 NumRays)
{
	VOXEL_ASYNC_FUNCTION_COUNTER();

	check(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Ray);

	check(Bounds.Size().X == Bounds.Size().Y && Bounds.Size().Y == Bounds.Size().Z);
	const int32 BoundsSize = Bounds.Size().X;
	const FIntVector& ChunkPosition = Bounds.Min;

	struct FVoxelRay
	{
		FVector Position;
		FVector Direction;
	};
	TArray<FVoxelFoliageHit> Hits;

	TArray<FVoxelRay> QueuedRays;
	for (int32 Index = 0; Index < NumRays; Index++)
	{
		if ((Index & 0xFF) == 0 && CancelTracker.IsCanceled())
		{
			return {};
		}
		Stats.NumRays++;

		const FVector2D RandomValue = 2 * RandomGenerator.GetPosition() - 1; // Map from 0,1 to -1,1
		const FVector Start = BoundsSize / 2.f * (Basis.X * RandomValue.X + Basis.Y * RandomValue.Y + 1); // +1: we want to be in the center
		const FVector Direction = FVoxelFoliageUtilities::GetRayDirection(Settings, Start, ChunkPosition);
		FVoxelFoliageHit Hit;
		if (RayHandler.TraceRay(
			Start - Direction * 4 * BoundsSize /* Ray offset */,
			Direction,
			Hit.Normal,
			Hit.LocalPosition))
		{
			Hits.Add(Hit);
			QueuedRays.Add({ Hit.LocalPosition, Direction });
		}
	}

	// Process consecutive hits
	while (QueuedRays.Num() > 0)
	{
		Stats.NumRays++;
		Stats.NumConsecutiveRays_Ray++;

		const auto Ray = QueuedRays.Pop(false);
		const float Offset = 1;
		FVoxelFoliageHit Hit;
		if (RayHandler.TraceRay(
			Ray.Position + Ray.Direction * Offset,
			Ray.Direction,
			Hit.Normal,
			Hit.LocalPosition))
		{
			Stats.NumHitsFromConsecutiveRays_Ray++;
			Hits.Add(Hit);
			QueuedRays.Add({ Hit.LocalPosition, Ray.Direction });
		}
	}
	
	return Hits;
}