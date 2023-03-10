// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.h"
#include "VoxelMinimal.h"
#include "UObject/Interface.h"
#include "VoxelPhysicsPartSpawnerInterface.generated.h"

class AStaticMeshActor;
class UStaticMesh;
class UMaterialInterface;
class FVoxelData;
class AVoxelWorld;
struct FVoxelFloatingPart;

USTRUCT(BlueprintType)
struct FVoxelPositionValueMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	FIntVector Position = FIntVector(ForceInit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	float Value = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	FVoxelMaterial Material = FVoxelMaterial(ForceInit);
};

// Represents the result for a single part
UINTERFACE(BlueprintType)
class VOXEL_API UVoxelPhysicsPartSpawnerResult : public UInterface
{
	GENERATED_BODY()
};

class VOXEL_API IVoxelPhysicsPartSpawnerResult : public IInterface
{
	GENERATED_BODY()
};


UINTERFACE(BlueprintType)
class VOXEL_API UVoxelPhysicsPartSpawner : public UInterface
{
	GENERATED_BODY()
};

class VOXEL_API IVoxelPhysicsPartSpawner : public IInterface
{
	GENERATED_BODY()
	
public:
	/**
	 * @param OutOnWorldUpdateDone	Will be triggered once the world update of the removed voxels will be done
	 * @param World		            The voxel world
	 * @param Part		            The voxel part
	 */
	virtual TScriptInterface<IVoxelPhysicsPartSpawnerResult> SpawnPart(
		TVoxelSharedPtr<FSimpleDelegate>& OutOnWorldUpdateDone,
		AVoxelWorld* World,
		FVoxelFloatingPart&& Part)
	{
		return nullptr;
	}

	virtual bool NeedData() const { return false; }
	virtual bool NeedVoxels() const { return false; }
};
