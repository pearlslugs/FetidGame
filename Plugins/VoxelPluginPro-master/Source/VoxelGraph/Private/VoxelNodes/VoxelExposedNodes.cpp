// Copyright 2021 Phyronnaz

#include "VoxelNodes/VoxelExposedNodes.h"
#include "VoxelNodes/VoxelNodeColors.h"
#include "CppTranslation/VoxelVariables.h"
#include "Compilation/VoxelDefaultCompilationNodes.h"
#include "VoxelGraphGenerator.h"
#include "Compilation/VoxelGraphContext.h"
#include "VoxelGenerators/VoxelGeneratorParameters.h"

#include "EdGraph/EdGraphNode.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"

TMap<FName, FString> UVoxelExposedNode::GetMetaData() const
{
	auto Result = CustomMetaData;
	Result.Add("DisplayName", DisplayName);

	if (!UIMin.IsEmpty())
	{
		Result.Add("UIMin", UIMin);
	}
	if (!UIMax.IsEmpty())
	{
		Result.Add("UIMax", UIMax);
	}
	
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FLinearColor UVoxelExposedNode::GetColor() const
{
	return FVoxelNodeColors::ExposedNode;
}

FText UVoxelExposedNode::GetTitle() const
{
	auto& Property = GetParameterProperty();
	if (Property.IsA<FFloatProperty>() ||
		Property.IsA<FIntProperty>() ||
		Property.IsA<FBoolProperty>())
	{
		FString Value;
		Property.ExportTextItem(Value, Property.ContainerPtrToValuePtr<void>(this), nullptr, nullptr, PPF_None);
		
		if (Property.IsA<FFloatProperty>())
		{
			Value = FString::SanitizeFloat(FCString::Atof(*Value));
		}
		
		return FText::FromString(FString::Printf(TEXT("%s = %s"), *DisplayName, *Value));
	}
	
	return FText::FromString(DisplayName);
}

bool UVoxelExposedNode::CanRenameNode() const
{
	return bCanBeRenamed;
}

FString UVoxelExposedNode::GetEditableName() const
{
	return DisplayName;
}

void UVoxelExposedNode::SetEditableName(const FString& NewName)
{
	bCanBeRenamed = false;
	DisplayName = *NewName;
	MakeNameUnique();
	MarkPackageDirty();
#if WITH_EDITOR
	if (GraphNode)
	{
		GraphNode->bCanRenameNode = bCanBeRenamed;
		GraphNode->ReconstructNode();
	}
#endif
}

void UVoxelExposedNode::ApplyParameters(const TMap<FName, FString>& Parameters)
{
	auto* NewValuePtr = Parameters.Find(UniqueName);
	if (!NewValuePtr) 
	{
		return;
	}
	
	auto& Property = GetParameterProperty();

	Modify();

	if (!ensure(Property.ImportText(**NewValuePtr, Property.ContainerPtrToValuePtr<void>(this), PPF_None, this)))
	{
		return;
	}
	
#if WITH_EDITOR
	if (GraphNode)
	{
		GraphNode->ReconstructNode();
	}
#endif
}

void UVoxelExposedNode::GetParameters(TArray<FVoxelGeneratorParameter>& OutParameters) const
{
	auto& Property = GetParameterProperty();

	FString DefaultValue;
	Property.ExportTextItem(DefaultValue, Property.ContainerPtrToValuePtr<void>(this), nullptr, nullptr, PPF_None);

	OutParameters.Add(FVoxelGeneratorParameter(
		UniqueName,
		FVoxelGeneratorParameterType(Property),
		DisplayName,
		Category,
		Tooltip,
		Priority,
		GetMetaData(),
		DefaultValue));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelExposedNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		MakeNameUnique();
	}
}
#endif

void UVoxelExposedNode::PostEditImport()
{
	Super::PostEditImport();

	MakeNameUnique();
}

void UVoxelExposedNode::PostLoad()
{
	Super::PostLoad();

	if (DisplayName.IsEmpty() && !UniqueName.IsNone())
	{
		// Fixup old versions that only had UniqueName
		DisplayName = UniqueName.ToString();
		DisplayName = DisplayName.Replace(TEXT("_"), TEXT(" "));
	}
}

const FVoxelGraphContext& UVoxelExposedNode::GetContext(const FVoxelCompilationNode& CompilationNode)
{
	return CompilationNode.Context;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelExposedNode::MakeNameUnique()
{
	if (!Graph)
	{
		return;
	}

	const auto OriginalUniqueName = UniqueName;

	UniqueName = *FVoxelVariable::SanitizeName(DisplayName);

	TSet<FName> Names;
	for (auto* Node : Graph->AllNodes)
	{
		auto* ExposedNode = Cast<UVoxelExposedNode>(Node);
		if (ExposedNode && ExposedNode != this && ExposedNode->GetClass() != GetClass())
		{
			Names.Add(ExposedNode->UniqueName);
		}
	}

	int32 Number = UniqueName.GetNumber();
	while (Names.Contains(UniqueName))
	{
		UniqueName.SetNumber(++Number);
	}
	
	if (OriginalUniqueName != UniqueName)
	{
		MarkPackageDirty();
	}
}

FProperty& UVoxelExposedNode::GetParameterProperty() const
{
	const FName PropertyName = GetParameterPropertyNameInternal();
	FProperty* Property = GetClass()->FindPropertyByName(PropertyName);

	if (ensure(Property))
	{
		return *Property;
	}
	else
	{
		return *GetClass()->FindPropertyByName(GET_MEMBER_NAME_STATIC(UVoxelExposedNode, Priority));
	}
}

const FString* UVoxelExposedNode::GetParameterOverride(const FVoxelGraphContext& Context) const
{
	return Context.Parameters.Find(UniqueName);
}
