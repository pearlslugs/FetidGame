// Copyright 2021 Phyronnaz

#include "VoxelLandscapeGrassGraph.h"
#include "VoxelMessages.h"
#include "VoxelNode.h"
#include "VoxelNodes/VoxelExecNodes.h"
#include "VoxelNodes/VoxelMathNodes.h"
#include "VoxelNodes/VoxelParameterNodes.h"
#include "VoxelNodes/VoxelFoliageNodes.h"
#include "VoxelNodes/VoxelGetLandscapeCollectionIndexNode.h"
#include "VoxelGraphGenerator.h"
#include "VoxelGraphPreviewSettings.h"
#include "VoxelRender/VoxelMaterialExpressions.h"
#include "VoxelRender/MaterialCollections/VoxelLandscapeMaterialCollection.h"

#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionLandscapeGrassOutput.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionLandscapeLayerSample.h"
#include "Subsystems/AssetEditorSubsystem.h"

void FVoxelLandscapeGrassGraph::Register()
{
	MakeInstance = [](TFunction<UObject* (UClass* Class)> CreateGraph)
	{
		auto Instance = MakeVoxelShared<FVoxelLandscapeGrassGraph>();
		Instance->CreateGraph = CreateGraph;
		return Instance;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelGenerator* FVoxelLandscapeGrassGraph::Convert(const UVoxelLandscapeMaterialCollection& MaterialCollection)
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

	UVoxelGraphGenerator* Graph = Cast<UVoxelGraphGenerator>(CreateGraph(UVoxelGraphGenerator::StaticClass()));
	if (!ensure(Graph))
	{
		return nullptr;
	}
	
	for (const FGrassInput& GrassInput : GrassOutput->GrassTypes)
	{
		FVoxelGraphOutput Output;
		Output.Name = GrassInput.Name;
		Output.Category = EVoxelDataPinCategory::Float;
		Output.GUID = FGuid::NewGuid();
		Graph->CustomOutputs.Add(Output);
	}
	
	Graph->PreviewSettings = NewObject<UVoxelGraphPreviewSettings>(Graph);
	Graph->PreviewSettings->MaterialCollection = const_cast<UVoxelLandscapeMaterialCollection*>(&MaterialCollection);

	Graph->CreateGraphs();

	check(Graph->VoxelGraph->Nodes.Num() == 1);
	UEdGraphNode* FirstNode = Graph->VoxelGraph->Nodes[0];

	FirstNode->NodePosX = GrassOutput->MaterialExpressionEditorX;
	FirstNode->NodePosY = GrassOutput->MaterialExpressionEditorY;
	
	UEdGraphPin* ExecOutputPin = GetOutputPin(FirstNode, 0);

	TMap<FName, UVoxelNode_SetNode*> SetterNodes;
	for (auto& It : Graph->GetOutputs())
	{
		const int32 OutputIndex = It.Key;
		const int32 RelativeOutputIndex = OutputIndex - FVoxelGraphOutputsIndices::DefaultOutputsMax;
		if (RelativeOutputIndex < 0)
		{
			continue;
		}
		
		const FVector2D Position(GrassOutput->MaterialExpressionEditorX, GrassOutput->MaterialExpressionEditorY + (1 + RelativeOutputIndex) * 100);
		UVoxelNode_SetNode* NewNode = Graph->ConstructNewNode<UVoxelNode_SetNode>(Position, false);
		NewNode->GraphNode->ReconstructNode();
		NewNode->SetIndex(OutputIndex);

		SetterNodes.Add(It.Value.Name, NewNode);
		
		auto* NodeExecInputPin = GetInputPin(NewNode, 0);
		auto* NodeExecOutputPin = GetOutputPin(NewNode, 0);

		ExecOutputPin->MakeLinkTo(NodeExecInputPin);
		ExecOutputPin = NodeExecOutputPin;
	}
	
	for (const FGrassInput& GrassInput : GrassOutput->GrassTypes)
	{
		const FExpressionInput Input = GrassInput.Input.GetTracedInput();
		UVoxelNode* VoxelNode = GetExpressionNode(*Graph, Input.Expression);
		if (!VoxelNode)
		{
			continue;
		}
		
		const auto VoxelNodeOutputPins = GetPins(VoxelNode->GraphNode, EGPD_Output);
		if (!ensure(VoxelNodeOutputPins.IsValidIndex(Input.OutputIndex)))
		{
			continue;
		}
		UEdGraphPin* OutputPin = VoxelNodeOutputPins[Input.OutputIndex];
		
		UVoxelNode_SetNode* SetterNode = SetterNodes[GrassInput.Name];
		UEdGraphPin* SetterInputPin = GetInputPin(SetterNode->GraphNode, 1);
		
		OutputPin->MakeLinkTo(SetterInputPin);
	}

	for (UMaterialExpressionComment* Comment : LandscapeMaterial->EditorComments)
	{
		UEdGraphNode_Comment* NewComment = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate(
			Graph->VoxelGraph, 
			NewObject<UEdGraphNode_Comment>(), 
			FVector2D(Comment->MaterialExpressionEditorX, Comment->MaterialExpressionEditorY), 
			false);
		
		NewComment->NodeWidth = Comment->SizeX;
		NewComment->NodeHeight = Comment->SizeY;
		NewComment->NodeComment = Comment->Text;
		NewComment->CommentColor = Comment->CommentColor;
		NewComment->FontSize = Comment->FontSize;
	}
	
	Graph->CompileVoxelNodesFromGraphNodes();

	return Graph;
}

UVoxelNode* FVoxelLandscapeGrassGraph::GetExpressionNode(UVoxelGraphGenerator& Graph, UMaterialExpression* Expression)
{
	if (!Expression)
	{
		return nullptr;
	}
	if (UVoxelNode** NodePtr = ExpressionsToNodes.Find(Expression))
	{
		return *NodePtr;
	}

	const TMap<UClass*, UClass*> NodeMap =
	{
		{ UMaterialExpressionAdd::StaticClass(), UVoxelNode_FAdd::StaticClass() },
		{ UMaterialExpressionSubtract::StaticClass(), UVoxelNode_FSubstract::StaticClass() },
		{ UMaterialExpressionMultiply::StaticClass(), UVoxelNode_FMultiply::StaticClass() },
		{ UMaterialExpressionDivide::StaticClass(), UVoxelNode_FDivide::StaticClass() },
		{ UMaterialExpressionStaticSwitchParameter::StaticClass(), UVoxelNode_SwitchFloat::StaticClass() },
		{ UMaterialExpressionClamp::StaticClass(), UVoxelNode_Clamp::StaticClass() },
		{ UMaterialExpressionOneMinus::StaticClass(), UVoxelNode_1MinusX::StaticClass() },
		{ UMaterialExpressionLandscapeLayerSample::StaticClass(), UVoxelNode_SampleFoliageMaterialIndex::StaticClass() },
		{ UMaterialExpressionVoxelLandscapeLayerSample::StaticClass(), UVoxelNode_SampleFoliageMaterialIndex::StaticClass() },
	};

	if (auto* VoxelNodeClass = NodeMap.FindRef(Expression->GetClass()))
	{
		const FVector2D NodePosition(Expression->MaterialExpressionEditorX, Expression->MaterialExpressionEditorY);
		
		UVoxelNode* VoxelNode = Graph.ConstructNewNode(VoxelNodeClass, NodePosition, false);
		VoxelNode->GraphNode->bCommentBubbleVisible = !Expression->Desc.IsEmpty();
		VoxelNode->GraphNode->NodeComment = Expression->Desc;
		VoxelNode->GraphNode->ReconstructNode();
		// Cache it before a possible recursive call
		ExpressionsToNodes.Add(Expression, VoxelNode);

		MoveLinks(Graph, *Expression, *VoxelNode);

		if (auto* StaticSwitch = Cast<UMaterialExpressionStaticSwitchParameter>(Expression))
		{
			auto* BoolParameter = Graph.ConstructNewNode<UVoxelNode_BoolParameter>(NodePosition + FVector2D(0, 120), false);
			BoolParameter->GraphNode->ReconstructNode();
			BoolParameter->Value = StaticSwitch->DefaultValue;
			BoolParameter->SetEditableName(StaticSwitch->ParameterName.ToString());

			GetInputPin(VoxelNode->GraphNode, 2)->MakeLinkTo(GetOutputPin(BoolParameter->GraphNode, 0));
		}
		if (auto* LayerSample = Cast<UMaterialExpressionLandscapeLayerSample>(Expression))
		{
			auto* GetIndex = Graph.ConstructNewNode<UVoxelNode_GetLandscapeCollectionIndex>(NodePosition + FVector2D(0, 60), false);
			GetIndex->GraphNode->ReconstructNode();
			GetIndex->LayerName = LayerSample->ParameterName;
			GetIndex->Category = "Layers";
			GetIndex->SetEditableName(LayerSample->ParameterName.ToString());

			GetInputPin(VoxelNode->GraphNode, 0)->MakeLinkTo(GetOutputPin(GetIndex->GraphNode, 0));
		}

		return VoxelNode;
	}
	else
	{
		ExpressionsToNodes.Add(Expression, nullptr);
		FVoxelMessages::Error(FString::Printf(TEXT("%s nodes are not supported by voxel landscape grass"), *Expression->GetClass()->GetName()));
		return nullptr;
	}
}

void FVoxelLandscapeGrassGraph::MoveLinks(UVoxelGraphGenerator& Graph, UMaterialExpression& Expression, UVoxelNode& VoxelNode)
{
	const auto MaterialInputs = Expression.GetInputs();
	const auto VoxelNodeInputPins = GetPins(VoxelNode.GraphNode, EGPD_Input);

	for (int32 InputIndex = 0; InputIndex < MaterialInputs.Num(); InputIndex++)
	{
		if (!ensure(VoxelNodeInputPins.IsValidIndex(InputIndex)))
		{
			continue;
		}
		UEdGraphPin* VoxelNodeInputPin = VoxelNodeInputPins[InputIndex];

		const FExpressionInput Input = MaterialInputs[InputIndex]->GetTracedInput();
		if (!Input.Expression)
		{
			continue;
		}

		if (auto* Constant = Cast<UMaterialExpressionConstant>(Input.Expression))
		{
			VoxelNodeInputPin->DefaultValue = LexToString(Constant->R);
			continue;
		}

		UVoxelNode* OtherVoxelNode = GetExpressionNode(Graph, Input.Expression);
		if (!OtherVoxelNode)
		{
			continue;
		}
		const auto OtherVoxelNodeOutputPins = GetPins(OtherVoxelNode->GraphNode, EGPD_Output);

		if (!ensure(OtherVoxelNodeOutputPins.IsValidIndex(Input.OutputIndex)))
		{
			continue;
		}
		UEdGraphPin* OtherVoxelNodeOutputPin = OtherVoxelNodeOutputPins[Input.OutputIndex];

		OtherVoxelNodeOutputPin->MakeLinkTo(VoxelNodeInputPin);
	}
}

TArray<UEdGraphPin*> FVoxelLandscapeGrassGraph::GetPins(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	auto Pins = Node->Pins;
	Pins.RemoveAll([&](UEdGraphPin* Pin) { return Pin->Direction != Direction; });
	return Pins;
}

UEdGraphPin* FVoxelLandscapeGrassGraph::GetInputPin(UEdGraphNode* Node, int32 Index)
{
	return GetPins(Node, EGPD_Input)[Index];
}

UEdGraphPin* FVoxelLandscapeGrassGraph::GetOutputPin(UEdGraphNode* Node, int32 Index)
{
	return GetPins(Node, EGPD_Output)[Index];
}

UEdGraphPin* FVoxelLandscapeGrassGraph::GetInputPin(UVoxelNode* Node, int32 Index)
{
	return GetInputPin(Node->GraphNode, Index);
}

UEdGraphPin* FVoxelLandscapeGrassGraph::GetOutputPin(UVoxelNode* Node, int32 Index)
{
	return GetOutputPin(Node->GraphNode, Index);
}