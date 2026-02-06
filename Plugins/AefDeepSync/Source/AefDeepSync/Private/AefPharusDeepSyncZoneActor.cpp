/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Pharus Sync Zone Actor Implementation
========================================================================*/

#include "AefPharusDeepSyncZoneActor.h"
#include "AefDeepSyncSubsystem.h"
#include "AefPharusSubsystem.h"
#include "AefPharusActorInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAefPharusSync, Log, All);

AAefPharusDeepSyncZoneActor::AAefPharusDeepSyncZoneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Create root trigger sphere
	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetSphereRadius(ZoneRadius);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
	TriggerSphere->SetHiddenInGame(true);
	RootComponent = TriggerSphere;

	// Create visual mesh (cylinder)
	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetupAttachment(RootComponent);
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Try to load default cylinder mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void AAefPharusDeepSyncZoneActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap events
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AAefPharusDeepSyncZoneActor::OnTriggerBeginOverlap);
	TriggerSphere->OnComponentEndOverlap.AddDynamic(this, &AAefPharusDeepSyncZoneActor::OnTriggerEndOverlap);

	// Setup visuals
	SetupComponents();
	UpdateMaterialColor();

	// Register with subsystem
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		Subsystem->RegisterZone(this);
	}

	// Auto-activate if configured
	if (bAutoActivate)
	{
		ActivateZone();
	}

	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d] initialized at %s"), 
		WearableId, *GetActorLocation().ToString());
}

void AAefPharusDeepSyncZoneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister from subsystem
	if (UAefDeepSyncSubsystem* Subsystem = GetDeepSyncSubsystem())
	{
		Subsystem->UnregisterZone(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AAefPharusDeepSyncZoneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive || !bIsSyncing)
	{
		return;
	}

	// Check if Pharus track is still valid
	if (CurrentPharusTrackID >= 0)
	{
		UAefPharusSubsystem* PharusSub = GetPharusSubsystem();
		if (PharusSub)
		{
			// Check if track still exists via the subsystem
			// Note: We rely on EndOverlap for track loss, but also check wearable
		}
	}

	// Check if wearable is still connected
	UAefDeepSyncSubsystem* DeepSyncSub = GetDeepSyncSubsystem();
	if (DeepSyncSub && !DeepSyncSub->IsWearableActive(WearableId))
	{
		UE_LOG(LogAefPharusSync, Warning, TEXT("SyncZone [WearableId=%d]: Wearable lost during sync!"), WearableId);
		OnWearableLost.Broadcast(WearableId);
		FailSync(EAefPharusSyncStatus::Failed, TEXT("Wearable connection lost"));
		return;
	}

	// Update sync progress
	SyncElapsedTime += DeltaTime;
	CurrentSyncProgress = FMath::Clamp(SyncElapsedTime / SyncDuration, 0.0f, 1.0f);
	SyncTimeRemaining = FMath::Max(0.0f, SyncDuration - SyncElapsedTime);

	// Broadcast progress
	OnSyncing.Broadcast(CurrentPharusTrackID, CurrentSyncProgress);

	if (bShowDebugInfo)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
			FString::Printf(TEXT("Sync [%d -> %d]: %.0f%% (%.1fs remaining)"),
				CurrentPharusTrackID, WearableId, CurrentSyncProgress * 100.0f, SyncTimeRemaining));
	}

	// Check if sync is complete
	if (SyncElapsedTime >= SyncDuration)
	{
		CompleteSync();
	}
}

void AAefPharusDeepSyncZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupComponents();
	UpdateMaterialColor();
}

#if WITH_EDITOR
void AAefPharusDeepSyncZoneActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) 
		? PropertyChangedEvent.Property->GetFName() 
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AAefPharusDeepSyncZoneActor, ZoneColor))
	{
		UpdateMaterialColor();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AAefPharusDeepSyncZoneActor, ZoneRadius))
	{
		SetupComponents();
	}
}
#endif

void AAefPharusDeepSyncZoneActor::SetupComponents()
{
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(ZoneRadius);
	}

	if (ZoneMesh)
	{
		// Scale cylinder to match zone radius
		// Default cylinder is 100 units diameter, 100 units tall
		float Scale = ZoneRadius / 50.0f;  // Diameter = 100, so radius = 50
		ZoneMesh->SetRelativeScale3D(FVector(Scale, Scale, 0.05f));  // Flat cylinder
		ZoneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));  // Slightly above ground
	}
}

void AAefPharusDeepSyncZoneActor::UpdateMaterialColor()
{
	if (!ZoneMesh)
	{
		return;
	}

	if (!DynamicMaterial)
	{
		UMaterialInterface* BaseMaterial = ZoneMesh->GetMaterial(0);
		if (BaseMaterial)
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			ZoneMesh->SetMaterial(0, DynamicMaterial);
		}
	}

	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ZoneColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ZoneColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), ZoneColor * 0.5f);
	}
}

void AAefPharusDeepSyncZoneActor::ActivateZone()
{
	bIsActive = true;
	TriggerSphere->SetGenerateOverlapEvents(true);
	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d] activated"), WearableId);
}

void AAefPharusDeepSyncZoneActor::DeactivateZone()
{
	if (bIsSyncing)
	{
		CancelSync();
	}
	bIsActive = false;
	TriggerSphere->SetGenerateOverlapEvents(false);
	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d] deactivated"), WearableId);
}

void AAefPharusDeepSyncZoneActor::CancelSync()
{
	if (!bIsSyncing)
	{
		return;
	}

	int32 CancelledTrackID = CurrentPharusTrackID;
	
	bIsSyncing = false;
	CurrentSyncProgress = 0.0f;
	SyncTimeRemaining = 0.0f;
	SyncElapsedTime = 0.0f;
	CurrentPharusTrackID = -1;
	OverlappingActor.Reset();

	OnSyncCancelled.Broadcast(CancelledTrackID);
	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Sync cancelled for TrackID=%d"), 
		WearableId, CancelledTrackID);
}

void AAefPharusDeepSyncZoneActor::SetZoneColor(FLinearColor NewColor)
{
	ZoneColor = NewColor;
	UpdateMaterialColor();
}

void AAefPharusDeepSyncZoneActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive || bIsSyncing || !OtherActor)
	{
		return;
	}

	int32 TrackID = -1;
	if (ValidatePharusActor(OtherActor, TrackID))
	{
		StartSync(TrackID, OtherActor);
	}
}

void AAefPharusDeepSyncZoneActor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bIsSyncing || !OtherActor)
	{
		return;
	}

	// Check if this is the actor that was syncing
	if (OverlappingActor.IsValid() && OverlappingActor.Get() == OtherActor)
	{
		UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Pharus actor left zone, cancelling sync"), WearableId);
		CancelSync();
	}
}

void AAefPharusDeepSyncZoneActor::StartSync(int32 TrackID, AActor* PharusActor)
{
	UAefDeepSyncSubsystem* DeepSyncSub = GetDeepSyncSubsystem();
	if (!DeepSyncSub)
	{
		UE_LOG(LogAefPharusSync, Warning, TEXT("SyncZone [WearableId=%d]: DeepSync subsystem not available"), WearableId);
		return;
	}

	// Blocking checks - cannot sync if already linked
	if (DeepSyncSub->IsZoneBlocked(this))
	{
		UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Zone is blocked (already synced)"), WearableId);
		return;
	}

	if (DeepSyncSub->IsPharusTrackBlocked(TrackID))
	{
		UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: TrackID=%d is blocked (already synced)"), WearableId, TrackID);
		return;
	}

	if (DeepSyncSub->IsWearableBlocked(WearableId))
	{
		UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Wearable is blocked (already synced)"), WearableId);
		return;
	}

	// Check if wearable is available
	if (!DeepSyncSub->IsWearableActive(WearableId))
	{
		UE_LOG(LogAefPharusSync, Warning, TEXT("SyncZone [WearableId=%d]: Wearable not active, cannot start sync"), WearableId);
		OnWearableLost.Broadcast(WearableId);
		return;
	}

	// Start sync
	bIsSyncing = true;
	CurrentPharusTrackID = TrackID;
	SyncElapsedTime = 0.0f;
	CurrentSyncProgress = 0.0f;
	SyncTimeRemaining = SyncDuration;
	OverlappingActor = PharusActor;

	OnSyncStarted.Broadcast(TrackID);
	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Sync started for TrackID=%d (%.1fs duration)"), 
		WearableId, TrackID, SyncDuration);
}

void AAefPharusDeepSyncZoneActor::CompleteSync()
{
	UAefDeepSyncSubsystem* DeepSyncSub = GetDeepSyncSubsystem();
	if (!DeepSyncSub)
	{
		FailSync(EAefPharusSyncStatus::Failed, TEXT("DeepSync subsystem unavailable"));
		return;
	}

	FAefDeepSyncWearableData WearableData;
	if (!DeepSyncSub->GetWearableById(WearableId, WearableData))
	{
		FailSync(EAefPharusSyncStatus::Failed, TEXT("Wearable data not found"));
		return;
	}

	// Get Pharus position if actor still valid
	FVector PharusPosition = FVector::ZeroVector;
	if (OverlappingActor.IsValid())
	{
		PharusPosition = OverlappingActor->GetActorLocation();
	}

	// Build success result
	FAefPharusSyncResult Result = FAefPharusSyncResult::MakeSuccess(
		CurrentPharusTrackID,
		WearableId,
		WearableData,
		ZoneColor,
		SyncElapsedTime
	);
	Result.PharusPosition = PharusPosition;

	// Reset state
	int32 CompletedTrackID = CurrentPharusTrackID;
	bIsSyncing = false;
	CurrentSyncProgress = 1.0f;
	SyncTimeRemaining = 0.0f;
	SyncElapsedTime = 0.0f;
	AActor* CachedPharusActor = OverlappingActor.Get();
	CurrentPharusTrackID = -1;
	OverlappingActor.Reset();

	// Notify subsystem to create link
	DeepSyncSub->NotifySyncCompleted(Result, this, CachedPharusActor);

	OnSyncCompleted.Broadcast(Result);
	UE_LOG(LogAefPharusSync, Log, TEXT("SyncZone [WearableId=%d]: Sync COMPLETED for TrackID=%d, HR=%d"), 
		WearableId, CompletedTrackID, WearableData.HeartRate);
}

void AAefPharusDeepSyncZoneActor::FailSync(EAefPharusSyncStatus Status, const FString& Error)
{
	FAefPharusSyncResult Result = FAefPharusSyncResult::MakeFailure(
		Status,
		Error,
		CurrentPharusTrackID,
		WearableId
	);

	int32 FailedTrackID = CurrentPharusTrackID;
	bIsSyncing = false;
	CurrentSyncProgress = 0.0f;
	SyncTimeRemaining = 0.0f;
	SyncElapsedTime = 0.0f;
	CurrentPharusTrackID = -1;
	OverlappingActor.Reset();

	OnSyncCompleted.Broadcast(Result);
	UE_LOG(LogAefPharusSync, Warning, TEXT("SyncZone [WearableId=%d]: Sync FAILED for TrackID=%d: %s"), 
		WearableId, FailedTrackID, *Error);
}

bool AAefPharusDeepSyncZoneActor::ValidatePharusActor(AActor* Actor, int32& OutTrackID)
{
	if (!Actor)
	{
		return false;
	}

	// Check if actor implements IAefPharusActorInterface
	// We need to cast to AAefPharusActor to get the TrackID
	// The interface doesn't have a GetActorTrackID method, but AAefPharusActor has GetTrackID()
	
	// First check if it's directly an AAefPharusActor (or derived)
	// We use the interface check to be more generic
	if (Actor->GetClass()->ImplementsInterface(UAefPharusActorInterface::StaticClass()))
	{
		// Try to get TrackID via reflection (AAefPharusActor stores it as a UPROPERTY)
		// This works because AAefPharusActor has a BlueprintReadOnly TrackID property
		
		// Use FindPropertyByName to get the track ID
		if (FProperty* TrackIDProp = Actor->GetClass()->FindPropertyByName(TEXT("TrackID")))
		{
			if (const FIntProperty* IntProp = CastField<FIntProperty>(TrackIDProp))
			{
				OutTrackID = IntProp->GetPropertyValue_InContainer(Actor);
				return OutTrackID >= 0;
			}
		}
		
		// Fallback: Try to call GetTrackID function if it exists
		if (UFunction* GetTrackIDFunc = Actor->FindFunction(TEXT("GetTrackID")))
		{
			struct { int32 ReturnValue; } Params;
			Actor->ProcessEvent(GetTrackIDFunc, &Params);
			OutTrackID = Params.ReturnValue;
			return OutTrackID >= 0;
		}
	}

	return false;
}

UAefDeepSyncSubsystem* AAefPharusDeepSyncZoneActor::GetDeepSyncSubsystem()
{
	if (!DeepSyncSubsystem.IsValid())
	{
		UGameInstance* GI = GetGameInstance();
		if (GI)
		{
			DeepSyncSubsystem = GI->GetSubsystem<UAefDeepSyncSubsystem>();
		}
	}
	return DeepSyncSubsystem.Get();
}

UAefPharusSubsystem* AAefPharusDeepSyncZoneActor::GetPharusSubsystem()
{
	if (!PharusSubsystem.IsValid())
	{
		UGameInstance* GI = GetGameInstance();
		if (GI)
		{
			PharusSubsystem = GI->GetSubsystem<UAefPharusSubsystem>();
		}
	}
	return PharusSubsystem.Get();
}
