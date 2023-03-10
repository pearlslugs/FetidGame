// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Textures/SlateIcon.h"
#include "SceneManagement.h"

#include "Framework/Commands/UICommandInfo.h"
#include "VirtualAnimationToolsEdModeToolkit.h"
#include "VAT_EditorCommands.h"

#include "VAT_EditorTool.generated.h"

// class UFractureModalTool;
//class FFractureToolContext;

template <typename T>
class TManagedArray;

// DECLARE_LOG_CATEGORY_EXTERN(LogTool, Log, All);

UCLASS(Abstract, config = EditorPerProjectUserSettings)
class UVAT_EditorToolSettings : public UObject
{
	GENERATED_BODY()
public:
	UVAT_EditorToolSettings(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

};


/** Tools derived from this class should require parameter inputs from the user, only the bone selection. */
UCLASS(Abstract)
class UVAT_EditorActionTool : public UObject
{
public:
	GENERATED_BODY()

	UVAT_EditorActionTool(const FObjectInitializer& ObjInit) : Super(ObjInit) {}

	/** This is the Text that will appear on the tool button to execute the tool **/
	virtual FText GetDisplayText() const { return FText(); }
	virtual FText GetTooltipText() const { return FText(); }

	virtual FSlateIcon GetToolIcon() const { return FSlateIcon(); }

	/** Executes the command.  Derived types need to be implemented in a thread safe way*/
	virtual void Execute(TWeakPtr<FVirtualAnimationToolsEdModeToolkit> InToolkit) {}
	virtual bool CanExecute() const;

	/** Gets the UI command info for this command */
	const TSharedPtr<FUICommandInfo>& GetUICommandInfo() const;

	virtual void RegisterUICommand(FVAT_EditorCommands* BindingContext) {}

	//virtual TArray<FVAT_EditorToolContext> GetVATEditorToolContexts() const;
	
protected:
	//static void Refresh(FFractureToolContext& Context, FFractureEditorModeToolkit* Toolkit, bool bClearSelection=false);
	//static void SetOutlinerComponents(TArray<FFractureToolContext>& InContexts, FFractureEditorModeToolkit* Toolkit);


protected:
	TSharedPtr<FUICommandInfo> UICommandInfo;

};