// Copyright 2021 Phyronnaz

#include "VoxelLandscapeGrass.h"
#include "VoxelFoliage.h"
#include "VoxelFoliageCollection.h"
#include "VoxelRender/MaterialCollections/VoxelLandscapeMaterialCollection.h"

#include "LandscapeGrassType.h"
#include "Engine/Blueprint.h"
#include "Engine/CollisionProfile.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionLandscapeGrassOutput.h"

IVoxelLandscapeGrassGraph::FMake IVoxelLandscapeGrassGraph::MakeInstance;

UVoxelFoliageCollection* FVoxelLandscapeGrass::Convert(const UVoxelLandscapeMaterialCollection& MaterialCollection)
{
	VOXEL_FUNCTION_COUNTER();

	UMaterialInterface* LandscapeMaterialInterface = MaterialCollection.Material;
	if (!LandscapeMaterialInterface)
	{
		return nullptr;
	}
	
	UMaterial* LandscapeMaterial = LandscapeMaterialInterface->GetMaterial();
	if (!ensure(LandscapeMaterial))
	{
		return nullptr;
	}

	UMaterialExpressionLandscapeGrassOutput* GrassOutput = nullptr;
	for (UMaterialExpression* Expression : LandscapeMaterial->Expressions)
	{
		if (auto* AsGrassOutput = Cast<UMaterialExpressionLandscapeGrassOutput>(Expression))
		{
			ensure(!GrassOutput);
			GrassOutput = AsGrassOutput;
		}
	}

	if (!GrassOutput)
	{
		return nullptr;
	}

	UVoxelGenerator* Generator = nullptr;
	if (ensure(IVoxelLandscapeGrassGraph::MakeInstance))
	{
		auto Instance = IVoxelLandscapeGrassGraph::MakeInstance(CreateGraph);
		Generator = Instance->Convert(MaterialCollection);
	}
	ensure(Generator);

	UVoxelFoliageCollection* SpawnerCollection = nullptr;
	for (const FGrassInput& GrassInput : GrassOutput->GrassTypes)
	{
		if (GrassInput.GrassType)
		{
			if (!SpawnerCollection)
			{
				SpawnerCollection = CreateSpawnerCollection();
			}
			if (!ensure(SpawnerCollection))
			{
				return nullptr;
			}
			ConvertGrassType(*SpawnerCollection, *GrassInput.GrassType, Generator, GrassInput.Name);
		}
	}

	return SpawnerCollection;
}

void FVoxelLandscapeGrass::ConvertGrassType(UVoxelFoliageCollection& SpawnerCollection, const ULandscapeGrassType& GrassType, UVoxelGenerator* Generator, FName OutputName)
{
	FName AssetName = OutputName;
	for (const FGrassVariety& GrassVariety : GrassType.GrassVarieties)
	{
		AssetName.SetNumber(AssetName.GetNumber() + 1);
		UVoxelFoliage* Foliage = ConvertGrassVariety(GrassVariety, AssetName, Generator, OutputName);
		SpawnerCollection.Foliages.Add(Foliage);
	}
}

UVoxelFoliage* FVoxelLandscapeGrass::ConvertGrassVariety(const FGrassVariety& GrassVariety, FName AssetName, UVoxelGenerator* Generator, FName OutputName)
{
	UVoxelFoliage* Foliage = CreateFoliage(AssetName);
	if (!ensure(Foliage))
	{
		return nullptr;
	}
	
	Foliage->Guid = FGuid::NewGuid();

	FVoxelFoliageDensity& Density = Foliage->Densities.Emplace_GetRef();
	Density.Type = EVoxelFoliageDensityType::GeneratorOutput;
	Density.bUseMainGenerator = false;
	Density.CustomGenerator = Generator;
	Density.GeneratorOutputName = OutputName;
	
	Foliage->Meshes.Add({ GrassVariety.GrassMesh });
	Foliage->SpawnSettings.DistanceBetweenInstances = FVoxelDistance::Centimeters(FMath::Sqrt(1. / (GrassVariety.GrassDensity.Default / 1000. / 1000.)));
	Foliage->InstanceSettings.CullDistance = { GrassVariety.StartCullDistance.Default, GrassVariety.EndCullDistance.Default };

	switch (GrassVariety.Scaling)
	{
	case EGrassScaling::Uniform:
	{
		Foliage->Scaling.Scaling = EVoxelFoliageScaling::Uniform;
		break;
	}
	case EGrassScaling::Free:
	{
		Foliage->Scaling.Scaling = EVoxelFoliageScaling::Free;
		break;
	}
	case EGrassScaling::LockXY:
	{
		Foliage->Scaling.Scaling = EVoxelFoliageScaling::LockXY;
		break;
	}
	default: ensure(false);
	}

	Foliage->Scaling.ScaleX = GrassVariety.ScaleX;
	Foliage->Scaling.ScaleY = GrassVariety.ScaleY;
	Foliage->Scaling.ScaleZ = GrassVariety.ScaleZ;

	Foliage->bRandomYaw = GrassVariety.RandomRotation;
	Foliage->RotationAlignment = GrassVariety.AlignToSurface ? EVoxelFoliageRotation::AlignToSurface : EVoxelFoliageRotation::AlignToWorldUp;

	Foliage->InstanceSettings.LightingChannels = GrassVariety.LightingChannels;
	Foliage->InstanceSettings.bReceivesDecals = GrassVariety.bReceivesDecals;
	Foliage->InstanceSettings.bCastShadow = GrassVariety.bCastDynamicShadow;

	return Foliage;
}