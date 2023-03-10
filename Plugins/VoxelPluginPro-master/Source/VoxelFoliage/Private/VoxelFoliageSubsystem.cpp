// Copyright 2021 Phyronnaz

#include "VoxelFoliageSubsystem.h"

#include "VoxelMessages.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelEvents/VoxelEventManager.h"
#include "VoxelGenerators/VoxelGeneratorCache.h"

#include "VoxelFoliageSave.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageBiome.h"
#include "VoxelFoliageMatrix.h"
#include "VoxelFoliageCollection.h"

#include "VoxelUtilities/VoxelThreadingUtilities.h"
#include "VoxelUtilities/VoxelGeneratorUtilities.h"
#include "VoxelUtilities/VoxelSerializationUtilities.h"

#include "Misc/UObjectToken.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/LargeMemoryReader.h"
#include "Serialization/LargeMemoryWriter.h"

DEFINE_VOXEL_SUBSYSTEM_PROXY(UVoxelFoliageSubsystemProxy);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageSubsystem::PostCreate(const IVoxelSubsystem* OldSubsystem)
{
	VOXEL_FUNCTION_COUNTER();
	Super::PostCreate(OldSubsystem);

	if (OldSubsystem)
	{
		// This is needed to regenerate spawners without regenerating the entire generator
		ensure(BiomeTypes.Num() == 0);
		BiomeTypes = static_cast<const FVoxelFoliageSubsystem*>(OldSubsystem)->BiomeTypes;
	}

	auto& Data = GetSubsystemChecked<FVoxelData>();
	
	const auto AddFoliage = [&](
		const UObject* Container, 
		int32 FoliageIndex, 
		UVoxelFoliage& Foliage,
		const FVoxelFoliageProxy::FInit& BaseInit)
	{
		if (!Foliage.Guid.IsValid())
		{
			Foliage.Guid = FGuid::NewGuid();
			Foliage.MarkPackageDirty();
			
			const auto NewMessage = FTokenizedMessage::Create(EMessageSeverity::Warning);
			NewMessage->AddToken(FTextToken::Create(VOXEL_LOCTEXT("Foliage has an invalid GUID: ")));
			NewMessage->AddToken(FUObjectToken::Create(Container));
			NewMessage->AddToken(FTextToken::Create(FText::Format(VOXEL_LOCTEXT("(index {0}). A new one has been assigned to it."), FoliageIndex)));
			FVoxelMessages::LogMessage(NewMessage, EVoxelShowNotification::Hide);
		}

		const FFoliageProxyInfo* ExistingGuid = GuidToProxies.Find(Foliage.Guid);
		if (ExistingGuid)
		{
			if (ExistingGuid->Container == Container && ExistingGuid->FoliageIndex == FoliageIndex)
			{
				const auto NewMessage = FTokenizedMessage::Create(EMessageSeverity::Warning);
				NewMessage->AddToken(FTextToken::Create(VOXEL_LOCTEXT("Foliage is used twice: ")));
				NewMessage->AddToken(FUObjectToken::Create(ExistingGuid->Container.Get()));
				NewMessage->AddToken(FTextToken::Create(FText::Format(VOXEL_LOCTEXT("(foliage #{0})"), ExistingGuid->FoliageIndex)));
				FVoxelMessages::LogMessage(NewMessage, EVoxelShowNotification::Hide);
			}
			else
			{
				Foliage.Guid = FGuid::NewGuid();
				Foliage.MarkPackageDirty();

				const auto NewMessage = FTokenizedMessage::Create(EMessageSeverity::Warning);
				NewMessage->AddToken(FTextToken::Create(VOXEL_LOCTEXT("Foliage GUID collision: ")));
				NewMessage->AddToken(FUObjectToken::Create(ExistingGuid->Container.Get()));
				NewMessage->AddToken(FTextToken::Create(FText::Format(VOXEL_LOCTEXT("(foliage #{0}) and "), ExistingGuid->FoliageIndex)));
				NewMessage->AddToken(FUObjectToken::Create(Container));
				NewMessage->AddToken(FTextToken::Create(FText::Format(VOXEL_LOCTEXT("(foliage #{0}) have the same GUID. The second foliage will have a new GUID assigned to it."), FoliageIndex)));
				FVoxelMessages::LogMessage(NewMessage, EVoxelShowNotification::Hide);
			}
		}

		const FVoxelFoliageGroupId GroupId = FVoxelFoliageGroupId::New();
		
		TArray<FVoxelFoliageProxy::FInit> Inits;
		for (const FVoxelFoliageMesh& Mesh : Foliage.Meshes)
		{
			FVoxelFoliageProxy::FInit Init = BaseInit;
			Init.MeshKey = { Mesh.Mesh, Mesh.Materials, Foliage.ActorClass, Foliage.InstanceSettings, Foliage.CustomDatas.Num() };
			Init.Group.Id = GroupId;
			Init.Group.Strength = Mesh.Strength;
			Init.Name += FString::Printf(TEXT(" Mesh %d"), Inits.Num());
			Inits.Add(Init);
		}

		for (const FVoxelFoliageProxy::FInit& Init : Inits)
		{
			if (!Init.MeshKey.Mesh)
			{
				continue;
			}
			
			const auto FoliageProxy = MakeVoxelShared<FVoxelFoliageProxy>(Foliage, *this, Init);

			TVoxelSharedPtr<FVoxelFoliageBucket> Bucket;
			{
				FVoxelFoliageBucketKey BucketKey;
				BucketKey.LOD = FoliageProxy->SpawnSettings.GetLOD();
				BucketKey.GenerationDistanceInChunks = FoliageProxy->SpawnSettings.GetGenerationDistanceInChunks(Settings.VoxelSize);
				BucketKey.bInfiniteGenerationDistance = FoliageProxy->SpawnSettings.bInfiniteGenerationDistance;
				BucketKey.SpawnType = FoliageProxy->SpawnSettings.SpawnType;

				Bucket = Buckets.FindRef(BucketKey);
				if (!Bucket)
				{
					Bucket = Buckets.Add(BucketKey, MakeVoxelShared<FVoxelFoliageBucket>(BucketKey));
				}
			}

			const FFoliageProxyInfo FoliageProxyInfo{ Container, FoliageIndex, Bucket, FoliageProxy };
			GuidToProxies.Add(FoliageProxy->Guid, FoliageProxyInfo);

			Bucket->Foliages.Add(FoliageProxy->UniqueId, FoliageProxy);
		}
	};

	for (TWeakObjectPtr<UVoxelFoliageCollectionBase> WeakCollection : Settings.FoliageCollections)
	{
		auto* Collection = Cast<UVoxelFoliageCollection>(WeakCollection.Get());
		if (!Collection)
		{
			continue;
		}

		for (int32 FoliageIndex = 0; FoliageIndex < Collection->Foliages.Num(); FoliageIndex++)
		{
			UVoxelFoliage* Foliage = Collection->Foliages[FoliageIndex];
			if (Foliage)
			{
				FVoxelFoliageProxy::FInit Init;
				Init.Name = Foliage->GetName();
				Init.Biome.Id = FVoxelFoliageBiomeId::New();
				Init.Biome.bDummyBiome = true;
				AddFoliage(Collection, FoliageIndex, *Foliage, Init);
			}
		}
	}
	for (auto& TypeIt : BiomeTypes)
	{
		UVoxelFoliageBiomeType* BiomeType = TypeIt.Key.Get();
		if (!BiomeType)
		{
			continue;
		}

		if (BiomeType->BiomeOutput.IsNone())
		{
			FVoxelMessages::Warning("BiomeOutput is empty!", BiomeType);
			continue;
		}
		if (!Data.Generator->GetOutputsPtrMap<int32>().Contains(BiomeType->BiomeOutput))
		{
			FVoxelMessages::Warning(FVoxelUtilities::GetMissingGeneratorOutputErrorString<v_flt>(BiomeType->BiomeOutput, *Data.Generator), BiomeType);
		}

		TArray<FVoxelFoliageBiomeId> BiomeIds;
		for (auto& Entry : BiomeType->Entries)
		{
			BiomeIds.Add(FVoxelFoliageBiomeId::New());
		}

		for (auto& BiomeIt : TypeIt.Value)
		{
			const int32 BiomeIndex = BiomeIt.Key;
			UVoxelFoliageBiome* Biome = BiomeIt.Value.Get();
			if (!Biome)
			{
				continue;
			}
			Biome->Fixup();
			
			for (int32 EntryIndex = 0; EntryIndex < BiomeType->Entries.Num(); EntryIndex++)
			{
				const FVoxelFoliageBiomeId BiomeId = BiomeIds[EntryIndex];
				const FVoxelFoliageBiomeTypeEntry& TypeEntry = BiomeType->Entries[EntryIndex];
				
				const FVoxelFoliageBiomeEntry* BiomeEntry = nullptr;
				for (const FVoxelFoliageBiomeEntry& Entry : Biome->Entries)
				{
					if (Entry.Name == TypeEntry.Name)
					{
						BiomeEntry = &Entry;
						break;
					}
				}

				if (!BiomeEntry || !BiomeEntry->Foliage)
				{
					continue;
				}
				UVoxelFoliage& Foliage = *BiomeEntry->Foliage;

				FVoxelFoliageProxy::FInit Init;
				Init.Name = FString::Printf(TEXT("%s (%s:%s)"), *Foliage.GetName(), *BiomeType->GetName(), *Biome->GetName());
				Init.SeedOverride = TypeEntry.Seed;
				Init.SpawnSettingsOverride = TypeEntry.SpawnSettings;

				Init.Biome.Id = BiomeId;
				Init.Biome.bDummyBiome = false;
				Init.Biome.OutputName = BiomeType->BiomeOutput;
				Init.Biome.OutputValue = BiomeIndex;

				AddFoliage(Biome, EntryIndex, Foliage, Init);
			}
		}
	}

	// Then bind delegates. This separation is needed as BindEvent might fire the delegate now.
	for (auto& It : Buckets)
	{
		FVoxelFoliageBucket& Bucket = *It.Value;

		if (Bucket.Key.bInfiniteGenerationDistance)
		{
			TArray<FVoxelIntBox> Children;
			const int32 MaxChildren = 1 << 20;
			if (!Data.WorldBounds.Subdivide(Bucket.ChunkSize, Children, MaxChildren))
			{
				// If we have more than a million chunks, something went terribly wrong
				FVoxelMessages::Error(FString::Printf(TEXT("bInfiniteGenerationDistance = true: trying to spawn more than %d chunks. Abording"), MaxChildren), Settings.Owner.Get());
				continue;
			}
			for (auto& Child : Children)
			{
				Bucket.Spawn(Child, *this);
			}
		}
		else
		{
			// Bind generation delegates
			GetSubsystemChecked<FVoxelEventManager>().BindEvent(
				true,
				Bucket.ChunkSize,
				Bucket.Key.GenerationDistanceInChunks,
				MakeVoxelWeakPtrDelegate(&Bucket, [this, &Bucket](const FVoxelIntBox& Bounds)
				{
					Bucket.Spawn(Bounds, *this);
				}),
				MakeVoxelWeakPtrDelegate(&Bucket, [this, &Bucket](const FVoxelIntBox& Bounds)
				{
					Bucket.Despawn(Bounds, *this);
				}));
		}
	}
}

void FVoxelFoliageSubsystem::Destroy()
{
	Super::Destroy();
	
	Buckets.Reset();

	StopTicking();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelFoliageBiomeToGenerate
{
	FVoxelFoliageBucket* Bucket = nullptr;
	FVoxelIntBox ChunkBounds;
	FVoxelFoliageBiomeId BiomeId;

	bool operator==(const FVoxelFoliageBiomeToGenerate& Other) const
	{
		return Bucket == Other.Bucket
			&& ChunkBounds == Other.ChunkBounds
			&& BiomeId == Other.BiomeId;
	}
	friend uint32 GetTypeHash(FVoxelFoliageBiomeToGenerate Biome)
	{
		return HashCombine(HashCombine(GetTypeHash(Biome.Bucket), GetTypeHash(Biome.ChunkBounds)), GetTypeHash(Biome.BiomeId));
	}
};

void FVoxelFoliageSubsystem::Regenerate(const FVoxelIntBox& Bounds)
{
	VOXEL_FUNCTION_COUNTER();

	TSet<FVoxelFoliageBiomeToGenerate> ChunksToSpawn;
	
	IterateChunksInBounds(
		Bounds,
		[&](FVoxelFoliageBucket& Bucket, const FVoxelIntBox& ChunkBounds, FVoxelFoliageProxyId ProxyId, FVoxelFoliageChunk::FFoliageData& Data)
		{
			if (Data.FoliageResult.IsValid() && !Data.FoliageResult->NeedsToBeSaved())
			{
				if (Data.FoliageResult->IsCreated())
				{
					Data.FoliageResult->Destroy(*this);
					ChunksToSpawn.Add({ &Bucket, ChunkBounds, Bucket.Foliages[ProxyId]->Biome.Id });
				}
				
				Data.CancelTask();
				Data.FoliageResult.Reset();
			}
		});

	for (const FVoxelFoliageBiomeToGenerate& Biome : ChunksToSpawn)
	{
		Biome.Bucket->Spawn(Biome.ChunkBounds, *this, &Biome.BiomeId);
	}
}

void FVoxelFoliageSubsystem::MarkDirty(const FVoxelIntBox& Bounds)
{
	VOXEL_FUNCTION_COUNTER();

	IterateChunksInBounds(
		Bounds,
		[](FVoxelFoliageBucket& Bucket, const FVoxelIntBox& ChunkBounds, FVoxelFoliageProxyId ProxyId, FVoxelFoliageChunk::FFoliageData& Data)
		{
			if (Data.FoliageResult.IsValid())
			{
				Data.FoliageResult->MarkDirty();
			}
		});
}

void FVoxelFoliageSubsystem::Serialize(FArchive& Ar, FVoxelFoliageSaveVersion::Type Version)
{
	VOXEL_FUNCTION_COUNTER();

	if (Version < FVoxelFoliageSaveVersion::SpawnerRefactor)
	{
		FVoxelMessages::Error("Trying to load a legacy foliage save that's not supported anymore");
		return;
	}
	
	if (Ar.IsSaving())
	{
		check(Version == FVoxelFoliageSaveVersion::LatestVersion);

		struct FGuidInfo
		{
			const FVoxelFoliageProxy* Proxy = nullptr;
			TArray<FIntVector> Positions;
			TArray<TVoxelSharedPtr<FVoxelFoliageProxyResult>> Results;
		};
		TMap<FGuid, FGuidInfo> GuidInfos;

		// Expand buckets to get FoliageGuid -> Results
		for (auto& BucketIt : Buckets)
		{
			for (auto& ChunkIt : BucketIt.Value->Chunks)
			{
				const FIntVector& ChunkPosition = ChunkIt.Key;

				for (auto& FoliageIt : ChunkIt.Value->Foliages)
				{
					const TVoxelSharedPtr<FVoxelFoliageProxyResult>& FoliageResult = FoliageIt.Value.FoliageResult;
					if (!FoliageResult || !FoliageResult->NeedsToBeSaved())
					{
						continue;
					}
					
					FGuidInfo& GuidInfo = GuidInfos.FindOrAdd(FoliageResult->Proxy.Guid);

					ensure(!GuidInfo.Proxy || GuidInfo.Proxy == &FoliageResult->Proxy);
					GuidInfo.Proxy = &FoliageResult->Proxy;

					GuidInfo.Positions.Add(ChunkPosition);
					GuidInfo.Results.Add(FoliageResult);
				}
			}
		}

		int32 NumGuid = GuidInfos.Num();
		Ar << NumGuid;

		// Now serialize each GUID
		for (auto& It : GuidInfos)
		{
			FGuid Guid = It.Key;
			FGuidInfo& GuidInfo = It.Value;

			// Create one archive per GUID to be safer
			FBufferArchive GuidArchive;
			{
				int32 NumProxyResults = GuidInfo.Positions.Num();
				check(GuidInfo.Results.Num() == NumProxyResults);
				check(NumProxyResults > 0);
				
				GuidArchive << NumProxyResults;
				GuidArchive.Serialize(GuidInfo.Positions.GetData(), NumProxyResults * sizeof(FIntVector));

				for (auto& Result : GuidInfo.Results)
				{
					// Make sure not to save any removed foliage
					Result->ApplyRemovedIndices(*this);
					
					FVoxelIntBox Bounds = Result->Bounds;
					GuidArchive << Bounds;
					GuidArchive << Result->Data->Transforms;
					GuidArchive << Result->Data->CustomData;
				}
			}

			Ar << Guid;

			int32 GuidArchiveNum = GuidArchive.Num();
			Ar << GuidArchiveNum;

			Ar.Serialize(GuidArchive.GetData(), GuidArchiveNum * sizeof(uint8));
		}
	}
	else
	{
		check(Ar.IsLoading());

		int32 NumGuid = -1;
		Ar << NumGuid;
		check(NumGuid >= 0);

		// Deserialize each GUID
		for (int32 GuidIndex = 0; GuidIndex < NumGuid; GuidIndex++)
		{
			FGuid Guid;
			TArray<uint8> GuidArchiveData;
			{
				Ar << Guid;

				int32 GuidArchiveNum = -1;
				Ar << GuidArchiveNum;
				check(GuidArchiveNum >= 0);

				GuidArchiveData.SetNumUninitialized(GuidArchiveNum);
				Ar.Serialize(GuidArchiveData.GetData(), GuidArchiveNum * sizeof(uint8));
			}
			
			const FFoliageProxyInfo* ProxyInfo = GuidToProxies.Find(Guid);
			if (!ensure(ProxyInfo))
			{
				FVoxelMessages::Error(FString::Printf(TEXT("Voxel Foliage Serialization: Loading: Foliage with GUID %s not found, skipping it"), *Guid.ToString()), Settings.Owner.Get());
				ensure(false);
				continue;
			}

			const TVoxelSharedPtr<FVoxelFoliageBucket> Bucket = ProxyInfo->Bucket.Pin();
			const TVoxelSharedPtr<FVoxelFoliageProxy> FoliageProxy = ProxyInfo->FoliageProxy.Pin();
			if (!ensure(Bucket) || !ensure(FoliageProxy))
			{
				continue;
			}
			
			TArray<FIntVector> Positions;
			TArray<TVoxelSharedPtr<FVoxelFoliageProxyResult>> ProxyResults;

			// Load GUID archive
			{
				FMemoryReader GuidArchive(GuidArchiveData);
				
				int32 NumProxyResults = -1;
				GuidArchive << NumProxyResults;
				check(NumProxyResults > 0);

				Positions.SetNumUninitialized(NumProxyResults);
				GuidArchive.Serialize(Positions.GetData(), NumProxyResults * sizeof(FIntVector));

				ProxyResults.Reserve(NumProxyResults);
				for (int32 ResultIndex = 0; ResultIndex < NumProxyResults; ResultIndex++)
				{
					FVoxelIntBox Bounds;
					const auto FoliageData = MakeVoxelShared<FVoxelFoliageResultData>();
					GuidArchive << Bounds;
					GuidArchive << FoliageData->Transforms;
					GuidArchive << FoliageData->CustomData;
					
					const auto Result = MakeVoxelShared<FVoxelFoliageProxyResult>(*FoliageProxy, Bounds, FoliageData);
					Result->MarkDirty();
					ProxyResults.Add(Result);
				}

				ensure(ProxyResults.Num() == NumProxyResults);
				ensure(GuidArchive.AtEnd());
			}
			check(Positions.Num() == ProxyResults.Num());

			// Load the results into the bucket data
			// Note that we assume the existing data is cleared/fresh already - we only replace the chunks that are in the save
			for (int32 ResultIndex = 0; ResultIndex < ProxyResults.Num(); ResultIndex++)
			{
				const FIntVector Position = Positions[ResultIndex];
				const TVoxelSharedPtr<FVoxelFoliageProxyResult> Result = ProxyResults[ResultIndex];

				TVoxelSharedPtr<FVoxelFoliageChunk>& Chunk = Bucket->Chunks.FindOrAdd(Position);
				if (!Chunk)
				{
					Chunk = MakeVoxelShared<FVoxelFoliageChunk>();
				}

				FVoxelFoliageChunk::FFoliageData& FoliageData = Chunk->Foliages.FindOrAdd(FoliageProxy->UniqueId);
				// Cancel any pending task
				FoliageData.CancelTask();

				if (FoliageData.FoliageResult.IsValid() && FoliageData.FoliageResult->IsCreated())
				{
					// Destroy existing result, and create the new one
					FoliageData.FoliageResult->Destroy(*this);
					FoliageData.FoliageResult = Result;
					FoliageData.FoliageResult->Create(*this);
				}
				else
				{
					// Just replace
					FoliageData.FoliageResult = Result;
				}
			}
		}
	}
}

void FVoxelFoliageSubsystem::SaveTo(FVoxelFoliageSaveImpl& Save)
{
	VOXEL_FUNCTION_COUNTER();

	Save.Guid = FGuid::NewGuid();

	FLargeMemoryWriter Archive;
	
	int32 VoxelCustomVersion = FVoxelFoliageSaveVersion::LatestVersion;
	Archive << VoxelCustomVersion;
	
	Serialize(Archive, FVoxelFoliageSaveVersion::LatestVersion);

	FVoxelSerializationUtilities::CompressData(Archive, Save.CompressedData);
}

void FVoxelFoliageSubsystem::LoadFrom(const FVoxelFoliageSaveImpl& Save)
{
	VOXEL_FUNCTION_COUNTER();

	TArray64<uint8> UncompressedData;
	FVoxelSerializationUtilities::DecompressData(Save.CompressedData, UncompressedData);

	FLargeMemoryReader Archive(UncompressedData.GetData(), UncompressedData.Num());

	int32 Version = -1;
	Archive << Version;
	check(Version >= 0);
	
	Serialize(Archive, FVoxelFoliageSaveVersion::Type(Version));

	ensure(Archive.AtEnd() && !Archive.IsError());
}

bool FVoxelFoliageSubsystem::GetFoliageData(const FGuid& FoliageGuid, TArray<TVoxelSharedPtr<FVoxelFoliageResultData>>& OutData) const
{
	const FFoliageProxyInfo* ProxyInfo = GuidToProxies.Find(FoliageGuid);
	if (!ensure(ProxyInfo))
	{
		return false;
	}

	const TVoxelSharedPtr<FVoxelFoliageBucket> Bucket = ProxyInfo->Bucket.Pin();
	const TVoxelSharedPtr<FVoxelFoliageProxy> FoliageProxy = ProxyInfo->FoliageProxy.Pin();
	if (!ensure(Bucket) || !ensure(FoliageProxy))
	{
		return false;
	}

	for (auto& It : Bucket->Chunks)
	{
		const FVoxelFoliageChunk::FFoliageData* FoliageData = It.Value->Foliages.Find(FoliageProxy->UniqueId);
		if (FoliageData && FoliageData->FoliageResult)
		{
			OutData.Add(FoliageData->FoliageResult->Data);
		}
	}

	return true;
}

int32 FVoxelFoliageSubsystem::RegisterBiome(UVoxelFoliageBiomeBase* BiomeBase)
{
	auto* Biome = Cast<UVoxelFoliageBiome>(BiomeBase);
	if (!Biome || !Biome->Type)
	{
		return -1;
	}

	TMap<int32, TWeakObjectPtr<UVoxelFoliageBiome>>& Biomes = BiomeTypes.FindOrAdd(Biome->Type);
	
	int32 Index = 0;
	while (Biomes.Contains(Index))
	{
		Index++;
	}
	Biomes.Add(Index, Biome);

	return Index;
}

#if WITH_EDITOR
bool FVoxelFoliageSubsystem::NeedsToRebuild(UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent) const
{
	if (Object == FVoxelGeneratorPicker(Settings.Generator).GetObject())
	{
		// If the main generator changed, we probably need to rebuild
		return true;
	}
	
	for (TWeakObjectPtr<UVoxelFoliageCollectionBase> WeakCollection : Settings.FoliageCollections)
	{
		auto* Collection = Cast<UVoxelFoliageCollection>(WeakCollection.Get());
		if (!Collection)
		{
			continue;
		}
		
		if (Collection == Object)
		{
			return true;
		}
		for (const UVoxelFoliage* Foliage : Collection->Foliages)
		{
			if (Foliage && Foliage->NeedsToRebuild(Object, PropertyChangedEvent))
			{
				return true;
			}
		}
	}
	
	for (auto& BiomeTypeIt : BiomeTypes)
	{
		UVoxelFoliageBiomeType* Type = BiomeTypeIt.Key.Get();
		if (Type && Type->NeedsToRebuild(Object, PropertyChangedEvent))
		{
			return true;
		}

		for (auto& BiomeIt : BiomeTypeIt.Value)
		{
			UVoxelFoliageBiome* Biome = BiomeIt.Value.Get();
			if (Biome && Biome->NeedsToRebuild(Object, PropertyChangedEvent))
			{
				return true;
			}
		}
	}

	return false;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageSubsystem::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	// TODO: time limit?
	ProcessTaskCallbacks();
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
void FVoxelFoliageSubsystem::IterateChunksInBounds(const FVoxelIntBox& Bounds, T Lambda)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	for (auto& BucketIt : Buckets)
	{
		FVoxelFoliageBucket& Bucket = *BucketIt.Value;
		if (!Bucket.SpawnedBounds.IsValid())
		{
			continue;
		}

		const FVoxelIntBox BucketBounds = Bucket.SpawnedBounds.GetBox().Overlap(Bounds);
		const FVoxelIntBox KeyBounds = BucketBounds.MakeMultipleOfBigger(Bucket.ChunkSize);

		// Check if it's faster to iterate Chunks directly
		if (FVoxelIntBox(KeyBounds.Min / Bucket.ChunkSize, KeyBounds.Max / Bucket.ChunkSize).Count() < Bucket.Chunks.Num())
		{
			KeyBounds.Iterate(Bucket.ChunkSize, [&](int32 X, int32 Y, int32 Z)
			{
				const FVoxelIntBox ChunkBounds = FVoxelIntBox(FIntVector(X, Y, Z), FIntVector(X, Y, Z) + Bucket.ChunkSize);
				if (auto* Chunk = Bucket.Chunks.Find(Bucket.GetChunkKey(ChunkBounds)))
				{
					for (auto& It : (*Chunk)->Foliages)
					{
						Lambda(Bucket, ChunkBounds, It.Key, It.Value);
					}
				}
			});
		}
		else
		{
			for (auto& ChunkIt : Bucket.Chunks)
			{
				const FVoxelIntBox ChunkBounds(ChunkIt.Key, ChunkIt.Key + Bucket.ChunkSize);
				if (ChunkBounds.Intersect(Bounds))
				{
					for (auto& It : ChunkIt.Value->Foliages)
					{
						Lambda(Bucket, ChunkBounds, It.Key, It.Value);
					}
				}
			}
		}
	}
}

void FVoxelFoliageSubsystem::ProcessTaskCallbacks()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	FTaskCallback Callback;
	while (TaskCallbacks.Dequeue(Callback))
	{
		const auto Bucket = Callback.Bucket.Pin();
		if (!Bucket)
		{
			continue;
		}

		Bucket->FinishSpawning(Callback.Bounds, *this, Callback.FoliageId, Callback.TaskId, Callback.Data);
	}
}