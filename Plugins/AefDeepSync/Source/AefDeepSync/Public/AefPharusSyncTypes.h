/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Pharus Sync Types

   Data structures for Pharus-DeepSync synchronization.
   Enables linking Pharus TrackIDs to DeepSync WearableIds.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "AefDeepSyncTypes.h"
#include "AefPharusSyncTypes.generated.h"

//--------------------------------------------------------------------------------
// ENUMS
//--------------------------------------------------------------------------------

/**
 * Sync operation status
 */
UENUM(BlueprintType)
enum class EAefPharusSyncStatus : uint8
{
	/** No sync in progress */
	Idle		UMETA(DisplayName = "Idle"),
	
	/** Sync timer running, person in zone */
	Syncing		UMETA(DisplayName = "Syncing"),
	
	/** Sync completed successfully */
	Success		UMETA(DisplayName = "Success"),
	
	/** Sync failed (wearable not found, etc.) */
	Failed		UMETA(DisplayName = "Failed"),
	
	/** Sync timed out */
	Timeout		UMETA(DisplayName = "Timeout")
};

//--------------------------------------------------------------------------------
// DATA STRUCTURES
//--------------------------------------------------------------------------------

/**
 * Result of a Pharus-DeepSync synchronization
 * 
 * Contains all data from both systems after successful sync.
 * Used in OnSyncCompleted event for Blueprint processing.
 */
USTRUCT(BlueprintType)
struct AEFDEEPSYNC_API FAefPharusSyncResult
{
	GENERATED_BODY()

	//--------------------------------------------------------------------------------
	// Status
	//--------------------------------------------------------------------------------

	/** Was the sync successful? */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync")
	bool bSuccess = false;

	/** Current sync status */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync")
	EAefPharusSyncStatus Status = EAefPharusSyncStatus::Idle;

	/** Error message if sync failed */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync")
	FString ErrorMessage;

	//--------------------------------------------------------------------------------
	// Pharus Data
	//--------------------------------------------------------------------------------

	/** The Pharus track ID that was synced */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Pharus")
	int32 PharusTrackID = -1;

	/** World position of the Pharus track at sync completion */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Pharus")
	FVector PharusPosition = FVector::ZeroVector;

	//--------------------------------------------------------------------------------
	// Wearable Data
	//--------------------------------------------------------------------------------

	/** The WearableId that was synced */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Wearable")
	int32 WearableId = -1;

	/** Heart rate at sync completion */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Wearable")
	int32 HeartRate = 0;

	/** Current LED color of wearable */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Wearable")
	FLinearColor WearableColor = FLinearColor::Black;

	/** Full wearable data snapshot */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Wearable")
	FAefDeepSyncWearableData WearableData;

	//--------------------------------------------------------------------------------
	// Sync Zone Data
	//--------------------------------------------------------------------------------

	/** Color of the sync zone */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Zone")
	FLinearColor ZoneColor = FLinearColor::Green;

	//--------------------------------------------------------------------------------
	// Timing
	//--------------------------------------------------------------------------------

	/** How long the sync took (seconds) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Timing")
	float SyncDuration = 0.0f;

	/** World time when sync completed */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Timing")
	FDateTime SyncCompletedTime;

	//--------------------------------------------------------------------------------
	// Helpers
	//--------------------------------------------------------------------------------

	static FAefPharusSyncResult MakeSuccess(int32 InTrackID, int32 InWearableId, const FAefDeepSyncWearableData& InWearableData, FLinearColor InZoneColor, float InDuration)
	{
		FAefPharusSyncResult Result;
		Result.bSuccess = true;
		Result.Status = EAefPharusSyncStatus::Success;
		Result.PharusTrackID = InTrackID;
		Result.WearableId = InWearableId;
		Result.WearableData = InWearableData;
		Result.HeartRate = InWearableData.HeartRate;
		Result.WearableColor = InWearableData.Color;
		Result.ZoneColor = InZoneColor;
		Result.SyncDuration = InDuration;
		Result.SyncCompletedTime = FDateTime::Now();
		return Result;
	}

	static FAefPharusSyncResult MakeFailure(EAefPharusSyncStatus InStatus, const FString& InError, int32 InTrackID = -1, int32 InWearableId = -1)
	{
		FAefPharusSyncResult Result;
		Result.bSuccess = false;
		Result.Status = InStatus;
		Result.ErrorMessage = InError;
		Result.PharusTrackID = InTrackID;
		Result.WearableId = InWearableId;
		return Result;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("SyncResult[%s, TrackID=%d, WearableId=%d, HR=%d]"),
			bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"),
			PharusTrackID, WearableId, HeartRate);
	}
};

//--------------------------------------------------------------------------------
// DELEGATES
//--------------------------------------------------------------------------------

/** Fired when sync starts (person enters zone) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnSyncStarted, int32, PharusTrackID);

/** Fired every tick during sync (for progress UI) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnSyncing, int32, PharusTrackID, float, Progress);

/** Fired when sync completes (success or failure) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnSyncCompleted, const FAefPharusSyncResult&, Result);

/** Fired when sync is cancelled (person leaves zone early) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnSyncCancelled, int32, PharusTrackID);

/** Fired when wearable connection is lost during sync */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnSyncWearableLost, int32, WearableId);

/** Fired when Pharus track is lost during sync */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnSyncPharusTrackLost, int32, PharusTrackID);

//--------------------------------------------------------------------------------
// SYNCED LINK - Active Connection Between Pharus Track and Wearable
//--------------------------------------------------------------------------------

// Forward declaration
class AAefPharusDeepSyncZoneActor;

/**
 * Represents an active link between a Pharus Track and a DeepSync Wearable
 * 
 * Created when sync completes successfully. Objects in a link are BLOCKED
 * from other syncs until the link is broken (lost/disconnect).
 */
USTRUCT(BlueprintType)
struct AEFDEEPSYNC_API FAefSyncedLink
{
	GENERATED_BODY()

	/** Unique link identifier */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	int32 LinkId = -1;

	/** The sync zone that created this link */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	TWeakObjectPtr<AAefPharusDeepSyncZoneActor> Zone;

	/** The Pharus Track ID (blocked for other syncs) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	int32 PharusTrackID = -1;

	/** The Wearable ID (blocked for other syncs) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	int32 WearableId = -1;

	/** Reference to the Pharus actor */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	TWeakObjectPtr<AActor> PharusActor;

	/** Zone color at time of sync */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	FLinearColor ZoneColor = FLinearColor::Green;

	/** When the sync was established */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Sync|Link")
	FDateTime SyncTime;

	/** Is this link still valid? */
	bool IsValid() const
	{
		return LinkId >= 0 && PharusTrackID >= 0 && WearableId >= 0;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("Link[%d: Track=%d <-> Wearable=%d]"), 
			LinkId, PharusTrackID, WearableId);
	}
};

/** Fired when a new link is established (sync completed) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnLinkEstablished, const FAefSyncedLink&, Link);

/** Fired when a link is broken (lost/manual disconnect) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnLinkBroken, const FAefSyncedLink&, Link, const FString&, Reason);

/** Fired when a zone is registered with the subsystem */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnZoneRegistered, AAefPharusDeepSyncZoneActor*, Zone);

/** Fired when a zone is unregistered from the subsystem */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnZoneUnregistered, AAefPharusDeepSyncZoneActor*, Zone);

