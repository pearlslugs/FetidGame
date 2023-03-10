// Copyright 2021 Phyronnaz

#include "VoxelFoliageDensityHelper.h"
#include "VoxelFoliageProxy.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelUtilities/VoxelMaterialUtilities.h"

v_flt FVoxelFoliageDensityQuery::GetDensity() const
{
	v_flt Result = 1;

	for (const FVoxelFoliageDensity& Density : Foliage.Densities)
	{
		Result *= GetDensity(Density);
	}
	
	return Result;
}

float FVoxelFoliageDensityQuery::GetDensity(const FVoxelFoliageDensity& FoliageDensity) const
{
	const float Density = GetBaseDensity(FoliageDensity);

	if (FoliageDensity.bInvertDensity)
	{
		return 1 - Density;
	}
	else
	{
		return Density;
	}
}

float FVoxelFoliageDensityQuery::GetBaseDensity(const FVoxelFoliageDensity& FoliageDensity) const
{
	switch (FoliageDensity.Type)
	{
	default: ensure(false);
	case EVoxelFoliageDensityType::Constant:
	{
		return FoliageDensity.Constant;
	}
	case EVoxelFoliageDensityType::GeneratorOutput:
	{
		return GetFloatCustomOutput(FoliageDensity.GeneratorOutputName, FoliageDensity.GeneratorInstance.Get());
	}
	case EVoxelFoliageDensityType::MaterialRGBA:
	{
		const FVoxelMaterial Material = GetMaterial();
		switch (FoliageDensity.RGBAChannel)
		{
		default: ensure(false);
		case EVoxelRGBA::R: return Material.GetR_AsFloat();
		case EVoxelRGBA::G: return Material.GetG_AsFloat();
		case EVoxelRGBA::B: return Material.GetB_AsFloat();
		case EVoxelRGBA::A: return Material.GetA_AsFloat();
		}
	}
	case EVoxelFoliageDensityType::MaterialUVs:
	{
		const FVoxelMaterial Material = GetMaterial();
		return FoliageDensity.UVAxis == EVoxelUVAxis::U
			? Material.GetU_AsFloat(FoliageDensity.UVChannel)
			: Material.GetV_AsFloat(FoliageDensity.UVChannel);
	}
	case EVoxelFoliageDensityType::MaterialFiveWayBlend:
	{
		const FVoxelMaterial Material = GetMaterial();

		if (Settings.bIsFourWayBlend)
		{
			const TVoxelStaticArray<float, 4> Strengths = FVoxelUtilities::GetFourWayBlendStrengths(Material);
			return Strengths[FMath::Clamp(FoliageDensity.FiveWayBlendChannel, 0, 3)];
		}
		else
		{
			const TVoxelStaticArray<float, 5> Strengths = FVoxelUtilities::GetFiveWayBlendStrengths(Material);
			return Strengths[FMath::Clamp(FoliageDensity.FiveWayBlendChannel, 0, 4)];
		}
	}
	case EVoxelFoliageDensityType::SingleIndex:
	{
		const FVoxelMaterial Material = GetMaterial();
		return FoliageDensity.SingleIndexChannels.Contains(Material.GetSingleIndex()) ? 1.f : 0.f;
	}
	case EVoxelFoliageDensityType::MultiIndex:
	{
		const FVoxelMaterial Material = GetMaterial();
		const TVoxelStaticArray<float, 4> Strengths = FVoxelUtilities::GetMultiIndexStrengths(Material);

		float Strength = 0.f;
		for (int32 Channel : FoliageDensity.MultiIndexChannels)
		{
			const int32 Index = FVoxelUtilities::GetMultiIndexIndex(Material, Channel);
			if (Index != -1)
			{
				Strength += Strengths[Index];
			}
		}

		return Strength;
	}
	}
}