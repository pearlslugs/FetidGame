// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_BoneTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_BoneTools"

#pragma region Bone Bake Tool
FText UVAT_BoneBakeTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneBakeTool", "BoneBakeTool"));
}

FText UVAT_BoneBakeTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneBakeTooltip", "Bake animation bone track data."));
}

FSlateIcon UVAT_BoneBakeTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneBakeTool");
}

void UVAT_BoneBakeTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneBakeTool", "Bake", "Bake animation bone track data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneBakeTool = UICommandInfo;
}

bool UVAT_BoneBakeTool::CanExecute() const
{
	return true;
}

void UVAT_BoneBakeTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::BakeBoneTransform);
	}
}
#pragma endregion


#pragma region Bone Blend Track Tool
FText UVAT_BoneBlendTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneBlendTrackTool", "BoneBlendTool"));
}

FText UVAT_BoneBlendTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneBlendTrackTooltip", "Blend animation bone tracks data."));
}

FSlateIcon UVAT_BoneBlendTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneBlendTrackTool");
}

void UVAT_BoneBlendTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneBlendTrackTool", "BlendTrack", "Blend animation bone tracks data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneBlendTrackTool = UICommandInfo;
}

bool UVAT_BoneBlendTrackTool::CanExecute() const
{
	return true;
}

void UVAT_BoneBlendTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::BlendBoneTrack);
	}
}
#pragma endregion


#pragma region Modify Bone Track Tool
FText UVAT_BoneModifyTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneModifyTrackTool", "BoneModifyTrackTool"));
}

FText UVAT_BoneModifyTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneModifyTrackTooltip", "Modify the specified bone animation track."));
}

FSlateIcon UVAT_BoneModifyTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneModifyTrackTool");
}

void UVAT_BoneModifyTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneModifyTrackTool", "ModifyTrack", "Modify the specified bone animation track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneModifyTrackTool = UICommandInfo;
}

bool UVAT_BoneModifyTrackTool::CanExecute() const
{
	return true;
}

void UVAT_BoneModifyTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::ModifyBoneTrack);
	}
}
#pragma endregion


#pragma region Bone Layer Tool
FText UVAT_BoneLayerTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneLayerTool", "BoneLayerTool"));
}

FText UVAT_BoneLayerTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneLayerTooltip", "Layered blend per bone."));
}

FSlateIcon UVAT_BoneLayerTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneLayerTool");
}

void UVAT_BoneLayerTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneLayerTool", "Layer", "Layered blend per bone", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneLayerPoseTool = UICommandInfo;
}

bool UVAT_BoneLayerTool::CanExecute() const
{
	return true;
}

void UVAT_BoneLayerTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::LayeredBoneBlend);
	}
}
#pragma endregion


#pragma region Bone Constraint Tool
FText UVAT_BoneConstraintTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneConstraintTool", "BoneConstraintTool"));
}

FText UVAT_BoneConstraintTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneConstraintTooltip", "Constraint Animation Bone Transform Tool."));
}

FSlateIcon UVAT_BoneConstraintTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneConstraintTool");
}

void UVAT_BoneConstraintTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneConstraintTool", "Constraint", "Constraint Animation Bone Transform Tool", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneConstraintTool = UICommandInfo;
}

bool UVAT_BoneConstraintTool::CanExecute() const
{
	return true;
}

void UVAT_BoneConstraintTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::ConstraintBone);
	}
}
#pragma endregion


#pragma region Bone Add Track Tool
FText UVAT_BoneAddTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneAddTrackTool", "BoneAddTrackTool"));
}

FText UVAT_BoneAddTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneAddTrackTooltip", "Add the specified bone animation track."));
}

FSlateIcon UVAT_BoneAddTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneAddTrackTool");
}

void UVAT_BoneAddTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneAddTrackTool", "AddTrack", "Add the specified bone animation track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneAddTrackTool = UICommandInfo;
}

bool UVAT_BoneAddTrackTool::CanExecute() const
{
	return true;
}

void UVAT_BoneAddTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::AddBoneTrack);
	}
}
#pragma endregion


#pragma region Bone Remove Track Tool
FText UVAT_BoneRemoveTrackTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneRemoveTrackTool", "BoneRemoveTrackTool"));
}

FText UVAT_BoneRemoveTrackTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneRemoveTrackTooltip", "Remove the specified bone animation track."));
}

FSlateIcon UVAT_BoneRemoveTrackTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneRemoveTrackTool");
}

void UVAT_BoneRemoveTrackTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneRemoveTrackTool", "RemoveTrack", "Remove the specified bone animation track", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneRemoveTrackTool = UICommandInfo;
}

bool UVAT_BoneRemoveTrackTool::CanExecute() const
{
	return true;
}

void UVAT_BoneRemoveTrackTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::RemoveBoneTrack);
	}
}
#pragma endregion


#pragma region Bone Add Tool
FText UVAT_BoneAddTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneAddTool", "BoneAddTool"));
}

FText UVAT_BoneAddTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AddBoneTooltip", "Add bones without weights."));
}

FSlateIcon UVAT_BoneAddTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneAddTool");
}

void UVAT_BoneAddTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneAddTool", "AddBone", "Add bones without weights", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneAddTool = UICommandInfo;
}

bool UVAT_BoneAddTool::CanExecute() const
{
	return true;
}

void UVAT_BoneAddTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::AddBone);
	}
}
#pragma endregion


#pragma region Bone Remove Tool
FText UVAT_BoneRemoveTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneRemoveTool", "BoneRemoveTool"));
}

FText UVAT_BoneRemoveTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneRemoveTooltip", "Delete the specified bone chain."));
}

FSlateIcon UVAT_BoneRemoveTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneRemoveTool");
}

void UVAT_BoneRemoveTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneRemoveTool", "RemoveBone", "Delete the specified bone chain", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneRemoveTool = UICommandInfo;
}

bool UVAT_BoneRemoveTool::CanExecute() const
{
	return true;
}

void UVAT_BoneRemoveTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::RemoveBone);
	}
}
#pragma endregion


#pragma region Bone Filter Tool
FText UVAT_BoneFilterTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneFilterTool", "BoneFilterTool"));
}

FText UVAT_BoneFilterTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "BoneFilterTooltip", "Filter animation bone tracks data."));
}

FSlateIcon UVAT_BoneFilterTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "BoneFilterTool");
}

void UVAT_BoneFilterTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "BoneFilterTool", "Filter", "Filter animation bone tracks data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->BoneFilterTool = UICommandInfo;
}

bool UVAT_BoneFilterTool::CanExecute() const
{
	return true;
}

void UVAT_BoneFilterTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualBoneToolType(EVirtualBoneToolsType::FilterBone);
	}
}
#pragma endregion
#undef LOCTEXT_NAMESPACE
