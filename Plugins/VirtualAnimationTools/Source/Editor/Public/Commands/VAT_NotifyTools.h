// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "VAT_EditorTool.h"
#include "VAT_NotifyTools.generated.h"

UCLASS()
class UVAT_NotifyAddTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyAddTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyModifyTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyModifyTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyRemoveTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyRemoveTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyAddTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyAddTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyModifyTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyModifyTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyRemoveTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyRemoveTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyFootStepTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyFootStepTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};