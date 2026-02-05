# Changelog

All notable changes to AefDeepSync will be documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
- 5 logging flags

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
