// Copyright Ran 001, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAddVirtualBonePluginsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
