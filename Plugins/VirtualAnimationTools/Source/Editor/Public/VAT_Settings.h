// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "VAT_DetailsViewData.h"
#include "VAT_Settings.generated.h"

#define GET_VAT_CONFIG_VAR(a) (GetDefault<UVAT_Settings>()->a)

/**
 *	Class for importing VirtualAnimationTools directly from a config file.
 */
UCLASS(config = VirtualAnimationTools, defaultconfig, notplaceable)
class VIRTUALANIMATIONTOOLSEDITOR_API UVAT_Settings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Relative path to the ini file that is backing this list */
	UPROPERTY()
	FString ConfigFileName;

public:

	/** Default virtual animation tools class */
	UPROPERTY(globalconfig, EditDefaultsOnly, Category = Config)
	TSubclassOf<UVAT_DetailsViewData> DetailsViewDataClass;
};
