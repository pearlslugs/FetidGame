// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#if ENGINE_MAJOR_VERSION < 5
#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#endif

/**
 * Slate style set that defines all the styles for the take recorder UI
 */
class FVAT_EditorStyle
	: public FSlateStyleSet
{
public:
	static FName StyleName;

	/** Access the singleton instance for this style set */
	static FVAT_EditorStyle& Get();

private:

	FVAT_EditorStyle();
	~FVAT_EditorStyle();
};