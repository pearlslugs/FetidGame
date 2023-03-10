// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "VAT_EditorTool.h"

#include "VAT_EditorToolGenerators.generated.h"

UCLASS()
class UVAT_BoneTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};


UCLASS()
class UVAT_CurveTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_CurveTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_NotifyTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_NotifyTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_MontageTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_MontageTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_AssetTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_AssetTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_MirrorTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_MirrorTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_RetargetTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_RetargetTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};