// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

#include "VAT_AnimNotifyInstance.h"

//////////////////////////////////////////////////////////////////////////
// UVAT_AnimNotifyInstance
//////////////////////////////////////////////////////////////////////////

UVAT_AnimNotifyInstance::UVAT_AnimNotifyInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UVAT_AnimNotifyInstance::TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent)
{
	InNotifyEvent.TriggerWeightThreshold = NotifyEventData.TriggerWeightThreshold;
	InNotifyEvent.MontageTickType = NotifyEventData.MontageTickType;
	InNotifyEvent.NotifyTriggerChance = NotifyEventData.NotifyTriggerChance;
	InNotifyEvent.NotifyFilterLOD = NotifyEventData.NotifyFilterLOD;
	InNotifyEvent.bTriggerOnFollower = NotifyEventData.bTriggerOnFollower;
	InNotifyEvent.bTriggerOnDedicatedServer = NotifyEventData.bTriggerOnDedicatedServer;
	InNotifyEvent.NotifyFilterType = NotifyEventData.NotifyFilterType;
}

//////////////////////////////////////////////////////////////////////////
// UVAT_AnimNotifyStateInstance
//////////////////////////////////////////////////////////////////////////

UVAT_AnimNotifyStateInstance::UVAT_AnimNotifyStateInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UVAT_AnimNotifyStateInstance::TransferNotifyEventDataToInstance(FAnimNotifyEvent& InNotifyEvent)
{
	InNotifyEvent.TriggerWeightThreshold = NotifyEventData.TriggerWeightThreshold;
	InNotifyEvent.MontageTickType = NotifyEventData.MontageTickType;
	InNotifyEvent.NotifyTriggerChance = NotifyEventData.NotifyTriggerChance;
	InNotifyEvent.NotifyFilterLOD = NotifyEventData.NotifyFilterLOD;
	InNotifyEvent.bTriggerOnFollower = NotifyEventData.bTriggerOnFollower;
	InNotifyEvent.bTriggerOnDedicatedServer = NotifyEventData.bTriggerOnDedicatedServer;
	InNotifyEvent.NotifyFilterType = NotifyEventData.NotifyFilterType;
}
