/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - DeepSync Manager Actor

   Place in level to expose DeepSync subsystem events in Blueprint
   with easy event binding via Details panel (+) buttons.

   USAGE:
   1. Place AAefDeepSyncManager in your level
   2. Select in editor → Details panel
   3. Click (+) next to events to bind them in Blueprint
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AefDeepSyncTypes.h"
#include "AefPharusSyncTypes.h"
#include "AefDeepSyncSubsystem.h"
#include "AefDeepSyncManager.generated.h"

class AAefPharusDeepSyncZoneActor;

/**
 * DeepSync Manager Actor
 *
 * Exposes UAefDeepSyncSubsystem events as Blueprint-assignable delegates.
 * Place one in your level to easily bind sync events in the Details panel.
 */
UCLASS(Blueprintable, meta = (DisplayName = "AEF DeepSync Manager"))
class AEFDEEPSYNC_API AAefDeepSyncManager : public AActor
{
	GENERATED_BODY()

public:
	AAefDeepSyncManager();

	//--------------------------------------------------------------------------------
	// Wearable Events (from Subsystem)
	//--------------------------------------------------------------------------------

	/** Fired when a new wearable connects */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableConnected OnWearableConnected;

	/** Fired when a wearable disconnects (timeout) */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableLost OnWearableLost;

	/** Fired when wearable data is updated */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableUpdated OnWearableUpdated;

	/** Fired when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnConnectionStatusChanged OnConnectionStatusChanged;

	//--------------------------------------------------------------------------------
	// Sync Link Events (from Subsystem)
	//--------------------------------------------------------------------------------

	/** Fired when a sync link is established (Pharus ↔ Wearable) */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync Events")
	FAefOnLinkEstablished OnLinkEstablished;

	/** Fired when a sync link is broken */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync Events")
	FAefOnLinkBroken OnLinkBroken;

	/** Fired when a zone is registered with the subsystem */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync Events")
	FAefOnZoneRegistered OnZoneRegistered;

	/** Fired when a zone is unregistered */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Sync Events")
	FAefOnZoneUnregistered OnZoneUnregistered;

	//--------------------------------------------------------------------------------
	// Blueprint Callable Functions
	//--------------------------------------------------------------------------------

	/** Get the DeepSync subsystem */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	UAefDeepSyncSubsystem* GetDeepSyncSubsystem() const;

	/** Get all active wearables */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Wearables")
	TArray<FAefDeepSyncWearableData> GetActiveWearables() const;

	/** Get all active sync links */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Links")
	TArray<FAefSyncedLink> GetAllSyncedLinks() const;

	/** Get Pharus actor by wearable ID */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Links")
	AActor* GetPharusActorByWearableId(int32 WearableId) const;

	/** Send color command to wearable */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Commands")
	bool SendColorCommand(int32 WearableId, FLinearColor Color);

	/** Disconnect a sync link */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Links")
	bool DisconnectLink(int32 WearableId);

	/** Disconnect all sync links */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Links")
	void DisconnectAllLinks();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Cached subsystem reference */
	UPROPERTY()
	TWeakObjectPtr<UAefDeepSyncSubsystem> CachedSubsystem;

	/** Bind to subsystem events */
	void BindSubsystemEvents();
	void UnbindSubsystemEvents();

	//--------------------------------------------------------------------------------
	// Internal Event Handlers (forward to our delegates)
	//--------------------------------------------------------------------------------

	UFUNCTION()
	void HandleWearableConnected(const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleWearableLost(const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleWearableUpdated(int32 WearableId, const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleConnectionStatusChanged(EAefDeepSyncConnectionStatus Status);

	UFUNCTION()
	void HandleLinkEstablished(const FAefSyncedLink& Link);

	UFUNCTION()
	void HandleLinkBroken(const FAefSyncedLink& Link, const FString& Reason);

	UFUNCTION()
	void HandleZoneRegistered(AAefPharusDeepSyncZoneActor* Zone);

	UFUNCTION()
	void HandleZoneUnregistered(AAefPharusDeepSyncZoneActor* Zone);
};
