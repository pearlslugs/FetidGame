// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "VAT_EditorTool.h"
#include "VAT_RetargetTools.generated.h"

UCLASS()
class UVAT_RetargetPoseTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_RetargetPoseTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_RetargetAnimationTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_RetargetAnimationTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_RetargetRebuildTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_RetargetRebuildTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};