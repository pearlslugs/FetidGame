// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VirtualAnimationToolsEdModeToolkit.h"
#include "VirtualAnimationToolsEdMode.h"
#include "VAT_Settings.h"

#include "EditorStyleSet.h"
#include "EditorModeManager.h"

#include "Misc/Attribute.h"
#include "Engine/Selection.h"
#include "ObjectEditorUtils.h"
#include "UObject/UObjectGlobals.h"

#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "Persona/Public/PersonaDelegates.h"

#include "Style/VAT_MirrorDataDetails.h"
#include "Style/VAT_MontageSlotDetails.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"

#include "Commands/VAT_EditorTool.h"
#include "Commands/VAT_EditorCommands.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FVirtualAnimationToolsEdModeToolkit"

// Define animation tool classes
TArray<UClass*> FindAnimationToolClasses()
{
	TArray<UClass*> Classes;

	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		if (ClassIterator->IsChildOf(UVAT_EditorActionTool::StaticClass()) && !ClassIterator->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			Classes.Add(*ClassIterator);
		}
	}

	return Classes;
}

FVirtualAnimationToolsEdModeToolkit::FVirtualAnimationToolsEdModeToolkit()
{
	DetailsViewDataClass = UVAT_DetailsViewData::StaticClass();

	ActiveTool = nullptr;
	bShowOnlyWhitelisted_AT = true;
	bShowOnlyWhitelisted_RT = true;

	BoneToolsType = EVirtualBoneToolsType::AddBone;
	CurveToolsType = EVirtualCurveToolsType::Motion;
	NotifyToolsType = EVirtualNotifyToolsType::AddNotifies;
	MontageToolsType = EVirtualMontageToolsType::Modifier;
	AssetToolsType = EVirtualAssetToolsType::Crop;
	MirrorToolsType = EVirtualMirrorToolsType::Mirror;
	RetargetToolsType = EVirtualRetargetToolsType::Pose;
	PoseSearchToolsType = EVirtualPoseSearchToolsType::Distance;
	AssetRootMotionProcessType = EVirtualRootMotionProcessType::SamplingCurves;
}

#if ENGINE_MAJOR_VERSION > 4
void FVirtualAnimationToolsEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
#else
void FVirtualAnimationToolsEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
#endif
{
	// Load the system settings
	UVAT_Settings* theMutableDefaultSettings = GetMutableDefault<UVAT_Settings>();
	if (theMutableDefaultSettings != nullptr)
	{
		DetailsViewDataClass = theMutableDefaultSettings->DetailsViewDataClass;
	}
	else
	{
		DetailsViewDataClass = UVAT_DetailsViewData::StaticClass();
	}
	DetailsViewDataObject = NewObject<UVAT_DetailsViewData>(GEditor, DetailsViewDataClass);

	// If the data is valid, initialize details view
	if (HasValidData())
	{
		InitializeDetailsView_AT();
		InitializeDetailsView_RT();
		InitializeDetailsViewDataDelegate();
	}

	float Padding = 4.0f;
	FMargin MorePadding = FMargin(10.0f, 2.0f);

	TSharedRef<SExpandableArea> ReferenceExpanderRef = SNew(SExpandableArea)
		.AreaTitle(FText(LOCTEXT("Reference", "Reference")))
		.HeaderPadding(FMargin(2.0, 2.0))
		.Padding(MorePadding)
		.BorderBackgroundColor(FLinearColor(.6, .6, .6, 1.0f))
		.BodyBorderBackgroundColor(FLinearColor(1.0, 0.0, 0.0))
		.AreaTitleFont(FEditorStyle::Get().GetFontStyle("DetailsView.CategoryFontStyle"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsView_RT.ToSharedRef()
		]
		];
	ReferenceExpander = ReferenceExpanderRef;


	SAssignNew(ToolkitWidget, SBox)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		[

			SNew(SSplitter)
			.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.SizeRule(TAttribute<SSplitter::ESizeRule>::Create([this]() {
		return true ? SSplitter::ESizeRule::FractionOfParent : SSplitter::ESizeRule::SizeToContent;
	}))
		.Value(1.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.FillHeight(1.0)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
		[
			DetailsView_AT.ToSharedRef()
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(SSpacer)
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_SampleBonesTrack_Button", "SampleBonesTrack"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetBoneToolsVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnSampleBonesTrackButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(LOCTEXT("VAT_SampleBonesTrack_ButtonTipText", "Sample bones track data."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_Alignment_Button", "Alignment"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetResizeRootMotionVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnAlignmentMotionCurvesButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(LOCTEXT("VAT_Alignment_ButtonTipText", "Alignment Root Motion to Motion Capture."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(this, &FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesButtonTooltip)
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnSampleMotionCurvesButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(this, &FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesButtonTooltip)
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_SampleCurves_Button", "SampleCurves"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetCurveToolsVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnSampleCurvesButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(LOCTEXT("VAT_SampleCurves_ButtonTipText", "Sample desired curves data."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_SampleAccumulateCurves_Button", "SampleAccumulateCurves"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetCurveToolsVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnSampleAccuCurvesButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(LOCTEXT("VAT_SampleAccumulateCurves_ButtonTipText", "Sample accumulate desired curves data."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_SampleNotifiesTrack_Button", "SampleNotifiesTrack"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetNotifyToolsVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnSampleNotifiesButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Green)
		.ToolTipText(LOCTEXT("VAT_SampleNotifiesTrack_ButtonTipText", "Sample notifies data."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_ConvertToMoCap_Button", "ConvertToMoCap"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetResizeRootMotionVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnApplyConvertToMotionCaptureButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Red)
		.ToolTipText(LOCTEXT("VAT_ConvertToMoCap_ButtonTipText", "Convert Root Motion to Motion Capture."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VAT_ConvertToRootMotion_Button", "ConvertToRootMotion"))
		.HAlign(HAlign_Left)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetResizeRootMotionVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnApplyConvertToRootMotionButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Red)
		.ToolTipText(LOCTEXT("VAT_ConvertToRootMotion_ButtonTipText", "Convert Motion Capture to Root Motion."))
		]

	+ SHorizontalBox::Slot()
		.Padding(4.0)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("VATApplyButton", "APPLY"))
		.HAlign(HAlign_Center)
		.Visibility(this, &FVirtualAnimationToolsEdModeToolkit::GetApplyButtonVisibility)
		.ContentPadding(FMargin(10.f, Padding))
		.OnClicked(this, &FVirtualAnimationToolsEdModeToolkit::OnApplyButtonClickedEvent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
		.ButtonColorAndOpacity(FLinearColor::Red)
		]
		]
		]

	+ SSplitter::Slot()
		.SizeRule(TAttribute<SSplitter::ESizeRule>::Create([this, ReferenceExpanderRef]() {
		return ReferenceExpander->IsExpanded() ? SSplitter::ESizeRule::FractionOfParent : SSplitter::ESizeRule::SizeToContent;
	}))
		.Value(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
		[
			ReferenceExpanderRef
		]
		]

		]
		];

	// Bind commands
	BindCommands();

	// Initialize tool type
	SetVirtualToolType(EVirtualToolsType::Bone);

	// Init the tool kit
#if ENGINE_MAJOR_VERSION > 4
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
#else
	FModeToolkit::Init(InitToolkitHost);
#endif
}

void FVirtualAnimationToolsEdModeToolkit::Shutdown()
{
	ActiveTool = nullptr;
	DetailsViewDataObject = nullptr;
}

FName FVirtualAnimationToolsEdModeToolkit::GetToolkitFName() const
{
	return FName("VirtualAnimationToolsEdMode");
}

FText FVirtualAnimationToolsEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("VirtualAnimationToolsEdModeToolkit", "DisplayName", "VirtualAnimationToolsEdMode Tool");
}

class FEdMode* FVirtualAnimationToolsEdModeToolkit::GetEditorMode() const
{
#if ENGINE_MAJOR_VERSION > 4
	return GLevelEditorModeTools().GetActiveMode(UVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId);
#else
	return GLevelEditorModeTools().GetActiveMode(FVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId);
#endif
}

void FVirtualAnimationToolsEdModeToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ActiveTool);
}

void FVirtualAnimationToolsEdModeToolkit::ExecuteAction(UVAT_EditorActionTool* InActionTool)
{
	if (InActionTool)
	{
		InActionTool->Execute(StaticCastSharedRef<FVirtualAnimationToolsEdModeToolkit>(AsShared()));
		ActiveTool = InActionTool;
	}
}

bool FVirtualAnimationToolsEdModeToolkit::CanExecuteAction(UVAT_EditorActionTool* InActionTool) const
{
	if (InActionTool)
	{
		return InActionTool->CanExecute();
	}
	else
	{
		return false;
	}
}

FText FVirtualAnimationToolsEdModeToolkit::GetActiveToolDisplayName() const
{
	if (ActiveTool != nullptr)
	{
		return ActiveTool->GetDisplayText();
	}
	return LOCTEXT("VirtualAnimationTools", "Virtual Animation Tool");
}

FText FVirtualAnimationToolsEdModeToolkit::GetActiveToolMessage() const
{
	if (ActiveTool != nullptr)
	{
		return ActiveTool->GetTooltipText();
	}
	return LOCTEXT("VAT_EditorNoToolMessage", "Virtual animation tools v2.By ZhangJiaBin.");
}

const TArray<FName> FVirtualAnimationToolsEdModeToolkit::PaletteNames =
{
FName(TEXT("Bone")),
FName(TEXT("Curve")),
FName(TEXT("Notify")),
FName(TEXT("Montage")),
FName(TEXT("Asset")),
FName(TEXT("Mirror")),
FName(TEXT("Retarget")),
FName(TEXT("PoseSearch")),
FName(TEXT("GameFramework"))
};

FText FVirtualAnimationToolsEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	return FText::FromName(PaletteName);
}

void FVirtualAnimationToolsEdModeToolkit::BuildToolPalette(FName Palette, class FToolBarBuilder& ToolbarBuilder)
{
	const FVAT_EditorCommands& Commands = FVAT_EditorCommands::Get();

	const ISlateStyle* ToolbarBuilderSlateStyle = ToolbarBuilder.GetStyleSet();

	if (Palette == TEXT("Bone"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.BoneAddTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneRemoveTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneAddTrackTool);
		//ToolbarBuilder.AddToolBarButton(Commands.BoneBlendTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneModifyTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneRemoveTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneBakeTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneFilterTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneLayerPoseTool);
		ToolbarBuilder.AddToolBarButton(Commands.BoneConstraintTool);
	}
	else if (Palette == TEXT("Curve"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.CurveMotionTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveBoneTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveCopyTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveTransferTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveSortTool);
		//ToolbarBuilder.AddToolBarButton(Commands.CurveScaleTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveRemoveTool);
		ToolbarBuilder.AddToolBarButton(Commands.CurveOutputTool);
	}
	else if (Palette == TEXT("Notify"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.NotifyAddTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyModifyTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyRemoveTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyAddTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyModifyTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyRemoveTrackTool);
		ToolbarBuilder.AddToolBarButton(Commands.NotifyFootStepTool);
	}
	else if (Palette == TEXT("Montage"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.MontageModifierTool);
		ToolbarBuilder.AddToolBarButton(Commands.MontageLoopTool);
		ToolbarBuilder.AddToolBarButton(Commands.MontageTransferTool);
		//ToolbarBuilder.AddToolBarButton(Commands.MontageSortTool);
	}
	else if (Palette == TEXT("Asset"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.AssetCropTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetInsertTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetResizeTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetReplaceTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetCompositeTool);

		ToolbarBuilder.AddToolBarButton(Commands.AssetResizeRootMotionTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetConvertRootMotionTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetAlignmentRootMotionTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetMotionCaptureToRefPoseTool);

		ToolbarBuilder.AddToolBarButton(Commands.AssetGenerateLODTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetRemoveLODTool);
		ToolbarBuilder.AddToolBarButton(Commands.AssetExportTool);
	}
	else if (Palette == TEXT("Mirror"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.MirrorBakeTool);
		ToolbarBuilder.AddToolBarButton(Commands.MirrorSmartMirrorBoneTreeTool);
	}
	else if (Palette == TEXT("Retarget"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.RetargetPoseTool);
		ToolbarBuilder.AddToolBarButton(Commands.RetargetAnimationTool);
		ToolbarBuilder.AddToolBarButton(Commands.RetargetRebuildTool);
	}
	else if (Palette == TEXT("PoseSearch"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.PoseSearchDistanceTool);
		ToolbarBuilder.AddToolBarButton(Commands.PoseSearchTimeSyncTool);
		ToolbarBuilder.AddToolBarButton(Commands.PoseSearchAnimationTool);
	}
	else if (Palette == TEXT("GameFramework"))
	{
		ToolbarBuilder.AddToolBarButton(Commands.GameFootIKTool);
		ToolbarBuilder.AddToolBarButton(Commands.GameFootLockTool);
		ToolbarBuilder.AddToolBarButton(Commands.GameFootOffsetTool);
		ToolbarBuilder.AddToolBarButton(Commands.GameFootWeightTool);
		ToolbarBuilder.AddToolBarButton(Commands.GameFootPositionTool);
		ToolbarBuilder.AddToolBarButton(Commands.GamePosesCurveTool);
	}
}

void FVirtualAnimationToolsEdModeToolkit::OnToolPaletteChanged(FName PaletteName)
{
	if (PaletteName == TEXT("Bone"))
	{
		SetVirtualToolType(EVirtualToolsType::Bone);
	}
	else if (PaletteName == TEXT("Curve"))
	{
		SetVirtualToolType(EVirtualToolsType::Curve);
	}
	else if (PaletteName == TEXT("Notify"))
	{
		SetVirtualToolType(EVirtualToolsType::Notify);
	}
	else if (PaletteName == TEXT("Montage"))
	{
		SetVirtualToolType(EVirtualToolsType::Montage);
	}
	else if (PaletteName == TEXT("Asset"))
	{
		SetVirtualToolType(EVirtualToolsType::Asset);
	}
	else if (PaletteName == TEXT("Mirror"))
	{
		SetVirtualToolType(EVirtualToolsType::Mirror);
	}
	else if (PaletteName == TEXT("Retarget"))
	{
		SetVirtualToolType(EVirtualToolsType::Retarget);
	}
	else if (PaletteName == TEXT("PoseSearch"))
	{
		SetVirtualToolType(EVirtualToolsType::PoseSearch);
	}
	else if (PaletteName == TEXT("GameFramework"))
	{
		SetVirtualToolType(EVirtualToolsType::GameFramework);
	}
}

bool FVirtualAnimationToolsEdModeToolkit::ProcessCommandBindings(const FKeyEvent& InKeyEvent) const
{
	if (ReferenceExpander && InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		ReferenceExpander->SetExpanded(!ReferenceExpander->IsExpanded());
	}

	if (ToolkitCommands->ProcessCommandBindings(InKeyEvent))
	{
		return true;
	}

	return false;
}

#pragma region Details View
void FVirtualAnimationToolsEdModeToolkit::BindCommands()
{
	// Map actions of all the Fracture Tools
	TArray<UClass*> SourceClasses = FindAnimationToolClasses();
	for (UClass* Class : SourceClasses)
	{
		TSubclassOf<UVAT_EditorActionTool> SubclassOf = Class;
		UVAT_EditorActionTool* VAT_EditorTool = SubclassOf->GetDefaultObject<UVAT_EditorActionTool>();

		// Only Bind Commands With Legitimately Set Commands
		if (VAT_EditorTool->GetUICommandInfo())
		{
			ToolkitCommands->MapAction(
				VAT_EditorTool->GetUICommandInfo(),
				FExecuteAction::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::ExecuteAction, VAT_EditorTool),
				FCanExecuteAction::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::CanExecuteAction, VAT_EditorTool)
			);
		}
	}
}
#pragma endregion


#pragma region Anim Tools Details View
void FVirtualAnimationToolsEdModeToolkit::InitializeDetailsView_AT()
{
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsViewArgs.bShowKeyablePropertiesOption = false;
	DetailsViewArgs.bShowModifiedPropertiesOption = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowAnimatedPropertiesOption = false;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	//PropertyEditorModule.RegisterCustomClassLayout("FractureSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimToolViewSettingsCustomization::MakeInstance, this));

	DetailsView_AT = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView_AT->SetObject(GetDetailsViewDataObject());

	DetailsView_AT->SetCustomFilterLabel(LOCTEXT("ShowAllParameters", "Show All Parameters"));
	DetailsView_AT->SetCustomFilterDelegate(FSimpleDelegate::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::ToggleWhitelistedProperties_AT));
	DetailsView_AT->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::GetIsPropertyVisible_AT));
#if ENGINE_MAJOR_VERSION < 5
	DetailsView_AT->SetIsCustomRowVisibilityFilteredDelegate(FIsCustomRowVisibilityFiltered::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::IsRowVisibilityFiltered_AT));
#endif
	DetailsView_AT->SetIsCustomRowVisibleDelegate(FIsCustomRowVisible::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::GetIsRowVisible_AT));
	DetailsView_AT->OnFinishedChangingProperties().AddSP(this, &FVirtualAnimationToolsEdModeToolkit::OnFinishedChangingProperties_AT);
}

void FVirtualAnimationToolsEdModeToolkit::ToggleWhitelistedProperties_AT()
{
	bShowOnlyWhitelisted_AT = !bShowOnlyWhitelisted_AT;
	if (DetailsView_AT.IsValid())
	{
		DetailsView_AT->ForceRefresh();
	}
}

bool FVirtualAnimationToolsEdModeToolkit::IsRowVisibilityFiltered_AT() const
{
	return bShowOnlyWhitelisted_AT && (PropertiesToShow_AT.Num() > 0 || CategoriesToShow_AT.Num() > 0);
}

bool FVirtualAnimationToolsEdModeToolkit::GetIsPropertyVisible_AT(const FPropertyAndParent& PropertyAndParent) const
{
	if (!IsRowVisibilityFiltered_AT())
	{
		return true;
	}

	const FName& PropertyName = PropertyAndParent.Property.GetFName();
	if (PropertiesToShow_AT.Contains(PropertyName))
	{
		return true;
	}

	const FName& CategoryName = FObjectEditorUtils::GetCategoryFName(&PropertyAndParent.Property);
	if (CategoriesToShow_AT.Contains(CategoryName))
	{
		return true;
	}

	return false;
}

bool FVirtualAnimationToolsEdModeToolkit::GetIsRowVisible_AT(FName InRowName, FName InParentName) const
{
	if (!IsRowVisibilityFiltered_AT())
	{
		return true;
	}

	if (PropertiesToShow_AT.Contains(InRowName))
	{
		return true;
	}

	if (CategoriesToShow_AT.Contains(InParentName))
	{
		return true;
	}

	return false;
}

void FVirtualAnimationToolsEdModeToolkit::OnFinishedChangingProperties_AT(const FPropertyChangedEvent& PropertyChangedEvent)
{
}
#pragma endregion


#pragma region Reference Tools Details View
void FVirtualAnimationToolsEdModeToolkit::InitializeDetailsView_RT()
{
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;

	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowModifiedPropertiesOption = false;
	DetailsViewArgs.bShowKeyablePropertiesOption = true;
	DetailsViewArgs.bShowAnimatedPropertiesOption = true;
	DetailsViewArgs.bShowScrollBar = true;
	DetailsViewArgs.bForceHiddenPropertyVisibility = false;
	DetailsViewArgs.bShowCustomFilterOption = true/*PropertiesToShow.Num() != 0 || CategoriesToShow.Num() != 0*/;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	DetailsView_RT = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView_RT->SetObject(GetDetailsViewDataObject());
	
	DetailsView_RT->SetCustomFilterLabel(LOCTEXT("ShowAllParameters", "Show All Parameters"));
	DetailsView_RT->SetCustomFilterDelegate(FSimpleDelegate::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::ToggleWhitelistedProperties_RT));
	DetailsView_RT->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::GetIsPropertyVisible_RT));
#if ENGINE_MAJOR_VERSION < 5
	DetailsView_RT->SetIsCustomRowVisibilityFilteredDelegate(FIsCustomRowVisibilityFiltered::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::IsRowVisibilityFiltered_RT));
#endif
	DetailsView_RT->SetIsCustomRowVisibleDelegate(FIsCustomRowVisible::CreateSP(this, &FVirtualAnimationToolsEdModeToolkit::GetIsRowVisible_RT));
	DetailsView_RT->OnFinishedChangingProperties().AddSP(this, &FVirtualAnimationToolsEdModeToolkit::OnFinishedChangingProperties_RT);
}

void FVirtualAnimationToolsEdModeToolkit::ToggleWhitelistedProperties_RT()
{
	bShowOnlyWhitelisted_RT = !bShowOnlyWhitelisted_RT;
	if (DetailsView_RT.IsValid())
	{
		DetailsView_RT->ForceRefresh();
	}
}

bool FVirtualAnimationToolsEdModeToolkit::IsRowVisibilityFiltered_RT() const
{
	return bShowOnlyWhitelisted_RT && (PropertiesToShow_RT.Num() > 0 || CategoriesToShow_RT.Num() > 0);
}

bool FVirtualAnimationToolsEdModeToolkit::GetIsPropertyVisible_RT(const FPropertyAndParent& PropertyAndParent) const
{
	if (!IsRowVisibilityFiltered_RT())
	{
		return true;
	}

	if (PropertiesToShow_RT.Contains(PropertyAndParent.Property.GetFName()))
	{
		return true;
	}

	if (CategoriesToShow_RT.Contains(FObjectEditorUtils::GetCategoryFName(&PropertyAndParent.Property)))
	{
		return true;
	}

	return false;
}

bool FVirtualAnimationToolsEdModeToolkit::GetIsRowVisible_RT(FName InRowName, FName InParentName) const
{
	if (!IsRowVisibilityFiltered_RT())
	{
		return true;
	}

	if (PropertiesToShow_RT.Contains(InRowName))
	{
		return true;
	}

	if (CategoriesToShow_RT.Contains(InParentName))
	{
		return true;
	}

	return false;
}

void FVirtualAnimationToolsEdModeToolkit::OnFinishedChangingProperties_RT(const FPropertyChangedEvent& PropertyChangedEvent)
{

}
#pragma endregion


#pragma region Tools
void FVirtualAnimationToolsEdModeToolkit::OnTick(float DeltaTime)
{
	// Ignore game running tick event
	if (GEditor && GEditor->IsPlaySessionInProgress())
	{
		return;
	}

	// Check the object is valid
// 	if (!DetailsViewDataObject.IsValid() || DetailsViewDataObject->HasAllFlags(RF_Standalone)/* || DetailsViewDataObject->GetClass() == NULL*/)
// 	{
// 		bDelayUpdateDataClass = false;
// 		bDelayUpdateVirtualToolsType = false;
// 		DetailsViewDataObject = nullptr;
// 	}

	if (bDelayUpdateDataClass)
	{
		bDelayUpdateDataClass = false;
		if (DetailsView_AT && DetailsView_RT && GetDetailsViewDataObject() && DetailsViewDataObject->GetClass() != DetailsViewDataClass && DetailsViewDataClass != NULL)
		{
			DetailsViewDataObject = NewObject<UVAT_DetailsViewData>(GEditor, DetailsViewDataClass);
			if (GetDetailsViewDataObject())
			{
				DetailsViewDataObject->SetDataClass(DetailsViewDataClass);
				DetailsView_AT->SetObject(GetDetailsViewDataObject());
				DetailsView_RT->SetObject(GetDetailsViewDataObject());
				SetVirtualToolType(VirtualToolsType);
				InitializeDetailsViewDataDelegate();
			}
		}
	}
	else if (bDelayUpdateVirtualToolsType)
	{
		bDelayUpdateVirtualToolsType = false;
		SetVirtualToolType(VirtualToolsType);
	}
	else if (!DetailsViewDataObject.IsValid())
	{
		if (DetailsView_AT && DetailsView_RT && DetailsViewDataClass != NULL)
		{
			DetailsViewDataObject = NewObject<UVAT_DetailsViewData>(GEditor, DetailsViewDataClass);
			if (GetDetailsViewDataObject())
			{
				DetailsViewDataObject->SetDataClass(DetailsViewDataClass);
				DetailsView_AT->SetObject(GetDetailsViewDataObject());
				DetailsView_RT->SetObject(GetDetailsViewDataObject());
				SetVirtualToolType(VirtualToolsType);
				InitializeDetailsViewDataDelegate();
			}
		}
	}

	// Update skeleton reference
	if (HasValidData())
	{
		DetailsViewDataObject->OnSelectionChanged();
	}
}

void FVirtualAnimationToolsEdModeToolkit::OnSelectionChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Response data class clicked
	DetailsViewDataObject->OnSelectionChanged();
}

#pragma endregion


#pragma region Delegate
void FVirtualAnimationToolsEdModeToolkit::InitializeDetailsViewDataDelegate()
{
	if (GetDetailsViewDataObject())
	{
		DetailsViewDataObject->OnDataClassChangingPropertiesDelegate.Clear();
		DetailsViewDataObject->OnVirtualToolsChangingPropertiesDelegate.Clear();
		DetailsViewDataObject->OnDataClassChangingPropertiesDelegate.AddSP(this, &FVirtualAnimationToolsEdModeToolkit::OnDataClassEditProperties);
		DetailsViewDataObject->OnVirtualToolsChangingPropertiesDelegate.AddSP(this, &FVirtualAnimationToolsEdModeToolkit::OnVirtualToolsEditProperties);
	}
}

void FVirtualAnimationToolsEdModeToolkit::OnDataClassEditProperties(const TSubclassOf<UVAT_DetailsViewData>& InDataClass)
{
	bDelayUpdateDataClass = true;
	DetailsViewDataClass = InDataClass;
}

void FVirtualAnimationToolsEdModeToolkit::OnVirtualToolsEditProperties(const FPropertyChangedEvent* InPropertyChangedEvent)
{
	bDelayUpdateVirtualToolsType = true;
}
#pragma endregion


#pragma region Virtual Tools
void FVirtualAnimationToolsEdModeToolkit::SetVirtualToolType(EVirtualToolsType InToolsType)
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	DetailsViewDataObject->SetVirtualToolsType(InToolsType);

	if (DetailsView_AT && DetailsView_RT)
	{
		VirtualToolsType = InToolsType;

		PropertiesToShow_AT.Reset();
		CategoriesToShow_AT.Reset();
		CategoriesToShow_AT.AddUnique(FName(TEXT("Animation")));

		CategoriesToShow_RT.Reset();
		PropertiesToShow_RT.Reset();

		ReferenceExpander->SetExpanded(false);

		FPropertyEditorModule& thePropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		thePropertyModule.UnregisterCustomClassLayout(UVAT_DetailsViewData::StaticClass()->GetFName());

		switch (InToolsType)
		{
		case EVirtualToolsType::Bone:
			OnBoneToolsChanged();
			break;

		case EVirtualToolsType::Curve:
			OnCurveToolsChanged();
			break;

		case EVirtualToolsType::Notify:
			OnNotifyToolsChanged();
			break;

		case EVirtualToolsType::Montage:
			OnMontageToolsChanged();
			break;

		case EVirtualToolsType::Asset:
			OnAssetToolsChanged();
			break;

		case EVirtualToolsType::Mirror:
			OnMirrorToolsChanged();
			break;

		case EVirtualToolsType::Retarget:
			OnRetargetToolsChanged();
			break;

		case EVirtualToolsType::PoseSearch:
			OnPoseSearchToolsChanged();
			break;

		case EVirtualToolsType::GameFramework:
			OnGameFrameworkToolsChanged();
			break;
		}

		DetailsView_AT->ForceRefresh();
		if (InToolsType == EVirtualToolsType::Mirror)
		{
			DetailsView_RT->SetObject(nullptr, true);
		}
		else if(GetDetailsViewDataObject())
		{
			DetailsView_RT->SetObject(GetDetailsViewDataObject(), true);
		}
	}
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetApplyButtonVisibility() const
{
	if (GetCurveToolsVisibility() == EVisibility::Visible)
	{
		return EVisibility::Collapsed;
	}
	else if (GetResizeRootMotionVisibility() == EVisibility::Visible)
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

FReply FVirtualAnimationToolsEdModeToolkit::OnApplyButtonClickedEvent()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return FReply::Handled();
	}

	// Handle the tools type
	switch (VirtualToolsType)
	{
	case EVirtualToolsType::Bone:
		DetailsViewDataObject->OnResponseBoneTools(BoneToolsType);
		break;

	case EVirtualToolsType::Curve:
		DetailsViewDataObject->OnResponseCurveTools(CurveToolsType);
		break;

	case EVirtualToolsType::Notify:
		DetailsViewDataObject->OnResponseNotifyTools(NotifyToolsType);
		break;

	case EVirtualToolsType::Montage:
		DetailsViewDataObject->OnResponseMontageTools(MontageToolsType);
		break;

	case EVirtualToolsType::Asset:
		DetailsViewDataObject->OnResponseAssetTools(AssetToolsType, /*AssetRootMotionProcessType*/EVirtualRootMotionProcessType::ConvertingAsset);
		break;

	case EVirtualToolsType::Mirror:
		DetailsViewDataObject->OnResponseMirrorTools(MirrorToolsType);
		break;

	case EVirtualToolsType::Retarget:
		DetailsViewDataObject->OnResponseRetargetTools(RetargetToolsType);
		break;

	case EVirtualToolsType::PoseSearch:
		DetailsViewDataObject->OnResponsePoseSearchTools(PoseSearchToolsType);
		break;

	case EVirtualToolsType::GameFramework:
		DetailsViewDataObject->OnResponseGamerFrameworkTools(GameFrameworkToolsType);
		break;
	}

	// Success
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnAnimToolsButtonClickedEvent(EVirtualToolsType InToolsType)
{
	SetVirtualToolType(InToolsType);
	return FReply::Handled();
}
#pragma endregion


#pragma region Bone
void FVirtualAnimationToolsEdModeToolkit::OnBoneToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Clear the bone tools cache data
	DetailsViewDataObject->OnClearBoneTools(BoneToolsType);

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Bone Params")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));

	PropertiesToShow_RT.AddUnique(FName(TEXT("ReferenceBonesData")));
	PropertiesToShow_RT.AddUnique(FName(TEXT("TMap_ReferenceBoneName")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("BonePreset")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("BoneReference")));

	// Handle bone tools type
	switch (BoneToolsType)
	{
	case EVirtualBoneToolsType::AddBone:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AddBonesData")));
		break;

	case EVirtualBoneToolsType::RemoveBone:
		PropertiesToShow_AT.AddUnique(FName(TEXT("RemoveBonesData")));
		break;

	case EVirtualBoneToolsType::AddBoneTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SampleBonsData")));
		break;

	case EVirtualBoneToolsType::BlendBoneTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SampleBonsData")));
		break;

	case EVirtualBoneToolsType::ModifyBoneTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("ModifyBonesData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("BonesConstraintData")));
		break;

	case EVirtualBoneToolsType::RemoveBoneTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SampleBonsData")));
		break;

	case EVirtualBoneToolsType::BakeBoneTransform:
		PropertiesToShow_AT.AddUnique(FName(TEXT("BakeBonesData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("bIncludeChildren")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("BonesConstraintData")));
		break;

	case EVirtualBoneToolsType::FilterBone:
		PropertiesToShow_AT.AddUnique(FName(TEXT("BoneFilterData")));
		break;

	case EVirtualBoneToolsType::LayeredBoneBlend:
		PropertiesToShow_AT.AddUnique(FName(TEXT("BoneLayerData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("BonesConstraintData")));
		break;

	case EVirtualBoneToolsType::ConstraintBone:
		PropertiesToShow_AT.AddUnique(FName(TEXT("BonesConstraintData")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetVirtualBoneToolType(EVirtualBoneToolsType InToolsType)
{
	BoneToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Bone);
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetBoneToolsVisibility() const
{
	return VirtualToolsType == EVirtualToolsType::Bone && (BoneToolsType == EVirtualBoneToolsType::ModifyBoneTrack) ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FVirtualAnimationToolsEdModeToolkit::OnSampleBonesTrackButtonClickedEvent()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return FReply::Handled();
	}

	DetailsViewDataObject->OnSampleBoneTracks(BoneToolsType);
	return FReply::Handled();
}

#pragma endregion


#pragma region Curve
void FVirtualAnimationToolsEdModeToolkit::OnCurveToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Curve Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Params")));

	// Response the tools type
	switch (CurveToolsType)
	{
	case EVirtualCurveToolsType::Motion:
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurveFilter")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetName")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetsMap")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;

	case EVirtualCurveToolsType::Bone:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SampleBonesCurve")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;

	case EVirtualCurveToolsType::Sort:
		break;

	case EVirtualCurveToolsType::Scale:
		PropertiesToShow_AT.Reset();
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurveToolsType")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("TMap_MaxCurveValue")));
		break;

	case EVirtualCurveToolsType::Remove:
		PropertiesToShow_AT.AddUnique(FName(TEXT("bClearAllCurves")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetName")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetsMap")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;

	case EVirtualCurveToolsType::Output:
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurveFilter")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetName")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("CurvePresetsMap")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("OutputCurveAssets")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;

	case EVirtualCurveToolsType::Copy:
		PropertiesToShow_AT.AddUnique(FName(TEXT("CopyCurvesData")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;

	case EVirtualCurveToolsType::Transfer:
		PropertiesToShow_AT.AddUnique(FName(TEXT("TransferCurvesData")));

		CategoriesToShow_RT.AddUnique(FName(TEXT("Curve Reference")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("Curve Attributes")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetVirtualCurveToolType(EVirtualCurveToolsType InToolsType)
{
	CurveToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Curve);
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetCurveToolsVisibility() const
{
	return VirtualToolsType == EVirtualToolsType::Curve && (CurveToolsType == EVirtualCurveToolsType::Motion || CurveToolsType == EVirtualCurveToolsType::Output) ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FVirtualAnimationToolsEdModeToolkit::OnSampleCurvesButtonClickedEvent()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return FReply::Handled();
	}

	DetailsViewDataObject->SetAccumulateCurve(false);
	DetailsViewDataObject->OnResponseCurveTools(CurveToolsType);
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnSampleAccuCurvesButtonClickedEvent()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return FReply::Handled();
	}

	DetailsViewDataObject->SetAccumulateCurve(true);
	DetailsViewDataObject->OnResponseCurveTools(CurveToolsType);
	return FReply::Handled();
}
#pragma endregion


#pragma region Notify
void FVirtualAnimationToolsEdModeToolkit::OnNotifyToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Notify Params")));

	// Response the tools type
	switch (NotifyToolsType)
	{
	case EVirtualNotifyToolsType::AddNotifies:
		PropertiesToShow_AT.AddUnique(FName(TEXT("NotifiesData")));
		return;

	case EVirtualNotifyToolsType::ModifyNotifies:
		PropertiesToShow_AT.AddUnique(FName(TEXT("ModifyNotifiesData")));
		break;

	case EVirtualNotifyToolsType::RemoveNotifies:
		PropertiesToShow_AT.AddUnique(FName(TEXT("NotifiesData")));
		break;

	case EVirtualNotifyToolsType::AddNotifiesTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("NotifiesTrackData")));
		break;

	case EVirtualNotifyToolsType::ModifyNotifiesTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("ModifyNotifiesTrackMap")));
		break;

	case EVirtualNotifyToolsType::RemoveNotifiesTrack:
		PropertiesToShow_AT.AddUnique(FName(TEXT("NotifiesTrackData")));
		break;

	case EVirtualNotifyToolsType::FootStep:
		PropertiesToShow_AT.AddUnique(FName(TEXT("NotifiesData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootStepSampleData")));
		CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetVirtualNotifyToolType(EVirtualNotifyToolsType InToolsType)
{
	NotifyToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Notify);
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetNotifyToolsVisibility() const
{
	return VirtualToolsType == EVirtualToolsType::Notify
		&& (NotifyToolsType == EVirtualNotifyToolsType::ModifyNotifies || NotifyToolsType == EVirtualNotifyToolsType::ModifyNotifiesTrack
			|| NotifyToolsType == EVirtualNotifyToolsType::RemoveNotifiesTrack) ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FVirtualAnimationToolsEdModeToolkit::OnSampleNotifiesButtonClickedEvent()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return FReply::Handled();
	}

	DetailsViewDataObject->OnSampleNotifiesData(NotifyToolsType);
	return FReply::Handled();
}
#pragma endregion


#pragma region Montage
void FVirtualAnimationToolsEdModeToolkit::OnMontageToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Initialize montage tools reference property
	DetailsViewDataObject->InitializeMontageSlotsData();

	// Response the tools type
	switch (MontageToolsType)
	{
	case EVirtualMontageToolsType::Sort:
	case EVirtualMontageToolsType::Modifier:
	case EVirtualMontageToolsType::Loop:
		PropertiesToShow_AT.AddUnique(FName(TEXT("MontageSlotsData")));
		CategoriesToShow_AT.AddUnique(FName(TEXT("Montage Params")));
		CategoriesToShow_AT.AddUnique(FName(TEXT("Montage Slots")));
		CategoriesToShow_AT.AddUnique(FName(TEXT("Montage Attributes")));

		PropertiesToShow_RT.AddUnique(FName(TEXT("bSortMontageSlots")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("MontageGroupsMap")));
		CategoriesToShow_RT.AddUnique(FName(TEXT("Montage Params")));
		break;

	case EVirtualMontageToolsType::Transfer:
		PropertiesToShow_AT.AddUnique(FName(TEXT("bClearInvalidSlots")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("TargetSkeletons_Montage")));
		CategoriesToShow_AT.AddUnique(FName(TEXT("Montage Slots")));
		break;
	}

#if 1
	// Get the property module
	FPropertyEditorModule& thePropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register montage slot details
	thePropertyModule.RegisterCustomClassLayout(UVAT_DetailsViewData::StaticClass()->GetFName()
		, FOnGetDetailCustomizationInstance::CreateStatic(&FVAT_MontageSlotDetails::MakeInstance, FOnInvokeTab()));
#endif
}

void FVirtualAnimationToolsEdModeToolkit::SetVirtualMontageToolType(EVirtualMontageToolsType InToolsType)
{
	MontageToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Montage);
}
#pragma endregion


#pragma region Asset
void FVirtualAnimationToolsEdModeToolkit::OnAssetToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Asset")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("Asset Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("AsserReference")));

	// Response the tools type
	switch (AssetToolsType)
	{
	case EVirtualAssetToolsType::Crop:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetCropData")));
		return;

	case EVirtualAssetToolsType::Insert:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetInsertData")));
		return;

	case EVirtualAssetToolsType::Resize:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetResizeData")));
		return;

	case EVirtualAssetToolsType::Replace:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetReplaceData")));
		return;

	case EVirtualAssetToolsType::Composite:
		CategoriesToShow_AT.AddUnique(FName(TEXT("Blend")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetCompositesData")));
		return;

	case EVirtualAssetToolsType::ResizeRootMotion:
		CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionCurvesData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionSampleData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionAlignmentData")));
		return;

	case EVirtualAssetToolsType::ConvertRootMotion:
		CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionCurvesData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionConvertData")));
		return;

	case EVirtualAssetToolsType::AlignmentRootMotion:
		CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionAlignmentData")));
		return;

	case EVirtualAssetToolsType::ConvertMotionToReferencePose:
		PropertiesToShow_AT.AddUnique(FName(TEXT("MotionCaptureReferenceData")));
		return;

	case EVirtualAssetToolsType::GenerateLOD:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SkeletalMeshs")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("LODsData")));
		return;

	case EVirtualAssetToolsType::RemoveLOD:
		PropertiesToShow_AT.AddUnique(FName(TEXT("SkeletalMeshs")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("RemoveLODGroups")));
		return;

	case EVirtualAssetToolsType::Export:
		PropertiesToShow_AT.AddUnique(FName(TEXT("AssetExportData")));
		return;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetVirtualAssetToolType(EVirtualAssetToolsType InToolsType)
{
	AssetToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Asset);
}

FReply FVirtualAnimationToolsEdModeToolkit::OnAlignmentMotionCurvesButtonClickedEvent()
{
	if (HasValidData())
	{
		DetailsViewDataObject->OnResponseAlignmentMotionCurves();
	}
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnSampleMotionCurvesButtonClickedEvent()
{
	if (HasValidData())
	{
		DetailsViewDataObject->OnResponseAssetTools(AssetToolsType/*EVirtualAssetToolsType::SampleRootMotion*/, EVirtualRootMotionProcessType::SamplingCurves);
	}
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnConvertRootMotionButtonClickedEvent()
{
	AssetRootMotionProcessType = EVirtualRootMotionProcessType::ConvertingAsset;
	SetVirtualToolType(EVirtualToolsType::Asset);
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnApplyConvertToMotionCaptureButtonClickedEvent()
{
	if (HasValidData())
	{
		DetailsViewDataObject->SetRootMotionToolsType(EVirtualRootMotionToolsType::ConvertRootMotionToMotionCapture);
		DetailsViewDataObject->OnResponseAssetTools(EVirtualAssetToolsType::ResizeRootMotion, EVirtualRootMotionProcessType::ConvertingAsset);
	}
	return FReply::Handled();
}

FReply FVirtualAnimationToolsEdModeToolkit::OnApplyConvertToRootMotionButtonClickedEvent()
{
	if (HasValidData())
	{
		DetailsViewDataObject->SetRootMotionToolsType(EVirtualRootMotionToolsType::ConvertMotionCaptureToRootMotion);
		DetailsViewDataObject->OnResponseAssetTools(EVirtualAssetToolsType::ResizeRootMotion, EVirtualRootMotionProcessType::ConvertingAsset);
	}
	return FReply::Handled();
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetResizeRootMotionVisibility() const
{
	return VirtualToolsType == EVirtualToolsType::Asset && AssetToolsType == EVirtualAssetToolsType::ResizeRootMotion ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesVisibility() const
{
	return VirtualToolsType == EVirtualToolsType::Asset && (AssetToolsType == EVirtualAssetToolsType::ResizeRootMotion || AssetToolsType == EVirtualAssetToolsType::ConvertRootMotion) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesButtonText() const
{
	return AssetToolsType == EVirtualAssetToolsType::ConvertRootMotion
		? LOCTEXT("VAT_SampleMotionCurves_ButtonTipText", "Sampling root motion data for root bone.")
		: LOCTEXT("VAT_SampleMotionCurves_ButtonTipText", "Sampling motion data for motion bone.");
}

FText FVirtualAnimationToolsEdModeToolkit::GetSampleMotionCurvesButtonTooltip() const
{
	return AssetToolsType == EVirtualAssetToolsType::ConvertRootMotion
		? LOCTEXT("VAT_SampleMotionCurves_Button", "SampleRootMotionCurves")
		: LOCTEXT("VAT_SampleMotionCurves_Button", "SampleMotionCurves");
}
#pragma endregion


#pragma region Mirror
void FVirtualAnimationToolsEdModeToolkit::OnMirrorToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Mirror Params")));

	// Response the tools type
	switch (MirrorToolsType)
	{
	case EVirtualMirrorToolsType::Mirror:
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorAssetType")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorAssetSuffixName")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("MirrorBoneTree")));
		break;

	case EVirtualMirrorToolsType::MirrorBoneTree:
		PropertiesToShow_AT.AddUnique(FName(TEXT("bSaveMirrorBoneTree")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorAxis")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorTwinNamesMap")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorSampleStringLength")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("MirrorBoneTree")));
		break;
	}

	// Get the property module
	FPropertyEditorModule& thePropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register mirror data details
	thePropertyModule.RegisterCustomClassLayout(UVAT_DetailsViewData::StaticClass()->GetFName()
		, FOnGetDetailCustomizationInstance::CreateStatic(&FVAT_MirrorDataDetails::MakeInstance));
}

void FVirtualAnimationToolsEdModeToolkit::SetMirrorToolsType(EVirtualMirrorToolsType InToolsType)
{
	MirrorToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Mirror);
}
#pragma endregion


#pragma region Retarget
void FVirtualAnimationToolsEdModeToolkit::OnRetargetToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Bone Params")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("Retarget Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("Retarget Reference")));

	// Response the tools type
	switch (RetargetToolsType)
	{
	case EVirtualRetargetToolsType::Pose:
		PropertiesToShow_AT.AddUnique(FName(TEXT("ReferenceSkeleton_Retarget")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("ReferenceAnimSequence_Retarget")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("RetargetBoneTree")));
		break;

	case EVirtualRetargetToolsType::Animation:
		PropertiesToShow_AT.AddUnique(FName(TEXT("RetargetAnimationsData")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetRetargetToolsType(EVirtualRetargetToolsType InToolsType)
{
	RetargetToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::Retarget);
}
#pragma endregion


#pragma region PoseSearch
void FVirtualAnimationToolsEdModeToolkit::OnPoseSearchToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("PoseSearchParams")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("PoseSearchParams")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("PoseSearchReference")));

	// Response the tools type
	switch (PoseSearchToolsType)
	{
	case EVirtualPoseSearchToolsType::Distance:
		PropertiesToShow_AT.AddUnique(FName(TEXT("PoseSearchSampleData")));
		break;

	case EVirtualPoseSearchToolsType::TimeSync:
		PropertiesToShow_AT.AddUnique(FName(TEXT("PoseSearchSampleData")));
		PropertiesToShow_AT.AddUnique(FName(TEXT("PoseSearchTimeSyncData")));
		break;

	case EVirtualPoseSearchToolsType::Animation:
		PropertiesToShow_AT.AddUnique(FName(TEXT("ToAnimSequencesPosesMap")));
		CategoriesToShow_RT.AddUnique(FName(TEXT("PoseSearchSampleData")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetPoseSearchToolsType(EVirtualPoseSearchToolsType InToolsType)
{
	PoseSearchToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::PoseSearch);
}
#pragma endregion


#pragma region GameFramework
void FVirtualAnimationToolsEdModeToolkit::OnGameFrameworkToolsChanged()
{
	// Check the data is valid
	if (!HasValidData())
	{
		return;
	}

	// Global
	CategoriesToShow_AT.AddUnique(FName(TEXT("Blend")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("LegsBaseData")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("Skeleton Params")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("Global Params")));
	CategoriesToShow_AT.AddUnique(FName(TEXT("GameFramework Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("Global Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("GameFramework Params")));
	CategoriesToShow_RT.AddUnique(FName(TEXT("GameFramework Reference")));

	// Response the tools type
	switch (GameFrameworkToolsType)
	{
	case EVirtualGameFrameworkToolsType::FootIK:
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootIKSampleData")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("LegsBaseData")));
		break;

	case EVirtualGameFrameworkToolsType::FootLock:
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootLockSampleData")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("LegsBaseData")));
		break;

	case EVirtualGameFrameworkToolsType::FootOffset:
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootOffsetSampleData")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("LegsBaseData")));
		break;

	case EVirtualGameFrameworkToolsType::FootWeight:
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootWeightSampleData")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("LegsBaseData")));
		break;

	case EVirtualGameFrameworkToolsType::FootPosition:
		PropertiesToShow_AT.AddUnique(FName(TEXT("FootPositionSampleData")));
		PropertiesToShow_RT.AddUnique(FName(TEXT("LegsBaseData")));
		break;

	case EVirtualGameFrameworkToolsType::PosesCurve:
		PropertiesToShow_AT.AddUnique(FName(TEXT("PosesBlendSampleData")));
		break;
	}
}

void FVirtualAnimationToolsEdModeToolkit::SetGameFrameworkToolsType(EVirtualGameFrameworkToolsType InToolsType)
{
	GameFrameworkToolsType = InToolsType;
	SetVirtualToolType(EVirtualToolsType::GameFramework);
}
#pragma endregion


#undef LOCTEXT_NAMESPACE
