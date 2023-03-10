// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelIntBox.h"

class FVoxelFoliageProxy;
enum class EVoxelFoliageRandomGenerator : uint8;

class FVoxelFoliageRandomGenerator
{
public:
	FVoxelFoliageRandomGenerator(const FVoxelIntBox& Bounds, const FVoxelFoliageProxy& Foliage);

	FVector2D GetPosition() const
	{
		return PositionGenerator.GetValue();
	}
	
	float GetGroupSplitFraction() const
	{
		return GroupSplitStream.GetFraction();
	}
	float GetDensityFraction() const
	{
		return DensityStream.GetFraction();
	}
	float GetHeightRestrictionFraction() const
	{
		return HeightRestrictionStream.GetFraction();
	}
	float GetRandomInstanceIdFraction() const
	{
		return RandomInstanceIdStream.GetFraction();
	}

	float GetScaleFraction() const
	{
		return ScaleStream.GetFraction();
	}
	float GetYawFraction() const
	{
		return YawStream.GetFraction();
	}
	float GetPitchFraction() const
	{
		return PitchStream.GetFraction();
	}
	FVector GetRandomAlignVector() const
	{
		return RandomAlignStream.GetUnitVector();
	}

private:
	class FPositionGenerator
	{
	public:
		void Init(EVoxelFoliageRandomGenerator InRandomGenerator, uint32 Seed);
		FVector2D GetValue() const;

	protected:
		int32 CellBits = 1;
		EVoxelFoliageRandomGenerator RandomGenerator = {};

		mutable int32 Index = 0;
		mutable FVector2D Value;
	};
	FPositionGenerator PositionGenerator;

	FRandomStream GroupSplitStream;
	FRandomStream DensityStream;
	FRandomStream HeightRestrictionStream;
	FRandomStream RandomInstanceIdStream;
	
	FRandomStream ScaleStream;
	FRandomStream YawStream;
	FRandomStream PitchStream;
	FRandomStream RandomAlignStream;
};