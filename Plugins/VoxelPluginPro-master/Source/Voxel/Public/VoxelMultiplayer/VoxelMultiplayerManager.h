// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelTickable.h"
#include "VoxelSubsystem.h"
#include "VoxelMultiplayerManager.generated.h"

class IVoxelMultiplayerClient;
class IVoxelMultiplayerServer;

DECLARE_MULTICAST_DELEGATE(FVoxelMultiplayerManagerOnClientConnection);

UCLASS()
class VOXEL_API UVoxelMultiplayerSubsystemProxy : public UVoxelStaticSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelMultiplayerManager);

	virtual bool ShouldCreateSubsystem(const FVoxelRuntime& Runtime, const FVoxelRuntimeSettings& Settings) const override;
};

class VOXEL_API FVoxelMultiplayerManager : public IVoxelSubsystem, public FVoxelTickable
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelMultiplayerSubsystemProxy);
	
	FVoxelMultiplayerManagerOnClientConnection OnClientConnection;
	
	//~ Begin IVoxelSubsystem Interface
	virtual void Create() override;
	virtual void Destroy() override;
	//~ End IVoxelSubsystem Interface
	
	//~ Begin FVoxelTickable Interface
	virtual void Tick(float DeltaTime) override;
	//~ End FVoxelTickable Interface

private:
	double LastSyncTime = 0;

	TVoxelSharedPtr<IVoxelMultiplayerServer> Server;
	TVoxelSharedPtr<IVoxelMultiplayerClient> Client;
	
	void ReceiveData() const;
	void SendData() const;
	void OnConnection();
};
