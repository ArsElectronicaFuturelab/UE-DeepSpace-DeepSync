# AefDeepSync - API Documentation

Complete API reference for the AefDeepSync Wearable Plugin.

---

## Table of Contents

1. [Overview](#overview)
2. [Configuration](#configuration)
3. [UAefDeepSyncSubsystem](#uaefdeepsyncsubsystem)
4. [AAefDeepSyncManager](#aefdeepsyncmanager)
5. [Data Types](#data-types)
6. [Events](#events)
7. [Logging](#logging)

---

## Overview

The AefDeepSync plugin provides real-time wearable device tracking with heartrate monitoring. It connects to the `deepsyncwearablev2-server` via TCP.

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    deepsyncwearablev2-server                    │
└───────────────────────┬─────────────┬───────────────────────────┘
              Port 43397│             │Port 43396
              (Data)    │             │(Commands)
                        ▼             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   UAefDeepSyncSubsystem                         │
│                   (UGameInstanceSubsystem)                      │
└─────────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Blueprint / C++ API                          │
│  OnWearableConnected | OnWearableLost | GetActiveWearables()   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Configuration

All settings are read from `Config/AefConfig.ini` section `[DeepSync]`.

### Startup

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `autoStart` | bool | `false` | Auto-start connection on subsystem init |

### Connection Settings

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `deepSyncIp` | string | `127.0.0.1` | Server IP address |
| `deepSyncReceiverPort` | int | `43397` | Port for receiving data |
| `deepSyncSenderPort` | int | `43396` | Port for sending commands |

### Wearable Settings

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `wearableLostTimeout` | float | `2.0` | Seconds before timeout |
| `wearableIds` | string | (empty) | Optional: comma-separated IDs to allow. Empty = allow all |

### Reconnection Settings

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `reconnectDelay` | float | `2.0` | Initial reconnect delay |
| `maxReconnectAttempts` | int | `10` | Max attempts (0 = infinite) |

### Logging Flags

| Key | Default | Description |
|-----|---------|-------------|
| `logWearableConnected` | `true` | Log when new wearables connect |
| `logWearableLost` | `true` | Log when wearables timeout/disconnect |
| `logWearableUpdated` | `false` | Log every wearable data update (WARNING: Very verbose!) |
| `logHeartRateChanges` | `false` | Log heart rate value changes |
| `logColorCommands` | `true` | Log color commands sent to wearables |
| `logConnectionStatus` | `true` | Log connection status changes |
| `logSyncEvents` | `true` | Log Pharus sync zone events (link established/broken) |

---

## UAefDeepSyncSubsystem

Main subsystem class. Access via `GetGameInstance()->GetSubsystem<UAefDeepSyncSubsystem>()`.

### Connection Management

#### `StartDeepSync()`
```cpp
UFUNCTION(BlueprintCallable, Category = "AefDeepSync")
void StartDeepSync();
```
Start TCP connection to server.

#### `StopDeepSync()`
```cpp
UFUNCTION(BlueprintCallable, Category = "AefDeepSync")
void StopDeepSync();
```
Stop connection and clear all wearables.

#### `GetConnectionStatus()`
```cpp
UFUNCTION(BlueprintPure, Category = "AefDeepSync")
EAefDeepSyncConnectionStatus GetConnectionStatus() const;
```

#### `IsConnected()` / `IsRunning()`
```cpp
UFUNCTION(BlueprintPure, Category = "AefDeepSync")
bool IsConnected() const;

UFUNCTION(BlueprintPure, Category = "AefDeepSync")
bool IsRunning() const;
```

---

### Wearable Access

#### `GetActiveWearables()`
```cpp
UFUNCTION(BlueprintPure, Category = "AefDeepSync|Wearables")
TArray<FAefDeepSyncWearableData> GetActiveWearables() const;
```

#### `GetWearableById()`
```cpp
UFUNCTION(BlueprintPure, Category = "AefDeepSync|Wearables")
bool GetWearableById(int32 WearableId, FAefDeepSyncWearableData& OutData) const;
```

#### `GetActiveWearableCount()` / `IsWearableActive()`
```cpp
UFUNCTION(BlueprintPure, Category = "AefDeepSync|Wearables")
int32 GetActiveWearableCount() const;

UFUNCTION(BlueprintPure, Category = "AefDeepSync|Wearables")
bool IsWearableActive(int32 WearableId) const;
```

---

### Commands

#### `SendColorCommand()`
```cpp
UFUNCTION(BlueprintCallable, Category = "AefDeepSync|Commands")
bool SendColorCommand(int32 WearableId, FLinearColor Color);
```

#### `SendIdCommand()`
```cpp
UFUNCTION(BlueprintCallable, Category = "AefDeepSync|Commands", meta = (DisplayName = "Send ID Command"))
bool SendIdCommand(int32 WearableId, int32 NewId);
```
Changes the ID of a wearable device at runtime. The server receives the command and forwards it to the physical device.

**JSON Format:** `{"type":"id","Id":<WearableId>,"NewId":<NewId>}`

---

### Sync State Management (Pharus Integration)

#### Zone Registration
```cpp
// Zones auto-register in BeginPlay, but can be manual:
void RegisterZone(AAefPharusDeepSyncZoneActor* Zone);
void UnregisterZone(AAefPharusDeepSyncZoneActor* Zone);

// Get all zones
TArray<AAefPharusDeepSyncZoneActor*> GetAllZones();
AAefPharusDeepSyncZoneActor* GetZoneByWearableId(int32 WearableId);
```

#### Link Management
```cpp
// Get all active sync links
TArray<FAefSyncedLink> GetAllSyncedLinks();

// Find specific links
bool GetLinkByWearableId(int32 WearableId, FAefSyncedLink& OutLink);
bool GetLinkByPharusTrackId(int32 TrackID, FAefSyncedLink& OutLink);

// Get related actors
AActor* GetPharusActorByWearableId(int32 WearableId);
```

#### Blocking Checks
```cpp
// Check if objects are already synced (blocked for new syncs)
bool IsZoneBlocked(AAefPharusDeepSyncZoneActor* Zone);
bool IsPharusTrackBlocked(int32 TrackID);
bool IsWearableBlocked(int32 WearableId);
```

#### Manual Disconnect
```cpp
// Break link, triggers OnLinkBroken event
bool DisconnectLink(int32 WearableId);
void DisconnectAllLinks();
```

#### Sync Events
```cpp
// Fired when link is established (after successful sync)
UPROPERTY(BlueprintAssignable)
FAefOnLinkEstablished OnLinkEstablished;
// Signature: (FAefSyncedLink Link)

// Fired when link is broken (lost/disconnect)
UPROPERTY(BlueprintAssignable)
FAefOnLinkBroken OnLinkBroken;
// Signature: (FAefSyncedLink Link, FString Reason)
// Reasons: "WearableLost", "PharusActorDestroyed", "ManualDisconnect", "ZoneUnregistered"

// Zone registration events
UPROPERTY(BlueprintAssignable)
FAefOnZoneRegistered OnZoneRegistered;
FAefOnZoneUnregistered OnZoneUnregistered;
```

---

## AAefDeepSyncManager

Actor for easy Blueprint event binding via Details panel. Place one in your level.

### Quick Start

1. Add → All Classes → **"AEF DeepSync Manager"**
2. Select in Viewport
3. In **Details panel**, click **(+)** next to events to bind

### Events

All subsystem events are exposed as Blueprint-assignable delegates:

| Event | Parameters |
|-------|------------|
| `OnWearableConnected` | FAefDeepSyncWearableData |
| `OnWearableLost` | FAefDeepSyncWearableData |
| `OnWearableUpdated` | int32 WearableId, FAefDeepSyncWearableData |
| `OnConnectionStatusChanged` | EAefDeepSyncConnectionStatus |
| `OnLinkEstablished` | FAefSyncedLink |
| `OnLinkBroken` | FAefSyncedLink, FString Reason |
| `OnZoneRegistered` | AAefPharusDeepSyncZoneActor* |
| `OnZoneUnregistered` | AAefPharusDeepSyncZoneActor* |

### Blueprint Functions

```cpp
// Get subsystem
UAefDeepSyncSubsystem* GetDeepSyncSubsystem();

// Wearables
TArray<FAefDeepSyncWearableData> GetActiveWearables();

// Links
TArray<FAefSyncedLink> GetAllSyncedLinks();
AActor* GetPharusActorByWearableId(int32 WearableId);
bool DisconnectLink(int32 WearableId);
void DisconnectAllLinks();

// Commands
bool SendColorCommand(int32 WearableId, FLinearColor Color);
```

---

## Data Types

### FAefDeepSyncWearableData

| Property | Type | Description |
|----------|------|-------------|
| `WearableId` | int32 | Device ID (any positive integer) |
| `UniqueId` | int32 | Session-unique counter |
| `HeartRate` | int32 | BPM (0 = no reading) |
| `Color` | FLinearColor | Current LED color (linear color space) |
| `Timestamp` | int32 | Server timestamp (ms) |
| `TimeSinceLastUpdate` | float | Seconds since last update |

### FAefDeepSyncColor (Internal)

> **Note:** This struct is now internal-only. Use `FLinearColor` in Blueprints.

| Property | Type | Range |
|----------|------|-------|
| `R` | uint8 | 0-255 |
| `G` | uint8 | 0-255 |
| `B` | uint8 | 0-255 |

### EAefDeepSyncConnectionStatus

| Value | Description |
|-------|-------------|
| `Disconnected` | Not connected |
| `Connecting` | Initial connection |
| `Connected` | Active connection |
| `Reconnecting` | Lost, retrying |
| `Failed` | Max retries exceeded |

### FAefSyncedLink

Represents an active link between a Pharus Track and a DeepSync Wearable.

| Property | Type | Description |
|----------|------|-------------|
| `LinkId` | int32 | Unique link identifier |
| `Zone` | TWeakObjectPtr | Zone that created this link |
| `PharusTrackID` | int32 | Pharus Track ID (blocked) |
| `WearableId` | int32 | Wearable ID (blocked) |
| `PharusActor` | TWeakObjectPtr | Reference to Pharus actor |
| `ZoneColor` | FLinearColor | Zone color at sync time |
| `SyncTime` | FDateTime | When sync was established |

### FAefDeepSyncConfig

Blueprint-accessible configuration struct with all settings from AefConfig.ini.

---

## Events

### FAefOnWearableConnected
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnWearableConnected, 
    const FAefDeepSyncWearableData&, WearableData);
```

### FAefOnWearableLost
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnWearableLost, 
    const FAefDeepSyncWearableData&, WearableData);
```

### FAefOnWearableUpdated
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnWearableUpdated, 
    int32, WearableId, 
    const FAefDeepSyncWearableData&, WearableData);
```

### FAefOnConnectionStatusChanged
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnConnectionStatusChanged, 
    EAefDeepSyncConnectionStatus, Status);
```

---

## Logging

Log category: `LogAefDeepSync`

```
Log LogAefDeepSync Verbose    // Enable all
Log LogAefDeepSync Log        // Normal
Log LogAefDeepSync Warning    // Errors only
```

---

## UAefDeepSyncComponent

Actor component for per-actor wearable tracking. Add to any actor to receive data from a specific wearable ID.

### Quick Start

1. Add "AEF DeepSync Wearable" component to any Actor
2. Set `WearableId` to match your physical device
3. View live data in the Details panel
4. Bind to events for real-time notifications

### Properties

#### Configuration

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `WearableId` | int32 | 0 | ID of the wearable to track |
| `bAutoConnect` | bool | true | Auto-bind to subsystem on BeginPlay |

#### Live Data (Read-Only)

| Property | Type | Description |
|----------|------|-------------|
| `UniqueId` | int32 | Session-unique ID assigned by subsystem |
| `HeartRate` | int32 | Current heart rate in BPM (0 = no reading) |
| `Color` | FLinearColor | Current LED color on the wearable |
| `Timestamp` | int32 | Server timestamp (ms since start) |
| `TimeSinceLastUpdate` | float | Seconds since last data update |
| `bIsWearableConnected` | bool | True if wearable is active |
| `SubsystemStatus` | EAefDeepSyncConnectionStatus | Server connection status |

### Events

#### Change Detection Events

```cpp
// Fired when heart rate changes
UPROPERTY(BlueprintAssignable)
FAefOnHeartRateChanged OnHeartRateChanged;
// Signature: (int32 OldHeartRate, int32 NewHeartRate)

// Fired when LED color changes
UPROPERTY(BlueprintAssignable)
FAefOnColorChanged OnColorChanged;
// Signature: (FLinearColor OldColor, FLinearColor NewColor)

// Fired when wearable connects/disconnects
UPROPERTY(BlueprintAssignable)
FAefOnWearableConnectionChanged OnWearableConnectionChanged;
// Signature: (bool bIsConnected)
```

#### Update Event

```cpp
// Fired on every data update (high frequency!)
UPROPERTY(BlueprintAssignable)
FAefOnWearableDataUpdated OnWearableDataUpdated;
// Signature: (int32 WearableId, FAefDeepSyncWearableData WearableData)
```

### Functions

#### Control

```cpp
// Manual subsystem binding (if bAutoConnect = false)
void BindToSubsystem();
void UnbindFromSubsystem();

// Force data refresh
void RefreshWearableData();

// Send color to this wearable
bool SendColorCommand(FLinearColor InColor);
```

#### Getters

```cpp
int32 GetHeartRate();           // Current BPM
FLinearColor GetColor();        // Current LED color
float GetTimeSinceLastUpdate(); // Seconds since last data
bool IsWearableDataValid();     // True if connected and fresh
```

### Blueprint Example

```
Event BeginPlay
  -> Bind to OnHeartRateChanged
  
OnHeartRateChanged (OldHR, NewHR)
  -> Print String: "Heart rate changed from {OldHR} to {NewHR}"
  -> Branch: NewHR > 100
     True -> SendColorCommand(Red)
     False -> SendColorCommand(Green)
```

---

## AAefPharusDeepSyncZoneActor

Floor actor for synchronizing Pharus tracks with DeepSync wearables. Place on floor where visitors should sync their wearable.

> **Requires Plugin:** `AefPharus` (automatically enabled as dependency)

### Quick Start

1. Place `AAefPharusDeepSyncZoneActor` in your level
2. Set `WearableId` to match the physical device for this zone
3. Adjust `ZoneColor` for visual feedback
4. Bind to `OnSyncCompleted` in Blueprint
5. In the event handler, call `SendColorCommand` to set wearable color

### Mapping

**1:1:1 Mapping:** 1 Zone = 1 WearableId = 1 Pharus TrackID

- Each zone has one assigned WearableId
- Only one person can sync with a zone at a time
- Sync requires the person to stay in the zone for `SyncDuration` seconds

### Properties

#### Configuration

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `WearableId` | int32 | 0 | Wearable ID for this sync zone |
| `ZoneColor` | FLinearColor | Green | Visual color of the zone |
| `SyncDuration` | float | 5.0 | Seconds to complete sync |
| `ZoneRadius` | float | 100.0 | Trigger radius (cm) |
| `bAutoActivate` | bool | true | Activate on BeginPlay |
| `bShowDebugInfo` | bool | false | Show debug overlay |

#### Status (Read-Only in Details Panel)

| Property | Type | Description |
|----------|------|-------------|
| `CurrentSyncProgress` | float | 0.0 → 1.0 during sync |
| `SyncTimeRemaining` | float | Seconds until sync complete |
| `CurrentPharusTrackID` | int32 | TrackID in zone (-1 = empty) |
| `bIsSyncing` | bool | Sync in progress |
| `bIsActive` | bool | Zone is accepting overlaps |

### Events

```cpp
// Pharus track enters zone
UPROPERTY(BlueprintAssignable)
FAefOnSyncStarted OnSyncStarted;
// Signature: (int32 PharusTrackID)

// Progress update every tick
UPROPERTY(BlueprintAssignable)
FAefOnSyncing OnSyncing;
// Signature: (int32 PharusTrackID, float Progress)

// Sync completed (success or failure)
UPROPERTY(BlueprintAssignable)
FAefOnSyncCompleted OnSyncCompleted;
// Signature: (FAefPharusSyncResult Result)

// Person left zone before sync completed
UPROPERTY(BlueprintAssignable)
FAefOnSyncCancelled OnSyncCancelled;
// Signature: (int32 PharusTrackID)

// Wearable connection lost during sync
UPROPERTY(BlueprintAssignable)
FAefOnSyncWearableLost OnWearableLost;
// Signature: (int32 WearableId)

// Pharus track lost during sync
UPROPERTY(BlueprintAssignable)
FAefOnSyncPharusTrackLost OnPharusTrackLost;
// Signature: (int32 PharusTrackID)
```

### FAefPharusSyncResult

Returned by `OnSyncCompleted` event.

| Property | Type | Description |
|----------|------|-------------|
| `bSuccess` | bool | Was sync successful? |
| `Status` | EAefPharusSyncStatus | Idle/Syncing/Success/Failed/Timeout |
| `ErrorMessage` | FString | Error description if failed |
| `PharusTrackID` | int32 | Pharus track that synced |
| `PharusPosition` | FVector | World position at sync |
| `WearableId` | int32 | Wearable ID that synced |
| `HeartRate` | int32 | Heart rate at sync completion |
| `WearableColor` | FLinearColor | Wearable LED color |
| `WearableData` | FAefDeepSyncWearableData | Full wearable data |
| `ZoneColor` | FLinearColor | Sync zone color |
| `SyncDuration` | float | How long sync took |
| `SyncCompletedTime` | FDateTime | When sync completed |

### Functions

```cpp
// Activate/deactivate zone
void ActivateZone();
void DeactivateZone();

// Cancel ongoing sync
void CancelSync();

// Get sync progress (0.0 - 1.0)
float GetSyncProgress();

// Check if syncing
bool IsSyncing();

// Change zone color at runtime
void SetZoneColor(FLinearColor NewColor);
```

### Blueprint Example

```
Event OnSyncCompleted (Result)
  → Branch: Result.bSuccess
     True:
       → Get DeepSync Subsystem
       → Send Color Command (Result.WearableId, ZoneColor)
       → Print: "Synced TrackID {Result.PharusTrackID} with Wearable {Result.WearableId}"
     False:
       → Print: "Sync failed: {Result.ErrorMessage}"
```

### Sequence Diagram

```
 Pharus Actor          Sync Zone              DeepSync
      │                    │                      │
      │──── Enters Zone ──→│                      │
      │                    │←─ Check Wearable ───→│
      │                    │     IsActive?        │
      │                    │                      │
      │     OnSyncStarted ←│                      │
      │                    │                      │
      │    ┌───────────────┤ (5 seconds)         │
      │    │   OnSyncing   │                      │
      │    │   Progress    │                      │
      │    └───────────────┤                      │
      │                    │                      │
      │   OnSyncCompleted ←│←─ GetWearableById ──→│
      │   (with Result)    │                      │
      │                    │                      │
```

---

*Copyright (c) Ars Electronica Futurelab, 2025*

