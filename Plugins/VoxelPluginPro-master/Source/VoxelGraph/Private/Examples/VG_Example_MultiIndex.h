// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelGeneratedWorldGeneratorsIncludes.h"
#include "VG_Example_MultiIndex.generated.h"

UCLASS(Blueprintable)
class UVG_Example_MultiIndex : public UVoxelGraphGeneratorHelper
{
	GENERATED_BODY()
	
public:
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="", meta=(DisplayName="Layer 0"))
	TSoftObjectPtr<UMaterialInterface> Layer_0 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Materials/Quixel/AngledBlocks.AngledBlocks"));
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="", meta=(DisplayName="Layer 1"))
	TSoftObjectPtr<UMaterialInterface> Layer_1 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/MI_AerialGrassRock_BrownMudRocks.MI_AerialGrassRock_BrownMudRocks"));
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="", meta=(DisplayName="Layer 2"))
	TSoftObjectPtr<UMaterialInterface> Layer_2 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/Pavement/MI_Pavement.MI_Pavement"));
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="", meta=(DisplayName="Layer 3"))
	TSoftObjectPtr<UMaterialInterface> Layer_3 = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath("/Voxel/Examples/Shared/Textures/TextureHaven/RockGround/MI_RockGround.MI_RockGround"));
	
	UVG_Example_MultiIndex();
	virtual TVoxelSharedRef<FVoxelTransformableGeneratorInstance> GetTransformableInstance() override;
};
