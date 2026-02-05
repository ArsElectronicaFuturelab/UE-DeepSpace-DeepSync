# AefDeepSync - API Documentation

Complete API reference for the AefDeepSync Wearable Plugin.

---

## Table of Contents

1. [Overview](#overview)
2. [Configuration](#configuration)
3. [UAefDeepSyncSubsystem](#uaefdeepsyncsubsystem)
4. [Data Types](#data-types)
5. [Events](#events)
6. [Logging](#logging)

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
| `logConnections` | `true` | Log connect/disconnect |
| `logWearableEvents` | `true` | Log wearable added/lost |
| `logDataUpdates` | `false` | Log every update (verbose!) |
| `logNetworkErrors` | `true` | Log TCP errors |
| `logProtocolDebug` | `false` | Log JSON parsing (verbose!) |

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
bool SendColorCommand(int32 WearableId, FAefDeepSyncColor Color);
```

---

## Data Types

### FAefDeepSyncWearableData

| Property | Type | Description |
|----------|------|-------------|
| `WearableId` | int32 | Device ID (any positive integer) |
| `UniqueId` | int32 | Session-unique counter |
| `HeartRate` | int32 | BPM (0 = no reading) |
| `Color` | FAefDeepSyncColor | Current LED color |
| `Timestamp` | int32 | Server timestamp (ms) |
| `TimeSinceLastUpdate` | float | Seconds since last update |

### FAefDeepSyncColor

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

*Copyright (c) Ars Electronica Futurelab, 2025*
