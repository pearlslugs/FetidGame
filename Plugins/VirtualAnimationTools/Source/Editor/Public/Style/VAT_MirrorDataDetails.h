// Copyright 2017-2018 Theodor Luis Martin Bruna (Hethger). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"

class SEditableTextBox;
class FDetailWidgetRow;
class IPropertyHandle;
class IDetailChildrenBuilder;
class IDetailLayoutBuilder;

/* Mirror table preview data. */
class FVAT_MirrorDataNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FVAT_MirrorDataNodeBuilder>
{
public:
	FVAT_MirrorDataNodeBuilder(IDetailLayoutBuilder* InDetailLayoutBuilder, const TSharedPtr<IPropertyHandle>& InMirrorBoneDataProperty, const uint32& InArrayIndex);

	/** IDetailCustomNodeBuilder interface */
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick(float DeltaTime) override {}
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; }
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual bool InitiallyCollapsed() const override { return true; }
	virtual FName GetName() const override { return NAME_None; }
	virtual FText GetText() const;
	virtual FSlateColor GetDisplayNameColor() const;

private:

	FReply ToggleLayerVisibility(uint32 Index);
	FReply ToggleChildLayerVisibility(uint32 Index);

private:

	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;

	/** Detail layout builder reference */
	IDetailLayoutBuilder* DetailLayoutBuilderPtr;

	/** Property handle to mirror bone data */
	TSharedPtr<IPropertyHandle> MirrorBoneDataProperty;
	TSharedPtr<IPropertyHandle> EnableProperty;
	TSharedPtr<IPropertyHandle> IsTwinBoneProperty;
	TSharedPtr<IPropertyHandle> BoneReferenceProperty;
	TSharedPtr<IPropertyHandle> TwinBoneReferenceProperty;

	/** Details layout builder data visibility button reference */
	TSharedPtr<SWidget> VisibilityButton;

	/** Details layout builder children visibility button reference */
	TSharedPtr<SWidget> ChildrenVisibilityButton;

	/** Cached mirror array index in mirror bone tree */
	uint32 MirrorArrayIndex;
};

/**
 * Customizes a MirrorData
 */
class FVAT_MirrorDataDetails : public IDetailCustomization
{
private:

	/** Detail layout builder reference */
	IDetailLayoutBuilder* DetailLayoutBuilderPtr;

public:

	/** Property handle to the localization key format property within this context mapping */
	TSharedPtr<IPropertyHandle> SkeletonHandle;

public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** ILayoutDetails interface */
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;

protected:

	/** Response rebuild details */
	void OnRebuildDetails();
};
