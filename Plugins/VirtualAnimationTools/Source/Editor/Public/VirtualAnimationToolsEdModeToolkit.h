// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "Toolkits/BaseToolkit.h"
#include "VAT_DetailsViewData.h"
#include "IDetailCustomization.h"

class UVAT_DetailsViewData;
class UVAT_EditorModalTool;
class UVAT_EditorActionTool;

class FVirtualAnimationToolsEdModeToolkit : public FModeToolkit, public FGCObject
{	
private:
	TSharedPtr<SWidget> ToolkitWidget;
	TSharedPtr<SExpandableArea> ReferenceExpander;

public:
	FVirtualAnimationToolsEdModeToolkit();

	/** FModeToolkit interface */
#if 0 && ENGINE_MAJOR_VERSION > 4
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
#else
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;
#endif

public:

	/** Response shutdown in reset toolkit */
	void Shutdown();

	/** Response tick in editor */
	void OnTick(float DeltaTime);

	/** Response selected asset changed */
	void OnSelectionChanged();

public:
	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override { return ToolkitWidget; }

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FVirtualAnimationToolsEdModeToolkit");
	}

#pragma region Action Tools

private:

	/** Cached current active tool reference */
	UPROPERTY(Transient)
	UVAT_EditorActionTool* ActiveTool;

	/** Cached current active details view data reference */
	UPROPERTY(Transient)
	TWeakObjectPtr<UVAT_DetailsViewData> DetailsViewDataObject;

	/** Cached need active details view data class */
	TSubclassOf<UVAT_DetailsViewData> DetailsViewDataClass;

public:

	/** Response bind commands */
	void BindCommands();

	/** Execute action tool */
	void ExecuteAction(UVAT_EditorActionTool* InActionTool);

	/** Return the execute action condition */
	bool CanExecuteAction(UVAT_EditorActionTool* InActionTool) const;

	/** Modes Panel Header Information **/
	virtual FText GetActiveToolDisplayName() const;
	virtual FText GetActiveToolMessage() const;

	/** Returns the number of Mode specific tabs in the mode toolbar **/
	const static TArray<FName> PaletteNames;
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const { InPaletteName = PaletteNames; }
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const;

	/* Exclusive Tool Palettes only allow users to use tools from one palette at a time */
	virtual bool HasExclusiveToolPalettes() const { return false; }

	/* Integrated Tool Palettes show up in the same panel as their details */
	virtual bool HasIntegratedToolPalettes() const { return false; }

	/**
	 * @param PaletteIndex      The index of the ToolPalette to build
	 * @param ToolbarBuilder    The builder to use for given PaletteIndex
	**/
	virtual void BuildToolPalette(FName Palette, class FToolBarBuilder& ToolbarBuilder) override;

	/** Response tool palette changed */
	virtual void OnToolPaletteChanged(FName PaletteName) override;

	/** Process command bindings */
	virtual bool ProcessCommandBindings(const FKeyEvent& InKeyEvent) const override;

public:

	/** Return the toolkit has valid data */
	FORCEINLINE bool HasValidData() { return DetailsViewDataObject != nullptr && DetailsViewDataObject.IsValid(); }

	/** Return the details view data object reference */
	UVAT_DetailsViewData* GetDetailsViewDataObject() { return DetailsViewDataObject.Get(); }

#pragma endregion

#pragma region Animation Tools Details View

	TSharedPtr<class IDetailsView> DetailsView_AT;

	/** Showing properties in this details panel works by whitelisting categories and properties. This flag enables you to show all properties without needing to specify. */
	UPROPERTY()
	bool bShowOnlyWhitelisted_AT;

	/** Which categories to show in the details panel. If both this and the Properties To Show whitelist are empty, all properties will show. */
	UPROPERTY(EditAnywhere, Category = "View")
	TArray<FName> CategoriesToShow_AT;

	/** Which properties to show in the details panel. If both this and the Categories To Show whitelist are empty, all properties will show. */
	UPROPERTY(EditAnywhere, Category = "View")
	TArray<FName> PropertiesToShow_AT;

	void InitializeDetailsView_AT();
	void ToggleWhitelistedProperties_AT();
	bool IsRowVisibilityFiltered_AT() const;
	bool GetIsPropertyVisible_AT(const FPropertyAndParent& PropertyAndParent) const;
	bool GetIsRowVisible_AT(FName InRowName, FName InParentName) const;
	void OnFinishedChangingProperties_AT(const FPropertyChangedEvent& PropertyChangedEvent);

#pragma endregion


#pragma region Reference Tools Details View
	TSharedPtr<class IDetailsView> DetailsView_RT;

	/** Showing properties in this details panel works by whitelisting categories and properties. This flag enables you to show all properties without needing to specify. */
	UPROPERTY()
	bool bShowOnlyWhitelisted_RT;

	/** Which categories to show in the details panel. If both this and the Properties To Show whitelist are empty, all properties will show. */
	UPROPERTY(EditAnywhere, Category = "View")
	TArray<FName> CategoriesToShow_RT;

	/** Which properties to show in the details panel. If both this and the Categories To Show whitelist are empty, all properties will show. */
	UPROPERTY(EditAnywhere, Category = "View")
	TArray<FName> PropertiesToShow_RT;

	void InitializeDetailsView_RT();
	void ToggleWhitelistedProperties_RT();
	bool IsRowVisibilityFiltered_RT() const;
	bool GetIsPropertyVisible_RT(const FPropertyAndParent& PropertyAndParent) const;
	bool GetIsRowVisible_RT(FName InRowName, FName InParentName) const;
	void OnFinishedChangingProperties_RT(const FPropertyChangedEvent& PropertyChangedEvent);

#pragma endregion

public:

#pragma region Delegate

	/** Flag delay update data class state */
	UPROPERTY(Transient)
	uint8 bDelayUpdateDataClass : 1;

	/** Flag delay update virtual tools types state */
	UPROPERTY(Transient)
	uint8 bDelayUpdateVirtualToolsType : 1;

	/** Initialize details view data delegate */
	void InitializeDetailsViewDataDelegate();

	/** Response data class property changed */
	void OnDataClassEditProperties(const TSubclassOf<UVAT_DetailsViewData>& InDataClass);

	/** Response virtual tools type property changed */
	void OnVirtualToolsEditProperties(const FPropertyChangedEvent* InPropertyChangedEvent);

#pragma endregion


#pragma region Virtual Tools

	/** Cached current virtual tools type */
	UPROPERTY(Transient)
	EVirtualToolsType VirtualToolsType;

	/** Set the virtual tools types */
	void SetVirtualToolType(EVirtualToolsType InToolsType);

	/** Return the apply button visibility */
	EVisibility GetApplyButtonVisibility() const;

	/** Response apply clicked event */
	FReply OnApplyButtonClickedEvent();

	/** Response animation tools clicked event */
	FReply OnAnimToolsButtonClickedEvent(EVirtualToolsType InToolsType);

#pragma endregion


#pragma region Bone

	/** Cached current bone tools type */
	UPROPERTY(Transient)
	EVirtualBoneToolsType BoneToolsType;

	/** Response bone tools changed */
	void OnBoneToolsChanged();

	/** Set the bone tools types */
	void SetVirtualBoneToolType(EVirtualBoneToolsType InToolsType);

	/** Return the bone tools visibility */
	EVisibility GetBoneToolsVisibility() const;

	/** Response sample bones track clicked event */
	FReply OnSampleBonesTrackButtonClickedEvent();

#pragma endregion


#pragma region Curve

	/** Cached current curve tools type */
	UPROPERTY(Transient)
	EVirtualCurveToolsType CurveToolsType;

	/** Response curve tools changed */
	void OnCurveToolsChanged();

	/** Set the curve tools types */
	void SetVirtualCurveToolType(EVirtualCurveToolsType InToolsType);

	/** Return the curve tools visibility */
	EVisibility GetCurveToolsVisibility() const;

	/** Response sample curves clicked event */
	FReply OnSampleCurvesButtonClickedEvent();

	/** Response sample accumulate curves clicked event */
	FReply OnSampleAccuCurvesButtonClickedEvent();

#pragma endregion


#pragma region Notify

	/** Cached current notify tools type */
	UPROPERTY(Transient)
	EVirtualNotifyToolsType NotifyToolsType;

	/** Response notify tools changed */
	void OnNotifyToolsChanged();

	/** Set the notify tools types */
	void SetVirtualNotifyToolType(EVirtualNotifyToolsType InToolsType);

	/** Return the notify tools visibility */
	EVisibility GetNotifyToolsVisibility() const;

	/** Response sample notifies track clicked event */
	FReply OnSampleNotifiesButtonClickedEvent();
#pragma endregion


#pragma region Montage

	/** Cached current montage tools type */
	UPROPERTY(Transient)
	EVirtualMontageToolsType MontageToolsType;

	/** Response montage tools changed */
	void OnMontageToolsChanged();

	/** Set the montage tools types */
	void SetVirtualMontageToolType(EVirtualMontageToolsType InToolsType);

#pragma endregion


#pragma region Asset

	/** Cached current asset tools type */
	UPROPERTY(Transient)
	EVirtualAssetToolsType AssetToolsType;

	/** Cached current asset root motion process type */
	UPROPERTY(Transient)
	EVirtualRootMotionProcessType AssetRootMotionProcessType;

	/** Response asset tools changed */
	void OnAssetToolsChanged();

	/** Set the asset tools types */
	void SetVirtualAssetToolType(EVirtualAssetToolsType InToolsType);

protected:

	/** Response alignment root motion to motion capture clicked event */
	FReply OnAlignmentMotionCurvesButtonClickedEvent();

	/** Response sample motion curves clicked event */
	FReply OnSampleMotionCurvesButtonClickedEvent();

	/** Response convert motion capture and root motion clicked event */
	FReply OnConvertRootMotionButtonClickedEvent();

	/** Response apply convert root motion to motion capture clicked event */
	FReply OnApplyConvertToMotionCaptureButtonClickedEvent();

	/** Response apply convert motion capture to root motion clicked event */
	FReply OnApplyConvertToRootMotionButtonClickedEvent();

	/** Return the resize root motion tools visibility */
	EVisibility GetResizeRootMotionVisibility() const;

	/** Return the sample motion curve tools visibility */
	EVisibility GetSampleMotionCurvesVisibility() const;

	/** Return the sample motion curve tools text */
	FText GetSampleMotionCurvesButtonText() const;

	/** Return the sample motion curve tools tooltip */
	FText GetSampleMotionCurvesButtonTooltip() const;

#pragma endregion


#pragma region Mirror

public:

	/** Cached current mirror tools type */
	UPROPERTY(Transient)
	EVirtualMirrorToolsType MirrorToolsType;

	/** Response mirror tools changed */
	void OnMirrorToolsChanged();

	/** Set the mirror tools types */
	void SetMirrorToolsType(EVirtualMirrorToolsType InToolsType);

#pragma endregion


#pragma region Retarget

	/** Cached current retarget tools type */
	UPROPERTY(Transient)
	EVirtualRetargetToolsType RetargetToolsType;

	/** Response pose retarget tools changed */
	void OnRetargetToolsChanged();

	/** Set the retarget tools types */
	void SetRetargetToolsType(EVirtualRetargetToolsType InToolsType);

#pragma endregion


#pragma region PoseSearch

	/** Cached current pose search tools type */
	UPROPERTY(Transient)
	EVirtualPoseSearchToolsType PoseSearchToolsType;

	/** Response pose search tools changed */
	void OnPoseSearchToolsChanged();

	/** Set the pose search tools type */
	void SetPoseSearchToolsType(EVirtualPoseSearchToolsType InToolsType);

#pragma endregion


#pragma region GameFramework

	/** Cached current pose game framework tools type */
	UPROPERTY(Transient)
	EVirtualGameFrameworkToolsType GameFrameworkToolsType;

	/** Response game framework tools changed */
	void OnGameFrameworkToolsChanged();

	/** Set the game framework tools type */
	void SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType InToolsType);

#pragma endregion
};
