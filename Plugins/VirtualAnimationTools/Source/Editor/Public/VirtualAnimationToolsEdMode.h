// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"

#if 0 && ENGINE_MAJOR_VERSION > 4
#include "Tools/UEdMode.h"
#include "VirtualAnimationToolsEdMode.generated.h"
#endif

#if 0 && ENGINE_MAJOR_VERSION > 4
UCLASS(Transient)
class UVirtualAnimationToolsEdMode : public UEdMode
{
	GENERATED_BODY()

private:
	/** Editor Mode Toolkit that is associated with this toolkit mode */
	TSharedPtr<class FVirtualAnimationToolsEdModeToolkit> VAT_Toolkit;

public:
	const static FEditorModeID EM_VirtualAnimationToolsEdModeId;

	UVirtualAnimationToolsEdMode();
	virtual ~UVirtualAnimationToolsEdMode();

	// UEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void CreateToolkit() override;
	virtual void ModeTick(float DeltaTime) override;
	// End of FEdMode interface

	/**
	 * Check to see if this UEdMode wants to disallow AutoSave
	 * @return true if AutoSave can be applied right now
	 */
	virtual bool CanAutoSave() const override { return false; }
};
#else
class FVirtualAnimationToolsEdMode : public FEdMode
{
private:
	/** Editor Mode Toolkit that is associated with this toolkit mode */
	TSharedPtr<class FVirtualAnimationToolsEdModeToolkit> VAT_Toolkit;

public:
	const static FEditorModeID EM_VirtualAnimationToolsEdModeId;
public:
	FVirtualAnimationToolsEdMode();
	virtual ~FVirtualAnimationToolsEdMode();

	// FEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void SelectionChanged() override;
	// End of FEdMode interface

	/**
	 * Check to see if this UEdMode wants to disallow AutoSave
	 * @return true if AutoSave can be applied right now
	 */
	virtual bool CanAutoSave() const override { return false; }
};
#endif