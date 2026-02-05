/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Wearable Subsystem

   Main entry point for the DeepSync wearable tracking system.
   Manages TCP connections to the deepsyncwearablev2-server.

   USAGE:
   1. Configure [DeepSync] section in MozConfig.ini
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
#include "AefDeepSyncSubsystem.generated.h"

class FSocket;

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

	/** Start DeepSync connection (uses config from MozConfig.ini) */
	UFUNCTION(BlueprintCallable, Category = "AEF")
	void StartDeepSync();

	/** Stop DeepSync connection and clear all wearables */
	UFUNCTION(BlueprintCallable, Category = "AEF")
	void StopDeepSync();

	/** Get current connection status */
	UFUNCTION(BlueprintPure, Category = "AEF")
	EAefDeepSyncConnectionStatus GetConnectionStatus() const { return ConnectionStatus; }

	/** Check if currently connected */
	UFUNCTION(BlueprintPure, Category = "AEF")
	bool IsConnected() const { return ConnectionStatus == EAefDeepSyncConnectionStatus::Connected; }

	/** Check if DeepSync is running (connected or reconnecting) */
	UFUNCTION(BlueprintPure, Category = "AEF")
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

	/** Reload configuration from MozConfig.ini */
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
};
