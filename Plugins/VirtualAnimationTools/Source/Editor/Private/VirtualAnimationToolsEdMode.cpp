// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VirtualAnimationToolsEdMode.h"
#include "VirtualAnimationToolsEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "Style/VAT_EditorStyle.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "FVirtualAnimationToolsModeToolkit"

#if ENGINE_MAJOR_VERSION > 4
const FEditorModeID UVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId = TEXT("EM_VirtualAnimationToolsEdMode");

UVirtualAnimationToolsEdMode::UVirtualAnimationToolsEdMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	Info = FEditorModeInfo(
		UVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId,
		LOCTEXT("VirtualAnimationToolsEdModeName", "Virtual Animation Tools"),
		FSlateIcon(FVAT_EditorStyle::Get().GetStyleSetName(), "VirtualAnimationToolsEditor", "VirtualAnimationToolsEditor.Small"),
		true);
}

UVirtualAnimationToolsEdMode::~UVirtualAnimationToolsEdMode()
{}

void UVirtualAnimationToolsEdMode::Enter()
{
	UEdMode::Enter();
}

void UVirtualAnimationToolsEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		static_cast<FVirtualAnimationToolsEdModeToolkit*>(Toolkit.Get())->Shutdown();
	}

	// Call base Exit method to ensure proper cleanup
	Super::Exit();
}

void UVirtualAnimationToolsEdMode::CreateToolkit()
{
	if (!Toolkit.IsValid())
	{
		VAT_Toolkit = MakeShareable(new FVirtualAnimationToolsEdModeToolkit);
		Toolkit = VAT_Toolkit;
	}
}

void UVirtualAnimationToolsEdMode::ModeTick(float DeltaTime)
{
	Super::ModeTick(DeltaTime);
	if (VAT_Toolkit.IsValid())
	{
		VAT_Toolkit->OnTick(DeltaTime);
	}
}

#else
const FEditorModeID FVirtualAnimationToolsEdMode::EM_VirtualAnimationToolsEdModeId = TEXT("EM_VirtualAnimationToolsEdMode");
FVirtualAnimationToolsEdMode::FVirtualAnimationToolsEdMode()
{}

FVirtualAnimationToolsEdMode::~FVirtualAnimationToolsEdMode()
{}

void FVirtualAnimationToolsEdMode::Enter()
{
	FEdMode::Enter();
	
	if (!Toolkit.IsValid())
	{
		VAT_Toolkit = MakeShareable(new FVirtualAnimationToolsEdModeToolkit);
		Toolkit = VAT_Toolkit;
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FVirtualAnimationToolsEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		VAT_Toolkit->Shutdown();
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FVirtualAnimationToolsEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	if (VAT_Toolkit.IsValid())
	{
		VAT_Toolkit->OnTick(DeltaTime);
	}
}

void FVirtualAnimationToolsEdMode::SelectionChanged()
{
	if (VAT_Toolkit.IsValid())
	{
		VAT_Toolkit->OnSelectionChanged();
	}
}
#endif

#undef LOCTEXT_NAMESPACE
