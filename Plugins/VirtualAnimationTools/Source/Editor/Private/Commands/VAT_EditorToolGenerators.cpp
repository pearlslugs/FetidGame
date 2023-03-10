// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_EditorToolGenerators.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

//#include "VAT_EditorToolContext.h"


#define LOCTEXT_NAMESPACE "VAT_EditorToolGenerators"

#pragma region Bone Tool
FText UVAT_BoneTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneTool", "BoneTool"));
}

FText UVAT_BoneTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneTooltip", "Bake Animation Bone Tool."));
}

FSlateIcon UVAT_BoneTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.BoneTool");
}

void UVAT_BoneTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneTool", "Bone", "Bake Animation Bone Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneTool = UICommandInfo;
}

bool UVAT_BoneTool::CanExecute() const
{
	return true;
}

void UVAT_BoneTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::BakeBoneTransform);
	}
}
#pragma endregion


#pragma region Curve Tool
FText UVAT_CurveTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveTool", "CurveTool"));
}

FText UVAT_CurveTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "CurveTooltip", "Bake Animation Curve Tool."));
}

FSlateIcon UVAT_CurveTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.CurveTool");
}

void UVAT_CurveTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "CurveTool", "Curve", "Bake Animation Curve Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->CurveTool = UICommandInfo;
}

bool UVAT_CurveTool::CanExecute() const
{
	return true;
}

void UVAT_CurveTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualCurveToolType(EVirtualCurveToolsType::Motion);
	}
}
#pragma endregion


#pragma region Notify Tool
FText UVAT_NotifyTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyTool", "NotifyTool"));
}

FText UVAT_NotifyTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "NotifyTooltip", "Bake Animation Notify Tool."));
}

FSlateIcon UVAT_NotifyTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.NotifyTool");
}

void UVAT_NotifyTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "NotifyTool", "Notify", "Bake Animation Notify Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->NotifyTool = UICommandInfo;
}

bool UVAT_NotifyTool::CanExecute() const
{
	return true;
}

void UVAT_NotifyTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualNotifyToolType(EVirtualNotifyToolsType::AddNotifies);
	}
}
#pragma endregion


#pragma region Montage Tool
FText UVAT_MontageTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageTool", "MontageTool"));
}

FText UVAT_MontageTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MontageTooltip", "Bake Animation Montage Tool."));
}

FSlateIcon UVAT_MontageTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.MontageTool");
}

void UVAT_MontageTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MontageTool", "Montage", "Bake Animation Montage Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MontageTool = UICommandInfo;
}

bool UVAT_MontageTool::CanExecute() const
{
	return true;
}

void UVAT_MontageTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualMontageToolType(EVirtualMontageToolsType::Modifier);
	}
}
#pragma endregion


#pragma region Asset Tool
FText UVAT_AssetTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetTool", "AssetTool"));
}

FText UVAT_AssetTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetTooltip", "Bake Animation Asset Tool."));
}

FSlateIcon UVAT_AssetTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.AssetTool");
}

void UVAT_AssetTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetTool", "Asset", "Bake Animation Asset Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetTool = UICommandInfo;
}

bool UVAT_AssetTool::CanExecute() const
{
	return true;
}

void UVAT_AssetTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Crop);
	}
}
#pragma endregion


#pragma region Mirror Tool
FText UVAT_MirrorTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorTool", "MirrorTool"));
}

FText UVAT_MirrorTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "MirrorTooltip", "Bake Animation Mirror Tool."));
}

FSlateIcon UVAT_MirrorTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.MirrorTool");
}

void UVAT_MirrorTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "MirrorTool", "Mirror", "Bake Animation Mirror Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->MirrorTool = UICommandInfo;
}

bool UVAT_MirrorTool::CanExecute() const
{
	return true;
}

void UVAT_MirrorTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetMirrorToolsType(EVirtualMirrorToolsType::Mirror);
	}
}
#pragma endregion


#pragma region Retarget Tool
FText UVAT_RetargetTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetTool", "RetargetTool"));
}

FText UVAT_RetargetTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "RetargetTooltip", "Bake Animation Retarget Tool."));
}

FSlateIcon UVAT_RetargetTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "VAT_Tools.RetargetTool");
}

void UVAT_RetargetTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "RetargetTool", "Retarget", "Bake Animation Retarget Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->RetargetTool = UICommandInfo;
}

bool UVAT_RetargetTool::CanExecute() const
{
	return true;
}

void UVAT_RetargetTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetRetargetToolsType(EVirtualRetargetToolsType::Pose);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
