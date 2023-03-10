// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelGraphOutputs.h"
#include "VoxelMinimal.h"

class FVoxelGraphContext;
class UVoxelGraphPreviewSettings;
class UVoxelGraphGenerator;
class UVoxelNode;
class FVoxelGraphCompiler;
class FVoxelCompilationNode;
class FVoxelGraphErrorReporter;
struct FVoxelCompiledGraphs;
class FVoxelGraph;
class FVoxelComputeNode;

class FVoxelGraphCompilerManager
{
public:
	FVoxelGraphCompilerManager(
		const FVoxelGraphContext& Context,
		UVoxelGraphGenerator* Graph, 
		bool bEnableOptimizations,
		bool bPreview,
		const UVoxelGraphPreviewSettings* PreviewSettings,
		bool bAutomaticPreview,
		bool bOnlyShowAxisDependencies);
	~FVoxelGraphCompilerManager();

	// Might return true with OutError not empty
	bool Compile(FVoxelCompiledGraphs& OutGraphs);

private:
	const FVoxelGraphContext& Context;
	UVoxelGraphGenerator* const Graph;
	const bool bEnableOptimizations;
	const bool bPreview;
	const UVoxelGraphPreviewSettings* PreviewSettings;
	const bool bAutomaticPreview;
	const bool bOnlyShowAxisDependencies;
	
	TMap<UVoxelNode*, FVoxelCompilationNode*> NodesMap;

	bool CompileInternal(FVoxelGraphCompiler& Compiler, FVoxelCompiledGraphs& OutGraphs);
	bool CompileOutput(const FVoxelGraphPermutationArray& Permutation, const FString& OutputName, FVoxelGraphCompiler& Compiler, TVoxelSharedPtr<FVoxelGraph>& OutGraph);
	void SetupPreview(FVoxelGraphCompiler& Compiler);
	void DebugCompiler(const FVoxelGraphCompiler& Compiler) const;
	void DebugNodes(const TSet<FVoxelCompilationNode*>& Nodes) const;
	void Optimize(FVoxelGraphCompiler& Compiler) const;

	friend struct FVoxelCompilationFunction;
};
