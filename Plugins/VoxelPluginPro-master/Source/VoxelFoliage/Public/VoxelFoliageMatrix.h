// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"

// Matrix with special meaning for the last column
// Matrix[0][3] : random instance id, // Matrix[1 2 3][3]: position offset (used for voxel lookup)
struct FVoxelFoliageMatrix
{
	FVoxelFoliageMatrix() = default;
	explicit FVoxelFoliageMatrix(const FMatrix & Matrix)
		: Matrix(Matrix)
	{
	}

	FORCEINLINE float GetRandomInstanceId() const
	{
		return Matrix.M[0][3];
	}
	FORCEINLINE void SetRandomInstanceId(float RandomInstanceId)
	{
		Matrix.M[0][3] = RandomInstanceId;
	}
	FORCEINLINE void SetRandomInstanceId(uint32 PackedInt)
	{
		SetRandomInstanceId(*reinterpret_cast<const float*>(&PackedInt));
	}

	// Used for floating detection: the voxel position is GetMatrixTranslation() + GetPositionOffset()
	FORCEINLINE FVector GetPositionOffset() const
	{
		return FVector(Matrix.M[1][3], Matrix.M[2][3], Matrix.M[3][3]);
	}
	FORCEINLINE void SetPositionOffset(const FVector& PositionOffset)
	{
		Matrix.M[1][3] = PositionOffset.X;
		Matrix.M[2][3] = PositionOffset.Y;
		Matrix.M[3][3] = PositionOffset.Z;
	}

	FORCEINLINE FMatrix GetCleanMatrix() const
	{
		auto Copy = Matrix;
		Copy.M[0][3] = 0;
		Copy.M[1][3] = 0;
		Copy.M[2][3] = 0;
		Copy.M[3][3] = 1;
		return Copy;
	}

	FORCEINLINE bool operator==(const FVoxelFoliageMatrix& Other) const
	{
		return Matrix == Other.Matrix;
	}
	FORCEINLINE bool operator!=(const FVoxelFoliageMatrix& Other) const
	{
		return Matrix != Other.Matrix;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelFoliageMatrix& FoliageMatrix)
	{
		Ar << FoliageMatrix.Matrix;
		return Ar;
	}
	
private:
	FMatrix Matrix;
};

struct FVoxelFoliageTransform
{
	// Used to reduce precision errors
	FIntVector TransformOffset;
	// Relative to TransformOffset
	FVoxelFoliageMatrix Matrix;
	
	FORCEINLINE bool operator==(const FVoxelFoliageTransform& Other) const
	{
		return TransformOffset == Other.TransformOffset && Matrix == Other.Matrix;
	}
	FORCEINLINE bool operator!=(const FVoxelFoliageTransform& Other) const
	{
		return TransformOffset != Other.TransformOffset || Matrix != Other.Matrix;
	}
	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelFoliageTransform& Transform)
	{
		Ar << Transform.TransformOffset;
		Ar << Transform.Matrix;
		return Ar;
	}
};
struct FVoxelFoliageTransforms
{
	// Used to reduce precision errors. In voxels
	FIntVector TransformsOffset;
	// Relative to TransformsOffset. Not in voxels, but multiplied by Voxel Size!
	TArray<FVoxelFoliageMatrix> Matrices;

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelFoliageTransforms& Transforms)
	{
		Ar << Transforms.TransformsOffset;
		Ar << Transforms.Matrices;
		return Ar;
	}
};
