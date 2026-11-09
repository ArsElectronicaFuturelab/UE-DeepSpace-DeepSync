# AefDeepSync – DeepSync Wearables for Unreal Engine

| | |
|---|---|
| Version | 3.2.4 |
| Engine | Unreal Engine 5.7 |
| Platform | Windows 64-bit |
| Dependencies | AefPharus, AefUtility |
| Developer | Ars Electronica Futurelab |
| License | MIT |

> **Optional plugin.** AefDeepSync is not required by the DeepSpace Starter, AefPharus or AefUtility. Install it only for
> projects that use DeepSync wearables; the Starter works unchanged without it. It requires AefPharus and AefUtility,
> which are part of the Starter.

AefDeepSync connects Unreal Engine to the `deepsyncwearablev2-server` (TCP), receives heart rate and LED color of
DeepSync wearables and sends color and ID commands back. Sync zones link a wearable to a Pharus-tracked person
standing in a floor zone. It runs as a `UGameInstanceSubsystem`, is configured in the `[DeepSync]` section of
`Config/AefConfig.ini` and uses the same configuration file as [AefPharus](../AefPharus/README.md). The path of the file
is resolved by the [AefUtility](../AefUtility/README.md) plugin.

---

## How it works

```
deepsyncwearablev2-server
        │ TCP  deepSyncReceiverPort (43397)  wearable data ──►
        │ TCP  deepSyncSenderPort   (43396)  ◄── commands
        ▼
UAefDeepSyncSubsystem (game thread tick)
        │  parse messages / update wearables / reconnect / link checks
        ├──► UAefDeepSyncComponent          per actor, filters one WearableId
        ├──► AAefDeepSyncManager            exposes all subsystem events in the Details panel
        └──► AAefPharusDeepSyncZoneActor    links a Pharus track to a wearable
```

- **`UAefDeepSyncSubsystem`** – entry point. Reads `AefConfig.ini`, owns both TCP connections, the list of active wearables,
  the registered sync zones and the active sync links. Persists across level changes.
- **`UAefDeepSyncComponent`** – actor component (`AEF DeepSync Wearable`) that mirrors the data of one wearable and raises change events.
- **`AAefDeepSyncManager`** – level actor (`AEF DeepSync Manager`) that forwards the subsystem events as Blueprint-assignable delegates.
- **`AAefPharusDeepSyncZoneActor`** – floor actor (`AEF Pharus DeepSync Zone`) for the Pharus integration.

### Wearable data

- The server sends one JSON object per message, terminated by the character `X`:
  `{"Id":1001,"HeartRate":72,"Timestamp":123456,"Color":{"R":0,"G":255,"B":0}}`.
- Every message creates or updates the wearable with that `Id`; the first message raises `OnWearableConnected`.
  Color values (0-255) are converted to `FLinearColor`.
- A wearable without a message for `wearableLostTimeout` seconds is removed and raises `OnWearableLost`.
  Stopping the connection raises `OnWearableLost` for all active wearables.
- With `wearableIds` set, messages of other IDs are ignored.
- Commands are JSON objects terminated by `X` as well:
  `{"Id":<WearableId>,"Color":{"R":..,"G":..,"B":..}}` and `{"type":"id","Id":<WearableId>,"NewId":<NewId>}`.

### Connection

`StartDeepSync()` connects the receiver and the sender socket. If this fails, or the server closes the connection, the status
changes to `Reconnecting` and the subsystem retries with exponential backoff: `reconnectDelay`, then doubled up to 60 s.
After `maxReconnectAttempts` failed attempts (0 = unlimited) the status is `Failed` and the subsystem stops;
call `StartDeepSync()` again to restart.

| Status | Meaning |
|---|---|
| `Disconnected` | Not running. |
| `Connecting` | Initial connection attempt. |
| `Connected` | Receiving data. |
| `Reconnecting` | Connection lost, retrying. |
| `Failed` | Maximum attempts exceeded. |

### Pharus sync zones

1. A zone is a sphere (`ZoneRadius`) on the floor. When an actor implementing `IAefPharusActorInterface` with a valid
   `TrackID` (e.g. `AAefPharusClusterActor`) overlaps it, the zone starts a sync and raises `OnSyncStarted`.
2. The sync only starts if the zone, the track and the wearable `WearableId` are not part of an existing link and the wearable is active.
3. While the person stays in the zone, `OnSyncing` reports the progress every tick. Leaving the zone raises `OnSyncCancelled`;
   losing the wearable raises `OnWearableLost` and a failed `OnSyncCompleted`.
4. After `SyncDuration` seconds the zone raises `OnSyncCompleted` with an `FAefPharusSyncResult`. A successful sync creates an
   `FAefSyncedLink` in the subsystem (`OnLinkEstablished`).
5. Zone, track and wearable of a link are blocked for new syncs until the link breaks. Reasons passed to `OnLinkBroken`:
   `WearableLost`, `PharusActorDestroyed`, `ManualDisconnect`, `DisconnectAll`, `ZoneUnregistered`, `ZoneDestroyed`.

One zone stands for one `WearableId`, so a zone links one person at a time (1 zone = 1 wearable = 1 Pharus track).

---

## Setup

1. Copy `Plugins/AefDeepSync/` to `Project/Plugins/AefDeepSync/` of the project. The plugin requires `AefPharus` and `AefUtility`,
   which are enabled automatically. In the DeepSpace Starter the folder is listed in `.gitignore` and is not committed.
2. Start the `deepsyncwearablev2-server` (.NET 8) from its own folder; `-d` starts simulated wearables for development:

   ```powershell
   cd <path-to>/deepsyncwearablev2-server
   dotnet run -- --ip-app 127.0.0.1 --port-app 43397 -d
   ```

3. Configure the `[DeepSync]` section of `Project/Config/AefConfig.ini` (see reference below). The Starter's file already contains it.
4. Start the connection:
   - `autoStart=true` – the subsystem connects when the game instance is created, or
   - `autoStart=false` – call `StartDeepSync()` (Blueprint or C++).
5. Use the data:
   - add the component **AEF DeepSync Wearable** to an actor and set `WearableId`, or
   - place an **AEF DeepSync Manager** in the level and bind its events in the Details panel, or
   - place an **AEF Pharus DeepSync Zone** on the floor and set `WearableId`, `ZoneColor` and `SyncDuration`; bind `OnSyncCompleted`.

The plugin content (`Content/`) contains an example level `DeepSync_Map`, the example Blueprints `BP_DeepSync_*` and the materials `M_DeepSync_*`.

### Minimal C++ usage

```cpp
#include "AefDeepSyncSubsystem.h"

UAefDeepSyncSubsystem* DeepSync = GetGameInstance()->GetSubsystem<UAefDeepSyncSubsystem>();
DeepSync->OnWearableConnected.AddDynamic(this, &AMyActor::HandleWearableConnected);
DeepSync->StartDeepSync();

FAefDeepSyncWearableData Data;
if (DeepSync->GetWearableById(1001, Data))
{
	DeepSync->SendColorCommandLinear(1001, FLinearColor::Red);
}
```

The module name for `Build.cs` is `AefDeepSync`.

---

## Configuration reference (`Config/AefConfig.ini`)

AefDeepSync reads the file returned by `UAefUtilityBPLibrary::GetConfigFilePath()`:

| Start | File used |
|---|---|
| no argument | `<Project>/Config/AefConfig.ini` |
| `-dsConfig=<path>` | the given file (relative paths start at the project directory); if it does not exist, an error is logged (`LogAefUtility`) and the default file is used |

The configuration is read once when the subsystem is created; restart after changes. `ReloadConfiguration()` reads the file again
at runtime; connection settings (IP, ports) apply at the next connection. Missing keys use the defaults below. `AefConfig.ini` also contains the sections of
other plugins (e.g. `[PharusSubsystem]`); AefDeepSync only reads `[DeepSync]`.

### `[DeepSync]`

| Key | Default | Description |
|---|---|---|
| `autoStart` | `false` | Connect automatically when the subsystem is created. `false` = call `StartDeepSync()`. |
| `deepSyncIp` | `127.0.0.1` | IP address of the server. |
| `deepSyncReceiverPort` | `43397` | TCP port for wearable data (server option `--port-app`). |
| `deepSyncSenderPort` | `43396` | TCP port for commands. |
| `wearableIds` | – | Comma-separated whitelist of wearable IDs (e.g. `1001, 1002`). Empty = all IDs. |
| `wearableLostTimeout` | `2.0` | Seconds without data before a wearable counts as lost. |
| `reconnectDelay` | `2.0` | Initial delay (seconds) between reconnect attempts; doubled per attempt up to 60 s. |
| `maxReconnectAttempts` | `10` | Failed attempts before the status becomes `Failed` (0 = unlimited). |
| `logWearableConnected` / `logWearableLost` | `true` / `true` | Log when wearables connect or are lost. |
| `logWearableUpdated` | `false` | Log every message (very verbose). |
| `logColorCommands` / `logIdCommands` | `true` / `true` | Log color and ID commands that are sent. |
| `logConnectionStatus` | `true` | Log connection status changes and reconnect attempts. |
| `logSyncEvents` | `true` | Log established and broken links. |
| `logNetworkErrors` | `true` | Log connection, socket and JSON errors. |

Notes:
- `logHeartRateChanges` is parsed but currently has no effect.
- Values are `true` / `false`; any other text counts as `false`.

---

## Blueprint / C++ API

All functions are in the Blueprint category `AEF|DeepSync` (subcategories `Wearables`, `Commands`, `Config`, `Events`, `Sync|…`).

### `UAefDeepSyncSubsystem`

Access: `GetGameInstance()->GetSubsystem<UAefDeepSyncSubsystem>()` (Blueprint: *Get Game Instance Subsystem*).

| Function | Purpose |
|---|---|
| `StartDeepSync()` / `StopDeepSync()` | Connect / disconnect and clear all wearables. |
| `GetConnectionStatus()` / `IsConnected()` / `IsRunning()` | Connection state (`IsRunning` is also true while reconnecting). |
| `GetActiveWearables()` / `GetWearableById(Id, OutData)` | Current wearable data. |
| `GetActiveWearableCount()` / `IsWearableActive(Id)` | Wearable queries. |
| `SendColorCommand(Id, Color)` | Set the LED color of a wearable (Blueprint node *Send Color Command*, C++ `SendColorCommandLinear`). |
| `SendIdCommand(Id, NewId)` | Change the ID of a wearable. The server forwards the command to the device. |
| `GetConfig()` / `ReloadConfiguration()` | Read the current `FAefDeepSyncConfig` / read `AefConfig.ini` again. |
| `RegisterZone` / `UnregisterZone` | Zone registration. Zones do this themselves in `BeginPlay` / `EndPlay`. |
| `GetAllZones()` / `GetZoneByWearableId(Id)` | Zone queries. |
| `GetAllSyncedLinks()` / `GetLinkByWearableId(Id, OutLink)` / `GetLinkByPharusTrackId(TrackID, OutLink)` | Link queries. |
| `GetPharusActorByWearableId(Id)` | Pharus actor of a linked wearable. |
| `IsZoneBlocked(Zone)` / `IsPharusTrackBlocked(TrackID)` / `IsWearableBlocked(Id)` | Blocking checks. |
| `DisconnectLink(Id)` / `DisconnectAllLinks()` | Break links (reason `ManualDisconnect` / `DisconnectAll`). |
| `NotifySyncCompleted(Result, Zone, PharusActor)` | Called by zones; creates a link for a successful result. |

Events:

| Event | Parameters |
|---|---|
| `OnWearableConnected` / `OnWearableLost` | `FAefDeepSyncWearableData` |
| `OnWearableUpdated` | `WearableId`, `FAefDeepSyncWearableData` (every message, high frequency) |
| `OnConnectionStatusChanged` | `EAefDeepSyncConnectionStatus` |
| `OnLinkEstablished` | `FAefSyncedLink` |
| `OnLinkBroken` | `FAefSyncedLink`, `Reason` |
| `OnZoneRegistered` / `OnZoneUnregistered` | `AAefPharusDeepSyncZoneActor*` |

### `AAefDeepSyncManager` (`AEF DeepSync Manager`)

Place one in the level and click **(+)** next to an event in the Details panel to bind it. It forwards all subsystem events
listed above (same names and parameters) and offers `GetDeepSyncSubsystem`, `GetActiveWearables`, `GetAllSyncedLinks`,
`GetPharusActorByWearableId`, `SendColorCommand`, `DisconnectLink` and `DisconnectAllLinks`.

### `UAefDeepSyncComponent` (`AEF DeepSync Wearable`)

| Property | Default | Description |
|---|---|---|
| `WearableId` | `0` | Wearable to track (any non-negative ID). |
| `bAutoConnect` | `true` | Bind to the subsystem in `BeginPlay`. `false` = call `BindToSubsystem()`. |
| `UniqueId`, `HeartRate`, `Color`, `Timestamp`, `TimeSinceLastUpdate` | – | Live data (read-only, visible in the Details panel). |
| `bIsWearableConnected`, `SubsystemStatus` | – | Wearable and server connection state (read-only). |

| Function / Event | Purpose |
|---|---|
| `BindToSubsystem()` / `UnbindFromSubsystem()` / `RefreshWearableData()` | Binding and manual refresh. |
| `SendColorCommand(Color)` | Set the LED color of this wearable. |
| `GetHeartRate()` / `GetColor()` / `GetTimeSinceLastUpdate()` / `IsWearableDataValid()` | Convenience getters. |
| `OnHeartRateChanged(Old, New)` / `OnColorChanged(Old, New)` | Raised only when the value changes. |
| `OnWearableConnectionChanged(bIsConnected)` | Wearable connects or is lost. |
| `OnWearableDataUpdated(WearableId, Data)` | Every message (high frequency). |

### `AAefPharusDeepSyncZoneActor` (`AEF Pharus DeepSync Zone`)

| Property | Default | Description |
|---|---|---|
| `WearableId` | `0` | Wearable of this zone (must match the physical device). |
| `ZoneColor` | green | Color of the zone mesh and of the link. |
| `SyncDuration` | `5.0` | Seconds a person has to stay in the zone (0.5-60). |
| `ZoneRadius` | `100.0` | Radius of the trigger sphere (cm). |
| `bAutoActivate` | `true` | Activate in `BeginPlay`. |
| `bShowDebugInfo` | `false` | Show the sync progress on screen. |
| `CurrentSyncProgress`, `SyncTimeRemaining`, `CurrentPharusTrackID`, `bIsSyncing`, `bIsActive` | – | Status (read-only). |

| Function / Event | Purpose |
|---|---|
| `ActivateZone()` / `DeactivateZone()` / `CancelSync()` | Zone control (`DeactivateZone` cancels a running sync). |
| `GetSyncProgress()` / `IsSyncing()` / `SetZoneColor(Color)` | Queries and runtime color change. |
| `OnSyncStarted(PharusTrackID)` | A track entered the zone. |
| `OnSyncing(PharusTrackID, Progress)` | Every tick during the sync (0-1). |
| `OnSyncCompleted(FAefPharusSyncResult)` | Sync finished, successful or failed (check `bSuccess`). |
| `OnSyncCancelled(PharusTrackID)` | The person left the zone early. |
| `OnWearableLost(WearableId)` | The wearable is not active when starting or during the sync. |
| `OnPharusTrackLost(PharusTrackID)` | Declared, currently not raised; a lost track ends the overlap and raises `OnSyncCancelled`. |

The zone mesh looks for the material parameters `BaseColor`, `Color` and `EmissiveColor` to apply `ZoneColor`.

### Data types

| Type | Content |
|---|---|
| `FAefDeepSyncWearableData` | `WearableId`, `UniqueId` (session counter), `HeartRate` (BPM, 0 = no reading), `Color` (`FLinearColor`), `Timestamp` (server time in ms), `TimeSinceLastUpdate` (seconds). |
| `EAefDeepSyncConnectionStatus` | `Disconnected`, `Connecting`, `Connected`, `Reconnecting`, `Failed`. |
| `FAefSyncedLink` | `LinkId`, `Zone`, `PharusTrackID`, `WearableId`, `PharusActor`, `ZoneColor`, `SyncTime`. |
| `FAefPharusSyncResult` | `bSuccess`, `Status`, `ErrorMessage`, `PharusTrackID`, `PharusPosition`, `WearableId`, `HeartRate`, `WearableColor`, `WearableData`, `ZoneColor`, `SyncDuration`, `SyncCompletedTime`. |
| `EAefPharusSyncStatus` | `Idle`, `Syncing`, `Success`, `Failed`, `Timeout` (`Idle`, `Syncing` and `Timeout` are not set by the zone at the moment). |
| `FAefDeepSyncConfig` | All values of the `[DeepSync]` section (read-only in Blueprint). |
| `FAefDeepSyncColor` | RGB (0-255) used internally for the wire format. Use `FLinearColor` in Blueprints. |

---

## Troubleshooting

| Symptom | Check |
|---|---|
| Nothing happens, no wearables | With `autoStart=false` call `StartDeepSync()`. Check that the server runs and that `deepSyncIp`, `deepSyncReceiverPort` and `deepSyncSenderPort` match its `--ip-app` / `--port-app`. |
| Status stays `Reconnecting`, then `Failed` | The server is not reachable (IP, ports, firewall). After `maxReconnectAttempts` the subsystem stops; fix the connection and call `StartDeepSync()` again. |
| Wearables flicker between connected and lost | `wearableLostTimeout` is shorter than the interval of the server messages. Increase it. |
| A wearable never appears | Check `wearableIds` (an empty value allows all) and the ID of the device (`logWearableUpdated=true` shows every message). |
| Wrong or no config loaded | Log `LogAefUtility` shows the file in use ("Using default config" / "Using config override"). The warning `AefConfig.ini not found` from `LogAefDeepSync` means the file is missing at that path; in a packaged build check `<Build>/DeepSpaceStarter/Config/AefConfig.ini`. |
| Zone never starts a sync | The overlapping actor must implement `IAefPharusActorInterface` and have a valid `TrackID`; the wearable of the zone must be active; zone, track and wearable must not be linked already (`logSyncEvents=true`, log `LogAefPharusSync`). |
| Zone stays blocked | The link of the zone is still active. Break it with `DisconnectLink(WearableId)` or wait until the wearable or the Pharus actor is lost. |
| Colors look wrong on the wearable | `SendColorCommand` sends the R, G and B values of the linear color multiplied by 255 without gamma correction (incoming colors are divided by 255 the same way). |

Log categories: `LogAefDeepSync` (subsystem) and `LogAefPharusSync` (zones). Enable verbose output with `log LogAefDeepSync Verbose` in the console.

---

See [CHANGELOG.md](CHANGELOG.md) for version history.

© 2025-2026 Ars Electronica Futurelab
