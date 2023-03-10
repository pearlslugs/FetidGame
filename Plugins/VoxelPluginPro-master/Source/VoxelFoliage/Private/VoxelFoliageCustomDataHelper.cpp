// Copyright 2021 Phyronnaz

#include "VoxelFoliageCustomDataHelper.h"
#include "VoxelFoliageCustomData.h"

float FVoxelFoliageCustomDataQuery::GetCustomData(const FVoxelFoliageCustomData& FoliageCustomData) const
{
	switch (FoliageCustomData.Type)
	{
	default: ensure(false);
	case EVoxelFoliageCustomDataType::ColorGeneratorOutput:
	{
		return ColorToFloat(GetColorCustomOutput(FoliageCustomData.ColorGeneratorOutputName, FoliageCustomData.GeneratorInstance.Get()));
	}
	case EVoxelFoliageCustomDataType::FloatGeneratorOutput:
	{
		return GetFloatCustomOutput(FoliageCustomData.FloatGeneratorOutputName, FoliageCustomData.GeneratorInstance.Get());
	}
	case EVoxelFoliageCustomDataType::MaterialColor:
	{
		return ColorToFloat(GetMaterial().GetColor());
	}
	case EVoxelFoliageCustomDataType::MaterialSingleIndex:
	{
		return GetMaterial().GetSingleIndex();
	}
	case EVoxelFoliageCustomDataType::MaterialUV:
	{
		return FoliageCustomData.UVAxis == EVoxelUVAxis::U
			? GetMaterial().GetU_AsFloat(FoliageCustomData.UVChannel)
			: GetMaterial().GetV_AsFloat(FoliageCustomData.UVChannel);
	}
	}
}

float FVoxelFoliageCustomDataQuery::ColorToFloat(FColor Color)
{
	return FVoxelUtilities::PackIntIntoFloat(Color.ToPackedABGR());
}