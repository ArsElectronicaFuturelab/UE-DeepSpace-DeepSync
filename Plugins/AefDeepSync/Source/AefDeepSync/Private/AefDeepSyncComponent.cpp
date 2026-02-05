/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Wearable Component Implementation

   See AefDeepSyncComponent.h for class documentation.
========================================================================*/

#include "AefDeepSyncComponent.h"
#include "AefDeepSyncSubsystem.h"
#include "Engine/GameInstance.h"

UAefDeepSyncComponent::UAefDeepSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Every frame for responsive updates
}

//--------------------------------------------------------------------------------
// UActorComponent Interface
//--------------------------------------------------------------------------------

void UAefDeepSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoConnect)
	{
		BindToSubsystem();
	}
}

void UAefDeepSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromSubsystem();
	Super::EndPlay(EndPlayReason);
}

void UAefDeepSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update data from subsystem each tick for responsive editor display
	if (bWasBound)
	{
		RefreshWearableData();
	}
}

//--------------------------------------------------------------------------------
// Blueprint Functions
//--------------------------------------------------------------------------------

void UAefDeepSyncComponent::BindToSubsystem()
{
	if (bWasBound)
	{
		return; // Already bound
	}

	UAefDeepSyncSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("UAefDeepSyncComponent: Could not find UAefDeepSyncSubsystem"));
		return;
	}

	// Bind to subsystem events
	Subsystem->OnWearableConnected.AddDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableConnected);
	Subsystem->OnWearableLost.AddDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableLost);
	Subsystem->OnWearableUpdated.AddDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableUpdated);
	Subsystem->OnConnectionStatusChanged.AddDynamic(this, &UAefDeepSyncComponent::HandleSubsystemConnectionStatusChanged);

	bWasBound = true;

	// Get initial state
	SubsystemStatus = Subsystem->GetConnectionStatus();

	// Check if wearable is already active
	FAefDeepSyncWearableData InitialData;
	if (Subsystem->GetWearableById(WearableId, InitialData))
	{
		UpdateWearableData(InitialData);
		if (!bIsWearableConnected)
		{
			bIsWearableConnected = true;
			OnWearableConnectionChanged.Broadcast(true);
		}
	}

	UE_LOG(LogAefDeepSync, Log, TEXT("UAefDeepSyncComponent: Bound to subsystem, tracking WearableId=%d"), WearableId);
}

void UAefDeepSyncComponent::UnbindFromSubsystem()
{
	if (!bWasBound)
	{
		return;
	}

	UAefDeepSyncSubsystem* Subsystem = GetSubsystem();
	if (Subsystem)
	{
		Subsystem->OnWearableConnected.RemoveDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableConnected);
		Subsystem->OnWearableLost.RemoveDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableLost);
		Subsystem->OnWearableUpdated.RemoveDynamic(this, &UAefDeepSyncComponent::HandleSubsystemWearableUpdated);
		Subsystem->OnConnectionStatusChanged.RemoveDynamic(this, &UAefDeepSyncComponent::HandleSubsystemConnectionStatusChanged);
	}

	bWasBound = false;

	UE_LOG(LogAefDeepSync, Log, TEXT("UAefDeepSyncComponent: Unbound from subsystem"));
}

void UAefDeepSyncComponent::RefreshWearableData()
{
	UAefDeepSyncSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem)
	{
		return;
	}

	// Update subsystem status
	SubsystemStatus = Subsystem->GetConnectionStatus();

	// Get current wearable data
	FAefDeepSyncWearableData CurrentData;
	bool bNowConnected = Subsystem->GetWearableById(WearableId, CurrentData);

	if (bNowConnected)
	{
		UpdateWearableData(CurrentData);
	}
	else
	{
		// Update time since last update even when disconnected
		if (LastUpdateWorldTime > 0)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				TimeSinceLastUpdate = static_cast<float>(World->GetTimeSeconds() - LastUpdateWorldTime);
			}
		}
	}

	// Handle connection state change
	if (bNowConnected != bIsWearableConnected)
	{
		bIsWearableConnected = bNowConnected;
		OnWearableConnectionChanged.Broadcast(bNowConnected);
	}
}

bool UAefDeepSyncComponent::SendColorCommand(FLinearColor InColor)
{
	UAefDeepSyncSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("UAefDeepSyncComponent: Cannot send color command - no subsystem"));
		return false;
	}

	return Subsystem->SendColorCommandLinear(WearableId, InColor);
}

//--------------------------------------------------------------------------------
// Event Handlers (from Subsystem)
//--------------------------------------------------------------------------------

void UAefDeepSyncComponent::HandleSubsystemWearableConnected(const FAefDeepSyncWearableData& Data)
{
	// Filter for our WearableId
	if (Data.WearableId != WearableId)
	{
		return;
	}

	UE_LOG(LogAefDeepSync, Log, TEXT("UAefDeepSyncComponent: Wearable %d connected"), WearableId);

	UpdateWearableData(Data);

	if (!bIsWearableConnected)
	{
		bIsWearableConnected = true;
		OnWearableConnectionChanged.Broadcast(true);
	}
}

void UAefDeepSyncComponent::HandleSubsystemWearableLost(const FAefDeepSyncWearableData& Data)
{
	// Filter for our WearableId
	if (Data.WearableId != WearableId)
	{
		return;
	}

	UE_LOG(LogAefDeepSync, Log, TEXT("UAefDeepSyncComponent: Wearable %d lost"), WearableId);

	if (bIsWearableConnected)
	{
		bIsWearableConnected = false;
		OnWearableConnectionChanged.Broadcast(false);
	}
}

void UAefDeepSyncComponent::HandleSubsystemWearableUpdated(int32 Id, const FAefDeepSyncWearableData& Data)
{
	// Filter for our WearableId
	if (Id != WearableId)
	{
		return;
	}

	UpdateWearableData(Data);

	// Forward the update event
	OnWearableDataUpdated.Broadcast(Id, Data);
}

void UAefDeepSyncComponent::HandleSubsystemConnectionStatusChanged(EAefDeepSyncConnectionStatus Status)
{
	SubsystemStatus = Status;
}

//--------------------------------------------------------------------------------
// Internal Helpers
//--------------------------------------------------------------------------------

void UAefDeepSyncComponent::UpdateWearableData(const FAefDeepSyncWearableData& NewData)
{
	// Detect changes before updating
	DetectAndFireChangeEvents(NewData);

	// Update individual display properties
	UniqueId = NewData.UniqueId;
	HeartRate = NewData.HeartRate;
	Color = NewData.Color.ToLinearColor();
	Timestamp = NewData.Timestamp;
	TimeSinceLastUpdate = NewData.TimeSinceLastUpdate;
	LastUpdateWorldTime = NewData.LastUpdateWorldTime;

	// Store last known values for next comparison
	LastHeartRate = NewData.HeartRate;
	LastColor = NewData.Color.ToLinearColor();
}

void UAefDeepSyncComponent::DetectAndFireChangeEvents(const FAefDeepSyncWearableData& NewData)
{
	// Check HeartRate change
	if (NewData.HeartRate != LastHeartRate)
	{
		OnHeartRateChanged.Broadcast(LastHeartRate, NewData.HeartRate);
	}

	// Check Color change - compare as FLinearColor
	FLinearColor NewLinearColor = NewData.Color.ToLinearColor();
	if (!NewLinearColor.Equals(LastColor))
	{
		OnColorChanged.Broadcast(LastColor, NewLinearColor);
	}
}

UAefDeepSyncSubsystem* UAefDeepSyncComponent::GetSubsystem()
{
	// Use cached reference if valid
	if (CachedSubsystem.IsValid())
	{
		return CachedSubsystem.Get();
	}

	// Get GameInstance subsystem
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	UAefDeepSyncSubsystem* Subsystem = GameInstance->GetSubsystem<UAefDeepSyncSubsystem>();
	CachedSubsystem = Subsystem;
	return Subsystem;
}
