// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_MontageTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_MontageTools"

#pragma region Modifier Montage Slot Tool
FText UVAT_MontageModifierTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageModifierTool", "MontageModifierTool"));
}

FText UVAT_MontageModifierTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageModifierTooltip", "Modifier AnimMontage Group And Slot Tool."));
}

FSlateIcon UVAT_MontageModifierTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MontageModifierTool");
}

void UVAT_MontageModifierTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MontageModifierTool", "Modifier", "Modifier animation montage slots tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MontageModifierTool = UICommandInfo;
}

bool UVAT_MontageModifierTool::CanExecute() const
{
	return true;
}

void UVAT_MontageModifierTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualMontageToolType(EVirtualMontageToolsType::Modifier);
	}
}
#pragma endregion


#pragma region Sort Montage Slot Tool
FText UVAT_MontageSortTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageSortTool", "MontageSortTool"));
}

FText UVAT_MontageSortTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageSortTooltip", "Sort AnimMontage Group And Slot Tool."));
}

FSlateIcon UVAT_MontageSortTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MontageSortTool");
}

void UVAT_MontageSortTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MontageSortTool", "Sort", "Sort animation montage Group and Slots", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MontageSortTool = UICommandInfo;
}

bool UVAT_MontageSortTool::CanExecute() const
{
	return true;
}

void UVAT_MontageSortTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualMontageToolType(EVirtualMontageToolsType::Sort);
	}
}
#pragma endregion


#pragma region Montage Loop Tool
FText UVAT_MontageLoopTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageLoopTool", "MontageLoopTool"));
}

FText UVAT_MontageLoopTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageLoopTooltip", "Loop AnimMontage Group And Slot Tool."));
}

FSlateIcon UVAT_MontageLoopTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MontageLoopTool");
}

void UVAT_MontageLoopTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MontageLoopTool", "Loop", "Flag montage slot tracks is looping", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MontageLoopTool = UICommandInfo;
}

bool UVAT_MontageLoopTool::CanExecute() const
{
	return true;
}

void UVAT_MontageLoopTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualMontageToolType(EVirtualMontageToolsType::Loop);
	}
}
#pragma endregion


#pragma region Montage Transfer Tool
FText UVAT_MontageTransferTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageTransferTool", "MontageTransferTool"));
}

FText UVAT_MontageTransferTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageTransferTooltip", "Transfer Skeleton Group And Slot Tool."));
}

FSlateIcon UVAT_MontageTransferTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MontageTransferTool");
}

void UVAT_MontageTransferTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MontageTransferTool", "Transfer", "Transfer source montage groups to target skeleton", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MontageTransferTool = UICommandInfo;
}

bool UVAT_MontageTransferTool::CanExecute() const
{
	return true;
}

void UVAT_MontageTransferTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualMontageToolType(EVirtualMontageToolsType::Transfer);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
