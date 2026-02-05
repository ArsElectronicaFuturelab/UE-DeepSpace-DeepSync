/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Data Types and Structures

   This file defines all core data structures for the DeepSync wearable system:
   - FAefDeepSyncColor: RGB color for wearable LED
   - FAefDeepSyncWearableData: Complete wearable state
   - EAefDeepSyncConnectionStatus: TCP connection state
   - FAefDeepSyncConfig: Runtime configuration (Blueprint-ready)

   See DOCUMENTATION.md for detailed usage examples.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "AefDeepSyncTypes.generated.h"

// Log category for DeepSync
AEFDEEPSYNC_API DECLARE_LOG_CATEGORY_EXTERN(LogAefDeepSync, Log, All);

/**
 * DeepSync Wearable LED Color
 *
 * RGB color values (0-255) representing the current LED state
 * of a wearable device. Colors can be set remotely via SendColorCommand().
 */
USTRUCT(BlueprintType)
struct AEFDEEPSYNC_API FAefDeepSyncColor
{
	GENERATED_BODY()

	/** Red component (0-255) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AefDeepSync")
	uint8 R = 0;

	/** Green component (0-255) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AefDeepSync")
	uint8 G = 0;

	/** Blue component (0-255) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AefDeepSync")
	uint8 B = 0;

	FAefDeepSyncColor() = default;
	FAefDeepSyncColor(uint8 InR, uint8 InG, uint8 InB) : R(InR), G(InG), B(InB) {}

	FLinearColor ToLinearColor() const { return FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f); }
	FColor ToFColor() const { return FColor(R, G, B, 255); }

	bool operator==(const FAefDeepSyncColor& Other) const { return R == Other.R && G == Other.G && B == Other.B; }
	bool operator!=(const FAefDeepSyncColor& Other) const { return !(*this == Other); }

	FString ToString() const { return FString::Printf(TEXT("RGB(%d,%d,%d)"), R, G, B); }
};

/**
 * DeepSync Wearable Data
 *
 * Complete state of a connected wearable device.
 */
USTRUCT(BlueprintType)
struct AEFDEEPSYNC_API FAefDeepSyncWearableData
{
	GENERATED_BODY()

	/** Wearable ID from the physical device (0-15) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Identity")
	int32 WearableId = -1;

	/** Unique counter ID assigned by subsystem (readonly, monotonically increasing) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Identity")
	int32 UniqueId = -1;

	/** Current heart rate in BPM (0 = no reading) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Biometrics")
	int32 HeartRate = 0;

	/** Current LED color on the wearable device */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Visual")
	FAefDeepSyncColor Color;

	/** Timestamp from server (milliseconds since server start) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Timing")
	int32 Timestamp = 0;

	/** Time since last data update (seconds) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Timing")
	float TimeSinceLastUpdate = 0.0f;

	/** World time of last update (internal use) */
	double LastUpdateWorldTime = 0.0;

	bool IsValid() const { return WearableId >= 0 && UniqueId >= 0; }
	bool IsStale(float TimeoutSeconds) const { return TimeSinceLastUpdate >= TimeoutSeconds; }

	FString ToString() const
	{
		return FString::Printf(TEXT("Wearable[Id=%d, HR=%d, %s, Age=%.2fs]"),
			WearableId, HeartRate, *Color.ToString(), TimeSinceLastUpdate);
	}
};

/**
 * DeepSync TCP Connection Status
 */
UENUM(BlueprintType)
enum class EAefDeepSyncConnectionStatus : uint8
{
	Disconnected	UMETA(DisplayName = "Disconnected"),
	Connecting		UMETA(DisplayName = "Connecting"),
	Connected		UMETA(DisplayName = "Connected"),
	Reconnecting	UMETA(DisplayName = "Reconnecting"),
	Failed			UMETA(DisplayName = "Failed")
};

/**
 * DeepSync Configuration (Blueprint-ready)
 *
 * Runtime configuration loaded from AefConfig.ini [DeepSync] section.
 */
USTRUCT(BlueprintType)
struct AEFDEEPSYNC_API FAefDeepSyncConfig
{
	GENERATED_BODY()

	//--------------------------------------------------------------------------------
	// Startup
	//--------------------------------------------------------------------------------

	/** Automatically start connection when subsystem initializes */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Startup")
	bool bAutoStart = false;

	//--------------------------------------------------------------------------------
	// Connection Settings
	//--------------------------------------------------------------------------------

	/** Server IP address */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Connection")
	FString ServerIP = TEXT("127.0.0.1");

	/** Port for receiving data from server */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Connection")
	int32 ReceiverPort = 43397;

	/** Port for sending commands to server */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Connection")
	int32 SenderPort = 43396;

	//--------------------------------------------------------------------------------
	// Wearable Settings
	//--------------------------------------------------------------------------------

	/** Timeout in seconds before wearable is marked as lost */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Wearables")
	float WearableLostTimeout = 2.0f;

	/** Allowed wearable IDs. Empty = allow all IDs. */
	TArray<int32> AllowedWearableIds;

	//--------------------------------------------------------------------------------
	// Reconnection Settings
	//--------------------------------------------------------------------------------

	/** Initial reconnect delay in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Reconnection")
	float ReconnectDelay = 2.0f;

	/** Maximum reconnection attempts (0 = infinite) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Reconnection")
	int32 MaxReconnectAttempts = 10;

	//--------------------------------------------------------------------------------
	// Logging Flags
	//--------------------------------------------------------------------------------

	/** Log connection events */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Logging")
	bool bLogConnections = true;

	/** Log wearable events */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Logging")
	bool bLogWearableEvents = true;

	/** Log every data update (very verbose!) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Logging")
	bool bLogDataUpdates = false;

	/** Log network errors */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Logging")
	bool bLogNetworkErrors = true;

	/** Log protocol/JSON parsing details (very verbose!) */
	UPROPERTY(BlueprintReadOnly, Category = "AEF|DeepSync|Logging")
	bool bLogProtocolDebug = false;
};
