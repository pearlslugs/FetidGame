// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "Commands/VAT_EditorCommands.h"
#include "UObject/UObjectIterator.h"
#include "Style/VAT_EditorStyle.h"
#include "Commands/VAT_EditorTool.h"

#define LOCTEXT_NAMESPACE "VAT_EditorCommands"

FVAT_EditorCommands::FVAT_EditorCommands()
	: TCommands<FVAT_EditorCommands>("VirtualAnimationToolsEditor", LOCTEXT("VAT", "VAT"), NAME_None, FVAT_EditorStyle::StyleName)
{
}

void FVAT_EditorCommands::RegisterCommands()
{
	// Virtual Animation Tools
	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		if (ClassIterator->IsChildOf(UVAT_EditorActionTool::StaticClass()) && !ClassIterator->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			TSubclassOf<UVAT_EditorActionTool> SubclassOf = (*ClassIterator);
			UVAT_EditorActionTool* VAT_EditorTool = SubclassOf->GetDefaultObject<UVAT_EditorActionTool>();
			VAT_EditorTool->RegisterUICommand(this);
		}
	}

}

#undef LOCTEXT_NAMESPACE
