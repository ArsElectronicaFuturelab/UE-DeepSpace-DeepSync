# AefDeepSync Plugin

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-blue)](https://unrealengine.com)

Unreal Engine plugin for real-time wearable device tracking with heartrate monitoring and Pharus positional sync.

## Features

- **TCP Connection** to DeepSync server with automatic reconnection
- **Wearable Tracking** for any wearable ID (no restrictions)
- **Heartrate Monitoring** with ~10Hz update rate
- **Color Control** for wearable LEDs
- **ID Control** - Change wearable IDs at runtime
- **Pharus Integration** - Sync zones link Pharus tracks with wearables
- **Sync State Management** - Centralized tracking of Zone/Pharus/Wearable links
- **Blueprint Events** for wearable connect/disconnect and sync events
- **AutoStart Option** - start automatically or manually
- **Configurable Logging** via AefConfig.ini

## Quick Start

### 1. Configuration

Add to `Config/AefConfig.ini`:

```ini
[DeepSync]
; Connection Configuration
autoStart=false
deepSyncIp=127.0.0.1
deepSyncReceiverPort=43397
deepSyncSenderPort=43396
wearableLostTimeout=2.0

; Optional: Filter specific IDs (empty = allow all)
; wearableIds=1001, 1002, 1003

; Logging Configuration
logWearableConnected=true   ; Log when new wearables connect
logWearableLost=true        ; Log when wearables timeout/disconnect
logWearableUpdated=false    ; Log every data update (verbose!)
logHeartRateChanges=false   ; Log heart rate value changes
logColorCommands=true       ; Log color commands sent
logConnectionStatus=true    ; Log connection status changes
logSyncEvents=true          ; Log Pharus sync zone events
```

### 2. Blueprint Usage

```
// Get the subsystem
AefDeepSync = Get Game Instance -> Get Subsystem (UAefDeepSyncSubsystem)

// Start connection (if autoStart=false)
AefDeepSync -> Start Deep Sync

// Bind events
AefDeepSync -> On Wearable Connected (bind your handler)
AefDeepSync -> On Wearable Lost (bind your handler)

// Get active wearables
Array = AefDeepSync -> Get Active Wearables

// Stop connection
AefDeepSync -> Stop Deep Sync
```

### 3. C++ Usage

```cpp
#include "AefDeepSyncSubsystem.h"

// Get subsystem
UAefDeepSyncSubsystem* DeepSync = GetGameInstance()->GetSubsystem<UAefDeepSyncSubsystem>();

// Start connection
DeepSync->StartDeepSync();

// Bind events
DeepSync->OnWearableConnected.AddDynamic(this, &AMyActor::OnWearableConnected);

// Get wearable data
FAefDeepSyncWearableData Data;
if (DeepSync->GetWearableById(1001, Data))
{
    int32 HeartRate = Data.HeartRate;
}

// Stop connection
DeepSync->StopDeepSync();
```

### 4. Component Usage (Per-Actor)

Add the `AEF DeepSync Wearable` component to any Actor:

1. Select Actor → Add Component → "AEF DeepSync Wearable"
2. Set `WearableId` in Details panel
3. View live data: HeartRate, Color, Timestamp, etc.
4. Bind to events: `OnHeartRateChanged`, `OnColorChanged`

```cpp
// In your actor's header
UPROPERTY(VisibleAnywhere)
UAefDeepSyncComponent* DeepSyncComponent;

// In Blueprint: bind events
DeepSyncComponent->OnHeartRateChanged.AddDynamic(this, &AMyActor::OnHRChanged);
```

### 5. DeepSync Manager Actor (Recommended)

The easiest way to use DeepSync events in Blueprint:

1. Add → All Classes → **"AEF DeepSync Manager"**
2. Select the Manager in your level
3. In **Details panel**, click **(+)** next to events to bind them

**Available Events:**
- `OnWearableConnected`, `OnWearableLost`, `OnWearableUpdated`
- `OnConnectionStatusChanged`
- `OnLinkEstablished`, `OnLinkBroken`
- `OnZoneRegistered`, `OnZoneUnregistered`

**Blueprint Functions:**
```
DeepSyncManager → Get Active Wearables
DeepSyncManager → Get All Synced Links
DeepSyncManager → Send Color Command(WearableId, Color)
DeepSyncManager → Disconnect Link(WearableId)
```

### 6. Sync Zone Usage (Pharus Integration)

Place `AAefPharusDeepSyncZoneActor` on floor:

1. Add → All Classes → "AEF Pharus DeepSync Zone"
2. Set `WearableId` to match physical device
3. Set `ZoneColor` for visual feedback
4. Set `SyncDuration` (default: 5.0s)

**Event Handling:**
```
// In Level Blueprint or Game Mode
Get DeepSync Subsystem → Bind to OnLinkEstablished

// Event: OnLinkEstablished(FAefSyncedLink Link)
→ Print: "Synced Track {Link.PharusTrackID} with Wearable {Link.WearableId}"
→ SendColorCommand(Link.WearableId, Link.ZoneColor)
```

**Query Links:**
```cpp
// Get Pharus actor for a wearable
AActor* PharusActor = Subsystem->GetPharusActorByWearableId(1001);

// Check if already synced
bool bBlocked = Subsystem->IsWearableBlocked(1001);

// Manual disconnect
Subsystem->DisconnectLink(1001);  // → Triggers OnLinkBroken
```

## Server

Requires `deepsyncwearablev2-server` running:

```powershell
cd MozXR/DeepSync/deepsyncwearablev2-server
dotnet run -- --ip-app 127.0.0.1 --port-app 43397 -d
```

Use `-d` flag for simulated wearables during development.

## Documentation

- [DOCUMENTATION.md](DOCUMENTATION.md) - Full API reference
- [CHANGELOG.md](CHANGELOG.md) - Version history

## License

Copyright (c) Ars Electronica Futurelab, 2025
