// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_AssetTools.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Engine/Selection.h"
#include "Style/VAT_EditorStyle.h"

#define LOCTEXT_NAMESPACE "VAT_AssetTools"

#pragma region Crop Animation Tool
FText UVAT_AssetCropTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetCropTool", "AssetCropTool"));
}

FText UVAT_AssetCropTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetCropTooltip", "Crop animation pose frames."));
}

FSlateIcon UVAT_AssetCropTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetCropTool");
}

void UVAT_AssetCropTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetCropTool", "Crop", "Crop animation pose frames", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetCropTool = UICommandInfo;
}

bool UVAT_AssetCropTool::CanExecute() const
{
	return true;
}

void UVAT_AssetCropTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Crop);
	}
}
#pragma endregion


#pragma region Insert Asset Tool
FText UVAT_AssetInsertTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetInsertTool", "AssetInsertTool"));
}

FText UVAT_AssetInsertTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetInsertTooltip", "Insert animation pose frames."));
}

FSlateIcon UVAT_AssetInsertTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetInsertTool");
}

void UVAT_AssetInsertTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetInsertTool", "Insert", "Insert animation pose frames", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetInsertTool = UICommandInfo;
}

bool UVAT_AssetInsertTool::CanExecute() const
{
	return true;
}

void UVAT_AssetInsertTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Insert);
	}
}
#pragma endregion


#pragma region Resize Animation Tool
FText UVAT_AssetResizeTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetResizeTool", "AssetResizeTool"));
}

FText UVAT_AssetResizeTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetResizeTooltip", "Resize animation frame and length."));
}

FSlateIcon UVAT_AssetResizeTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetResizeTool");
}

void UVAT_AssetResizeTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetResizeTool", "Resize", "Resize animation frame and length", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetResizeTool = UICommandInfo;
}

bool UVAT_AssetResizeTool::CanExecute() const
{
	return true;
}

void UVAT_AssetResizeTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Resize);
	}
}
#pragma endregion


#pragma region Replace Animation Tool
FText UVAT_AssetReplaceTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetReplaceTool", "AssetReplaceTool"));
}

FText UVAT_AssetReplaceTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetReplaceTooltip", "Replace animation poses."));
}

FSlateIcon UVAT_AssetReplaceTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetReplaceTool");
}

void UVAT_AssetReplaceTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetReplaceTool", "Replace", "Replace animation frame and length", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetReplaceTool = UICommandInfo;
}

bool UVAT_AssetReplaceTool::CanExecute() const
{
	return true;
}

void UVAT_AssetReplaceTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Replace);
	}
}
#pragma endregion


#pragma region Composite Animation Tool
FText UVAT_AssetCompositeTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetCompositeTool", "AssetCompositeTool"));
}

FText UVAT_AssetCompositeTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetCompositeTooltip", "Merge multiple animation assets."));
}

FSlateIcon UVAT_AssetCompositeTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetCompositeTool");
}

void UVAT_AssetCompositeTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetCompositeTool", "Composite", "Merge multiple animation assets", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetCompositeTool = UICommandInfo;
}

bool UVAT_AssetCompositeTool::CanExecute() const
{
	return true;
}

void UVAT_AssetCompositeTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Composite);
	}
}
#pragma endregion


#pragma region Resize Root Motion Tool
FText UVAT_AssetResizeRootMotionTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetResizeRootMotionTool", "AssetResizeRootMotionTool"));
}

FText UVAT_AssetResizeRootMotionTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetResizeRootMotionTooltip", "Resize animation root motion data."));
}

FSlateIcon UVAT_AssetResizeRootMotionTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetResizeRootMotionTool");
}

void UVAT_AssetResizeRootMotionTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetResizeRootMotionTool", "SampleMotion", "Resize animation root motion data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetResizeRootMotionTool = UICommandInfo;
}

bool UVAT_AssetResizeRootMotionTool::CanExecute() const
{
	return true;
}

void UVAT_AssetResizeRootMotionTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::ResizeRootMotion);
	}
}
#pragma endregion


#pragma region Convert Root Motion Tool
FText UVAT_AssetConvertRootMotionTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetConvertRootMotionTool", "AssetConvertRootMotionTool"));
}

FText UVAT_AssetConvertRootMotionTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetConvertRootMotionTooltip", "Convert animation root motion data."));
}

FSlateIcon UVAT_AssetConvertRootMotionTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetConvertRootMotionTool");
}

void UVAT_AssetConvertRootMotionTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetConvertRootMotionTool", "ConvertMotion", "Convert animation root motion data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetConvertRootMotionTool = UICommandInfo;
}

bool UVAT_AssetConvertRootMotionTool::CanExecute() const
{
	return true;
}

void UVAT_AssetConvertRootMotionTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::ConvertRootMotion);
	}
}
#pragma endregion


#pragma region Alignment Root Motion Tool
FText UVAT_AssetAlignmentRootMotionTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetAlignmentRootMotionTool", "AssetAlignmentRootMotionTool"));
}

FText UVAT_AssetAlignmentRootMotionTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetAlignmentRootMotionTooltip", "Alignment animation root motion data."));
}

FSlateIcon UVAT_AssetAlignmentRootMotionTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetAlignmentRootMotionTool");
}

void UVAT_AssetAlignmentRootMotionTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetAlignmentRootMotionTool", "AlignmentMotion", "Alignment animation root motion data", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetAlignmentRootMotionTool = UICommandInfo;
}

bool UVAT_AssetAlignmentRootMotionTool::CanExecute() const
{
	return true;
}

void UVAT_AssetAlignmentRootMotionTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::AlignmentRootMotion);
	}
}
#pragma endregion


#pragma region Motion Capture To Ref Pose
FText UVAT_AssetMotionCaptureToRefPoseTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetMotionCaptureToRefPoseTool", "AssetMotionCaptureToRefPoseTool"));
}

FText UVAT_AssetMotionCaptureToRefPoseTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetMotionCaptureToRefPoseTooltip", "Convert motion capture asset to reference pose."));
}

FSlateIcon UVAT_AssetMotionCaptureToRefPoseTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetMoCapToRefPose");
}

void UVAT_AssetMotionCaptureToRefPoseTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetMotionCaptureToRefPoseTool", "MoCapToRef", "Convert motion capture asset to reference pose", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetMotionCaptureToRefPoseTool = UICommandInfo;
}

bool UVAT_AssetMotionCaptureToRefPoseTool::CanExecute() const
{
	return true;
}

void UVAT_AssetMotionCaptureToRefPoseTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::ConvertMotionToReferencePose);
	}
}
#pragma endregion


#pragma region GenerateLOD Tool
FText UVAT_AssetGenerateLODTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetGenerateLODTool", "AssetGenerateLODTool"));
}

FText UVAT_AssetGenerateLODTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetGenerateLODTooltip", "Generate asset LODs."));
}

FSlateIcon UVAT_AssetGenerateLODTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetGenerateLODTool");
}

void UVAT_AssetGenerateLODTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetGenerateLODTool", "GenerateLOD", "Generate assets LODs", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetGenerateLODTool = UICommandInfo;
}

bool UVAT_AssetGenerateLODTool::CanExecute() const
{
	return true;
}

void UVAT_AssetGenerateLODTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::GenerateLOD);
	}
}
#pragma endregion


#pragma region Remove LOD Tool
FText UVAT_AssetRemoveLODTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetRemoveLODTool", "AssetRemoveLODTool"));
}

FText UVAT_AssetRemoveLODTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetRemoveLODTooltip", "RemoveLOD Asset Tool."));
}

FSlateIcon UVAT_AssetRemoveLODTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetRemoveLODTool");
}

void UVAT_AssetRemoveLODTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetRemoveLODTool", "RemoveLOD", "Remove assets LODs", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetRemoveLODTool = UICommandInfo;
}

bool UVAT_AssetRemoveLODTool::CanExecute() const
{
	return true;
}

void UVAT_AssetRemoveLODTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::RemoveLOD);
	}
}
#pragma endregion


#pragma region Export Tool
FText UVAT_AssetExportTool::GetDisplayText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetExportTool", "AssetExportTool"));
}

FText UVAT_AssetExportTool::GetTooltipText() const
{
	return FText(NSLOCTEXT("VirtualAnimationTools", "AssetExportTooltip", "Export Asset Tool."));
}

FSlateIcon UVAT_AssetExportTool::GetToolIcon() const
{
	return FSlateIcon("VAT_EditorStyle", "AssetExportTool");
}

void UVAT_AssetExportTool::RegisterUICommand(FVAT_EditorCommands* BindingContext)
{
	UI_COMMAND_EXT(BindingContext, UICommandInfo, "AssetExportTool", "Export", "Export animation assets", EUserInterfaceActionType::Button, FInputChord());
	BindingContext->AssetExportTool = UICommandInfo;
}

bool UVAT_AssetExportTool::CanExecute() const
{
	return true;
}

void UVAT_AssetExportTool::Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit)
{
	if (InToolkit.IsValid())
	{
		TSharedPtr<FVirtualAnimationToolsEdModeToolkit> SharedToolkit(InToolkit.Pin());
		SharedToolkit->SetVirtualAssetToolType(EVirtualAssetToolsType::Export);
	}
}
#pragma endregion


#undef LOCTEXT_NAMESPACE
