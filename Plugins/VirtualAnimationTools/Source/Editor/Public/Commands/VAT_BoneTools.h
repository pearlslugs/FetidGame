// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "VAT_EditorTool.h"
#include "VAT_BoneTools.generated.h"

UCLASS()
class UVAT_BoneBakeTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneBakeTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneBlendTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneBlendTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneModifyTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneModifyTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneLayerTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneLayerTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneConstraintTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneConstraintTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneAddTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneAddTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneRemoveTrackTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneRemoveTrackTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneAddTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneAddTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneRemoveTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneRemoveTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};

UCLASS()
class UVAT_BoneFilterTool : public UVAT_EditorActionTool
{
public:
	GENERATED_BODY()

	UVAT_BoneFilterTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	// UVAT_EditorActionTool Interface
	virtual FText GetDisplayText() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetToolIcon() const override;
	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) override;
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) override;
	virtual bool CanExecute() const override;
};