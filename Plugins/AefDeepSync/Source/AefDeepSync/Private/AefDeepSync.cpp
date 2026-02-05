/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Plugin Module Implementation
========================================================================*/

#include "AefDeepSync.h"

#define LOCTEXT_NAMESPACE "FAefDeepSyncModule"

void FAefDeepSyncModule::StartupModule()
{
	// Module loaded. UDeepSyncSubsystem is auto-created by GameInstance.
}

void FAefDeepSyncModule::ShutdownModule()
{
	// Module cleanup. UDeepSyncSubsystem is auto-destroyed by GameInstance.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAefDeepSyncModule, AefDeepSync)