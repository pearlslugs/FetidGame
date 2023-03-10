// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_NotifyTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_NotifyTools"

#pragma region Add Notify Tool
FText UVAT_NotifyAddTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyAddTool", "NotifyAddTool"));
}

FText UVAT_NotifyAddTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyAddTooltip", "Add animation notifies."));
}

FSlateIcon UVAT_NotifyAddTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyAddTool");
}

void UVAT_NotifyAddTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyAddTool", "Add", "Add animation notifies", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyAddTool = UICommandInfo;
}

bool UVAT_NotifyAddTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyAddTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::AddNotifies);
	}
}
#pragma endregion


#pragma region Modify Notify Tool
FText UVAT_NotifyModifyTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyModifyTool", "NotifyModifyTool"));
}

FText UVAT_NotifyModifyTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyModifyTooltip", "Modify animation notifies."));
}

FSlateIcon UVAT_NotifyModifyTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyModifyTool");
}

void UVAT_NotifyModifyTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyModifyTool", "Modify", "Modify animation notifies", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyModifyTool = UICommandInfo;
}

bool UVAT_NotifyModifyTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyModifyTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::ModifyNotifies);
	}
}
#pragma endregion


#pragma region Remove Notify Tool
FText UVAT_NotifyRemoveTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyRemoveTool", "NotifyRemoveTool"));
}

FText UVAT_NotifyRemoveTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyRemoveTooltip", "Remove animation notifies."));
}

FSlateIcon UVAT_NotifyRemoveTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyRemoveTool");
}

void UVAT_NotifyRemoveTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyRemoveTool", "Remove", "Remove animation notifies", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyRemoveTool = UICommandInfo;
}

bool UVAT_NotifyRemoveTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyRemoveTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::RemoveNotifies);
	}
}
#pragma endregion


#pragma region Add Notify Track Tool
FText UVAT_NotifyAddTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyAddTrackTool", "NotifyAddTrackTool"));
}

FText UVAT_NotifyAddTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyAddTrackTooltip", "Add animation notify track."));
}

FSlateIcon UVAT_NotifyAddTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyAddTrackTool");
}

void UVAT_NotifyAddTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyAddTrackTool", "AddTrack", "Add animation notify track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyAddTrackTool = UICommandInfo;
}

bool UVAT_NotifyAddTrackTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyAddTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::AddNotifiesTrack);
	}
}
#pragma endregion


#pragma region Modify Notify Track Tool
FText UVAT_NotifyModifyTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyModifyTrackTool", "NotifyModifyTrackTool"));
}

FText UVAT_NotifyModifyTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyModifyTrackTooltip", "Modify animation notify track."));
}

FSlateIcon UVAT_NotifyModifyTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyModifyTrackTool");
}

void UVAT_NotifyModifyTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyModifyTrackTool", "ModifyTrack", "Modify animation notify track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyModifyTrackTool = UICommandInfo;
}

bool UVAT_NotifyModifyTrackTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyModifyTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::ModifyNotifiesTrack);
	}
}
#pragma endregion


#pragma region Remove Notify Track Tool
FText UVAT_NotifyRemoveTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyRemoveTrackTool", "NotifyRemoveTrackTool"));
}

FText UVAT_NotifyRemoveTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyRemoveTrackTooltip", "Remove animation notify track."));
}

FSlateIcon UVAT_NotifyRemoveTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyRemoveTrackTool");
}

void UVAT_NotifyRemoveTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyRemoveTrackTool", "RemoveTrack", "Remove animation notify track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyRemoveTrackTool = UICommandInfo;
}

bool UVAT_NotifyRemoveTrackTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyRemoveTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::RemoveNotifiesTrack);
	}
}
#pragma endregion


#pragma region Foot Step Notify Tool
FText UVAT_NotifyFootStepTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyFootStepTool", "NotifyFootStepTool"));
}

FText UVAT_NotifyFootStepTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyFootStepTooltip", "Sample foot step notifies."));
}

FSlateIcon UVAT_NotifyFootStepTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "NotifyFootStepTool");
}

void UVAT_NotifyFootStepTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyFootStepTool", "FootStep", "Sample foot step notifies", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyFootStepTool = UICommandInfo;
}

bool UVAT_NotifyFootStepTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyFootStepTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::FootStep);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
