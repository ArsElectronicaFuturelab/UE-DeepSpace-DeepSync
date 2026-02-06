/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - DeepSync Manager Actor Implementation
========================================================================*/

#include "AefDeepSyncManager.h"
#include "AefDeepSyncSubsystem.h"
#include "AefPharusDeepSyncZoneActor.h"
#include "Kismet/GameplayStatics.h"

AAefDeepSyncManager::AAefDeepSyncManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAefDeepSyncManager::BeginPlay()
{
	Super::BeginPlay();
	BindSubsystemEvents();
}

void AAefDeepSyncManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSubsystemEvents();
	Super::EndPlay(EndPlayReason);
}

UAefDeepSyncSubsystem* AAefDeepSyncManager::GetDeepSyncSubsystem() const
{
	if (CachedSubsystem.IsValid())
	{
		return CachedSubsystem.Get();
	}

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		return GI->GetSubsystem<UAefDeepSyncSubsystem>();
	}
	return nullptr;
}

TArray<FAefDeepSyncWearableData> AAefDeepSyncManager::GetActiveWearables() const
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		return Subsystem->GetActiveWearables();
	}
	return TArray<FAefDeepSyncWearableData>();
}

TArray<FAefSyncedLink> AAefDeepSyncManager::GetAllSyncedLinks() const
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		return Subsystem->GetAllSyncedLinks();
	}
	return TArray<FAefSyncedLink>();
}

AActor* AAefDeepSyncManager::GetPharusActorByWearableId(int32 WearableId) const
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		return Subsystem->GetPharusActorByWearableId(WearableId);
	}
	return nullptr;
}

bool AAefDeepSyncManager::SendColorCommand(int32 WearableId, FLinearColor Color)
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		return Subsystem->SendColorCommandLinear(WearableId, Color);
	}
	return false;
}

bool AAefDeepSyncManager::DisconnectLink(int32 WearableId)
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		return Subsystem->DisconnectLink(WearableId);
	}
	return false;
}

void AAefDeepSyncManager::DisconnectAllLinks()
{
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		Subsystem->DisconnectAllLinks();
	}
}

void AAefDeepSyncManager::BindSubsystemEvents()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("AefDeepSyncManager: No GameInstance available"));
		return;
	}

	UAefDeepSyncSubsystem* Subsystem = GI->GetSubsystem<UAefDeepSyncSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AefDeepSyncManager: DeepSync subsystem not available"));
		return;
	}

	CachedSubsystem = Subsystem;

	// Bind wearable events
	Subsystem->OnWearableConnected.AddDynamic(this, &AAefDeepSyncManager::HandleWearableConnected);
	Subsystem->OnWearableLost.AddDynamic(this, &AAefDeepSyncManager::HandleWearableLost);
	Subsystem->OnWearableUpdated.AddDynamic(this, &AAefDeepSyncManager::HandleWearableUpdated);
	Subsystem->OnConnectionStatusChanged.AddDynamic(this, &AAefDeepSyncManager::HandleConnectionStatusChanged);

	// Bind sync link events
	Subsystem->OnLinkEstablished.AddDynamic(this, &AAefDeepSyncManager::HandleLinkEstablished);
	Subsystem->OnLinkBroken.AddDynamic(this, &AAefDeepSyncManager::HandleLinkBroken);
	Subsystem->OnZoneRegistered.AddDynamic(this, &AAefDeepSyncManager::HandleZoneRegistered);
	Subsystem->OnZoneUnregistered.AddDynamic(this, &AAefDeepSyncManager::HandleZoneUnregistered);

	UE_LOG(LogTemp, Log, TEXT("AefDeepSyncManager: Bound to DeepSync subsystem events"));
}

void AAefDeepSyncManager::UnbindSubsystemEvents()
{
	if (!CachedSubsystem.IsValid())
	{
		return;
	}

	UAefDeepSyncSubsystem* Subsystem = CachedSubsystem.Get();

	// Unbind wearable events
	Subsystem->OnWearableConnected.RemoveDynamic(this, &AAefDeepSyncManager::HandleWearableConnected);
	Subsystem->OnWearableLost.RemoveDynamic(this, &AAefDeepSyncManager::HandleWearableLost);
	Subsystem->OnWearableUpdated.RemoveDynamic(this, &AAefDeepSyncManager::HandleWearableUpdated);
	Subsystem->OnConnectionStatusChanged.RemoveDynamic(this, &AAefDeepSyncManager::HandleConnectionStatusChanged);

	// Unbind sync link events
	Subsystem->OnLinkEstablished.RemoveDynamic(this, &AAefDeepSyncManager::HandleLinkEstablished);
	Subsystem->OnLinkBroken.RemoveDynamic(this, &AAefDeepSyncManager::HandleLinkBroken);
	Subsystem->OnZoneRegistered.RemoveDynamic(this, &AAefDeepSyncManager::HandleZoneRegistered);
	Subsystem->OnZoneUnregistered.RemoveDynamic(this, &AAefDeepSyncManager::HandleZoneUnregistered);

	CachedSubsystem.Reset();
}

//--------------------------------------------------------------------------------
// Event Handlers - Forward to our delegates
//--------------------------------------------------------------------------------

void AAefDeepSyncManager::HandleWearableConnected(const FAefDeepSyncWearableData& Data)
{
	OnWearableConnected.Broadcast(Data);
}

void AAefDeepSyncManager::HandleWearableLost(const FAefDeepSyncWearableData& Data)
{
	OnWearableLost.Broadcast(Data);
}

void AAefDeepSyncManager::HandleWearableUpdated(int32 WearableId, const FAefDeepSyncWearableData& Data)
{
	OnWearableUpdated.Broadcast(WearableId, Data);
}

void AAefDeepSyncManager::HandleConnectionStatusChanged(EAefDeepSyncConnectionStatus Status)
{
	OnConnectionStatusChanged.Broadcast(Status);
}

void AAefDeepSyncManager::HandleLinkEstablished(const FAefSyncedLink& Link)
{
	OnLinkEstablished.Broadcast(Link);
}

void AAefDeepSyncManager::HandleLinkBroken(const FAefSyncedLink& Link, const FString& Reason)
{
	OnLinkBroken.Broadcast(Link, Reason);
}

void AAefDeepSyncManager::HandleZoneRegistered(AAefPharusDeepSyncZoneActor* Zone)
{
	OnZoneRegistered.Broadcast(Zone);
}

void AAefDeepSyncManager::HandleZoneUnregistered(AAefPharusDeepSyncZoneActor* Zone)
{
	OnZoneUnregistered.Broadcast(Zone);
}
