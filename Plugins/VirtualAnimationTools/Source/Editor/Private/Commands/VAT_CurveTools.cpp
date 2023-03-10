// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_CurveTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_CurveTools"

#pragma region Curve Motion Tool
FText UVAT_CurveMotionTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveMotionTool", "CurveMotionTool"));
}

FText UVAT_CurveMotionTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveMotionTooltip", "Sample animation motion curves."));
}

FSlateIcon UVAT_CurveMotionTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveMotionTool");
}

void UVAT_CurveMotionTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveMotionTool", "Motion", "Sample animation motion curves", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveMotionTool = UICommandInfo;
}

bool UVAT_CurveMotionTool::CanExecute() const
{
	return true;
}

void UVAT_CurveMotionTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Motion);
	}
}
#pragma endregion


#pragma region Curve Bone Tool
FText UVAT_CurveBoneTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveBoneTool", "CurveBoneTool"));
}

FText UVAT_CurveBoneTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveBoneTooltip", "Sample animation bone curves."));
}

FSlateIcon UVAT_CurveBoneTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveBoneTool");
}

void UVAT_CurveBoneTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveBoneTool", "Bone", "Sample animation bone curves", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveBoneTool = UICommandInfo;
}

bool UVAT_CurveBoneTool::CanExecute() const
{
	return true;
}

void UVAT_CurveBoneTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Bone);
	}
}
#pragma endregion


#pragma region Curve Copy Tool
FText UVAT_CurveCopyTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveCopyTool", "CurveCopyTool"));
}

FText UVAT_CurveCopyTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveCopyTooltip", "Copy Animation Curve Tool."));
}

FSlateIcon UVAT_CurveCopyTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveCopyTool");
}

void UVAT_CurveCopyTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveCopyTool", "Copy", "Copy Animation Curve Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveCopyTool = UICommandInfo;
}

bool UVAT_CurveCopyTool::CanExecute() const
{
	return true;
}

void UVAT_CurveCopyTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Copy);
	}
}
#pragma endregion


#pragma region Curve Transfer Tool
FText UVAT_CurveTransferTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveTransferTool", "CurveTransferTool"));
}

FText UVAT_CurveTransferTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveTransferTooltip", "Transfer Animation Curve Tool."));
}

FSlateIcon UVAT_CurveTransferTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveTransferTool");
}

void UVAT_CurveTransferTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveTransferTool", "Transfer", "Transfer Animation Curve Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveTransferTool = UICommandInfo;
}

bool UVAT_CurveTransferTool::CanExecute() const
{
	return true;
}

void UVAT_CurveTransferTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Transfer);
	}
}
#pragma endregion


#pragma region Curve Sort Tool
FText UVAT_CurveSortTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveSortTool", "CurveSortTool"));
}

FText UVAT_CurveSortTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveSortTooltip", "Sorts the current animation curves according to the initial order of the curves."));
}

FSlateIcon UVAT_CurveSortTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveSortTool");
}

void UVAT_CurveSortTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveSortTool", "Sort", "Sorts the current animation curves according to the initial order of the curves", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveSortTool = UICommandInfo;
}

bool UVAT_CurveSortTool::CanExecute() const
{
	return true;
}

void UVAT_CurveSortTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Sort);
	}
}
#pragma endregion


#pragma region Curve Scale Tool
FText UVAT_CurveScaleTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveScaleTool", "CurveScaleTool"));
}

FText UVAT_CurveScaleTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveScaleTooltip", "Scale Animation Curve Tool."));
}

FSlateIcon UVAT_CurveScaleTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveScaleTool");
}

void UVAT_CurveScaleTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveScaleTool", "Scale", "Scale Animation Curve Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveScaleTool = UICommandInfo;
}

bool UVAT_CurveScaleTool::CanExecute() const
{
	return true;
}

void UVAT_CurveScaleTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Scale);
	}
}
#pragma endregion


#pragma region Curve Remove Tool
FText UVAT_CurveRemoveTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveRemoveTool", "CurveRemoveTool"));
}

FText UVAT_CurveRemoveTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveRemoveTooltip", "Remove Animation Curve Tool."));
}

FSlateIcon UVAT_CurveRemoveTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveRemoveTool");
}

void UVAT_CurveRemoveTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveRemoveTool", "Remove", "Remove animation curves", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveRemoveTool = UICommandInfo;
}

bool UVAT_CurveRemoveTool::CanExecute() const
{
	return true;
}

void UVAT_CurveRemoveTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Remove);
	}
}
#pragma endregion


#pragma region Curve Output Tool
FText UVAT_CurveOutputTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveOutputTool", "CurveOutputTool"));
}

FText UVAT_CurveOutputTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveOutputTooltip", "Output the specified animation curve to the specified curve asset."));
}

FSlateIcon UVAT_CurveOutputTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "CurveOutputTool");
}

void UVAT_CurveOutputTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveOutputTool", "Output", "Output the specified animation curve to the specified curve asset", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveOutputTool = UICommandInfo;
}

bool UVAT_CurveOutputTool::CanExecute() const
{
	return true;
}

void UVAT_CurveOutputTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Output);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
