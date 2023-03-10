// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "VoxelLandscapeGrass.h"

class UVoxelNode;
class UMaterialExpression;
class UVoxelGraphGenerator;

class FVoxelLandscapeGrassGraph : public IVoxelLandscapeGrassGraph
{
public:
	static void Register();

public:
	TFunction<UObject* (UClass* Class)> CreateGraph;

	virtual UVoxelGenerator* Convert(const UVoxelLandscapeMaterialCollection& MaterialCollection) override;

private:
	TMap<UMaterialExpression*, UVoxelNode*> ExpressionsToNodes;

	UVoxelNode* GetExpressionNode(UVoxelGraphGenerator& Graph, UMaterialExpression* Expression);
	void MoveLinks(UVoxelGraphGenerator& Graph, UMaterialExpression& Expression, UVoxelNode& VoxelNode);

	static TArray<UEdGraphPin*> GetPins(UEdGraphNode* Node, EEdGraphPinDirection Direction);
	
	static UEdGraphPin* GetInputPin(UEdGraphNode* Node, int32 Index);
	static UEdGraphPin* GetOutputPin(UEdGraphNode* Node, int32 Index);
	
	static UEdGraphPin* GetInputPin(UVoxelNode* Node, int32 Index);
	static UEdGraphPin* GetOutputPin(UVoxelNode* Node, int32 Index);
};