// Copyright 2021 Phyronnaz

#include "VoxelFoliageDebug.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelUtilities/VoxelConsoleUtilities.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"
#include "VoxelWorld.h"
#include "VoxelLogTable.h"
#include "VoxelDebug/VoxelDebugUtilities.h"
#include "VoxelDebug/VoxelLineBatchComponent.h"

DEFINE_VOXEL_SUBSYSTEM_PROXY(UVoxelFoliageDebugSubsystemProxy);

static FAutoConsoleCommandWithWorld PrintFoliageStatsCmd(
	TEXT("voxel.foliage.PrintStats"),
	TEXT("Print the foliage stats in the log"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
	{
		World.GetSubsystemChecked<FVoxelFoliageDebug>().PrintFoliageStats();
	}));

static TAutoConsoleVariable<bool> CVarFoliageDebugBounds(
	TEXT("voxel.foliage.DebugBounds"),
	false,
	TEXT("If true, will show the voxel foliage chunk bounds"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarFoliageDebugRays(
	TEXT("voxel.foliage.DebugRays"),
	false,
	TEXT("If true, will show the voxel foliage rays"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarFoliageDebugHitsBeforeDensity(
	TEXT("voxel.foliage.DebugHitsBeforeDensity"),
	false,
	TEXT("If true, will show the voxel foliage hits before the density cull"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarFoliageDebugHitsAfterDensity(
	TEXT("voxel.foliage.DebugHitsAfterDensity"),
	false,
	TEXT("If true, will show the voxel foliage hits after the density cull"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarFoliageDebugFinalHits(
	TEXT("voxel.foliage.DebugFinalHits"),
	false,
	TEXT("If true, will show the voxel foliage hits after the slope cull"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarFoliageDebugScaling(
	TEXT("voxel.foliage.DebugScaling"),
	1,
	TEXT("Percentages of rays/hits to show. Useful when you have too many."),
	ECVF_Default);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageDebug::PrintFoliageStats()
{
	FVoxelFoliageStats TotalStats;
	TMap<const FVoxelFoliageProxy*, FVoxelFoliageStats> FoliageStats;

	FVoxelFoliageSubsystem& FoliageSubsystem = GetSubsystemChecked<FVoxelFoliageSubsystem>();
	for (auto& BucketIt : FoliageSubsystem.GetBuckets())
	{
		for (auto& FoliageIt : BucketIt.Value->Foliages)
		{
			const FVoxelFoliageProxy& Foliage = *FoliageIt.Value;

			ensure(!FoliageStats.Contains(&Foliage));
			FoliageStats.FindOrAdd(&Foliage) += Foliage.GetTotalStats();

			TotalStats += Foliage.GetTotalStats();
		}
	}
	
	FoliageStats.ValueSort([](const FVoxelFoliageStats& A, const FVoxelFoliageStats& B)
	{
		return A.TotalShared.Cycles > B.TotalShared.Cycles;
	});

	const auto RayLogTable = MakeShared<FVoxelLogTable>();
	const auto HeightLogTable = MakeShared<FVoxelLogTable>();
	
	for (auto& It : FoliageStats)
	{
		const FVoxelFoliageStats& Stats = It.Value;
		const FVoxelFoliageProxy& Foliage = *It.Key;
		
		if (Foliage.SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Ray)
		{
			RayLogTable->AddRow()
			.Add("Name", It.Key->FoliageName)
			.AddNumber("Num chunks", Stats.TotalShared.Count, TotalStats.TotalShared.Count)
			.AddSeconds("Total time (shared)", Stats.TotalShared.Seconds(), TotalStats.TotalShared.Seconds())
			.AddSeconds("Total time (self)", Stats.TotalSelf.Seconds(), TotalStats.TotalSelf.Seconds())

			.AddNumber("Rays", Stats.NumRays)
			.AddNumber("Hits"              , Stats.NumHits             , Stats.NumRays)
			.AddNumber("Hits after biomes" , Stats.NumHitsAfterBiomes  , Stats.NumHits)
			.AddNumber("Hits after density", Stats.NumHitsAfterDensity , Stats.NumHitsAfterBiomes)
			.AddNumber("Hits after slope"  , Stats.NumHitsAfterCanSpawn, Stats.NumHitsAfterDensity)
			
			.AddNumber("Consecutive Rays"  , Stats.NumConsecutiveRays_Ray        , Stats.NumRays)
			.AddNumber("Consecutive Hits"  , Stats.NumHitsFromConsecutiveRays_Ray, Stats.NumConsecutiveRays_Ray)

			.AddSeconds("Generate mesh", Stats.GenerateMesh_Ray.Seconds(), Stats.TotalShared.Seconds())
			.AddSeconds("Setup embree" , Stats.SetupEmbree_Ray.Seconds() , Stats.TotalShared.Seconds())
			.AddSeconds("Find hits"    , Stats.FindHits.Seconds()        , Stats.TotalShared.Seconds())
			.AddSeconds("Find biome"   , Stats.FindBiome.Seconds()       , Stats.TotalShared.Seconds())
			.AddSeconds("Split groups" , Stats.SplitGroup.Seconds()      , Stats.TotalShared.Seconds())
			
			.AddSeconds("Apply density"       , Stats.ApplyDensity.Seconds()     , Stats.TotalSelf.Seconds())
			.AddSeconds("Get density value"   , Stats.GetValue.Seconds()         , Stats.ApplyDensity.Seconds())
			.AddSeconds("Get density material", Stats.GetMaterial.Seconds()      , Stats.ApplyDensity.Seconds())
			.AddSeconds("Get World Up"        , Stats.GetWorldUp.Seconds()       , Stats.TotalSelf.Seconds())
			.AddSeconds("Check slope"         , Stats.CanSpawn.Seconds()         , Stats.TotalSelf.Seconds())
			.AddSeconds("Create"              , Stats.CreateTransforms.Seconds() , Stats.TotalSelf.Seconds())
			.AddSeconds("Compute Custom Data" , Stats.ComputeCustomData.Seconds(), Stats.TotalSelf.Seconds())

			.AddNanoseconds("Trace cost per ray"           , Stats.TraceRay_Ray.Seconds()     / Stats.TraceRay_Ray.Count       * 1e9)
			.AddNanoseconds("Density cost per hit"         , Stats.ApplyDensity.Seconds()     / Stats.ApplyDensity.Count       * 1e9)
			.AddNanoseconds("Value cost per query"         , Stats.GetValue.Seconds()         / Stats.GetValue.Count           * 1e9)
			.AddNanoseconds("Material cost per query"      , Stats.GetMaterial.Seconds()      / Stats.GetMaterial.Count        * 1e9)
			.AddNanoseconds("World Up cost per query"      , Stats.GetWorldUp.Seconds()       / Stats.GetWorldUp.Count         * 1e9)
			.AddNanoseconds("Find Biome cost per query"    , Stats.FindBiome.Seconds()        / Stats.FindBiome.Count          * 1e9)
			.AddNanoseconds("Create cost per instance"     , Stats.CreateTransforms.Seconds() / Stats.CreateTransforms.Count   * 1e9)
			.AddNanoseconds("Custom Data cost per instance", Stats.ComputeCustomData.Seconds() / Stats.ComputeCustomData.Count * 1e9)

			.Add("GUID", Foliage.Guid.ToString())
			;
		}
		else
		{
			ensure(Foliage.SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height);
			
			HeightLogTable->AddRow()
			.Add("Name", It.Key->FoliageName)
			.AddNumber("Num chunks", Stats.TotalShared.Count, TotalStats.TotalShared.Count)
			.AddSeconds("Total time (shared)", Stats.TotalShared.Seconds(), TotalStats.TotalShared.Seconds())
			.AddSeconds("Total time (self)", Stats.TotalSelf.Seconds(), TotalStats.TotalSelf.Seconds())

			.AddNumber("Rays", Stats.NumRays)
			.AddNumber("Hits"               , Stats.NumHits                    , Stats.NumRays)
			.AddNumber("Hits after biomes"  , Stats.NumHitsAfterBiomes         , Stats.NumHits)
			.AddNumber("Hits after density" , Stats.NumHitsAfterDensity        , Stats.NumHitsAfterBiomes)
			.AddNumber("Hits after floating", Stats.NumHitsAfterFloating_Height, Stats.NumHitsAfterBiomes)
			.AddNumber("Hits after slope"   , Stats.NumHitsAfterCanSpawn       , Stats.NumHitsAfterDensity)

			.AddSeconds("Find hits"   , Stats.FindHits.Seconds()  , Stats.TotalShared.Seconds())
			.AddSeconds("Find biome"  , Stats.FindBiome.Seconds() , Stats.TotalShared.Seconds())
			.AddSeconds("Split groups", Stats.SplitGroup.Seconds(), Stats.TotalShared.Seconds())
			
			.AddSeconds("Apply density"       , Stats.ApplyDensity.Seconds()          , Stats.TotalSelf.Seconds())
			.AddSeconds("Get density value"   , Stats.GetValue.Seconds()              , Stats.ApplyDensity.Seconds())
			.AddSeconds("Get density material", Stats.GetMaterial.Seconds()           , Stats.ApplyDensity.Seconds())
			.AddSeconds("Check Floating"      , Stats.CheckFloating_Height.Seconds()  , Stats.TotalSelf.Seconds())
			.AddSeconds("Gradient"            , Stats.ComputeGradient_Height.Seconds(), Stats.TotalSelf.Seconds())
			.AddSeconds("Get World Up"        , Stats.GetWorldUp.Seconds()            , Stats.TotalSelf.Seconds())
			.AddSeconds("Check slope"         , Stats.CanSpawn.Seconds()              , Stats.TotalSelf.Seconds())
			.AddSeconds("Create"              , Stats.CreateTransforms.Seconds()      , Stats.TotalSelf.Seconds())
			.AddSeconds("Compute Custom Data" , Stats.ComputeCustomData.Seconds()     , Stats.TotalSelf.Seconds())

			.AddNanoseconds("Height cost per query"        , Stats.GetHeight_Height.Seconds() / Stats.GetHeight_Height.Count   * 1e9)
			.AddNanoseconds("Density cost per hit"         , Stats.ApplyDensity.Seconds()     / Stats.ApplyDensity.Count       * 1e9)
			.AddNanoseconds("Value cost per query"         , Stats.GetValue.Seconds()         / Stats.GetValue.Count           * 1e9)
			.AddNanoseconds("Material cost per query"      , Stats.GetMaterial.Seconds()      / Stats.GetMaterial.Count        * 1e9)
			.AddNanoseconds("World Up cost per query"      , Stats.GetWorldUp.Seconds()       / Stats.GetWorldUp.Count         * 1e9)
			.AddNanoseconds("Find Biome cost per query"    , Stats.FindBiome.Seconds()        / Stats.FindBiome.Count          * 1e9)
			.AddNanoseconds("Create cost per instance"     , Stats.CreateTransforms.Seconds() / Stats.CreateTransforms.Count   * 1e9)
			.AddNanoseconds("Custom Data cost per instance", Stats.ComputeCustomData.Seconds() / Stats.ComputeCustomData.Count * 1e9)

			.AddSeconds("Height Range", Stats.GetHeightRange_Height.Seconds(), Stats.FindHits.Seconds())

			.Add("GUID", Foliage.Guid.ToString());
			;
		}
	}

	FVoxelSystemUtilities::WriteTables("Voxel Foliage Stats",
	{
		{
			"Ray",
			RayLogTable
		},
		{
			"Height",
			HeightLogTable
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelFoliageDebug::ShouldStoreDebugRays() const
{
	return
		CVarFoliageDebugRays.GetValueOnAnyThread() ||
		CVarFoliageDebugHitsBeforeDensity.GetValueOnAnyThread();
}

void FVoxelFoliageDebug::ReportChunk_AnyThread(
	const FVoxelFoliageProxy& Proxy, 
	const FVoxelIntBox& Bounds,
	const TArray<FVoxelFoliageDebugRay>& Rays,
	const TArray<FVoxelFoliageHit>& Hits, 
	const TVoxelSharedPtr<FVoxelFoliageTransforms>& Transforms)
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	
	if (!EnableDebug())
	{
		return;
	}

	const auto Chunk = MakeVoxelShared<FDebugChunk>();
	Chunk->Proxy = Proxy.AsShared();
	Chunk->Bounds = Bounds;
	Chunk->Rays = Rays;
	Chunk->Hits = Hits;
	Chunk->Transforms = *Transforms;
	QueuedChunks.Enqueue(Chunk);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageDebug::Destroy()
{
	Super::Destroy();

	StopTicking();
}

void FVoxelFoliageDebug::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	const AVoxelWorld* World = Settings.VoxelWorld.Get();
	if (!World || Settings.bDisableDebugManager)
	{
		return;
	}

	{
		TVoxelSharedPtr<FDebugChunk> Chunk;
		while (QueuedChunks.Dequeue(Chunk))
		{
			Chunks.FindOrAdd(Chunk->Proxy).Add(Chunk->Bounds, Chunk);
		}
	}
	Chunks.Remove(nullptr);
	
	if (!EnableDebug())
	{
		Chunks.Empty();
		return;
	}

	const float DebugDT = DeltaTime * 1.5f;

	UVoxelLineBatchComponent& Component = World->GetLineBatchComponent();

	const float VoxelSize = World->VoxelSize;

	const bool bDebugBounds = CVarFoliageDebugBounds.GetValueOnAnyThread();
	const bool bDebugRays = CVarFoliageDebugRays.GetValueOnAnyThread();
	const bool bDebugHitsBeforeDensity = CVarFoliageDebugHitsBeforeDensity.GetValueOnAnyThread();
	const bool bDebugHitsAfterDensity = CVarFoliageDebugHitsAfterDensity.GetValueOnAnyThread();
	const bool bDebugFinalHits = CVarFoliageDebugFinalHits.GetValueOnAnyThread();
	const int32 Step = FMath::Max(1, FMath::CeilToInt(1. / CVarFoliageDebugScaling.GetValueOnAnyThread()));

	for (auto& FoliageIt : Chunks)
	{
		for (auto& ChunkIt : FoliageIt.Value)
		{
			FRandomStream Stream;
			Stream.Initialize(1337);

			const FDebugChunk& Chunk = *ChunkIt.Value;

			const auto Proxy = Chunk.Proxy.Pin();
			if (!ensure(Proxy))
			{
				continue;
			}
			const FVector ChunkPosition(Chunk.Bounds.Min);
			const int32 BoundsSize = Chunk.Bounds.Size().X;

			if (bDebugBounds)
			{
				UVoxelDebugUtilities::DrawDebugIntBox(World, Chunk.Bounds, DebugDT);
			}

			if (bDebugRays || bDebugHitsBeforeDensity)
			{
				for (int32 Index = 0; Index < Chunk.Rays.Num(); Index += Step)
				{
					const FVoxelFoliageDebugRay& Ray = Chunk.Rays[Index];

					if (bDebugRays)
					{
						// Rays start are 4 * BoundsSize off
						// TODO Ray box intersection for more precise debug
						if (Ray.bHit)
						{
							const FVector Start = World->LocalToGlobalFloat(ChunkPosition + Ray.Start + Ray.Direction * 3.5f * BoundsSize);
							const FVector End = World->LocalToGlobalFloat(ChunkPosition + Ray.HitPosition);

							Component.BatchedLines.Add(FBatchedLine(Start, End, FColor::Green, DebugDT, 0, 0));
						}
						else
						{
							const FVector Start = World->LocalToGlobalFloat(ChunkPosition + Ray.Start);
							const FVector End = World->LocalToGlobalFloat(ChunkPosition + Ray.Start + Ray.Direction * 0.5f * BoundsSize);

							Component.BatchedLines.Add(FBatchedLine(Start, End, FColor::Red, DebugDT, 0, 0));
						}
					}

					if (Ray.bHit && bDebugHitsBeforeDensity)
					{
						const FVector Position = World->LocalToGlobalFloat(ChunkPosition + Ray.HitPosition);

						Component.BatchedLines.Add(FBatchedLine(Position, Position + World->GetVoxelTransform().TransformVector(Ray.HitNormal) * VoxelSize, FColor::Magenta, DebugDT, 0, 0));
						Component.BatchedPoints.Add(FBatchedPoint(Position, FColor::Red, VoxelSize / 10, DebugDT, 0));
					}
				}
			}

			if (bDebugHitsAfterDensity)
			{
				for (int32 Index = 0; Index < Chunk.Hits.Num(); Index += Step)
				{
					const FVoxelFoliageHit& Hit = Chunk.Hits[Index];

					const FVector Position = World->LocalToGlobalFloat(ChunkPosition + Hit.LocalPosition);

					Component.BatchedLines.Add(FBatchedLine(Position, Position + World->GetVoxelTransform().TransformVector(Hit.Normal) * VoxelSize, FColor::Green, DebugDT, 0, 0));
					Component.BatchedPoints.Add(FBatchedPoint(Position, FColor::Green, VoxelSize / 10, DebugDT, 0));
				}
			}

			if (bDebugFinalHits)
			{
				for (int32 Index = 0; Index < Chunk.Transforms.Matrices.Num(); Index += Step)
				{
					const FVoxelFoliageMatrix& Matrix = Chunk.Transforms.Matrices[Index];

					const FVector Position = World->GetVoxelTransform().TransformPosition(Matrix.GetCleanMatrix().GetOrigin() + FVector(Chunk.Transforms.TransformsOffset));

					Component.BatchedPoints.Add(FBatchedPoint(Position, FColor::Blue, VoxelSize / 5, DebugDT, 0));
				}
			}
		}
	}
}

bool FVoxelFoliageDebug::EnableDebug() const
{
	return !Settings.bDisableDebugManager && (
		CVarFoliageDebugBounds.GetValueOnAnyThread() ||
		CVarFoliageDebugRays.GetValueOnAnyThread() ||
		CVarFoliageDebugHitsBeforeDensity.GetValueOnAnyThread() ||
		CVarFoliageDebugHitsAfterDensity.GetValueOnAnyThread() ||
		CVarFoliageDebugFinalHits.GetValueOnAnyThread()
		);
}