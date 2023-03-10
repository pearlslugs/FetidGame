// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_GameFrameworkTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_GameFrameworkTools"

#pragma region FootIK Game Tool
FText UVAT_GameFootIKTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootIKTool", "GameFootIKTool"));
}

FText UVAT_GameFootIKTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootIKTooltip", "FootIK Animation Game Tool."));
}

FSlateIcon UVAT_GameFootIKTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GameFootIKTool");
}

void UVAT_GameFootIKTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GameFootIKTool", "FootIK", "FootIK Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GameFootIKTool = UICommandInfo;
}

bool UVAT_GameFootIKTool::CanExecute() const
{
	return true;
}

void UVAT_GameFootIKTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::FootIK);
	}
}
#pragma endregion


#pragma region FootLock Game Tool
FText UVAT_GameFootLockTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootLockTool", "GameFootLockTool"));
}

FText UVAT_GameFootLockTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootLockTooltip", "FootLock Animation Game Tool."));
}

FSlateIcon UVAT_GameFootLockTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GameFootLockTool");
}

void UVAT_GameFootLockTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GameFootLockTool", "FootLock", "FootLock Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GameFootLockTool = UICommandInfo;
}

bool UVAT_GameFootLockTool::CanExecute() const
{
	return true;
}

void UVAT_GameFootLockTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::FootLock);
	}
}
#pragma endregion


#pragma region FootPosition Game Tool
FText UVAT_GameFootPositionTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootPositionTool", "GameFootPositionTool"));
}

FText UVAT_GameFootPositionTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootPositionTooltip", "FootPosition Animation Game Tool."));
}

FSlateIcon UVAT_GameFootPositionTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GameFootPositionTool");
}

void UVAT_GameFootPositionTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GameFootPositionTool", "FootPosition", "FootPosition Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GameFootPositionTool = UICommandInfo;
}

bool UVAT_GameFootPositionTool::CanExecute() const
{
	return true;
}

void UVAT_GameFootPositionTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::FootPosition);
	}
}
#pragma endregion


#pragma region FootWeight Game Tool
FText UVAT_GameFootWeightTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootWeightTool", "GameFootWeightTool"));
}

FText UVAT_GameFootWeightTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootWeightTooltip", "FootWeight Animation Game Tool."));
}

FSlateIcon UVAT_GameFootWeightTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GameFootWeightTool");
}

void UVAT_GameFootWeightTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GameFootWeightTool", "FootWeight", "FootWeight Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GameFootWeightTool = UICommandInfo;
}

bool UVAT_GameFootWeightTool::CanExecute() const
{
	return true;
}

void UVAT_GameFootWeightTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::FootWeight);
	}
}
#pragma endregion


#pragma region Foot Offset Game Tool
FText UVAT_GameFootOffsetTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootOffsetTool", "GameFootOffsetTool"));
}

FText UVAT_GameFootOffsetTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GameFootOffsetTooltip", "FootOffset Animation Game Tool."));
}

FSlateIcon UVAT_GameFootOffsetTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GameFootOffsetTool");
}

void UVAT_GameFootOffsetTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GameFootOffsetTool", "FootOffset", "FootOffset Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GameFootOffsetTool = UICommandInfo;
}

bool UVAT_GameFootOffsetTool::CanExecute() const
{
	return true;
}

void UVAT_GameFootOffsetTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::FootOffset);
	}
}
#pragma endregion


#pragma region Poses Curve Game Tool
FText UVAT_GamePosesCurveTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GamePosesCurveTool", "GamePosesCurveTool"));
}

FText UVAT_GamePosesCurveTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "GamePosesCurveTooltip", "Output poses blend curve game tool."));
}

FSlateIcon UVAT_GamePosesCurveTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "GamePosesCurveTool");
}

void UVAT_GamePosesCurveTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "GamePosesCurveTool", "PosesCurve", "PosesCurve Animation Game Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->GamePosesCurveTool = UICommandInfo;
}

bool UVAT_GamePosesCurveTool::CanExecute() const
{
	return true;
}

void UVAT_GamePosesCurveTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType::PosesCurve);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
