// Copyright 2021 Phyronnaz

#include "VoxelNodes/VoxelGetLandscapeCollectionIndexNode.h"
#include "VoxelNodes/VoxelNodeHelpers.h"
#include "VoxelNodes/VoxelNodeVariables.h"

#include "VoxelGraphGenerator.h"
#include "VoxelGraphErrorReporter.h"
#include "Runtime/VoxelComputeNode.h"

#include "CppTranslation/VoxelCppIds.h"
#include "CppTranslation/VoxelCppConfig.h"
#include "CppTranslation/VoxelCppConstructor.h"

#include "VoxelGenerators/VoxelGeneratorInit.h"
#include "VoxelRender/MaterialCollections/VoxelMaterialCollectionBase.h"

#include "Misc/UObjectToken.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"

UVoxelNode_GetLandscapeCollectionIndex::UVoxelNode_GetLandscapeCollectionIndex()
{
	SetOutputs(EVoxelPinCategory::Int);
}

FText UVoxelNode_GetLandscapeCollectionIndex::GetTitle() const
{
	return FText::Format(VOXEL_LOCTEXT("Get Landscape Collection Index: {0}"), Super::GetTitle());
}

TVoxelSharedPtr<FVoxelComputeNode> UVoxelNode_GetLandscapeCollectionIndex::GetComputeNode(const FVoxelCompilationNode& InCompilationNode) const
{
	class FLocalVoxelComputeNode : public FVoxelDataComputeNode
	{
	public:
		GENERATED_DATA_COMPUTE_NODE_BODY();

		FLocalVoxelComputeNode(const UVoxelNode_GetLandscapeCollectionIndex& Node, const FVoxelCompilationNode& CompilationNode)
			: FVoxelDataComputeNode(Node, CompilationNode)
			, LayerName(Node.GetParameter(CompilationNode))
			, IndexVariable("int32", UniqueName.ToString() + "_Index")
			, ExposedVariable(MakeShared<FVoxelNameVariable>(Node, Node.LayerName))
		{
		}

		virtual void Init(FVoxelGraphSeed Inputs[], const FVoxelGeneratorInit& InitStruct) override
		{
			if (!InitStruct.MaterialCollection)
			{
				if (Node.IsValid())
				{
					FVoxelGraphErrorReporter ErrorReporter(Node->Graph);
					ErrorReporter.AddMessageToNode(Node.Get(), "material collection is null\nYou can set in the Preview Settings", EVoxelGraphNodeMessageType::Warning);
					ErrorReporter.Apply(false);
				}
				return;
			}

			Index = InitStruct.MaterialCollection->GetMaterialIndex(LayerName);
			if (Index != -1)
			{
				return;
			}
			
			TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Warning);
			Message->AddToken(FTextToken::Create(FText::Format(VOXEL_LOCTEXT("GetLandscapeCollectionIndex: Layer {0} not found in "), FText::FromName(LayerName))));
			Message->AddToken(FUObjectToken::Create(InitStruct.MaterialCollection));

			Message->AddToken(FTextToken::Create(VOXEL_LOCTEXT("Graph:")));
			
			for (auto& SourceNode : SourceNodes)
			{
				if (SourceNode.IsValid())
				{
					Message->AddToken(FUObjectToken::Create(SourceNode->Graph));
				}
			}
			if (Node.IsValid())
			{
				FVoxelGraphErrorReporter ErrorReporter(Node->Graph);
				ErrorReporter.AddMessageToNode(Node.Get(), "material index not found", EVoxelGraphNodeMessageType::Warning);
				ErrorReporter.Apply(false);
			}
			FMessageLog("PIE").AddMessage(Message);
		}
		virtual void InitCpp(const TArray<FString> & Inputs, FVoxelCppConstructor & Constructor) const override
		{
			Constructor.AddLinef(TEXT("if (%s.MaterialCollection)"), *FVoxelCppIds::InitStruct);
			Constructor.StartBlock();
			Constructor.AddLinef(TEXT("%s = %s.MaterialCollection->GetMaterialIndex(%s);"), *IndexVariable.CppName, *FVoxelCppIds::InitStruct, *ExposedVariable->CppName);
			Constructor.EndBlock();
			Constructor.AddLine("else");
			Constructor.StartBlock();
			Constructor.AddLinef(TEXT("%s = -1;"), *IndexVariable.CppName);
			Constructor.EndBlock();
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
			ComputeCpp(Inputs, Outputs, Constructor);
		}

		virtual void SetupCpp(FVoxelCppConfig& Config) const override
		{
			Config.AddExposedVariable(ExposedVariable);
		}
		virtual void GetPrivateVariables(TArray<FVoxelVariable>& PrivateVariables) const override
		{
			PrivateVariables.Add(IndexVariable);
		}

	private:
		int32 Index = -1;
		const FName LayerName;
		const FVoxelVariable IndexVariable;
		const TSharedRef<FVoxelNameVariable> ExposedVariable;
	};
	return MakeShareable(new FLocalVoxelComputeNode(*this, InCompilationNode));
}
