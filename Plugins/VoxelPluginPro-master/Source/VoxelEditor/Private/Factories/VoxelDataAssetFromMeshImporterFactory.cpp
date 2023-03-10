// Copyright 2021 Phyronnaz

#include "Factories/VoxelDataAssetFromMeshImporterFactory.h"
#include "VoxelAssets/VoxelDataAsset.h"
#include "VoxelAssets/VoxelDataAssetData.h"
#include "VoxelFeedbackContext.h"
#include "VoxelImporters/VoxelMeshImporter.h"
#include "VoxelMessages.h"

#include "Engine/StaticMesh.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

UVoxelDataAssetFromMeshImporterFactory::UVoxelDataAssetFromMeshImporterFactory()
{
	bEditAfterNew = true;
	bEditorImport = true;
	SupportedClass = UVoxelDataAsset::StaticClass();
}

UObject* UVoxelDataAssetFromMeshImporterFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	FVoxelScopedSlowTask Progress(2, VOXEL_LOCTEXT("Importing from mesh"));
	Progress.MakeDialog(true);

	auto* NewDataAsset = NewObject<UVoxelDataAsset>(InParent, Class, Name, Flags | RF_Transactional);
	int32 NumLeaks = 0;
	Progress.EnterProgressFrame();
	const auto Data = MakeVoxelShared<FVoxelDataAssetData>();

	FVoxelMeshImporterInputData InputData;
	UVoxelMeshImporterLibrary::CreateMeshDataFromStaticMesh(MeshImporter->StaticMesh, InputData);
	FTransform Transform = MeshImporter->GetTransform();
	Transform.SetTranslation(FVector::ZeroVector);
	FVoxelMeshImporterRenderTargetCache Cache;
	if (UVoxelMeshImporterLibrary::ConvertMeshToVoxels(MeshImporter, InputData, Transform, MeshImporter->Settings, Cache, *Data, NewDataAsset->PositionOffset, NumLeaks))
	{
		Progress.EnterProgressFrame();
		NewDataAsset->Source = EVoxelDataAssetImportSource::Mesh;
		NewDataAsset->SetData(Data);
		if (NumLeaks > 0)
		{
			FNotificationInfo Info = FNotificationInfo(FText::Format(
				VOXEL_LOCTEXT("{0} leaks in the mesh (or bug in the voxelizer)"), 
				FText::AsNumber(NumLeaks)));
			Info.ExpireDuration = 10;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		return NewDataAsset;
	}
	else
	{
		Progress.EnterProgressFrame();
		return nullptr;
	}
}

FString UVoxelDataAssetFromMeshImporterFactory::GetDefaultNewAssetName() const
{
	return MeshImporter->StaticMesh->GetName();
}