// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VAT_Settings.h"

UVAT_Settings::UVAT_Settings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ConfigFileName(GetDefaultConfigFilename())
	, DetailsViewDataClass(UVAT_DetailsViewData::StaticClass())
{}