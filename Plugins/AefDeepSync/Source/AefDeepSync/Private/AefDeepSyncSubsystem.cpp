/*========================================================================
   Copyright (c) Ars Electronica Futurelab, 2025

   AefDeepSync - Wearable Subsystem Implementation
========================================================================*/

#include "AefDeepSyncSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogAefDeepSync);

//--------------------------------------------------------------------------------
// USubsystem Interface
//--------------------------------------------------------------------------------

void UAefDeepSyncSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfiguration();

	if (Config.bLogConnections)
	{
		UE_LOG(LogAefDeepSync, Log, TEXT("AefDeepSync initialized (AutoStart=%s)"),
			Config.bAutoStart ? TEXT("true") : TEXT("false"));
	}

	if (Config.bAutoStart)
	{
		StartDeepSync();
	}
}

void UAefDeepSyncSubsystem::Deinitialize()
{
	StopDeepSync();
	if (Config.bLogConnections)
	{
		UE_LOG(LogAefDeepSync, Log, TEXT("AefDeepSync deinitialized"));
	}
	Super::Deinitialize();
}

//--------------------------------------------------------------------------------
// FTickableGameObject Interface
//--------------------------------------------------------------------------------

void UAefDeepSyncSubsystem::Tick(float DeltaTime)
{
	if (!bWantsToRun) return;

	// Handle reconnection
	if (ConnectionStatus == EAefDeepSyncConnectionStatus::Reconnecting)
	{
		ReconnectTimer -= DeltaTime;
		if (ReconnectTimer <= 0.0f)
		{
			if (Config.bLogConnections)
			{
				UE_LOG(LogAefDeepSync, Log, TEXT("Reconnection attempt %d/%d..."),
					ReconnectAttempts + 1, Config.MaxReconnectAttempts);
			}

			if (ConnectToServer())
			{
				SetConnectionStatus(EAefDeepSyncConnectionStatus::Connected);
				ReconnectAttempts = 0;
				CurrentReconnectDelay = Config.ReconnectDelay;
			}
			else
			{
				ReconnectAttempts++;
				if (Config.MaxReconnectAttempts > 0 && ReconnectAttempts >= Config.MaxReconnectAttempts)
				{
					SetConnectionStatus(EAefDeepSyncConnectionStatus::Failed);
					bWantsToRun = false;
					if (Config.bLogNetworkErrors)
					{
						UE_LOG(LogAefDeepSync, Error, TEXT("Max reconnection attempts reached"));
					}
				}
				else
				{
					CurrentReconnectDelay = FMath::Min(CurrentReconnectDelay * 2.0f, MaxReconnectDelay);
					ReconnectTimer = CurrentReconnectDelay;
				}
			}
		}
		return;
	}

	// Process data when connected
	if (ConnectionStatus == EAefDeepSyncConnectionStatus::Connected)
	{
		ProcessReceivedData();
		CheckWearableTimeouts(DeltaTime);
	}
}

bool UAefDeepSyncSubsystem::IsTickable() const
{
	return bWantsToRun;
}

TStatId UAefDeepSyncSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAefDeepSyncSubsystem, STATGROUP_Tickables);
}

//--------------------------------------------------------------------------------
// Connection Management
//--------------------------------------------------------------------------------

void UAefDeepSyncSubsystem::StartDeepSync()
{
	if (bWantsToRun)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("StartDeepSync called but already running"));
		return;
	}

	bWantsToRun = true;
	SetConnectionStatus(EAefDeepSyncConnectionStatus::Connecting);

	if (ConnectToServer())
	{
		SetConnectionStatus(EAefDeepSyncConnectionStatus::Connected);
	}
	else
	{
		SetConnectionStatus(EAefDeepSyncConnectionStatus::Reconnecting);
		ReconnectTimer = Config.ReconnectDelay;
		ReconnectAttempts = 0;
		CurrentReconnectDelay = Config.ReconnectDelay;
	}
}

void UAefDeepSyncSubsystem::StopDeepSync()
{
	bWantsToRun = false;

	// Fire OnWearableLost for all active wearables
	for (const auto& Pair : ActiveWearables)
	{
		if (Config.bLogWearableEvents)
		{
			UE_LOG(LogAefDeepSync, Log, TEXT("Wearable lost (stopped): %s"), *Pair.Value.ToString());
		}
		OnWearableLost.Broadcast(Pair.Value);
	}
	ActiveWearables.Empty();

	DisconnectFromServer();
	SetConnectionStatus(EAefDeepSyncConnectionStatus::Disconnected);
}

bool UAefDeepSyncSubsystem::IsRunning() const
{
	return bWantsToRun && (ConnectionStatus == EAefDeepSyncConnectionStatus::Connected ||
		ConnectionStatus == EAefDeepSyncConnectionStatus::Reconnecting ||
		ConnectionStatus == EAefDeepSyncConnectionStatus::Connecting);
}

bool UAefDeepSyncSubsystem::ConnectToServer()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Socket subsystem unavailable"));
		return false;
	}

	// Create receiver socket address
	TSharedRef<FInternetAddr> ReceiverAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	ReceiverAddr->SetIp(*Config.ServerIP, bIsValid);
	if (!bIsValid)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Invalid IP: %s"), *Config.ServerIP);
		return false;
	}
	ReceiverAddr->SetPort(Config.ReceiverPort);

	// Create receiver socket as BLOCKING for reliable connection
	ReceiverSocket = FTcpSocketBuilder(TEXT("AefDeepSyncReceiver"))
		.AsReusable()
		.Build();
	
	if (!ReceiverSocket)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Failed to create receiver socket"));
		return false;
	}

	// Set non-blocking for the connect attempt, then wait
	ReceiverSocket->SetNonBlocking(true);
	bool bConnectStarted = ReceiverSocket->Connect(*ReceiverAddr);
	
	if (!bConnectStarted)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Failed to initiate connection to %s:%d"), *Config.ServerIP, Config.ReceiverPort);
		DisconnectFromServer();
		return false;
	}

	// Wait for connection to complete (WaitForWrite indicates connection ready)
	if (!ReceiverSocket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(5.0)))
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Connection timeout to %s:%d"), *Config.ServerIP, Config.ReceiverPort);
		DisconnectFromServer();
		return false;
	}

	// Verify connection state
	ESocketConnectionState ConnState = ReceiverSocket->GetConnectionState();
	if (ConnState != ESocketConnectionState::SCS_Connected)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("[Receiver] Connection failed (state=%d)"), static_cast<int32>(ConnState));
		DisconnectFromServer();
		return false;
	}

	if (Config.bLogConnections) UE_LOG(LogAefDeepSync, Log, TEXT("[Receiver] Connected to %s:%d"), *Config.ServerIP, Config.ReceiverPort);

	// Create sender socket address
	TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();
	SenderAddr->SetIp(*Config.ServerIP, bIsValid);
	SenderAddr->SetPort(Config.SenderPort);

	// Create sender socket - use BLOCKING connect for reliable handshake
	SenderSocket = FTcpSocketBuilder(TEXT("AefDeepSyncSender"))
		.AsReusable()
		.AsBlocking()  // Connect in blocking mode
		.Build();
	
	if (!SenderSocket)
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Error, TEXT("Failed to create sender socket"));
		DisconnectFromServer();
		return false;
	}

	// Blocking connect with timeout
	bool bConnectResult = SenderSocket->Connect(*SenderAddr);
	
	if (Config.bLogConnections)
	{
		UE_LOG(LogAefDeepSync, Log, TEXT("[Sender] Connect() to %s:%d returned %s"), 
			*Config.ServerIP, Config.SenderPort, bConnectResult ? TEXT("true") : TEXT("false"));
	}

	if (!bConnectResult)
	{
		if (Config.bLogNetworkErrors) 
		{
			UE_LOG(LogAefDeepSync, Error, TEXT("[Sender] Connection failed to %s:%d"), *Config.ServerIP, Config.SenderPort);
		}
		DisconnectFromServer();
		return false;
	}

	// Verify sender connection state
	ESocketConnectionState SenderConnState = SenderSocket->GetConnectionState();
	if (SenderConnState != ESocketConnectionState::SCS_Connected)
	{
		if (Config.bLogNetworkErrors) 
		{
			UE_LOG(LogAefDeepSync, Error, TEXT("[Sender] Connection not established (state=%d)"), 
				static_cast<int32>(SenderConnState));
		}
		DisconnectFromServer();
		return false;
	}

	// Now switch to non-blocking for send operations
	SenderSocket->SetNonBlocking(true);

	if (Config.bLogConnections)
	{
		UE_LOG(LogAefDeepSync, Log, TEXT("[Sender] Connected to %s:%d"), *Config.ServerIP, Config.SenderPort);
	}
	return true;
}

void UAefDeepSyncSubsystem::DisconnectFromServer()
{
	if (ReceiverSocket)
	{
		ReceiverSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ReceiverSocket);
		ReceiverSocket = nullptr;
	}
	if (SenderSocket)
	{
		SenderSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SenderSocket);
		SenderSocket = nullptr;
	}
	ReceiveBuffer.Empty();
}

void UAefDeepSyncSubsystem::ProcessReceivedData()
{
	if (!ReceiverSocket) return;

	// Check for pending data
	uint32 PendingSize = 0;
	if (!ReceiverSocket->HasPendingData(PendingSize) || PendingSize == 0)
	{
		return; // No data available
	}

	// Data is available, read it
	TArray<uint8> ReceivedData;
	ReceivedData.SetNumUninitialized(4096);
	int32 BytesRead = 0;

	if (!ReceiverSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, ESocketReceiveFlags::None))
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Warning, TEXT("Receive failed"));
		SetConnectionStatus(EAefDeepSyncConnectionStatus::Reconnecting);
		DisconnectFromServer();
		ReconnectTimer = Config.ReconnectDelay;
		return;
	}

	if (BytesRead == 0)
	{
		// Connection closed by server
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Warning, TEXT("Server closed connection"));
		SetConnectionStatus(EAefDeepSyncConnectionStatus::Reconnecting);
		DisconnectFromServer();
		ReconnectTimer = Config.ReconnectDelay;
		return;
	}

	ReceiveBuffer += FString(BytesRead, UTF8_TO_TCHAR(ReceivedData.GetData()));

	if (Config.bLogProtocolDebug) UE_LOG(LogAefDeepSync, Log, TEXT("Received %d bytes: %s"), BytesRead, *ReceiveBuffer);

	// Parse messages (delimiter: 'X')
	int32 DelimiterIndex = 0;
	while (ReceiveBuffer.FindChar(TEXT('X'), DelimiterIndex))
	{
		FString JsonMessage = ReceiveBuffer.Left(DelimiterIndex);
		ReceiveBuffer = ReceiveBuffer.Mid(DelimiterIndex + 1);

		if (JsonMessage.IsEmpty()) continue;

		if (Config.bLogProtocolDebug) UE_LOG(LogAefDeepSync, Log, TEXT("Parsing JSON: %s"), *JsonMessage);

		FAefDeepSyncWearableData WearableData;
		if (ParseWearableMessage(JsonMessage, WearableData) && IsWearableIdAllowed(WearableData.WearableId))
		{
			UpdateWearable(WearableData);
		}
	}
}

bool UAefDeepSyncSubsystem::ParseWearableMessage(const FString& JsonMessage, FAefDeepSyncWearableData& OutData)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonMessage);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		if (Config.bLogNetworkErrors) UE_LOG(LogAefDeepSync, Warning, TEXT("JSON parse failed: %s"), *JsonMessage);
		return false;
	}

	OutData.WearableId = JsonObject->GetIntegerField(TEXT("Id"));
	OutData.HeartRate = JsonObject->GetIntegerField(TEXT("HeartRate"));
	OutData.Timestamp = JsonObject->GetIntegerField(TEXT("Timestamp"));

	const TSharedPtr<FJsonObject>* ColorObject;
	if (JsonObject->TryGetObjectField(TEXT("Color"), ColorObject))
	{
		OutData.Color.R = static_cast<uint8>((*ColorObject)->GetIntegerField(TEXT("R")));
		OutData.Color.G = static_cast<uint8>((*ColorObject)->GetIntegerField(TEXT("G")));
		OutData.Color.B = static_cast<uint8>((*ColorObject)->GetIntegerField(TEXT("B")));
	}
	return true;
}

void UAefDeepSyncSubsystem::SetConnectionStatus(EAefDeepSyncConnectionStatus NewStatus)
{
	if (ConnectionStatus != NewStatus)
	{
		ConnectionStatus = NewStatus;
		if (Config.bLogConnections)
		{
			const TCHAR* StatusName = TEXT("Unknown");
			switch (NewStatus)
			{
				case EAefDeepSyncConnectionStatus::Disconnected: StatusName = TEXT("Disconnected"); break;
				case EAefDeepSyncConnectionStatus::Connecting: StatusName = TEXT("Connecting"); break;
				case EAefDeepSyncConnectionStatus::Connected: StatusName = TEXT("Connected"); break;
				case EAefDeepSyncConnectionStatus::Reconnecting: StatusName = TEXT("Reconnecting"); break;
				case EAefDeepSyncConnectionStatus::Failed: StatusName = TEXT("Failed"); break;
			}
			UE_LOG(LogAefDeepSync, Log, TEXT("Connection status: %s"), StatusName);
		}
		OnConnectionStatusChanged.Broadcast(NewStatus);
	}
}

//--------------------------------------------------------------------------------
// Wearable Management
//--------------------------------------------------------------------------------

TArray<FAefDeepSyncWearableData> UAefDeepSyncSubsystem::GetActiveWearables() const
{
	TArray<FAefDeepSyncWearableData> Result;
	ActiveWearables.GenerateValueArray(Result);
	return Result;
}

bool UAefDeepSyncSubsystem::GetWearableById(int32 WearableId, FAefDeepSyncWearableData& OutData) const
{
	const FAefDeepSyncWearableData* Found = ActiveWearables.Find(WearableId);
	if (Found)
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UAefDeepSyncSubsystem::UpdateWearable(const FAefDeepSyncWearableData& Data)
{
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();

	FAefDeepSyncWearableData* Existing = ActiveWearables.Find(Data.WearableId);
	if (Existing)
	{
		Existing->HeartRate = Data.HeartRate;
		Existing->Color = Data.Color;
		Existing->Timestamp = Data.Timestamp;
		Existing->TimeSinceLastUpdate = 0.0f;
		Existing->LastUpdateWorldTime = CurrentTime;

		if (Config.bLogDataUpdates) UE_LOG(LogAefDeepSync, Verbose, TEXT("Updated: %s"), *Existing->ToString());
		OnWearableUpdated.Broadcast(Data.WearableId, *Existing);
	}
	else
	{
		FAefDeepSyncWearableData NewWearable = Data;
		NewWearable.UniqueId = NextUniqueId++;
		NewWearable.TimeSinceLastUpdate = 0.0f;
		NewWearable.LastUpdateWorldTime = CurrentTime;
		ActiveWearables.Add(Data.WearableId, NewWearable);

		if (Config.bLogWearableEvents) UE_LOG(LogAefDeepSync, Log, TEXT("New wearable: %s"), *NewWearable.ToString());
		OnWearableConnected.Broadcast(NewWearable);
	}
}

void UAefDeepSyncSubsystem::CheckWearableTimeouts(float DeltaTime)
{
	TArray<int32> WearablesToRemove;

	for (auto& Pair : ActiveWearables)
	{
		Pair.Value.TimeSinceLastUpdate += DeltaTime;
		if (Pair.Value.IsStale(Config.WearableLostTimeout))
		{
			WearablesToRemove.Add(Pair.Key);
		}
	}

	for (int32 WearableId : WearablesToRemove)
	{
		FAefDeepSyncWearableData LostWearable;
		if (ActiveWearables.RemoveAndCopyValue(WearableId, LostWearable))
		{
			if (Config.bLogWearableEvents) UE_LOG(LogAefDeepSync, Log, TEXT("Wearable lost (timeout): %s"), *LostWearable.ToString());
			OnWearableLost.Broadcast(LostWearable);
		}
	}
}

bool UAefDeepSyncSubsystem::IsWearableIdAllowed(int32 WearableId) const
{
	// If no filter configured, allow any ID
	if (Config.AllowedWearableIds.Num() == 0)
		return true;
	return Config.AllowedWearableIds.Contains(WearableId);
}

//--------------------------------------------------------------------------------
// Commands
//--------------------------------------------------------------------------------

bool UAefDeepSyncSubsystem::SendColorCommandLinear(int32 WearableId, FLinearColor InColor)
{
	// Convert to internal color struct
	FAefDeepSyncColor ColorStruct;
	ColorStruct.R = static_cast<uint8>(FMath::Clamp(InColor.R * 255.0f, 0.0f, 255.0f));
	ColorStruct.G = static_cast<uint8>(FMath::Clamp(InColor.G * 255.0f, 0.0f, 255.0f));
	ColorStruct.B = static_cast<uint8>(FMath::Clamp(InColor.B * 255.0f, 0.0f, 255.0f));
	return SendColorCommand(WearableId, ColorStruct);
}

bool UAefDeepSyncSubsystem::SendColorCommand(int32 WearableId, FAefDeepSyncColor InColor)
{
	if (!SenderSocket)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("Cannot send - SenderSocket is null"));
		return false;
	}

	if (ConnectionStatus != EAefDeepSyncConnectionStatus::Connected)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("Cannot send - not connected (status=%d)"), static_cast<int32>(ConnectionStatus));
		return false;
	}

	// Check socket connection state
	ESocketConnectionState SocketState = SenderSocket->GetConnectionState();
	if (SocketState != ESocketConnectionState::SCS_Connected)
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("Cannot send - SenderSocket not connected (state=%d)"), static_cast<int32>(SocketState));
		return false;
	}

	FString JsonCommand = FString::Printf(TEXT("{\"Id\":%d,\"Color\":{\"R\":%d,\"G\":%d,\"B\":%d}}X"),
		WearableId, InColor.R, InColor.G, InColor.B);

	FTCHARToUTF8 Converter(*JsonCommand);
	int32 BytesSent = 0;

	UE_LOG(LogAefDeepSync, Verbose, TEXT("Sending: %s (%d bytes)"), *JsonCommand, Converter.Length());

	if (!SenderSocket->Send((const uint8*)Converter.Get(), Converter.Length(), BytesSent))
	{
		// Get socket error for diagnostics
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		ESocketErrors LastError = SocketSubsystem ? SocketSubsystem->GetLastErrorCode() : SE_NO_ERROR;
		FString ErrorString = SocketSubsystem ? SocketSubsystem->GetSocketError(LastError) : TEXT("Unknown");
		
		UE_LOG(LogAefDeepSync, Warning, TEXT("Send failed - Socket error: %s (code=%d, BytesSent=%d)"), 
			*ErrorString, static_cast<int32>(LastError), BytesSent);
		return false;
	}

	if (Config.bLogDataUpdates) UE_LOG(LogAefDeepSync, Log, TEXT("Color cmd: Wearable %d -> %s (%d bytes sent)"), WearableId, *InColor.ToString(), BytesSent);
	return true;
}

//--------------------------------------------------------------------------------
// Configuration
//--------------------------------------------------------------------------------

void UAefDeepSyncSubsystem::LoadConfiguration()
{
	FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("AefConfig.ini");

	if (!FPaths::FileExists(ConfigPath))
	{
		UE_LOG(LogAefDeepSync, Warning, TEXT("AefConfig.ini not found - using defaults"));
		return;
	}

	FConfigFile ConfigFile;
	ConfigFile.Read(ConfigPath);
	const TCHAR* Section = TEXT("DeepSync");

	auto GetBool = [&](const TCHAR* Key, bool& OutValue) {
		FString Value;
		if (ConfigFile.GetString(Section, Key, Value))
			OutValue = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
	};

	// Startup
	GetBool(TEXT("autoStart"), Config.bAutoStart);

	// Connection
	ConfigFile.GetString(Section, TEXT("deepSyncIp"), Config.ServerIP);
	ConfigFile.GetInt(Section, TEXT("deepSyncReceiverPort"), Config.ReceiverPort);
	ConfigFile.GetInt(Section, TEXT("deepSyncSenderPort"), Config.SenderPort);

	// Wearables (no ID restrictions - any positive ID allowed)
	FString WearableIdsStr;
	if (ConfigFile.GetString(Section, TEXT("wearableIds"), WearableIdsStr))
	{
		TArray<FString> IdStrings;
		WearableIdsStr.ParseIntoArray(IdStrings, TEXT(","), true);
		for (const FString& IdStr : IdStrings)
		{
			int32 Id = FCString::Atoi(*IdStr.TrimStartAndEnd());
			if (Id >= 0) Config.AllowedWearableIds.Add(Id);
		}
	}

	ConfigFile.GetFloat(Section, TEXT("wearableLostTimeout"), Config.WearableLostTimeout);

	// Reconnection
	ConfigFile.GetFloat(Section, TEXT("reconnectDelay"), Config.ReconnectDelay);
	ConfigFile.GetInt(Section, TEXT("maxReconnectAttempts"), Config.MaxReconnectAttempts);

	// Logging
	GetBool(TEXT("logConnections"), Config.bLogConnections);
	GetBool(TEXT("logWearableEvents"), Config.bLogWearableEvents);
	GetBool(TEXT("logDataUpdates"), Config.bLogDataUpdates);
	GetBool(TEXT("logNetworkErrors"), Config.bLogNetworkErrors);
	GetBool(TEXT("logProtocolDebug"), Config.bLogProtocolDebug);
}

void UAefDeepSyncSubsystem::ReloadConfiguration()
{
	LoadConfiguration();
	if (Config.bLogConnections) UE_LOG(LogAefDeepSync, Log, TEXT("Configuration reloaded"));
}
