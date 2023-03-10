// Copyright 2021 Phyronnaz

#include "Factories/VoxelDataAssetFromRawVoxFactory.h"
#include "VoxelAssets/VoxelDataAsset.h"
#include "VoxelAssets/VoxelDataAssetData.h"
#include "Importers/RawVox.h"
#include "VoxelMessages.h"

#include "Misc/Paths.h"

UVoxelDataAssetFromRawVoxFactory::UVoxelDataAssetFromRawVoxFactory()
{
	bEditorImport = true;
	SupportedClass = UVoxelDataAsset::StaticClass();
	Formats.Add(TEXT("rawvox;3D Coat RawVox"));
}

bool UVoxelDataAssetFromRawVoxFactory::FactoryCanImport(const FString& Filename)
{
	return FPaths::GetExtension(Filename) == TEXT("rawvox");
}

UObject* UVoxelDataAssetFromRawVoxFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	auto NewDataAsset = NewObject<UVoxelDataAsset>(InParent, InName, Flags | RF_Transactional);
	const auto Data = MakeVoxelShared<FVoxelDataAssetData>();
	if (RawVox::ImportToAsset(Filename, *Data))
	{
		NewDataAsset->Source = EVoxelDataAssetImportSource::RawVox;
		NewDataAsset->Paths = { Filename };
		NewDataAsset->SetData(Data);
		return NewDataAsset;
	}
	else
	{
		return nullptr;
	}
}

bool UVoxelDataAssetFromRawVoxFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if (auto* Asset = Cast<UVoxelDataAsset>(Obj))
	{
		if (Asset->Source == EVoxelDataAssetImportSource::RawVox)
		{
			OutFilenames = Asset->Paths;
			return true;
		}
	}
	return false;
}

void UVoxelDataAssetFromRawVoxFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	if (auto* Asset = Cast<UVoxelDataAsset>(Obj))
	{
		Asset->Paths = NewReimportPaths;
	}
}

EReimportResult::Type UVoxelDataAssetFromRawVoxFactory::Reimport(UObject* Obj)
{
	if (auto* Asset = Cast<UVoxelDataAsset>(Obj))
	{
		const auto Data = MakeVoxelShared<FVoxelDataAssetData>();
		if (RawVox::ImportToAsset(Asset->Paths[0], *Data))
		{
			Asset->SetData(Data);
			return EReimportResult::Succeeded;
		}
	}
	return EReimportResult::Failed;
}

int32 UVoxelDataAssetFromRawVoxFactory::GetPriority() const
{
	return ImportPriority;
}