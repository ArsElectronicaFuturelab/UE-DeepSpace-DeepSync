/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Pharus Sync Zone Actor

   Floor actor for synchronizing Pharus tracks with DeepSync wearables.
   Place on floor where visitors should sync their wearable.

   USAGE:
   1. Place actor in level at desired sync location
   2. Set WearableId to match the physical wearable for this zone
   3. Adjust ZoneColor for visual feedback
   4. Bind to OnSyncCompleted in Blueprint to set wearable color

   See DOCUMENTATION.md for detailed examples.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AefPharusSyncTypes.h"
#include "AefPharusDeepSyncZoneActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UAefDeepSyncSubsystem;
class UAefPharusSubsystem;

/**
 * Sync Zone Actor for Pharus-DeepSync Integration
 *
 * Place on floor to create a sync zone. When a Pharus-tracked person
 * enters the zone and stays for SyncDuration seconds, their TrackID
 * is linked to the configured WearableId.
 *
 * 1:1:1 Mapping: 1 Zone = 1 WearableId = 1 Person at a time
 */
UCLASS(Blueprintable, meta = (DisplayName = "AEF Pharus DeepSync Zone"))
class AEFDEEPSYNC_API AAefPharusDeepSyncZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AAefPharusDeepSyncZoneActor();

	//--------------------------------------------------------------------------------
	// Configuration
	//--------------------------------------------------------------------------------

	/** WearableId for this sync zone (must match physical device) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone", meta = (ClampMin = "0"))
	int32 WearableId = 0;

	/** Visual color of the sync zone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone")
	FLinearColor ZoneColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green

	/** Seconds person must stay in zone to complete sync */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float SyncDuration = 5.0f;

	/** Radius of the sync zone trigger (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone", meta = (ClampMin = "10.0"))
	float ZoneRadius = 100.0f;

	/** Automatically activate zone on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone")
	bool bAutoActivate = true;

	/** Show debug visualization in game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AEF|DeepSync|Zone|Debug")
	bool bShowDebugInfo = false;

	//--------------------------------------------------------------------------------
	// Debug Display (Read-Only in Details Panel)
	//--------------------------------------------------------------------------------

	/** Current sync progress (0.0 to 1.0) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Zone|Status")
	float CurrentSyncProgress = 0.0f;

	/** Remaining seconds until sync completes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Zone|Status")
	float SyncTimeRemaining = 0.0f;

	/** TrackID currently in zone (-1 = empty) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Zone|Status")
	int32 CurrentPharusTrackID = -1;

	/** Is a sync currently in progress? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Zone|Status")
	bool bIsSyncing = false;

	/** Is this zone currently active? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AEF|DeepSync|Zone|Status")
	bool bIsActive = false;

	//--------------------------------------------------------------------------------
	// Events
	//--------------------------------------------------------------------------------

	/** Fired when a Pharus track enters the zone and sync begins */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncStarted OnSyncStarted;

	/** Fired every tick during sync (for progress bars, etc.) */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncing OnSyncing;

	/** Fired when sync completes (success or failure) */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncCompleted OnSyncCompleted;

	/** Fired when person leaves zone before sync completes */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncCancelled OnSyncCancelled;

	/** Fired if wearable connection is lost during sync */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncWearableLost OnWearableLost;

	/** Fired if Pharus track is lost during sync */
	UPROPERTY(BlueprintAssignable, Category = "AEF|DeepSync|Zone|Events")
	FAefOnSyncPharusTrackLost OnPharusTrackLost;

	//--------------------------------------------------------------------------------
	// Blueprint Functions
	//--------------------------------------------------------------------------------

	/** Activate the sync zone */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Zone")
	void ActivateZone();

	/** Deactivate the sync zone */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Zone")
	void DeactivateZone();

	/** Cancel any ongoing sync */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Zone")
	void CancelSync();

	/** Get current sync progress (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Zone")
	float GetSyncProgress() const { return CurrentSyncProgress; }

	/** Check if sync is in progress */
	UFUNCTION(BlueprintPure, Category = "AEF|DeepSync|Zone")
	bool IsSyncing() const { return bIsSyncing; }

	/** Update zone color at runtime */
	UFUNCTION(BlueprintCallable, Category = "AEF|DeepSync|Zone")
	void SetZoneColor(FLinearColor NewColor);

protected:
	//--------------------------------------------------------------------------------
	// AActor Interface
	//--------------------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//--------------------------------------------------------------------------------
	// Components
	//--------------------------------------------------------------------------------

	/** Trigger sphere for overlap detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	/** Visual mesh for the zone */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

private:
	//--------------------------------------------------------------------------------
	// Internal State
	//--------------------------------------------------------------------------------

	/** Dynamic material for color changes */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	/** Cached subsystem references */
	TWeakObjectPtr<UAefDeepSyncSubsystem> DeepSyncSubsystem;
	TWeakObjectPtr<UAefPharusSubsystem> PharusSubsystem;

	/** Sync timer */
	float SyncElapsedTime = 0.0f;

	/** Actor currently in zone (for validation) */
	TWeakObjectPtr<AActor> OverlappingActor;

	//--------------------------------------------------------------------------------
	// Overlap Handlers
	//--------------------------------------------------------------------------------

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//--------------------------------------------------------------------------------
	// Internal Helpers
	//--------------------------------------------------------------------------------

	void SetupComponents();
	void UpdateMaterialColor();
	void StartSync(int32 TrackID, AActor* PharusActor);
	void CompleteSync();
	void FailSync(EAefPharusSyncStatus Status, const FString& Error);
	bool ValidatePharusActor(AActor* Actor, int32& OutTrackID);
	UAefDeepSyncSubsystem* GetDeepSyncSubsystem();
	UAefPharusSubsystem* GetPharusSubsystem();
};

