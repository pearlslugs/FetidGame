// Copyright 2021 Phyronnaz

#include "VoxelFoliageModule.h"
#include "VoxelMinimal.h"
#include "VoxelUtilities/VoxelSystemUtilities.h"

#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"

void FVoxelFoliageModule::StartupModule()
{
	IPlugin& Plugin = FVoxelSystemUtilities::GetPlugin();

	const FString PluginBaseDir = FPaths::ConvertRelativePathToFull(Plugin.GetBaseDir());

#if USE_EMBREE_VOXEL
	const FString EmbreeBase = PluginBaseDir / "Source" / "ThirdParty" / "VoxelEmbree3/";

	bool bSuccess = true;

#if PLATFORM_WINDOWS
	{
		const FString Folder = EmbreeBase / "Win64" / "lib";
		LOG_VOXEL(Log, TEXT("Loading embree3.dll from %s"), *Folder);

		FPlatformProcess::PushDllDirectory(*Folder);
		auto* Handle = FPlatformProcess::GetDllHandle(TEXT("embree3.dll"));
		FPlatformProcess::PopDllDirectory(*Folder);

		bSuccess = Handle != nullptr;
	}
#elif PLATFORM_MAC
	{
		const FString Folder = EmbreeBase / "MacOSX" / "lib";

		const TArray<FString> Libs =
		{
			"libtbb.dylib",
			"libtbbmalloc.dylib",
			"libembree3.3.dylib"
		};

		for (auto& Lib : Libs)
		{
			const FString Path = Folder / Lib;
			LOG_VOXEL(Log, TEXT("Loading %s"), *Path);
			auto* Handle = FPlatformProcess::GetDllHandle(*Path);
			bSuccess &= Handle != nullptr;
		}
	}
#endif

	if (!bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, VOXEL_LOCTEXT("Voxel Plugin: embree not found, voxel foliage won't work."));
	}
#endif
}

IMPLEMENT_MODULE(FVoxelFoliageModule, VoxelFoliage)