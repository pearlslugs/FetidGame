// Copyright 2021 Phyronnaz

#include "HitGenerators/VoxelHeightHitGenerator.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageRandomGenerator.h"

#include "VoxelBasis.h"
#include "VoxelCancelCounter.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "Misc/ScopeExit.h"

TArray<FVoxelFoliageHit> FVoxelHeightHitGenerator::GenerateImpl(int32 NumRays)
{
	VOXEL_ASYNC_FUNCTION_COUNTER();

	check(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height);

	// Note: assets are ignored when querying the height and the density, as it gets way too messy
	// For flat worlds, the height and the density is queried at Z = 0 if density is computed first, or Z = Height if height first
	// For sphere worlds at a normalized (X, Y, Z) if density is computed first, or normalized (X, Y, Z) * Height if height first
	// In theory the density could be computed at the exact position if bComputeDensityFirst is false, but this makes the behavior unpredictable

	const int32 ChunkSize = SpawnSettings.GetChunkSize();
	
	ensure(Bounds.Size().X == ChunkSize);
	ensure(Bounds.Size().Y == ChunkSize);
	ensure(Bounds.Size().Z == ChunkSize);

	const FVoxelIntBox BoundsLimit = Bounds.Overlap(Accelerator.Data.WorldBounds);

	const FIntVector& ChunkPosition = Bounds.Min;
	const bool bIsSphere = Settings.FoliageWorldType == EVoxelFoliageWorldType::Planet;

	// This is the value to use if the generator doesn't have the custom output.
	constexpr v_flt DefaultHeight = 0;

	const auto GetHeight = [&](const FVoxelVector& Position)
	{
		VOXEL_SCOPED_STAT(Stats.GetHeight_Height, 1);

		// Can't use the octree for height, as it's always near Z = 0 (or 0 0 0 for sphere) and not in the locked bounds
		return Accelerator.Data.Generator->GetCustomOutput<v_flt>(
			DefaultHeight,
			SpawnSettings.HeightGraphOutputName_HeightOnly,
			Position.X,
			Position.Y,
			Position.Z,
			0,
			FVoxelItemStack::Empty);
	};
	
	{
		VOXEL_SCOPED_STAT(Stats.GetHeightRange_Height, 1);
		
		// Note: might be expensive!
		const auto Range = Accelerator.Data.GetCustomOutputRange(
			DefaultHeight,
			SpawnSettings.HeightGraphOutputName_HeightOnly,
			Bounds,
			0);

		if (bIsSphere)
		{
			auto Corners = Bounds.GetCorners(0);
			if (!Range.Intersects(TVoxelRange<v_flt>::FromList(
				FVoxelVector(Corners[0]).Size(),
				FVoxelVector(Corners[1]).Size(),
				FVoxelVector(Corners[2]).Size(),
				FVoxelVector(Corners[3]).Size(),
				FVoxelVector(Corners[4]).Size(),
				FVoxelVector(Corners[5]).Size(),
				FVoxelVector(Corners[6]).Size(),
				FVoxelVector(Corners[7]).Size())))
			{
				return {};
			}
		}
		else
		{
			if (!Range.Intersects(TVoxelRange<v_flt>(Bounds.Min.Z, Bounds.Max.Z)))
			{
				return {};
			}
		}
	}

	Stats.NumRays = NumRays;
	
	TArray<FVoxelFoliageHit> OutHits;
	for (int32 Index = 0; Index < NumRays; Index++)
	{
		if ((Index & 0xFF) == 0 && CancelTracker.IsCanceled())
		{
			return {};
		}

		if (bIsSphere)
		{
			const FVector2D RandomValue = 2 * RandomGenerator.GetPosition() - 1; // Map from 0,1 to -1,1
			const auto SamplePosition = [&](v_flt X, v_flt Y)
			{
				return (Bounds.GetCenter() + ChunkSize / 2.f * (Basis.X * X + Basis.Y * Y)).GetSafeNormal();
			};
			const FVoxelVector Start = SamplePosition(RandomValue.X, RandomValue.Y);

			const v_flt Height = GetHeight(Start);
			const FVoxelVector Position = Start * Height;
			if (!BoundsLimit.ContainsFloat(Position))
			{
				continue;
			}

			FVoxelFoliageHit Hit;
			Hit.LocalPosition = Position - ChunkPosition;
			
			// Will cancel out in SamplePosition
			const v_flt Delta = 1. / ChunkSize;

			Hit.HeightGradient.MinX = SamplePosition(RandomValue.X - Delta, RandomValue.Y);
			Hit.HeightGradient.MaxX = SamplePosition(RandomValue.X + Delta, RandomValue.Y);
			Hit.HeightGradient.MinY = SamplePosition(RandomValue.X, RandomValue.Y - Delta);
			Hit.HeightGradient.MaxY = SamplePosition(RandomValue.X, RandomValue.Y + Delta);

			Hit.HeightFloating.InsideSurface = Start * (Height - 1);
			Hit.HeightFloating.OutsideSurface = Start * (Height + 1);

			OutHits.Add(Hit);
		}
		else
		{
			const FVector2D LocalPosition = RandomGenerator.GetPosition() * ChunkSize;
			const v_flt X = v_flt(LocalPosition.X) + Bounds.Min.X;
			const v_flt Y = v_flt(LocalPosition.Y) + Bounds.Min.Y;

			const v_flt Z = GetHeight({ X, Y, 0 });
			if (BoundsLimit.Min.Z > Z || Z > BoundsLimit.Max.Z)
			{
				continue;
			}
			ensure(BoundsLimit.Min.Z <= Z && Z <= BoundsLimit.Max.Z);
			
			FVoxelFoliageHit Hit;
			Hit.LocalPosition = FVoxelVector(X, Y, Z) - ChunkPosition;

			Hit.HeightGradient.MinX = { X - 1, Y, Z };
			Hit.HeightGradient.MaxX = { X + 1, Y, Z };
			Hit.HeightGradient.MinY = { X, Y - 1, Z };
			Hit.HeightGradient.MaxY = { X, Y + 1, Z };

			Hit.HeightFloating.InsideSurface = { X, Y, Z - 1 };
			Hit.HeightFloating.OutsideSurface = { X, Y , Z + 1 };

			OutHits.Add(Hit);
		}
	}

	return OutHits;
}