// Copyright Ran 001, Inc. All Rights Reserved.

#include "AddVirtualBonePluginsBPLibrary.h"
#include "AddVirtualBonePlugins.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h"
#include "Kismet/KismetStringLibrary.h"

UAddVirtualBonePluginsBPLibrary::UAddVirtualBonePluginsBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float UAddVirtualBonePluginsBPLibrary::AddVirtualBonePluginsSampleFunction(float Param)
{
	return -1;
}

void UAddVirtualBonePluginsBPLibrary::MyAddVirtualBone(USkeletalMesh* SkeletalMesh, const FName SourceBoneName, const FName TargetBoneName)
{
	USkeleton* Skeleton = SkeletalMesh->Skeleton;

	int32 BoneTreeTestA;
	int32 BoneTreeTestB;
	BoneTreeTestB = SkeletalMesh->RefSkeleton.FindBoneIndex(SourceBoneName);
	BoneTreeTestA = SkeletalMesh->RefSkeleton.FindBoneIndex(TargetBoneName);

	if (BoneTreeTestB != -1 && BoneTreeTestA != -1)
	{
		Skeleton->AddNewVirtualBone(SourceBoneName, TargetBoneName);
	}
	else if (BoneTreeTestB == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("SourceBoneName is not find: %s"), *SourceBoneName.ToString());
	}
	else if (BoneTreeTestA == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetBoneName is not find: %s"), *TargetBoneName.ToString());
	}
}

void UAddVirtualBonePluginsBPLibrary::MyRenameVirtualBone(USkeletalMesh* SkeletalMesh, FName OriginalName, const FName InVBName)
{
	USkeleton* Skeleton = SkeletalMesh->Skeleton;
	Skeleton->RenameVirtualBone(OriginalName, InVBName);
}

FString UAddVirtualBonePluginsBPLibrary::MyGetPathNameForLoadedAsset(UObject* LoadedAsset)
{
	return LoadedAsset->GetPathName();
}

void UAddVirtualBonePluginsBPLibrary::MyDeleteVirtualBone(USkeletalMesh* SkeletalMesh)
{
	USkeleton* Skeleton = SkeletalMesh->Skeleton;
	TArray<FName> BonesToRemove;
	FName BoneName = "TEST";

	for (int32 i = -300; i < 300; ++i)
	{
		BoneName = "TEST";
		bool IsValidBoneInS = SkeletalMesh->RefSkeleton.IsValidIndex(i);

		if (IsValidBoneInS)
		{
			BoneName = SkeletalMesh->RefSkeleton.GetBoneName(i);
			FString BoneNameS = BoneName.ToString();
			bool testBool = UKismetStringLibrary::Contains(BoneNameS, "VB");

			if (testBool)
			{
				BonesToRemove.Add(BoneName);
			}
		}
	}
	Skeleton->RemoveVirtualBones(BonesToRemove);
}