// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_MirrorTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_MirrorTools"

#pragma region Bake Mirror Tool
FText UVAT_MirrorBakeTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorBakeTool", "MirrorBakeTool"));
}

FText UVAT_MirrorBakeTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorBakeTooltip", "Sample mirror animation asset."));
}

FSlateIcon UVAT_MirrorBakeTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MirrorBakeTool");
}

void UVAT_MirrorBakeTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MirrorBakeTool", "Mirror", "Sample mirror animation asset", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MirrorBakeTool = UICommandInfo;
}

bool UVAT_MirrorBakeTool::CanExecute() const
{
	return true;
}

void UVAT_MirrorBakeTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetMirrorToolsType(EVirtualMirrorToolsType::Mirror);
	}
}
#pragma endregion


#pragma region Smart Mirror Tree Tool
FText UVAT_MirrorSmartTreeTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorSmartTreeTool", "MirrorSmartTreeTool"));
}

FText UVAT_MirrorSmartTreeTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorSmartTreeTooltip", "Sample mirror bone tree."));
}

FSlateIcon UVAT_MirrorSmartTreeTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "MirrorSmartMirrorBoneTreeTool");
}

void UVAT_MirrorSmartTreeTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MirrorSmartMirrorBoneTreeTool", "MirrorTree", "Sample mirror bone tree", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MirrorSmartMirrorBoneTreeTool = UICommandInfo;
}

bool UVAT_MirrorSmartTreeTool::CanExecute() const
{
	return true;
}

void UVAT_MirrorSmartTreeTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetMirrorToolsType(EVirtualMirrorToolsType::MirrorBoneTree);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
