// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"

class UMaterial;
class UVoxelFoliage;
class UVoxelGenerator;
class ULandscapeGrassType;
class UVoxelFoliageCollection;
class UVoxelLandscapeMaterialCollection;
struct FGrassVariety;

class FVoxelLandscapeGrass
{
public:
	TFunction<UVoxelFoliageCollection* ()> CreateSpawnerCollection;
	TFunction<UVoxelFoliage* (FName Name)> CreateFoliage;
	TFunction<UObject* (UClass* Class)> CreateGraph;
	
	FVoxelLandscapeGrass() = default;
	
	UVoxelFoliageCollection* Convert(const UVoxelLandscapeMaterialCollection& MaterialCollection);

private:
	void ConvertGrassType(UVoxelFoliageCollection& SpawnerCollection, const ULandscapeGrassType& GrassType, UVoxelGenerator* Generator, FName OutputName);
	UVoxelFoliage* ConvertGrassVariety(const FGrassVariety& GrassVariety, FName AssetName, UVoxelGenerator* Generator, FName OutputName);
};

class IVoxelLandscapeGrassGraph
{
public:
	virtual ~IVoxelLandscapeGrassGraph() = default;

	virtual UVoxelGenerator* Convert(const UVoxelLandscapeMaterialCollection& MaterialCollection) = 0;

	using FMake = TFunction<TVoxelSharedRef<IVoxelLandscapeGrassGraph>(TFunction<UObject* (UClass* Class)> CreateGraph)>;
	static VOXELEDITOR_API FMake MakeInstance;
};