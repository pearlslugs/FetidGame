// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Style/VAT_EditorStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#if ENGINE_MAJOR_VERSION > 4
#include "Styling/SlateStyleMacros.h"
#endif


FName FVAT_EditorStyle::StyleName("VAT_EditorStyle");

FVAT_EditorStyle::FVAT_EditorStyle()
	: FSlateStyleSet(StyleName)
{
	const FVector2D IconSize(40.f, 40.f);
	const FVector2D SmallIconSize(20.0f, 20.0f);
	const FVector2D ToolIconSize(20.f, 20.0f);

	SetContentRoot(IPluginManager::Get().FindPlugin("VirtualAnimationTools")->GetBaseDir() / TEXT("Resources"));

	const FSlateColor DefaultForeground(FLinearColor(0.72f, 0.72f, 0.72f, 1.f));

	// Edit mode styles
	{
		Set("VirtualAnimationToolsEditor", new IMAGE_BRUSH("VAT_Mode", IconSize));
		Set("VirtualAnimationToolsEditor.Small", new IMAGE_BRUSH("VAT_Mode", SmallIconSize));
	}

	// Tool styles
	{
		Set("VirtualAnimationToolsEditor.BoneAddTool", new IMAGE_BRUSH("BoneTools/AddBone", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneRemoveTool", new IMAGE_BRUSH("BoneTools/RemoveBone", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneAddTrackTool", new IMAGE_BRUSH("BoneTools/AddTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneModifyTrackTool", new IMAGE_BRUSH("BoneTools/ModifyTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneRemoveTrackTool", new IMAGE_BRUSH("BoneTools/RemoveTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneBakeTool", new IMAGE_BRUSH("BoneTools/Bake", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneFilterTool", new IMAGE_BRUSH("BoneTools/Filter", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneLayerTool", new IMAGE_BRUSH("BoneTools/Layer", ToolIconSize));
		Set("VirtualAnimationToolsEditor.BoneConstraintTool", new IMAGE_BRUSH("BoneTools/Constraint", ToolIconSize));

		Set("VirtualAnimationToolsEditor.CurveMotionTool", new IMAGE_BRUSH("CurveTools/Motion", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveBoneTool", new IMAGE_BRUSH("CurveTools/Bone", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveCopyTool", new IMAGE_BRUSH("CurveTools/Copy", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveSortTool", new IMAGE_BRUSH("CurveTools/Sort", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveScaleTool", new IMAGE_BRUSH("CurveTools/Scale", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveRemoveTool", new IMAGE_BRUSH("CurveTools/Remove", ToolIconSize));
		Set("VirtualAnimationToolsEditor.CurveOutputTool", new IMAGE_BRUSH("CurveTools/Output", ToolIconSize));

		Set("VirtualAnimationToolsEditor.NotifyAddTool", new IMAGE_BRUSH("NotifyTools/Add", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyModifyTool", new IMAGE_BRUSH("NotifyTools/Modify", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyRemoveTool", new IMAGE_BRUSH("NotifyTools/Remove", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyAddTrackTool", new IMAGE_BRUSH("NotifyTools/AddTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyModifyTrackTool", new IMAGE_BRUSH("NotifyTools/ModifyTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyRemoveTrackTool", new IMAGE_BRUSH("NotifyTools/RemoveTrack", ToolIconSize));
		Set("VirtualAnimationToolsEditor.NotifyFootStepTool", new IMAGE_BRUSH("NotifyTools/FootStep", ToolIconSize));

		Set("VirtualAnimationToolsEditor.MontageModifierTool", new IMAGE_BRUSH("MontageTools/Modifier", ToolIconSize));
		Set("VirtualAnimationToolsEditor.MontageSortTool", new IMAGE_BRUSH("MontageTools/Sort", ToolIconSize));
		Set("VirtualAnimationToolsEditor.MontageLoopTool", new IMAGE_BRUSH("MontageTools/Loop", ToolIconSize));

		Set("VirtualAnimationToolsEditor.AssetCropTool", new IMAGE_BRUSH("AssetTools/Crop", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetInsertTool", new IMAGE_BRUSH("AssetTools/Insert", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetResizeTool", new IMAGE_BRUSH("AssetTools/Resize", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetCompositeTool", new IMAGE_BRUSH("AssetTools/Composite", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetResizeRootMotionTool", new IMAGE_BRUSH("AssetTools/ResizeRootMotion", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetConvertRootMotionTool", new IMAGE_BRUSH("AssetTools/ConvertRootMotion", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetMoCapToRefPose", new IMAGE_BRUSH("AssetTools/MoCapToRefPose", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetGenerateLODTool", new IMAGE_BRUSH("AssetTools/GenerateLOD", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetRemoveLODTool", new IMAGE_BRUSH("AssetTools/RemoveLOD", ToolIconSize));
		Set("VirtualAnimationToolsEditor.AssetExportTool", new IMAGE_BRUSH("AssetTools/Export", ToolIconSize));

		Set("VirtualAnimationToolsEditor.MirrorBakeTool", new IMAGE_BRUSH("MirrorTools/Mirror", ToolIconSize));
		Set("VirtualAnimationToolsEditor.MirrorSmartMirrorBoneTreeTool", new IMAGE_BRUSH("MirrorTools/SmartMirrorTree", ToolIconSize));

		Set("VirtualAnimationToolsEditor.RetargetPoseTool", new IMAGE_BRUSH("RetargetTools/Pose", ToolIconSize));
		Set("VirtualAnimationToolsEditor.RetargetAnimationTool", new IMAGE_BRUSH("RetargetTools/Animation", ToolIconSize));

		Set("VirtualAnimationToolsEditor.PoseSearchDistanceTool", new IMAGE_BRUSH("PoseSearchTools/Distance", ToolIconSize));
		Set("VirtualAnimationToolsEditor.PoseSearchAnimationTool", new IMAGE_BRUSH("PoseSearchTools/Animation", ToolIconSize));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FVAT_EditorStyle::~FVAT_EditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

FVAT_EditorStyle& FVAT_EditorStyle::Get()
{
	static FVAT_EditorStyle Inst;
	return Inst;
}


