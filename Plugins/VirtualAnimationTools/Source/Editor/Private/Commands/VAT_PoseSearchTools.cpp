// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_PoseSearchTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_PoseSearchTools"

#pragma region PoseSearch Skeleton Tool
FText UVAT_PoseSearchPoseTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchSkeletonTool", "PoseSearchSkeletonTool"));
}

FText UVAT_PoseSearchPoseTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchSkeletonTooltip", "Bake Skeleton PoseSearch Tool."));
}

FSlateIcon UVAT_PoseSearchPoseTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "PoseSearchSkeletonTool");
}

void UVAT_PoseSearchPoseTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "PoseSearchSkeletonTool", "Pose", "Bake Skeleton PoseSearch Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->PoseSearchAnimationTool = UICommandInfo;
}

bool UVAT_PoseSearchPoseTool::CanExecute() const
{
	return true;
}

void UVAT_PoseSearchPoseTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetPoseSearchToolsType(EVirtualPoseSearchToolsType::Animation);
	}
}
#pragma endregion


#pragma region PoseSearch Distance Tool
FText UVAT_PoseSearchDistanceTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchDistanceTool", "PoseSearchDistanceTool"));
}

FText UVAT_PoseSearchDistanceTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchDistanceTooltip", "Bake Skeleton PoseSearch Tool."));
}

FSlateIcon UVAT_PoseSearchDistanceTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "PoseSearchDistanceTool");
}

void UVAT_PoseSearchDistanceTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "PoseSearchDistanceTool", "Distance", "Bake compressed data for pose search", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->PoseSearchDistanceTool = UICommandInfo;
}

bool UVAT_PoseSearchDistanceTool::CanExecute() const
{
	return true;
}

void UVAT_PoseSearchDistanceTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetPoseSearchToolsType(EVirtualPoseSearchToolsType::Distance);
	}
}
#pragma endregion


#pragma region PoseSearch TimeSync Tool
FText UVAT_PoseSearchTimeSyncTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchTimeSyncTool", "PoseSearchTimeSyncTool"));
}

FText UVAT_PoseSearchTimeSyncTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchTimeSyncTooltip", "Bake Skeleton PoseSearch Tool."));
}

FSlateIcon UVAT_PoseSearchTimeSyncTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "PoseSearchTimeSyncTool");
}

void UVAT_PoseSearchTimeSyncTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "PoseSearchTimeSyncTool", "Time Sync", "Output the jump position between two animation poses", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->PoseSearchTimeSyncTool = UICommandInfo;
}

bool UVAT_PoseSearchTimeSyncTool::CanExecute() const
{
	return true;
}

void UVAT_PoseSearchTimeSyncTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetPoseSearchToolsType(EVirtualPoseSearchToolsType::TimeSync);
	}
}
#pragma endregion


#pragma region PoseSearch Animation Tool
FText UVAT_PoseSearchAnimationTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchAnimationTool", "PoseSearchAnimationTool"));
}

FText UVAT_PoseSearchAnimationTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "PoseSearchAnimationTooltip", "Bake Skeleton PoseSearch Tool."));
}

FSlateIcon UVAT_PoseSearchAnimationTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "PoseSearchAnimationTool");
}

void UVAT_PoseSearchAnimationTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "PoseSearchAnimationTool", "Animation", "Output the jump position between two animation poses", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->PoseSearchAnimationTool = UICommandInfo;
}

bool UVAT_PoseSearchAnimationTool::CanExecute() const
{
	return true;
}

void UVAT_PoseSearchAnimationTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetPoseSearchToolsType(EVirtualPoseSearchToolsType::Animation);
	}
}
#pragma endregion

#undef LOCTEXT_NAMESPACE
