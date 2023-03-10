// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VirtualAnimationTools.h"

#define LOCTEXT_NAMESPACE "FVirtualAnimationToolsModule"

DEFINE_LOG_CATEGORY(LogVirtualAnimTools);

void FVirtualAnimationToolsModule::StartupModule()
{
}

void FVirtualAnimationToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVirtualAnimationToolsModule, VirtualAnimationTools)