// Copyright 2021 Phyronnaz

#include "VoxelNodes/VoxelFoliageNodes.h"
#include "VoxelNodes/VoxelNodeHelpers.h"
#include "VoxelNodes/VoxelNodeVariables.h"
#include "Runtime/VoxelComputeNode.h"
#include "NodeFunctions/VoxelFoliageNodeFunctions.h"
#include "CppTranslation/VoxelCppIds.h"
#include "CppTranslation/VoxelCppConfig.h"
#include "CppTranslation/VoxelCppConstructor.h"
#include "VoxelGenerators/VoxelGeneratorInit.h"

UVoxelNode_SampleFoliageMaterialIndex::UVoxelNode_SampleFoliageMaterialIndex()
{
	SetInputs({ "Index", EVoxelPinCategory::Int, "Index to sample" });
	SetOutputs({ "Value", EVoxelPinCategory::Float, "Between 0 and 1" });
}

GENERATED_VOXELNODE_IMPL
(
	UVoxelNode_SampleFoliageMaterialIndex,
	DEFINE_INPUTS(int32),
	DEFINE_OUTPUTS(v_flt),
	Output0 = FVoxelFoliageNodeFunctions::SampleFoliageMaterialIndex(Input0, Context);
)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelNode_GetBiomeIndex::UVoxelNode_GetBiomeIndex()
{
	SetOutputs(EC::Int);
}

FText UVoxelNode_GetBiomeIndex::GetTitle() const
{
	return FText::Format(VOXEL_LOCTEXT("Biome: {0}"), Super::GetTitle());
}

TVoxelSharedPtr<FVoxelComputeNode> UVoxelNode_GetBiomeIndex::GetComputeNode(const FVoxelCompilationNode& InCompilationNode) const
{
	class FLocalVoxelComputeNode : public FVoxelDataComputeNode
	{
	public:
		GENERATED_DATA_COMPUTE_NODE_BODY();

		FLocalVoxelComputeNode(const UVoxelNode_GetBiomeIndex& Node, const FVoxelCompilationNode& CompilationNode)
			: FVoxelDataComputeNode(Node, CompilationNode)
			, Biome(Node.GetParameter(CompilationNode))
			, IndexVariable("int32", UniqueName.ToString() + "_Index")
			, Variable(MakeShared<TVoxelObjectVariable<UVoxelFoliageBiomeBase>>(Node, Node.Biome))
		{
		}
		
		virtual void SetupCpp(FVoxelCppConfig& Config) const override
		{
			Config.AddExposedVariable(Variable);
		}
		virtual void GetPrivateVariables(TArray<FVoxelVariable>& PrivateVariables) const override
		{
			PrivateVariables.Add(IndexVariable);
		}

		virtual void Init(FVoxelGraphSeed Inputs[], const FVoxelGeneratorInit& InitStruct) override
		{
			Index = FVoxelFoliageNodeFunctions::InitBiome(InitStruct, Biome);
		}
		virtual void InitCpp(const TArray<FString>& Inputs, FVoxelCppConstructor& Constructor) const override
		{
			Constructor.AddLinef(TEXT("%s = FVoxelFoliageNodeFunctions::InitBiome(%s, %s)"), *IndexVariable.CppName, *FVoxelCppIds::InitStruct, *Variable->CppName);
		}

		virtual void Compute(FVoxelNodeInputBuffer Inputs, FVoxelNodeOutputBuffer Outputs, const FVoxelContext& Context) const override
		{
			Outputs[0].Get<int32>() = Index;
		}
		virtual void ComputeRange(FVoxelNodeInputRangeBuffer Inputs, FVoxelNodeOutputRangeBuffer Outputs, const FVoxelContextRange& Context) const override
		{
			Outputs[0].Get<int32>() = Index;
		}
		virtual void ComputeCpp(const TArray<FString>& Inputs, const TArray<FString>& Outputs, FVoxelCppConstructor& Constructor) const override
		{
			Constructor.AddLinef(TEXT("%s = %s;"), *Outputs[0], *IndexVariable.CppName);
		}
		virtual void ComputeRangeCpp(const TArray<FString>& Inputs, const TArray<FString>& Outputs, FVoxelCppConstructor& Constructor) const override
		{
			Constructor.AddLinef(TEXT("%s = %s;"), *Outputs[0], *IndexVariable.CppName);
		}

	private:
		const TWeakObjectPtr<UVoxelFoliageBiomeBase> Biome;
		int32 Index = -1;
		const FVoxelVariable IndexVariable;
		const TSharedRef<TVoxelObjectVariable<UVoxelFoliageBiomeBase>> Variable;
	};
	return MakeShareable(new FLocalVoxelComputeNode(*this, InCompilationNode));
}
