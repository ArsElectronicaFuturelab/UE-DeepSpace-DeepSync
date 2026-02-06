/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Wearable Subsystem

   Main entry point for the DeepSync wearable tracking system.
   Manages TCP connections to the deepsyncwearablev2-server.

   USAGE:
   1. Configure [DeepSync] section in AefConfig.ini
   2. Set autoStart=true OR call StartDeepSync() manually
   3. Bind to OnWearableConnected/OnWearableLost events
   4. Use GetActiveWearables() to access current data

   See DOCUMENTATION.md for detailed API reference.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AefDeepSyncTypes.h"
#include "AefPharusSyncTypes.h"
#include "AefDeepSyncSubsystem.generated.h"

class FSocket;
class AAefPharusDeepSyncZoneActor;

//--------------------------------------------------------------------------------
// Delegates
//--------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnWearableConnected, const FAefDeepSyncWearableData&, WearableData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnWearableLost, const FAefDeepSyncWearableData&, WearableData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnWearableUpdated, int32, WearableId, const FAefDeepSyncWearableData&, WearableData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnConnectionStatusChanged, EAefDeepSyncConnectionStatus, Status);

/**
 * DeepSync Wearable Subsystem
 *
 * GameInstance subsystem for managing wearable device connections.
 * Persists across level transitions.
 */
UCLASS()
class AEFDEEPSYNC_API UAefDeepSyncSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//--------------------------------------------------------------------------------
	// USubsystem Interface
	//--------------------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//--------------------------------------------------------------------------------
	// FTickableGameObject Interface
	//--------------------------------------------------------------------------------

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	//--------------------------------------------------------------------------------
	// Connection Management
	//--------------------------------------------------------------------------------

	/** Start DeepSync connection (uses config from AefConfig.ini) */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync")
	void StartDeepSync();

	/** Stop DeepSync connection and clear all wearables */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync")
	void StopDeepSync();

	/** Get current connection status */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	EAefDeepSyncConnectionStatus GetConnectionStatus() const { return ConnectionStatus; }

	/** Check if currently connected */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	bool IsConnected() const { return ConnectionStatus == EAefDeepSyncConnectionStatus::Connected; }

	/** Check if DeepSync is running (connected or reconnecting) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	bool IsRunning() const;

	//--------------------------------------------------------------------------------
	// Wearable Access
	//--------------------------------------------------------------------------------

	/** Get all active wearables */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Wearables")
	TArray<FAefDeepSyncWearableData> GetActiveWearables() const;

	/** Get specific wearable by ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Wearables")
	bool GetWearableById(int32 WearableId, FAefDeepSyncWearableData& OutData) const;

	/** Get active wearable count */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Wearables")
	int32 GetActiveWearableCount() const { return ActiveWearables.Num(); }

	/** Check if wearable is active */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Wearables")
	bool IsWearableActive(int32 WearableId) const { return ActiveWearables.Contains(WearableId); }

	//--------------------------------------------------------------------------------
	// Commands
	//--------------------------------------------------------------------------------

	/** Send color command to wearable (FLinearColor - recommended for Blueprints) */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Commands", meta = (DisplayName = "Send Color Command"))
	bool SendColorCommandLinear(int32 WearableId, FLinearColor InColor);

	/** Send color command to wearable (internal struct) */
	bool SendColorCommand(int32 WearableId, FAefDeepSyncColor InColor);

	/** Send ID change command to wearable */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Commands", meta = (DisplayName = "Send ID Command"))
	bool SendIdCommand(int32 WearableId, int32 NewId);

	//--------------------------------------------------------------------------------
	// Events
	//--------------------------------------------------------------------------------

	/** Fired when new wearable connects */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableConnected OnWearableConnected;

	/** Fired when wearable times out */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableLost OnWearableLost;

	/** Fired on each data update (high frequency!) */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableUpdated OnWearableUpdated;

	/** Fired when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnConnectionStatusChanged OnConnectionStatusChanged;

	//--------------------------------------------------------------------------------
	// Configuration
	//--------------------------------------------------------------------------------

	/** Get current configuration */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Config")
	FAefDeepSyncConfig GetConfig() const { return Config; }

	/** Reload configuration from AefConfig.ini */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Config")
	void ReloadConfiguration();

private:
	//--------------------------------------------------------------------------------
	// Configuration
	//--------------------------------------------------------------------------------

	FAefDeepSyncConfig Config;
	void LoadConfiguration();

	//--------------------------------------------------------------------------------
	// TCP Connection
	//--------------------------------------------------------------------------------

	FSocket* ReceiverSocket = nullptr;
	FSocket* SenderSocket = nullptr;
	FString ReceiveBuffer;

	EAefDeepSyncConnectionStatus ConnectionStatus = EAefDeepSyncConnectionStatus::Disconnected;
	float ReconnectTimer = 0.0f;
	float CurrentReconnectDelay = 2.0f;
	int32 ReconnectAttempts = 0;
	bool bWantsToRun = false;

	static constexpr float MaxReconnectDelay = 60.0f;

	bool ConnectToServer();
	void DisconnectFromServer();
	void ProcessReceivedData();
	bool ParseWearableMessage(const FString& JsonMessage, FAefDeepSyncWearableData& OutData);
	void SetConnectionStatus(EAefDeepSyncConnectionStatus NewStatus);

	//--------------------------------------------------------------------------------
	// Wearable Management
	//--------------------------------------------------------------------------------

	TMap<int32, FAefDeepSyncWearableData> ActiveWearables;
	int32 NextUniqueId = 0;

	void UpdateWearable(const FAefDeepSyncWearableData& Data);
	void CheckWearableTimeouts(float DeltaTime);
	bool IsWearableIdAllowed(int32 WearableId) const;

	//--------------------------------------------------------------------------------
	// Pharus Sync Zone Management (Internal)
	//--------------------------------------------------------------------------------

	TArray<TWeakObjectPtr<AAefPharusDeepSyncZoneActor>> RegisteredZones;
	TArray<FAefSyncedLink> SyncedLinks;
	int32 NextLinkId = 0;

	void CheckForBrokenLinks();
	void BreakLinkInternal(int32 Index, const FString& Reason);

public:
	//--------------------------------------------------------------------------------
	// Zone Management
	//--------------------------------------------------------------------------------

	/** Register a sync zone with the subsystem (called by zone on BeginPlay) */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Sync|Zones")
	void RegisterZone(AAefPharusDeepSyncZoneActor* Zone);

	/** Unregister a sync zone (called by zone on EndPlay) */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Sync|Zones")
	void UnregisterZone(AAefPharusDeepSyncZoneActor* Zone);

	/** Get all registered sync zones */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Zones")
	TArray<AAefPharusDeepSyncZoneActor*> GetAllZones() const;

	/** Get zone by wearable ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Zones")
	AAefPharusDeepSyncZoneActor* GetZoneByWearableId(int32 InWearableId) const;

	//--------------------------------------------------------------------------------
	// Sync Link Management
	//--------------------------------------------------------------------------------

	/** Called by zone when sync completes - creates a new link */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Sync|Links")
	void NotifySyncCompleted(const FAefPharusSyncResult& Result, AAefPharusDeepSyncZoneActor* Zone, AActor* PharusActor);

	/** Get all active synced links */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Links")
	TArray<FAefSyncedLink> GetAllSyncedLinks() const { return SyncedLinks; }

	/** Get link by wearable ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Links")
	bool GetLinkByWearableId(int32 InWearableId, FAefSyncedLink& OutLink) const;

	/** Get link by Pharus track ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Links")
	bool GetLinkByPharusTrackId(int32 TrackID, FAefSyncedLink& OutLink) const;

	/** Get Pharus actor by wearable ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Links")
	AActor* GetPharusActorByWearableId(int32 InWearableId) const;

	//--------------------------------------------------------------------------------
	// Blocking Checks
	//--------------------------------------------------------------------------------

	/** Check if zone is blocked (has active link) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Blocking")
	bool IsZoneBlocked(AAefPharusDeepSyncZoneActor* Zone) const;

	/** Check if Pharus track is blocked (linked to a wearable) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Blocking")
	bool IsPharusTrackBlocked(int32 TrackID) const;

	/** Check if wearable is blocked (linked to a Pharus track) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Sync|Blocking")
	bool IsWearableBlocked(int32 InWearableId) const;

	//--------------------------------------------------------------------------------
	// Manual Disconnect
	//--------------------------------------------------------------------------------

	/** Manually disconnect a link by wearable ID */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Sync|Links")
	bool DisconnectLink(int32 InWearableId);

	/** Disconnect all active links */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Sync|Links")
	void DisconnectAllLinks();

	//--------------------------------------------------------------------------------
	// Sync Link Events
	//--------------------------------------------------------------------------------

	/** Fired when a new link is established */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync|Events")
	FAefOnLinkEstablished OnLinkEstablished;

	/** Fired when a link is broken */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync|Events")
	FAefOnLinkBroken OnLinkBroken;

	/** Fired when a zone is registered */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync|Events")
	FAefOnZoneRegistered OnZoneRegistered;

	/** Fired when a zone is unregistered */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync|Events")
	FAefOnZoneUnregistered OnZoneUnregistered;
};

