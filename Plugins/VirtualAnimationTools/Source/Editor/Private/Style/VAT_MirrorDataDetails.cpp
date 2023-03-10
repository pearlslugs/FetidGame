// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.


#include "Style/VAT_MirrorDataDetails.h"
#include "VAT_DetailsViewData.h"

#include "Layout/Margin.h"

#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include "EditorStyleSet.h"
#include "IDetailChildrenBuilder.h"

#include "BoneContainer.h"
#include "Animation/Skeleton.h"

#include "IDetailsView.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "Misc/MessageDialog.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Library/VAT_Bone.h"

#define LOCTEXT_NAMESPACE "VAT_MirrorTableDetails"

/*-----------------------------------------------------------------------------
	FVAT_MirrorDataNodeBuilder implementation.
-----------------------------------------------------------------------------*/

FVAT_MirrorDataNodeBuilder::FVAT_MirrorDataNodeBuilder(IDetailLayoutBuilder* InDetailLayoutBuilder, const TSharedPtr<IPropertyHandle>& InMirrorBoneDataProperty, const uint32& InArrayIndex)
	: DetailLayoutBuilderPtr(InDetailLayoutBuilder)
	, MirrorBoneDataProperty(InMirrorBoneDataProperty)
	, MirrorArrayIndex(InArrayIndex)
{
	// Check the mirror bone data property is valid
	if (!MirrorBoneDataProperty->IsValidHandle())
	{
		return;
	}

	EnableProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, bEnable));
	IsTwinBoneProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, bIsTwinBone));
	BoneReferenceProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, Bone));
	TwinBoneReferenceProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, TwinBone));
}

void FVAT_MirrorDataNodeBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	// Check the mirror bone data property is valid
	if (!MirrorBoneDataProperty->IsValidHandle())
	{
		return;
	}

	// Check the mirror axis property is valid
	const TSharedPtr<IPropertyHandle> theMirrorAxisProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, MirrorAxis));
	if (!theMirrorAxisProperty->IsValidHandle())
	{
		return;
	}

	// Create the global visibility button clicked event
	FOnClicked theVisibilityClicked = FOnClicked::CreateSP(this, &FVAT_MirrorDataNodeBuilder::ToggleLayerVisibility, MirrorArrayIndex);
	VisibilityButton = PropertyCustomizationHelpers::MakeVisibilityButton(theVisibilityClicked);

	// Create the children visibility button clicked event
	FOnClicked ChildVisibilityClickedDelegate = FOnClicked::CreateSP(this, &FVAT_MirrorDataNodeBuilder::ToggleChildLayerVisibility, MirrorArrayIndex);
	ChildrenVisibilityButton = PropertyCustomizationHelpers::MakeVisibilityButton(ChildVisibilityClickedDelegate);

	// Build the row
	NodeRow
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ChildrenVisibilityButton.ToSharedRef()
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(this, &FVAT_MirrorDataNodeBuilder::GetText)
		    .ColorAndOpacity(this, &FVAT_MirrorDataNodeBuilder::GetDisplayNameColor)
		]
		]
		]

	+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			VisibilityButton.ToSharedRef()
		]
		];

	// Check the enable property is valid
	if (EnableProperty->IsValidHandle())
	{
		bool Enable = true;
		EnableProperty->GetValue(Enable);
		VisibilityButton->SetRenderOpacity(Enable ? 1.f : 0.5f);
		ChildrenVisibilityButton->SetRenderOpacity(Enable ? 1.f : 0.5f);
	}
}

void FVAT_MirrorDataNodeBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	// Check the mirror bone data property is valid
	if (!MirrorBoneDataProperty->IsValidHandle())
	{
		return;
	}

	// Get the mirror bone data children property number
	uint32 theChildrenNumber = 0;
	MirrorBoneDataProperty->GetNumChildren(theChildrenNumber);

	// Add all children property to builder
	for (uint32 ChildrenIndex = 0; ChildrenIndex < theChildrenNumber; ChildrenIndex++)
	{
		ChildrenBuilder.AddProperty(MirrorBoneDataProperty->GetChildHandle(ChildrenIndex).ToSharedRef());
	}
}

FText FVAT_MirrorDataNodeBuilder::GetText() const
{
	// Define the result text
	FString theOutputString;

	// Get the mirror bone name
	{
		TArray<UObject*> theOuterObjects;
		BoneReferenceProperty->GetOuterObjects(theOuterObjects);
		if (theOuterObjects.Num() > 0)
		{
			if (const FBoneReference* theBoneReferencePtr = (FBoneReference*)BoneReferenceProperty->GetValueBaseAddress((uint8*)theOuterObjects[0]))
			{
				theOutputString = theBoneReferencePtr->BoneName.ToString();
			}
		}
	}

	// Get the mirror twin bone name
	{
		TArray<UObject*> theOuterObjects;
		BoneReferenceProperty->GetOuterObjects(theOuterObjects);
		if (theOuterObjects.Num() > 0)
		{
			if (const FBoneReference* theBoneReferencePtr = (FBoneReference*)TwinBoneReferenceProperty->GetValueBaseAddress((uint8*)theOuterObjects[0]))
			{
				if (theBoneReferencePtr->BoneName != NAME_None)
				{
					theOutputString += ("  /  " + theBoneReferencePtr->BoneName.ToString());
				}
			}
		}
	}

	// Output the result text
	FText theResultText = FText::FromString(MoveTemp(theOutputString));
	return theResultText;
}


FSlateColor FVAT_MirrorDataNodeBuilder::GetDisplayNameColor() const
{
	// Get the enable property value
	bool bEnable = true;
	if (EnableProperty->IsValidHandle())
	{
		EnableProperty->GetValue(bEnable);
	}

	// Return the color
	return FSlateColor(bEnable ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) : FLinearColor(1.f, 0.f, 0.f, 1.f));
}

FReply FVAT_MirrorDataNodeBuilder::ToggleLayerVisibility(uint32 Index)
{
	// Check the button is valid
	if (VisibilityButton)
	{
		// Get the initial state
		bool bEnable = VisibilityButton->GetRenderOpacity() == 1.f ? false : true;

		// Set render opacity
		VisibilityButton->SetRenderOpacity(bEnable ? 1.f : 0.5f);

		// Set enable property value
		if (EnableProperty->IsValidHandle())
		{
			EnableProperty->SetValue(bEnable);
		}
	}

	// Return
	return FReply::Handled();
}


FReply FVAT_MirrorDataNodeBuilder::ToggleChildLayerVisibility(uint32 Index)
{
	// Check the button is valid
	if (!VisibilityButton)
	{
		return FReply::Handled();
	}

	// Get the initial state
	bool Enable = VisibilityButton->GetRenderOpacity() == 1.f ? false : true;

	// Set render opacity
	VisibilityButton->SetRenderOpacity(Enable ? 1.f : 0.5f);

	// Set enable property value
	if (EnableProperty->IsValidHandle())
	{
		EnableProperty->SetValue(Enable);
	}

	// Check the skeleton property handle is valid
	const TSharedPtr<IPropertyHandle> theSkeletonProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, Skeleton), UVAT_DetailsViewData::StaticClass());
	if (!theSkeletonProperty->IsValidHandle())
	{
		return FReply::Handled();
	}

	// Check the mirror bone tree property handle is valid
	const TSharedPtr<IPropertyHandle> theMirrorBoneTreeProperty = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, MirrorBoneTree), UVAT_DetailsViewData::StaticClass());
	if (!theMirrorBoneTreeProperty->IsValidHandle())
	{
		return FReply::Handled();
	}
	const TSharedPtr<IPropertyHandleArray> theMirrorBoneTreePropertyArray = theMirrorBoneTreeProperty->AsArray();

	// Check the mirror bone property handle is valid
	const TSharedPtr<IPropertyHandle> theMirrorBoneProperty = MirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, Bone));
	if (!theMirrorBoneProperty->IsValidHandle())
	{
		return FReply::Handled();
	}

	// Convert the object to skeleton
	UObject* theSkeletonPropertyObject;
	theSkeletonProperty->GetValue(theSkeletonPropertyObject);
	USkeleton* theSkeleton = Cast<USkeleton>(theSkeletonPropertyObject);

	// Check the selected bone reference is valid
	TArray<UObject*> theOuterObjects;
	BoneReferenceProperty->GetOuterObjects(theOuterObjects);
	const FBoneReference* theSelectedBoneReferencePtr = (FBoneReference*)theMirrorBoneProperty->GetValueBaseAddress((uint8*)theOuterObjects[0]);
	if (theSelectedBoneReferencePtr == nullptr)
	{
		return FReply::Handled();
	}

	// Get the bone tree array number
	uint32 theBoneTreeArrayNumber;
	theMirrorBoneTreePropertyArray->GetNumElements(theBoneTreeArrayNumber);

	// Each every bone tree index
	for (uint32 ArrayIndex = 0; ArrayIndex < theBoneTreeArrayNumber; ++ArrayIndex)
	{
		// Check the bone data property is valid
		const TSharedPtr<IPropertyHandle> theMirrorBoneDataProperty = theMirrorBoneTreePropertyArray->GetElement(ArrayIndex);
		if (!theMirrorBoneDataProperty->IsValidHandle())
		{
			continue;
		}

		// Get the enable property
		const TSharedPtr<IPropertyHandle> theEnableProperty = theMirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, bEnable));
		if (!theEnableProperty->IsValidHandle())
		{
			continue;
		}

		// Check the bone property is valid
		const TSharedPtr<IPropertyHandle> theBoneProperty = theMirrorBoneDataProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FVirtualMirrorBoneData, Bone));
		if (!theBoneProperty->IsValidHandle())
		{
			continue;
		}

		// Check the bone reference is valid
		const FBoneReference* theBoneReferencePtr = (FBoneReference*)theBoneProperty->GetValueBaseAddress((uint8*)theOuterObjects[0]);
		if (theBoneReferencePtr == nullptr)
		{
			continue;
		}

		// Check the bone is children
		if (UVAT_Bone::BoneIsChildOf(theSkeleton, theSelectedBoneReferencePtr->BoneName, theBoneReferencePtr->BoneName))
		{
			theEnableProperty->SetValue(Enable);
		}
	}

	// Force refresh details builder
	DetailLayoutBuilderPtr->ForceRefreshDetails();

	// Success
	return FReply::Handled();
}

/*-----------------------------------------------------------------------------
	FVAT_MirrorDataDetails implementation.
-----------------------------------------------------------------------------*/

TSharedRef<IDetailCustomization> FVAT_MirrorDataDetails::MakeInstance()
{
	return MakeShareable(new FVAT_MirrorDataDetails);
}

void FVAT_MirrorDataDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// Cache the details reference
	DetailLayoutBuilderPtr = &DetailBuilder;

	// Edit the mirror bone tree category
	IDetailCategoryBuilder& theMirrorBoneTreeCategoryBuilder = DetailBuilder.EditCategory("Mirror BoneTree", FText::GetEmpty(), ECategoryPriority::Uncommon);

	// Check the mirror bone tree property handle is valid
	const TSharedPtr<IPropertyHandle> theMirrorBoneTreePropertyHandle = DetailLayoutBuilderPtr->GetProperty(GET_MEMBER_NAME_CHECKED(UVAT_DetailsViewData, MirrorBoneTree), UVAT_DetailsViewData::StaticClass());
	if (!theMirrorBoneTreePropertyHandle->IsValidHandle())
	{
		return;
	}

	// Mark hidden the property handle
	theMirrorBoneTreePropertyHandle->MarkHiddenByCustomization();

	// Get the mirror bone tree array handle
	const TSharedPtr<IPropertyHandleArray> theMirrorBoneTreePropertyHandleArray = theMirrorBoneTreePropertyHandle->AsArray();

	// Get the mirror bone tree array number
	uint32 theMirrorBoneTreeArrayNumber;
	theMirrorBoneTreePropertyHandleArray->GetNumElements(theMirrorBoneTreeArrayNumber);

	// Always keep the array not zero number
	if (theMirrorBoneTreeArrayNumber == 0)
	{
		theMirrorBoneTreeArrayNumber = 1;
		theMirrorBoneTreePropertyHandleArray->AddItem();
	}

	// Create the delegate
	FSimpleDelegate theRebuildDetailsDelegate = FSimpleDelegate::CreateRaw(this, &FVAT_MirrorDataDetails::OnRebuildDetails);
	theMirrorBoneTreePropertyHandleArray->SetOnNumElementsChanged(theRebuildDetailsDelegate);
	
	// Each every array index
	for (uint32 ArrayIndex = 0; ArrayIndex < theMirrorBoneTreeArrayNumber; ArrayIndex++)
	{
		// Get the mirror bone handle in bone tree
		const TSharedPtr<IPropertyHandle> theMirrorBoneDataHandle = theMirrorBoneTreePropertyHandleArray->GetElement(ArrayIndex);

		// Create the mirror data builder
		const TSharedRef<FVAT_MirrorDataNodeBuilder> theMirrorDataBuilder = MakeShareable(new FVAT_MirrorDataNodeBuilder(DetailLayoutBuilderPtr, theMirrorBoneDataHandle, ArrayIndex));

		// Add the custom builder
		theMirrorBoneTreeCategoryBuilder.AddCustomBuilder(theMirrorDataBuilder);
	}
}

void FVAT_MirrorDataDetails::OnRebuildDetails()
{
	// Check the details builder reference is valid
	if (DetailLayoutBuilderPtr == nullptr)
	{
		return;
	}

	// Force refresh details
	DetailLayoutBuilderPtr->ForceRefreshDetails();
}
#undef LOCTEXT_NAMESPACE
