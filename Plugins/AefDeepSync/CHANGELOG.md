# Changelog

All notable changes to AefDeepSync will be documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.2.4] - 2026-02-06

### Changed

**Logging Configuration Overhaul**
- Replaced generic logging flags with granular controls in `AefConfig.ini`:
  - `logWearableConnected` - Log when new wearables connect
  - `logWearableLost` - Log when wearables timeout/disconnect
  - `logWearableUpdated` - Log every wearable data update (verbose!)
  - `logHeartRateChanges` - Log heart rate value changes
  - `logColorCommands` - Log color commands sent to wearables
  - `logConnectionStatus` - Log connection status changes
  - `logSyncEvents` - Log Pharus sync zone events (link established/broken)

### Removed
- Removed generic logging flags: `logConnections`, `logWearableEvents`, `logDataUpdates`, `logNetworkErrors`, `logProtocolDebug`
- Flag `logIdCommands` merged with general command logging

---

## [1.2.3] - 2026-02-06

### Changed

**Wearable Color Data**
- `FAefDeepSyncWearableData.Color` is now `FLinearColor` (was `FAefDeepSyncColor`)
- Color values are now directly usable in Blueprints without `.ToLinearColor()` conversion
- Byte values from server (0-255) are automatically converted to linear color space

### Deprecated
- `FAefDeepSyncColor` is now internal-only, use `FLinearColor` for all Blueprint operations

---

## [1.2.2] - 2026-02-06

### Added

**ID Command Support**
- `SendIdCommand(WearableId, NewId)` - Change wearable ID at runtime
- `bLogIdCommands` config flag to log ID command sends

### Fixed
- Typo in config loading: `bLogConnectionStatustatus` → `bLogConnectionStatus`

---

## [1.2.1] - 2026-02-06

### Added

**Centralized Sync State Management**
- Zone auto-registration with `UAefDeepSyncSubsystem` on BeginPlay
- `FAefSyncedLink` struct tracks active Zone/Pharus/Wearable connections
- Blocking logic: synced objects are blocked until link is broken

**Subsystem API**
- `RegisterZone/UnregisterZone` - Zone registration
- `GetAllZones()`, `GetZoneByWearableId()` - Zone lookup
- `GetAllSyncedLinks()` - Get all active links
- `GetLinkByWearableId()`, `GetLinkByPharusTrackId()` - Link lookup
- `GetPharusActorByWearableId()` - Get Pharus actor for a wearable
- `IsZoneBlocked()`, `IsPharusTrackBlocked()`, `IsWearableBlocked()` - Blocking checks
- `DisconnectLink()`, `DisconnectAllLinks()` - Manual disconnect

**Events**
- `OnLinkEstablished(FAefSyncedLink)` - New sync link created
- `OnLinkBroken(FAefSyncedLink, Reason)` - Link broken (lost/disconnect)
- `OnZoneRegistered/OnZoneUnregistered` - Zone registration events

**Automatic Link Breaking**
- Links automatically break when Wearable/Pharus is lost
- Reasons: `WearableLost`, `PharusActorDestroyed`, `ManualDisconnect`, `ZoneUnregistered`

**AAefDeepSyncManager Actor**
- Place in level to expose subsystem events in Details panel
- Blueprint-assignable events with (+) button binding
- Convenience functions: `GetActiveWearables`, `GetAllSyncedLinks`, `SendColorCommand`

---

## [1.2.0] - 2026-02-06

### Added

**Pharus-DeepSync Integration**
- New dependency: `AefPharus` plugin
- `AAefPharusDeepSyncZoneActor` - Floor actor for sync zones
  - Configure `WearableId`, `ZoneColor`, `SyncDuration`, `ZoneRadius`
  - Debug display in Details panel: `CurrentSyncProgress`, `SyncTimeRemaining`, `bIsSyncing`
  - 1:1:1 mapping: 1 Zone = 1 WearableId = 1 Pharus TrackID

**Sync Types**
- `FAefPharusSyncResult` - Complete sync result with Pharus + Wearable data
- `EAefPharusSyncStatus` - Idle/Syncing/Success/Failed/Timeout

**Sync Events**
- `OnSyncStarted(PharusTrackID)` - Person enters zone
- `OnSyncing(PharusTrackID, Progress)` - Tick during sync (0→1)
- `OnSyncCompleted(FAefPharusSyncResult)` - Sync finished with full result
- `OnSyncCancelled(PharusTrackID)` - Person left zone early
- `OnWearableLost(WearableId)` - Wearable connection lost during sync
- `OnPharusTrackLost(PharusTrackID)` - Pharus track lost during sync

**Functions**
- `ActivateZone()` / `DeactivateZone()` - Control zone state
- `CancelSync()` - Cancel ongoing sync
- `GetSyncProgress()` - Get current progress (0.0 - 1.0)
- `IsSyncing()` - Check if sync in progress
- `SetZoneColor(FLinearColor)` - Change zone color at runtime

---

## [1.1.0] - 2026-02-04

### Added

**UAefDeepSyncComponent**
- New ActorComponent for per-actor wearable tracking
- `WearableId` property to configure which wearable to track
- Individual live data properties visible in Details panel:
  - `UniqueId`, `HeartRate`, `Color`, `Timestamp`, `TimeSinceLastUpdate`
- `bAutoConnect` for automatic subsystem binding

**Change Detection Events**
- `OnHeartRateChanged(OldHeartRate, NewHeartRate)` - fires when BPM changes
- `OnColorChanged(OldColor, NewColor)` - fires when LED color changes
- `OnWearableConnectionChanged(bIsConnected)` - fires on connect/disconnect
- `OnWearableDataUpdated(WearableId, WearableData)` - fires on every update

**Convenience Functions**
- `GetHeartRate()` - quick access to current BPM
- `GetColor()` - quick access to current LED color (FLinearColor)
- `GetTimeSinceLastUpdate()` - seconds since last data
- `IsWearableDataValid()` - check if data is fresh
- `SendColorCommand(FLinearColor)` - send color to bound wearable

---

## [1.0.0] - 2026-02-04

### Added

**Core Subsystem**
- `UAefDeepSyncSubsystem` (GameInstance subsystem)
- Persists across level transitions
- Full Blueprint API

**Connection Management**
- `StartDeepSync()` / `StopDeepSync()` for manual control
- `autoStart` config flag for automatic startup
- `IsRunning()` to check connection state
- Automatic reconnection with exponential backoff

**Wearable Tracking**
- Support for any wearable ID (no restrictions)
- Optional ID filtering via `wearableIds` config
- Heartrate monitoring (~10Hz)
- LED color control via `SendColorCommand()`
- 2-second timeout (synced with server)
- Unique session ID assignment

**Events**
- `OnWearableConnected` - new wearable detected
- `OnWearableLost` - wearable timeout
- `OnWearableUpdated` - each data update
- `OnConnectionStatusChanged` - connection state changes

**Configuration (AefConfig.ini)**
- `autoStart` - auto-start on subsystem init
- `deepSyncIp` - server IP address
- `deepSyncReceiverPort` / `deepSyncSenderPort`
- `wearableIds` - optional ID filter (empty = allow all)
- `wearableLostTimeout` - timeout seconds
- Reconnection parameters
- 7 logging flags

**Types (Aef prefix)**
- `FAefDeepSyncColor` - RGB LED color
- `FAefDeepSyncWearableData` - complete wearable state
- `EAefDeepSyncConnectionStatus` - connection state enum
- `FAefDeepSyncConfig` - Blueprint-accessible config

**Documentation**
- README.md with quick start
- DOCUMENTATION.md with full API reference
- CHANGELOG.md (this file)
- Inline Doxygen comments

---

*Copyright (c) Ars Electronica Futurelab, 2025*
