// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_EditorTool.h"
#include "Editor.h"
#include "Engine/Selection.h"

//DEFINE_LOG_CATEGORY(LogVAT_EditorTool);

void UVAT_EditorToolSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UVAT_EditorToolSettings::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

const TSharedPtr<FUICommandInfo>& UVAT_EditorActionTool::GetUICommandInfo() const
{
	return UICommandInfo;
}

bool UVAT_EditorActionTool::CanExecute() const
{
	return true;
}