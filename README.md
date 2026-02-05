- # Unreal Engine 5.7 - DeepSync Plugin (Preview)

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-blue)](https://unrealengine.com)

Unreal Engine plugin for real-time wearable device tracking with heartrate monitoring.

## Features

- **TCP Connection** to DeepSync server with automatic reconnection
- **Wearable Tracking** for any wearable ID (no restrictions)
- **Heartrate Monitoring** with ~10Hz update rate
- **Color Control** for wearable LEDs
- **Blueprint Events** for wearable connect/disconnect
- **AutoStart Option** - start automatically or manually
- **Configurable Logging** via MozConfig.ini

## Quick Start

### 1. Configuration

Add to `Config/MozConfig.ini`:

```ini
[DeepSync]
autoStart=false
deepSyncIp=127.0.0.1
deepSyncReceiverPort=43397
deepSyncSenderPort=43396
wearableLostTimeout=2.0

; Optional: Filter specific IDs (empty = allow all)
; wearableIds=1001, 1002, 1003
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

Copyright (c) Ars Electronica Futurelab, 2026
