// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.
#pragma once

#include "Framework/Commands/Commands.h"

class FVAT_EditorCommands : public TCommands<FVAT_EditorCommands>
{
public:

	FVAT_EditorCommands();

	virtual void RegisterCommands() override;

public:

	// Bone Commands
	TSharedPtr< FUICommandInfo > BoneTool;
	TSharedPtr< FUICommandInfo > BoneBakeTool;
	TSharedPtr< FUICommandInfo > BoneAddTool;
	TSharedPtr< FUICommandInfo > BoneRemoveTool;
	TSharedPtr< FUICommandInfo > BoneFilterTool;
	TSharedPtr< FUICommandInfo > BoneAddTrackTool;
	TSharedPtr< FUICommandInfo > BoneBlendTrackTool;
	TSharedPtr< FUICommandInfo > BoneRemoveTrackTool;
	TSharedPtr< FUICommandInfo > BoneModifyTrackTool;
	TSharedPtr< FUICommandInfo > BoneLayerPoseTool;
	TSharedPtr< FUICommandInfo > BoneConstraintTool;

	// Curve Commands
	TSharedPtr< FUICommandInfo > CurveTool;
	TSharedPtr< FUICommandInfo > CurveMotionTool;
	TSharedPtr< FUICommandInfo > CurveBoneTool;
	TSharedPtr< FUICommandInfo > CurveCopyTool;
	TSharedPtr< FUICommandInfo > CurveTransferTool;
	TSharedPtr< FUICommandInfo > CurveSortTool;
	TSharedPtr< FUICommandInfo > CurveScaleTool;
	TSharedPtr< FUICommandInfo > CurveRemoveTool;
	TSharedPtr< FUICommandInfo > CurveOutputTool;

	// Notify Commands
	TSharedPtr< FUICommandInfo > NotifyTool;
	TSharedPtr< FUICommandInfo > NotifyAddTool;
	TSharedPtr< FUICommandInfo > NotifyModifyTool;
	TSharedPtr< FUICommandInfo > NotifyRemoveTool;
	TSharedPtr< FUICommandInfo > NotifyAddTrackTool;
	TSharedPtr< FUICommandInfo > NotifyModifyTrackTool;
	TSharedPtr< FUICommandInfo > NotifyRemoveTrackTool;
	TSharedPtr< FUICommandInfo > NotifyFootStepTool;

	// Montage Commands
	TSharedPtr< FUICommandInfo > MontageTool;
	TSharedPtr< FUICommandInfo > MontageModifierTool;
	TSharedPtr< FUICommandInfo > MontageSortTool;
	TSharedPtr< FUICommandInfo > MontageLoopTool;
	TSharedPtr< FUICommandInfo > MontageTransferTool;

	// Asset Commands
	TSharedPtr< FUICommandInfo > AssetTool;
	TSharedPtr< FUICommandInfo > AssetCropTool;
	TSharedPtr< FUICommandInfo > AssetInsertTool;
	TSharedPtr< FUICommandInfo > AssetResizeTool;
	TSharedPtr< FUICommandInfo > AssetReplaceTool;
	TSharedPtr< FUICommandInfo > AssetCompositeTool;

	TSharedPtr< FUICommandInfo > AssetResizeRootMotionTool;
	TSharedPtr< FUICommandInfo > AssetConvertRootMotionTool;
	TSharedPtr< FUICommandInfo > AssetAlignmentRootMotionTool;
	TSharedPtr< FUICommandInfo > AssetMotionCaptureToRefPoseTool;

	TSharedPtr< FUICommandInfo > AssetGenerateLODTool;
	TSharedPtr< FUICommandInfo > AssetRemoveLODTool;
	TSharedPtr< FUICommandInfo > AssetExportTool;

	// Mirror Commands
	TSharedPtr< FUICommandInfo > MirrorTool;
	TSharedPtr< FUICommandInfo > MirrorBakeTool;
	TSharedPtr< FUICommandInfo > MirrorSmartMirrorBoneTreeTool;

	// Retarget Commands
	TSharedPtr< FUICommandInfo > RetargetTool;
	TSharedPtr< FUICommandInfo > RetargetPoseTool;
	TSharedPtr< FUICommandInfo > RetargetAnimationTool;
	TSharedPtr< FUICommandInfo > RetargetRebuildTool;

	// Pose Search Commands
	TSharedPtr< FUICommandInfo > PoseSearchDistanceTool;
	TSharedPtr< FUICommandInfo > PoseSearchTimeSyncTool;
	TSharedPtr< FUICommandInfo > PoseSearchAnimationTool;

	// Game Commands
	TSharedPtr< FUICommandInfo > GameAnimationTool;
	TSharedPtr< FUICommandInfo > GameFootIKTool;
	TSharedPtr< FUICommandInfo > GameFootLockTool;
	TSharedPtr< FUICommandInfo > GameFootOffsetTool;
	TSharedPtr< FUICommandInfo > GameFootWeightTool;
	TSharedPtr< FUICommandInfo > GameFootPositionTool;
	TSharedPtr< FUICommandInfo > GamePosesCurveTool;
};

