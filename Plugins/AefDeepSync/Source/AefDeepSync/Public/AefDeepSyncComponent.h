/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Wearable Component

   Actor component for per-actor wearable tracking.
   Attach to any actor to receive data from a specific wearable ID.

   FEATURES:
   - Configurable WearableId binding
   - Live data display in Editor Details panel
   - Dedicated events for HeartRate and Color changes
   - Convenience functions for common operations

   See DOCUMENTATION.md for detailed usage examples.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AefDeepSyncTypes.h"
#include "AefDeepSyncComponent.generated.h"

class UAefDeepSyncSubsystem;

//--------------------------------------------------------------------------------
// Component-Specific Delegates
//--------------------------------------------------------------------------------

/** Fired when HeartRate value changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnHeartRateChanged, int32, OldHeartRate, int32, NewHeartRate);

/** Fired when LED Color changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnColorChanged, FLinearColor, OldColor, FLinearColor, NewColor);

/** Fired when wearable connection state changes for this component */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAefOnWearableConnectionChanged, bool, bIsConnected);

/** Fired on every data update from wearable (forwarded from subsystem) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAefOnWearableDataUpdated, int32, WearableId, const FAefDeepSyncWearableData&, WearableData);

/**
 * DeepSync Wearable Component
 *
 * Attach to any actor to track a specific wearable device.
 * The component binds to UAefDeepSyncSubsystem and filters events
 * for the configured WearableId.
 *
 * USAGE:
 * 1. Add component to any actor via "Add Component" menu
 * 2. Set WearableId to match the physical device
 * 3. Access WearableData for live values in Details panel
 * 4. Bind to OnHeartRateChanged/OnColorChanged for real-time events
 */
UCLASS(ClassGroup=(AEF), meta=(BlueprintSpawnableComponent, DisplayName="AEF DeepSync Wearable"))
class AEFDEEPSYNC_API UAefDeepSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAefDeepSyncComponent();

	//--------------------------------------------------------------------------------
	// Configuration
	//--------------------------------------------------------------------------------

	/**
	 * Wearable ID to track
	 * Must match the ID configured on the physical wearable device.
	 * Can be any positive integer (supports IDs like 1, 2, 1001, etc.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync", meta = (ClampMin = "0"))
	int32 WearableId = 0;

	/**
	 * Automatically bind to subsystem on BeginPlay
	 * Disable for manual control via BindToSubsystem()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync")
	bool bAutoConnect = true;

	//--------------------------------------------------------------------------------
	// Live Data (Read-Only in Editor)
	//--------------------------------------------------------------------------------

	/** Session-unique ID assigned by subsystem */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Live Data")
	int32 UniqueId = -1;

	/** Current heart rate in BPM (0 = no reading) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Live Data")
	int32 HeartRate = 0;

	/** Current LED color on the wearable device */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Live Data")
	FLinearColor Color = FLinearColor::Black;

	/** Server timestamp (milliseconds since server start) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Live Data")
	int32 Timestamp = 0;

	/** Time since last data update (seconds) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Live Data")
	float TimeSinceLastUpdate = 0.0f;

	//--------------------------------------------------------------------------------
	// Status
	//--------------------------------------------------------------------------------

	/**
	 * Is the wearable currently connected?
	 * True when the subsystem is receiving data for this WearableId.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Status")
	bool bIsWearableConnected = false;

	/**
	 * Current subsystem connection status
	 * Shows whether the subsystem itself is connected to the server.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Status")
	EAefDeepSyncConnectionStatus SubsystemStatus = EAefDeepSyncConnectionStatus::Disconnected;

	//--------------------------------------------------------------------------------
	// Events - Data Changes
	//--------------------------------------------------------------------------------

	/**
	 * Fired when HeartRate value changes
	 * Provides both old and new values for comparison.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnHeartRateChanged OnHeartRateChanged;

	/**
	 * Fired when LED Color changes
	 * Provides both old and new colors for comparison.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnColorChanged OnColorChanged;

	//--------------------------------------------------------------------------------
	// Events - Connection State
	//--------------------------------------------------------------------------------

	/**
	 * Fired when this wearable connects (data starts arriving)
	 */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableConnectionChanged OnWearableConnectionChanged;

	/**
	 * Fired on every data update from the wearable
	 * Note: This fires at high frequency! Use OnHeartRateChanged/OnColorChanged
	 * for change-only notifications.
	 */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Events")
	FAefOnWearableDataUpdated OnWearableDataUpdated;

	//--------------------------------------------------------------------------------
	// Blueprint Functions
	//--------------------------------------------------------------------------------

	/**
	 * Manually bind to subsystem
	 * Called automatically if bAutoConnect is true.
	 */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync")
	void BindToSubsystem();

	/**
	 * Unbind from subsystem
	 * Stops receiving updates for this component.
	 */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync")
	void UnbindFromSubsystem();

	/**
	 * Manually refresh data from subsystem
	 * Normally not needed as data updates automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync")
	void RefreshWearableData();

	/**
	 * Send color command to this wearable
	 * @param InColor RGB color to set on the device LED
	 * @return True if command was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Commands")
	bool SendColorCommand(FLinearColor InColor);

	//--------------------------------------------------------------------------------
	// Convenience Getters
	//--------------------------------------------------------------------------------

	/** Get current heart rate (0 if no reading or disconnected) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	int32 GetHeartRate() const { return HeartRate; }

	/** Get current LED color */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	FLinearColor GetColor() const { return Color; }

	/** Get seconds since last data update */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	float GetTimeSinceLastUpdate() const { return TimeSinceLastUpdate; }

	/** Check if wearable data is valid and fresh */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync")
	bool IsWearableDataValid() const { return bIsWearableConnected && UniqueId >= 0; }

protected:
	//--------------------------------------------------------------------------------
	// UActorComponent Interface
	//--------------------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	//--------------------------------------------------------------------------------
	// Internal State
	//--------------------------------------------------------------------------------

	/** Cached reference to subsystem */
	UPROPERTY(Transient)
	TWeakObjectPtr<UAefDeepSyncSubsystem> CachedSubsystem;

	/** Previous values for change detection */
	int32 LastHeartRate = 0;
	FLinearColor LastColor = FLinearColor::Black;
	double LastUpdateWorldTime = 0.0;
	bool bWasBound = false;

	//--------------------------------------------------------------------------------
	// Event Handlers (from Subsystem)
	//--------------------------------------------------------------------------------

	UFUNCTION()
	void HandleSubsystemWearableConnected(const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleSubsystemWearableLost(const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleSubsystemWearableUpdated(int32 Id, const FAefDeepSyncWearableData& Data);

	UFUNCTION()
	void HandleSubsystemConnectionStatusChanged(EAefDeepSyncConnectionStatus Status);

	//--------------------------------------------------------------------------------
	// Internal Helpers
	//--------------------------------------------------------------------------------

	/** Update local data and fire change events if needed */
	void UpdateWearableData(const FAefDeepSyncWearableData& NewData);

	/** Check for property changes and fire appropriate events */
	void DetectAndFireChangeEvents(const FAefDeepSyncWearableData& NewData);

	/** Get subsystem reference (cached) */
	UAefDeepSyncSubsystem* GetSubsystem();
};
