// Copyright 2021 Phyronnaz

#include "VoxelFoliageGroupSpawner.h"
#include "VoxelFoliageRayHandler.h"
#include "VoxelFoliageUtilities.h"
#include "VoxelFoliageDebug.h"
#include "VoxelFoliageSubsystem.h"
#include "HitGenerators/VoxelRayHitGenerator.h"
#include "HitGenerators/VoxelHeightHitGenerator.h"
#include "VoxelFoliageDensityHelper.h"
#include "VoxelFoliageRandomGenerator.h"
#include "VoxelFoliageCustomDataHelper.h"

#include "VoxelData/VoxelDataIncludes.h"

#define CHECK_CANCELED() if (CancelTracker.IsCanceled()) return {};

TMap<FVoxelFoliageProxyId, TVoxelSharedRef<FVoxelFoliageResultData>> FVoxelFoliageGroupSpawner::Spawn() const
{
	CHECK_CANCELED();
	
	// Find hits: create the hit generator, and run it
	const TArray<FVoxelFoliageHit> AllHits = FindHits();
	TotalStats.NumHits = AllHits.Num();

	if (AllHits.Num() == 0)
	{
		return {};
	}
	
	CHECK_CANCELED();

	// We have a single biome id for all the foliages, representing that biome entry (eg Grass 02)
	// However, we can have multiple groups: one for each of the biome instance foliage
	
	// This works because invalid group id are not allowed, so unrelated foliages can't get merged here
	const TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageHit>> HitsPerGroup = SplitHitsByBiome(AllHits);
	
	CHECK_CANCELED();
	
	TMap<FVoxelFoliageProxy*, TArray<FVoxelFoliageHit>> HitsPerFoliage;
	for (auto& It : HitsPerGroup)
	{
		SplitGroupHits(HitsPerFoliage, It.Key, It.Value);
	}
	
	CHECK_CANCELED();

	TMap<FVoxelFoliageProxyId, TVoxelSharedRef<FVoxelFoliageResultData>> Result;
	for (FVoxelFoliageProxy* Foliage : Foliages)
	{
		VOXEL_ASYNC_SCOPE_COUNTER("Foliage");
		CHECK_CANCELED();
		
		TArray<FVoxelFoliageHit>& Hits = HitsPerFoliage.FindOrAdd(Foliage);
		if (Hits.Num() == 0)
		{
			continue;
		}
		
		FVoxelScopedFoliageStats FoliageStats(*Foliage);
		VOXEL_SCOPED_STAT(FoliageStats.TotalSelf, 1);

		FoliageStats.NumHitsAfterBiomes = Hits.Num();
		
		ApplyDensity(*Foliage, FoliageStats, Hits);
		
		FoliageStats.NumHitsAfterDensity = Hits.Num();
		
		if (SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height)
		{
			CheckFloating_Height(FoliageStats, Hits);
			ComputeGradient_Height(FoliageStats, Hits);
		}
		
		FoliageStats.NumHitsAfterFloating_Height = Hits.Num();
		
		ComputeWorldUp(FoliageStats, Hits);

		if (Foliage->NeedCanSpawn())
		{
			CheckCanSpawn(*Foliage, FoliageStats, Hits);
		}
		
		FoliageStats.NumHitsAfterCanSpawn = Hits.Num();
		
		const auto FoliageData = MakeVoxelShared<FVoxelFoliageResultData>();
		CreateTransforms(FoliageData->Transforms, *Foliage, FoliageStats, Hits);
		ComputeCustomData(FoliageData->CustomData, *Foliage, FoliageStats, Hits);

		Result.Add(Foliage->UniqueId, FoliageData );	
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<FVoxelFoliageHit> FVoxelFoliageGroupSpawner::FindHits() const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(TotalStats.FindHits, 1);

	TArray<FVoxelFoliageHit> Hits;
	
	const FVoxelHitGenerator::FParameters Parameters{ TotalStats, Bounds, Settings, Accelerator, CancelTracker, RandomGenerator, SpawnSettings };

	if (SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height)
	{
		FVoxelHeightHitGenerator HitGenerator(Parameters);
		Hits = HitGenerator.Generate();
	}
	else
	{
		ensure(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Ray);
		check(RayHandler);

		FVoxelRayHitGenerator HitGenerator(Parameters, *RayHandler);
		Hits = HitGenerator.Generate();
	}
	
	{
		VOXEL_ASYNC_SCOPE_COUNTER("Bounds Check");

		Hits.RemoveAllSwap([&](const FVoxelFoliageHit& Hit)
		{
			const FVoxelVector Position = Hit.GetPosition(Bounds.Min);
			return !Bounds.ContainsFloat(Position) || !Accelerator.Data.IsInWorld(Position);
		});
	}

	return Hits;
}

TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageHit>> FVoxelFoliageGroupSpawner::SplitHitsByBiome(const TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(TotalStats.FindBiome, Hits.Num());
	
	if (BaseBiome.bDummyBiome)
	{
		const FVoxelFoliageGroupId GroupId = Foliages[0]->Group.Id;
		
		// No biomes - should be a single group
		for (const FVoxelFoliageProxy* Foliage : Foliages)
		{
			ensure(Foliage->Biome.OutputName.IsNone());
			ensure(Foliage->Biome.OutputValue == -1);
			ensure(Foliage->Group.Id == GroupId);
		}
		
		return { { GroupId, Hits } };
	}
	else
	{
		const FName BiomeOutputName = Foliages[0]->Biome.OutputName;
		ensure(!BiomeOutputName.IsNone());

		TMap<int32, FVoxelFoliageGroupId> OutputValueToGroup;
		for (FVoxelFoliageProxy* Foliage : Foliages)
		{
			const int32 Key = Foliage->Biome.OutputValue;
			const FVoxelFoliageGroupId Value = Foliage->Group.Id;
			ensure(Key != -1);
			
			if (const FVoxelFoliageGroupId* ExistingValue = OutputValueToGroup.Find(Key))
			{
				ensure(Value == *ExistingValue);
			}
			else
			{
				OutputValueToGroup.Add(Key, Value);
			}
		}

		TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageHit>> HitsPerGroup;
		for (const FVoxelFoliageHit& Hit : Hits)
		{
			const int32 BiomeIndex = Accelerator.GetCustomOutput<int32>(0, BiomeOutputName, FVoxelVector(Bounds.Min) + Hit.LocalPosition, SpawnSettings.GetLOD());
			if (const FVoxelFoliageGroupId* Id = OutputValueToGroup.Find(BiomeIndex))
			{
				HitsPerGroup.FindOrAdd(*Id).Add(Hit);
			}
		}
		return HitsPerGroup;
	}
}

void FVoxelFoliageGroupSpawner::SplitGroupHits(TMap<FVoxelFoliageProxy*, TArray<FVoxelFoliageHit>>& OutHits, FVoxelFoliageGroupId GroupId, const TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(TotalStats.SplitGroup, Hits.Num());
	ensure(GroupId.IsValid());

	const TArray<FVoxelFoliageProxy*>& GroupFoliages = GroupToFoliages[GroupId];
	ensure(GroupFoliages.Num() > 0);

	if (GroupFoliages.Num() == 1)
	{
		// No need to do complex logic here
		ensure(GroupFoliages[0]->Group.Id == GroupId);
		OutHits.FindOrAdd(GroupFoliages[0]).Append(Hits);
		return;
	}

	struct FEntry
	{
		double Value = 0;
		FVoxelFoliageProxy* Foliage = nullptr;
	};
	TArray<FEntry> Entries;

	for (FVoxelFoliageProxy* Foliage : GroupFoliages)
	{
		ensure(Foliage->Group.Id == GroupId);
		Entries.Add(FEntry{ FMath::Max(0.f, Foliage->Group.Strength), Foliage });
	}

	// Normalize
	{
		double TotalValue = 0;
		for (const FEntry& Entry : Entries)
		{
			TotalValue += Entry.Value;
		}

		if (FMath::IsNearlyZero(TotalValue))
		{
			// Invalid probabilities
			return;
		}

		for (FEntry& Entry : Entries)
		{
			Entry.Value /= TotalValue;
		}
	}

	// Compute running sum
	{
		double TotalValue = 0;
		for (FEntry& Entry : Entries)
		{
			TotalValue += Entry.Value;
			Entry.Value = TotalValue;
		}
		ensure(FMath::IsNearlyEqual(TotalValue, 1));

		// Avoid silly precision errors when comparing the fraction against the last entry
		Entries.Last().Value = 1.01f;
	}

	// Finally, split the hits according to RNG
	for (const FVoxelFoliageHit& Hit : Hits)
	{
		const double Fraction = RandomGenerator.GetGroupSplitFraction();
		ensureVoxelSlow(Fraction <= Entries.Last().Value);
		for (const FEntry& Entry : Entries)
		{
			if (Fraction <= Entry.Value)
			{
				OutHits.FindOrAdd(Entry.Foliage).Add(Hit);
				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageGroupSpawner::ApplyDensity(const FVoxelFoliageProxy& Foliage, FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.ApplyDensity, Hits.Num());

	if (Foliage.Densities.Num() == 0)
	{
		return;
	}
	
	Hits.RemoveAllSwap([&](const FVoxelFoliageHit& Hit)
	{
		const FVoxelFoliageDensityQuery Query(DensityCache, Hit.GetPosition(Bounds.Min), Foliage, Settings);
		return RandomGenerator.GetDensityFraction() > Query.GetDensity();
	});
}

void FVoxelFoliageGroupSpawner::CheckFloating_Height(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.CheckFloating_Height, Hits.Num());
	ensure(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height);

	if (!SpawnSettings.bCheckIfFloating_HeightOnly && !SpawnSettings.bCheckIfCovered_HeightOnly)
	{
		return;
	}
	
	Hits.RemoveAllSwap([&](const FVoxelFoliageHit& Hit)
	{
		if (SpawnSettings.bCheckIfFloating_HeightOnly)
		{
			const v_flt InsideValue = Accelerator.GetFloatValue(Hit.HeightFloating.InsideSurface, 0);
			if (InsideValue > 0)
			{
				return true;
			}
		}

		if (SpawnSettings.bCheckIfCovered_HeightOnly)
		{
			const v_flt OutsideValue = Accelerator.GetFloatValue(Hit.HeightFloating.OutsideSurface, 0);
			if (OutsideValue < 0)
			{
				return true;
			}
		}

		return false;
	});
}

void FVoxelFoliageGroupSpawner::ComputeGradient_Height(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.ComputeGradient_Height, Hits.Num());
	ensure(SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height);

	const FVoxelBasis Basis = FVoxelFoliageUtilities::GetBasisFromBounds(Settings, Bounds);

	const auto GetHeight = [&](const FVoxelVector& Position)
	{
		VOXEL_SCOPED_STAT(Stats.GetHeight_Height, 1);
		
		// Can't use the octree for height, as it's always near Z = 0 (or 0 0 0 for sphere) and not in the locked bounds
		return Accelerator.Data.Generator->GetCustomOutput<v_flt>(
			0,
			SpawnSettings.HeightGraphOutputName_HeightOnly,
			Position.X,
			Position.Y,
			Position.Z,
			0,
			FVoxelItemStack::Empty);
	};

	for (FVoxelFoliageHit& Hit : Hits)
	{
		// Value = Z - Height
		// D(Value) = D(Z) - D(Height)
		const FVoxelVector Gradient =
			Basis.Z
			- Basis.X * (GetHeight(Hit.HeightGradient.MaxX) - GetHeight(Hit.HeightGradient.MinX)) / 2
			- Basis.Y * (GetHeight(Hit.HeightGradient.MaxY) - GetHeight(Hit.HeightGradient.MinY)) / 2;
		Hit.Normal = Gradient.GetSafeNormal();
	}
}

void FVoxelFoliageGroupSpawner::ComputeWorldUp(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.GetWorldUp, Hits.Num());
	
	for (FVoxelFoliageHit& Hit : Hits)
	{
		Hit.WorldUp = Data.Generator->GetUpVector(Hit.GetPosition(Bounds.Min));
	}
}

void FVoxelFoliageGroupSpawner::CheckCanSpawn(const FVoxelFoliageProxy& Foliage, FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.CanSpawn, Hits.Num());
	ensure(Foliage.NeedCanSpawn());
	
	Hits.RemoveAllSwap([&](const FVoxelFoliageHit& Hit)
	{
		return !Foliage.CanSpawn(RandomGenerator, Hit.GetPosition(Bounds.Min), Hit.Normal, Hit.WorldUp);
	});
}

void FVoxelFoliageGroupSpawner::CreateTransforms(
	FVoxelFoliageTransforms& Transforms,
	const FVoxelFoliageProxy& Foliage,
	FVoxelFoliageStats& Stats,
	TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.CreateTransforms, Hits.Num());

	Transforms.TransformsOffset = Settings.ComputeFoliageTransformsOffset(Bounds);
	Transforms.Matrices.Reserve(Hits.Num());
	
	for (const FVoxelFoliageHit& Hit : Hits)
	{
		const FVector RelativeGlobalPosition = Settings.VoxelSize * (Hit.LocalPosition + FVector(Bounds.Min - Transforms.TransformsOffset));
		const FMatrix Transform = Foliage.GetTransform(RandomGenerator, Hit.Normal, Hit.WorldUp, RelativeGlobalPosition);

		FVoxelFoliageMatrix FoliageMatrix(Transform);
		FoliageMatrix.SetPositionOffset(RelativeGlobalPosition - Transform.GetOrigin() + Transform.TransformVector(Foliage.FloatingDetectionOffset));
		
		// This ensures the random id is always the same across different sessions
		FoliageMatrix.SetRandomInstanceId(RandomGenerator.GetRandomInstanceIdFraction());

		Transforms.Matrices.Add(FoliageMatrix);
	}

	Transforms.Matrices.Shrink();
}

void FVoxelFoliageGroupSpawner::ComputeCustomData(
	TArray<float>& CustomData, 
	const FVoxelFoliageProxy& Foliage, 
	FVoxelFoliageStats& Stats, 
	TArray<FVoxelFoliageHit>& Hits) const
{
	VOXEL_ASYNC_FUNCTION_COUNTER();
	VOXEL_SCOPED_STAT(Stats.ComputeCustomData, Hits.Num());

	if (Foliage.CustomDatas.Num() == 0)
	{
		return;
	}

	CustomData.Reserve(Hits.Num() * Foliage.CustomDatas.Num());
	
	for (const FVoxelFoliageHit& Hit : Hits)
	{
		FVoxelFoliageCustomDataQuery Query(DensityCache, Hit.GetPosition(Bounds.Min), Foliage, Settings);

		for (const FVoxelFoliageCustomData& Channel : Foliage.CustomDatas)
		{
			CustomData.Add(Query.GetCustomData(Channel));
		}
	}

	CustomData.Shrink();
}

#undef CHECK_CANCELED