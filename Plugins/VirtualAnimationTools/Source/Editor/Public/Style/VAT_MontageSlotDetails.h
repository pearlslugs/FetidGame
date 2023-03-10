// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "PersonaDelegates.h"
#include "IDetailCustomNodeBuilder.h"

class USkeleton;
class IPropertyHandle;
class IDetailLayoutBuilder;

/* Montage slot data preview data. */
class FVAT_MontageSlotDataBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FVAT_MontageSlotDataBuilder>
{
public:
	FVAT_MontageSlotDataBuilder(USkeleton* InSkeleton, IDetailLayoutBuilder* InDetailLayoutBuilder, const uint32& InSlotDataIndex, const TSharedPtr<IPropertyHandle>& InMontageSlotDataProperty);

	/** IDetailCustomNodeBuilder interface */
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick(float DeltaTime) override {}
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; }
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual bool InitiallyCollapsed() const override;
	virtual FName GetName() const override { return NAME_None; }
	virtual FText GetText() const;

private:

	/** Montage asset using skeleton */
	USkeleton* Skeleton;

	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;

	/** Detail layout builder reference */
	IDetailLayoutBuilder* DetailLayoutBuilderPtr;

private:

	/** Montage slot data index */
	uint32 SlotDataIndex;

	/** Delegate used to invoke a tab in the containing editor */
	FOnInvokeTab OnInvokeTab;

	/** Montage slot data property handle */
	TSharedPtr<IPropertyHandle> SlotDataPropertyHandle;

protected:

	/** Response rebuild details */
	void OnRebuildDetails();
};

/* Montage slot preview data. */
class FVAT_MontageSlotNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FVAT_MontageSlotNodeBuilder>
{
public:
	FVAT_MontageSlotNodeBuilder(USkeleton* InSkeleton, IDetailLayoutBuilder* InDetailLayoutBuilder, const TSharedPtr<IPropertyHandle>& InMontageSlotDataProperty, const TSharedPtr<IPropertyHandle>& InMontageSlotNameProperty);

	/** IDetailCustomNodeBuilder interface */
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick(float DeltaTime) override {}
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; }
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual bool InitiallyCollapsed() const override { return true; }
	virtual FName GetName() const override { return NAME_None; }

private:

	/** Montage asset using skeleton */
	USkeleton* Skeleton;

	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;

	/** Detail layout builder reference */
	IDetailLayoutBuilder* DetailLayoutBuilderPtr;

private:

	/** Delegate used to invoke a tab in the containing editor */
	FOnInvokeTab OnInvokeTab;

	/** Montage slot data property handle */
	TSharedPtr<IPropertyHandle> SlotDataPropertyHandle;

	/** Montage slot name property handle */
	TSharedPtr<IPropertyHandle> SlotNamePropertyHandle;

	/** Montage slot list name */
	TArray<FName> SlotNameList;

	/** Montage slot name combo selected name */
	FName SlotNameComboSelectedName;

	/** Montage slot name combo box widget */
	TSharedPtr<class STextComboBox>	SlotNameComboBox;

	/** Montage slot name combo list items */
	TArray<TSharedPtr<FString>> SlotNameComboListItems;

	/** Response open animation slot manager */
	FReply OnOpenAnimSlotManager();

	/** Response montage slot list opening */
	void OnSlotListOpening();

	/** Response montage slot name changed */
	void OnSlotNameChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Response refresh combo lists */
	void RefreshComboLists(bool bOnlyRefreshIfDifferent = false);

private:
	bool CanInsertSlotName();
	void OnAddSlotName();
	void OnInsertSlotName();
	void OnDeleteSlotName();
	void OnDuplicateSlotName();
};

/**
 * Customizes a MontageSlot
 */
class FVAT_MontageSlotDetails : public IDetailCustomization
{
private:

	/** Delegate used to invoke a tab in the containing editor */
	FOnInvokeTab OnInvokeTab;

	/** Detail layout builder reference */
	IDetailLayoutBuilder* DetailLayoutBuilderPtr;

public:

	FVAT_MontageSlotDetails(FOnInvokeTab InOnInvokeTab);

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance(FOnInvokeTab InOnInvokeTab);

	/** ILayoutDetails interface */
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;

protected:

	/** Response rebuild details */
	void OnRebuildDetails();
};
