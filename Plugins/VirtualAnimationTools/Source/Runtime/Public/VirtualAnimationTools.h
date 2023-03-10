// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVirtualAnimTools, Log, All);

DECLARE_STATS_GROUP(TEXT("Virtual AnimTools"), STATGROUP_VirtualAnimTools, STATCAT_Advanced);

class FVirtualAnimationToolsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
