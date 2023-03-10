// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_RetargetTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_RetargetTools"

#pragma region Retarget Skeleton Tool
FText UVAT_RetargetPoseTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetPoseTool", "RetargetPoseTool"));
}

FText UVAT_RetargetPoseTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetSkeletonTooltip", "Bake Skeleton Retarget Tool."));
}

FSlateIcon UVAT_RetargetPoseTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "RetargetPoseTool");
}

void UVAT_RetargetPoseTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "RetargetPoseTool", "Pose", "Bake Skeleton Retarget Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->RetargetPoseTool = UICommandInfo;
}

bool UVAT_RetargetPoseTool::CanExecute() const
{
	return true;
}

void UVAT_RetargetPoseTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetRetargetToolsType(EVirtualRetargetToolsType::Pose);
	}
}
#pragma endregion


#pragma region Retarget Animation Tool
FText UVAT_RetargetAnimationTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetAnimationTool", "RetargetAnimationTool"));
}

FText UVAT_RetargetAnimationTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetAnimationTooltip", "Bake Skeleton Retarget Tool."));
}

FSlateIcon UVAT_RetargetAnimationTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "RetargetAnimationTool");
}

void UVAT_RetargetAnimationTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "RetargetAnimationTool", "Animation", "Bake Skeleton Retarget Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->RetargetAnimationTool = UICommandInfo;
}

bool UVAT_RetargetAnimationTool::CanExecute() const
{
	return true;
}

void UVAT_RetargetAnimationTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetRetargetToolsType(EVirtualRetargetToolsType::Animation);
	}
}
#pragma endregion


#pragma region Retarget Rebuild Tool
FText UVAT_RetargetRebuildTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualRebuildTools", "RetargetRebuildTool", "RetargetRebuildTool"));
}

FText UVAT_RetargetRebuildTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualRebuildTools", "RetargetRebuildTooltip", "Bake Skeleton Retarget Tool."));
}

FSlateIcon UVAT_RetargetRebuildTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "RetargetRebuildTool");
}

void UVAT_RetargetRebuildTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "RetargetRebuildTool", "Rebuild", "Rebuild animation, fix error bone data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->RetargetRebuildTool = UICommandInfo;
}

bool UVAT_RetargetRebuildTool::CanExecute() const
{
	return true;
}

void UVAT_RetargetRebuildTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetRetargetToolsType(EVirtualRetargetToolsType::Rebuild);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
