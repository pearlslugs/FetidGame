// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VirtualAnimationToolsEditor.h"
#include "VirtualAnimationToolsEdMode.h"
#include "ISettingsModule.h"
#include "VAT_Settings.h"
#include "VAT_DetailsViewData.h"
#include "Style/VAT_EditorStyle.h"
#include "Style/VAT_MirrorDataDetails.h"
#include "Style/VAT_MontageSlotDetails.h"
#include "Commands/VAT_EditorCommands.h"
#include "Persona/Public/PersonaDelegates.h"

#if ENGINE_MAJOR_VERSION > 4
#include "Toolkits/AssetEditorToolkit.h"
#endif

#define LOCTEXT_NAMESPACE "FVirtualAnimationToolsEditorModule"

void FVirtualAnimationToolsEditorModule::StartupModule()
{
#if ENGINE_MAJOR_VERSION > 4
	FVAT_EditorStyle::Get();
#else
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FEditorModeRegistry::Get().RegisterMode<FVirtualAnimationToolsEdMode>(FVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId,
		LOCTEXT("VirtualAnimationToolsEdModeName", "Virtual Animation Tools"),
		FSlateIcon(FVAT_EditorStyle::Get().GetStyleSetName(), "VirtualAnimationToolsEditor", "VirtualAnimationToolsEditor.Small"), true);
#endif

#if 0 // We don't register in blueprint class 
	// Register mirror table
	thePropertyModule.RegisterCustomClassLayout(UVAT_DetailsViewData::StaticClass()->GetFName()
		, FOnGetDetailCustomizationInstance::CreateStatic(&FVAT_MirrorDataDetails::MakeInstance));

	// Register montage slot name
	thePropertyModule.RegisterCustomClassLayout(UVAT_DetailsViewData::StaticClass()->GetFName()
		, FOnGetDetailCustomizationInstance::CreateStatic(&FVAT_MontageSlotDetails::MakeInstance, FOnInvokeTab()));
#endif

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Virtual Animation Tools",
			LOCTEXT("VirtualAnimationToolsSettingsName", "VirtualAnimationTools"),
			LOCTEXT("VirtualAnimationToolsSettingsDescription", "Configure the Virtual Animation Tools plug-in."),
			GetMutableDefault<UVAT_Settings>()
		);
	}

	FVAT_EditorCommands::Register();
}

void FVirtualAnimationToolsEditorModule::ShutdownModule()
{
#if ENGINE_MAJOR_VERSION > 4
	/*FEditorModeRegistry::Get().UnregisterMode(UVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId);*/
#else
	FEditorModeRegistry::Get().UnregisterMode(FVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId);
#endif

	// Unregister settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Virtual Animation Tools");
	}

	FVAT_EditorCommands::Unregister();
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVirtualAnimationToolsEditorModule, VirtualAnimationToolsEditor)