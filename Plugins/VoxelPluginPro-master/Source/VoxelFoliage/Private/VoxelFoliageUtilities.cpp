// Copyright 2021 Phyronnaz

#include "VoxelFoliageUtilities.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageRandomGenerator.h"
#include "VoxelFoliageEmbreeRayHandler.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelRender/IVoxelRenderer.h"
#include "VoxelBasis.h"

TUniquePtr<FVoxelFoliageRayHandler> FVoxelFoliageUtilities::CreateRayHandler(
	const FVoxelFoliageSubsystem& FoliageSubsystem,
	const FVoxelFoliageSpawnSettings& SpawnSettings,
	const FVoxelCancelTrackerGroup& CancelTracker,
	const FVoxelIntBox& Bounds,
	FVoxelFoliageStats& Stats)
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	ensure(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Ray);
	
	const auto Renderer = FoliageSubsystem.GetSubsystem<IVoxelRenderer>();
	if (!Renderer)
	{
		return nullptr;
	}

	TArray<uint32> Indices;
	TArray<FVector> Vertices;
	VOXEL_INLINE_STAT(Stats.GenerateMesh_Ray, 1, Renderer->CreateGeometry_AnyThread(SpawnSettings.GetLOD(), Bounds.Min, Indices, Vertices));

	if (CancelTracker.IsCanceled() || Indices.Num() == 0)
	{
		return {};
	}

	TUniquePtr<FVoxelFoliageRayHandler> RayHandler;
#if USE_EMBREE_VOXEL_FOLIAGE
	RayHandler = MakeUnique<FVoxelFoliageEmbreeRayHandler>(Stats, MoveTemp(Indices), MoveTemp(Vertices));
#else
	LOG_VOXEL(Error, TEXT("Embree is required for ray foliage!"));
#endif

	if (!ensure(RayHandler))
	{
		return nullptr;
	}
	if (!ensure(!RayHandler->HasError()))
	{
		return nullptr;
	}

	return MoveTemp(RayHandler);
}

FVoxelBasis FVoxelFoliageUtilities::GetBasisFromBounds(const FVoxelRuntimeSettings& Settings,const FVoxelIntBox& Bounds)
{
	if (Settings.FoliageWorldType == EVoxelFoliageWorldType::Flat)
	{
		return FVoxelBasis::Unit();
	}
	else
	{
		check(Settings.FoliageWorldType == EVoxelFoliageWorldType::Planet);

		FVoxelBasis Basis = FVoxelBasis::MakePlanetBasisForBounds(Bounds);

		// Hack to avoid holes
		Basis.X *= 1.5;
		Basis.Y *= 1.5;

		return Basis;
	}
}

FVoxelVector FVoxelFoliageUtilities::GetRayDirection(const FVoxelRuntimeSettings& Settings, const FVoxelVector& Start, const FIntVector& ChunkPosition)
{
	if (Settings.FoliageWorldType == EVoxelFoliageWorldType::Flat)
	{
		return -FVoxelVector::UpVector;
	}
	else
	{
		check(Settings.FoliageWorldType == EVoxelFoliageWorldType::Planet);
		return -(FVoxelVector(ChunkPosition) + Start).GetSafeNormal();
	}
}