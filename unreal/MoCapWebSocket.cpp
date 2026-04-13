// MoCapWebSocket.cpp
// Part of UE MoCap Studio (open source, MIT license).

#include "MoCapWebSocket.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

AMoCapWebSocket::AMoCapWebSocket()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMoCapWebSocket::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoConnect)
    {
        ConnectToServer();
    }
}

void AMoCapWebSocket::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ReconnectTimerHandle);
    Disconnect();
    Super::EndPlay(Reason);
}

// ─────────────────────────────────────────────────
//  CONNECTION
// ─────────────────────────────────────────────────

void AMoCapWebSocket::ConnectToServer()
{
    // Make sure the WebSockets module is loaded.
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    // Clean up any existing connection.
    if (WebSocket.IsValid())
    {
        WebSocket->Close();
        WebSocket.Reset();
    }

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL, TEXT("ws"));

    WebSocket->OnConnected().AddLambda([this]()
    {
        bIsConnected = true;
        UE_LOG(LogTemp, Log, TEXT("MoCap: Connected to %s"), *ServerURL);
        OnConnected.Broadcast();
    });

    WebSocket->OnMessage().AddLambda([this](const FString& Message)
    {
        OnMessageReceived(Message);
    });

    WebSocket->OnConnectionError().AddLambda([this](const FString& Error)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoCap: Connection error: %s"), *Error);
        bIsConnected = false;
        ScheduleReconnect();
    });

    WebSocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
    {
        bIsConnected = false;
        UE_LOG(LogTemp, Log, TEXT("MoCap: Disconnected (code %d)"), StatusCode);
        OnDisconnected.Broadcast();
        ScheduleReconnect();
    });

    UE_LOG(LogTemp, Log, TEXT("MoCap: Connecting to %s ..."), *ServerURL);
    WebSocket->Connect();
}

void AMoCapWebSocket::Disconnect()
{
    GetWorldTimerManager().ClearTimer(ReconnectTimerHandle);

    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        WebSocket->Close();
    }
    bIsConnected = false;
}

void AMoCapWebSocket::ScheduleReconnect()
{
    if (ReconnectDelay <= 0.f) return;

    UE_LOG(LogTemp, Log, TEXT("MoCap: Reconnecting in %.1f seconds..."), ReconnectDelay);

    GetWorldTimerManager().SetTimer(
        ReconnectTimerHandle,
        this,
        &AMoCapWebSocket::ConnectToServer,
        ReconnectDelay,
        false
    );
}

// ─────────────────────────────────────────────────
//  JSON PARSING
// ─────────────────────────────────────────────────

void AMoCapWebSocket::OnMessageReceived(const FString& Message)
{
    ParsePoseJSON(Message);
}

void AMoCapWebSocket::ParsePoseJSON(const FString& JsonString)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return;
    }

    // ── Idle state ──
    bool bIdle = true;
    Root->TryGetBoolField(TEXT("idle"), bIdle);

    if (bIdle != bWasIdle)
    {
        bWasIdle = bIdle;
        bIsIdle = bIdle;
        OnIdleChanged.Broadcast();
    }
    bIsIdle = bIdle;

    if (bIdle)
    {
        // No person detected. Animation BP reads bIsIdle and plays idle anim.
        return;
    }

    // ── FPS ──
    int32 Fps = 0;
    if (Root->TryGetNumberField(TEXT("fps"), Fps))
    {
        CaptureFPS = Fps;
    }

    // ── Landmarks ──
    const TArray<TSharedPtr<FJsonValue>>* LandmarksArray = nullptr;
    if (Root->TryGetArrayField(TEXT("landmarks"), LandmarksArray))
    {
        Landmarks.SetNum(LandmarksArray->Num());

        for (int32 i = 0; i < LandmarksArray->Num(); i++)
        {
            const TSharedPtr<FJsonObject>& Obj = (*LandmarksArray)[i]->AsObject();
            if (!Obj.IsValid()) continue;

            FMoCapLandmark& LM = Landmarks[i];
            LM.Id = Obj->GetIntegerField(TEXT("id"));
            LM.Name = Obj->GetStringField(TEXT("name"));
            LM.X = Obj->GetNumberField(TEXT("x"));
            LM.Y = Obj->GetNumberField(TEXT("y"));
            LM.Z = Obj->GetNumberField(TEXT("z"));
            LM.Visibility = Obj->GetNumberField(TEXT("visibility"));
            LM.WorldPosition = NormalizedToWorld(LM.X, LM.Y, LM.Z);
        }
    }

    // ── Bone angles ──
    const TSharedPtr<FJsonObject>* BonesObj = nullptr;
    if (Root->TryGetObjectField(TEXT("bones"), BonesObj))
    {
        const TSharedPtr<FJsonObject>& B = *BonesObj;

        B->TryGetNumberField(TEXT("spine_tilt"), BoneAngles.SpineTilt);
        B->TryGetNumberField(TEXT("head_tilt"), BoneAngles.HeadTilt);
        B->TryGetNumberField(TEXT("left_elbow_angle"), BoneAngles.LeftElbowAngle);
        B->TryGetNumberField(TEXT("right_elbow_angle"), BoneAngles.RightElbowAngle);
        B->TryGetNumberField(TEXT("left_knee_angle"), BoneAngles.LeftKneeAngle);
        B->TryGetNumberField(TEXT("right_knee_angle"), BoneAngles.RightKneeAngle);
        B->TryGetNumberField(TEXT("left_shoulder_angle"), BoneAngles.LeftShoulderAngle);
        B->TryGetNumberField(TEXT("right_shoulder_angle"), BoneAngles.RightShoulderAngle);
        B->TryGetNumberField(TEXT("left_hip_angle"), BoneAngles.LeftHipAngle);
        B->TryGetNumberField(TEXT("right_hip_angle"), BoneAngles.RightHipAngle);
        B->TryGetNumberField(TEXT("shoulder_width"), BoneAngles.ShoulderWidth);
        B->TryGetNumberField(TEXT("hip_width"), BoneAngles.HipWidth);

        // Torso and hip centers
        const TSharedPtr<FJsonObject>* TorsoObj = nullptr;
        if (B->TryGetObjectField(TEXT("torso_center"), TorsoObj))
        {
            float TX = 0.f, TY = 0.f, TZ = 0.f;
            (*TorsoObj)->TryGetNumberField(TEXT("x"), TX);
            (*TorsoObj)->TryGetNumberField(TEXT("y"), TY);
            (*TorsoObj)->TryGetNumberField(TEXT("z"), TZ);
            BoneAngles.TorsoCenter = NormalizedToWorld(TX, TY, TZ);
        }

        const TSharedPtr<FJsonObject>* HipObj = nullptr;
        if (B->TryGetObjectField(TEXT("hip_center"), HipObj))
        {
            float HX = 0.f, HY = 0.f, HZ = 0.f;
            (*HipObj)->TryGetNumberField(TEXT("x"), HX);
            (*HipObj)->TryGetNumberField(TEXT("y"), HY);
            (*HipObj)->TryGetNumberField(TEXT("z"), HZ);
            BoneAngles.HipCenter = NormalizedToWorld(HX, HY, HZ);
        }
    }

    OnPoseReceived.Broadcast();
}

// ─────────────────────────────────────────────────
//  COORDINATE CONVERSION
// ─────────────────────────────────────────────────

FVector AMoCapWebSocket::NormalizedToWorld(float InX, float InY, float InZ) const
{
    // MediaPipe normalized coords:
    //   X = 0 (left) to 1 (right)
    //   Y = 0 (top) to 1 (bottom)
    //   Z = depth relative to hip midpoint (negative = closer to camera)
    //
    // Unreal Engine coords:
    //   X = right, Y = forward, Z = up

    return FVector(
        (InX - 0.5f) * WorldScale,   // Center horizontally
        InZ * WorldScale,              // Depth becomes forward
        (1.0f - InY) * WorldScale      // Flip vertical to up
    );
}

// ─────────────────────────────────────────────────
//  HELPER FUNCTIONS
// ─────────────────────────────────────────────────

FVector AMoCapWebSocket::GetLandmarkPosition(int32 LandmarkId) const
{
    if (LandmarkId >= 0 && LandmarkId < Landmarks.Num())
    {
        return Landmarks[LandmarkId].WorldPosition;
    }
    return FVector::ZeroVector;
}

float AMoCapWebSocket::GetLandmarkVisibility(int32 LandmarkId) const
{
    if (LandmarkId >= 0 && LandmarkId < Landmarks.Num())
    {
        return Landmarks[LandmarkId].Visibility;
    }
    return 0.f;
}
