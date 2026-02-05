/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Plugin Module

   Wearable heartrate tracking plugin for Unreal Engine.
   See README.md and DOCUMENTATION.md for usage.
========================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * AefDeepSync Module
 *
 * Plugin module for wearable device tracking.
 * Main API is provided by UDeepSyncSubsystem (GameInstance subsystem).
 */
class FAefDeepSyncModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
