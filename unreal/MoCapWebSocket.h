// MoCapWebSocket.h
// Self-contained WebSocket receiver for MoCap Studio.
// Handles connection, JSON parsing, and pose data storage.
// Just drop this actor into your level and it works.
//
// Part of UE MoCap Studio (open source, MIT license).
//
// SETUP:
//   1. Copy this file and MoCapWebSocket.cpp into Source/YourProject/
//   2. Add "WebSockets", "Json", "JsonUtilities" to Build.cs
//   3. Compile
//   4. Drag MoCapWebSocket into your level
//   5. Press Play — it auto-connects to the bridge

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IWebSocket.h"
#include "MoCapWebSocket.generated.h"

// Landmark data for a single tracked point
USTRUCT(BlueprintType)
struct FMoCapLandmark
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    int32 Id = 0;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    FString Name;

    // Normalized position from MediaPipe (0-1 range)
    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float X = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float Y = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float Z = 0.f;

    // Tracking confidence (0-1, higher is better)
    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float Visibility = 0.f;

    // Converted to Unreal world space
    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    FVector WorldPosition = FVector::ZeroVector;
};

// Pre-computed bone angle data
USTRUCT(BlueprintType)
struct FMoCapBoneAngles
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float SpineTilt = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float HeadTilt = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float LeftElbowAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float RightElbowAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float LeftKneeAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float RightKneeAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float LeftShoulderAngle = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float RightShoulderAngle = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float LeftHipAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float RightHipAngle = 180.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float ShoulderWidth = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    float HipWidth = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    FVector TorsoCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    FVector HipCenter = FVector::ZeroVector;
};


UCLASS(Blueprintable)
class AMoCapWebSocket : public AActor
{
    GENERATED_BODY()

public:
    AMoCapWebSocket();

    // ────────────────────────────────────────────
    //  SETTINGS (editable in the Details panel)
    // ────────────────────────────────────────────

    // WebSocket URL of the MoCap Bridge.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoCap|Connection")
    FString ServerURL = TEXT("ws://localhost:8081");

    // Automatically connect when the game starts.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoCap|Connection")
    bool bAutoConnect = true;

    // Seconds to wait before retrying after a disconnect.
    // Set to 0 to disable auto-reconnect.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoCap|Connection")
    float ReconnectDelay = 2.0f;

    // Scale factor when converting normalized coordinates to world space.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoCap|Mapping")
    float WorldScale = 200.0f;

    // ────────────────────────────────────────────
    //  POSE DATA (read these from Animation BP)
    // ────────────────────────────────────────────

    // True when no person is detected by the camera.
    // Use this to switch between idle animation and live tracking.
    UPROPERTY(BlueprintReadOnly, Category = "MoCap|Pose")
    bool bIsIdle = true;

    // True when connected to the bridge.
    UPROPERTY(BlueprintReadOnly, Category = "MoCap|Pose")
    bool bIsConnected = false;

    // All 33 MediaPipe landmarks with raw and world-space positions.
    UPROPERTY(BlueprintReadOnly, Category = "MoCap|Pose")
    TArray<FMoCapLandmark> Landmarks;

    // Pre-computed bone angles for direct use in Modify Bone nodes.
    UPROPERTY(BlueprintReadOnly, Category = "MoCap|Pose")
    FMoCapBoneAngles BoneAngles;

    // Frames per second reported by the browser capture app.
    UPROPERTY(BlueprintReadOnly, Category = "MoCap|Pose")
    int32 CaptureFPS = 0;

    // ────────────────────────────────────────────
    //  FUNCTIONS (callable from Blueprints)
    // ────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "MoCap")
    void ConnectToServer();

    UFUNCTION(BlueprintCallable, Category = "MoCap")
    void Disconnect();

    // Get world-space position of a landmark by ID (0-32).
    UFUNCTION(BlueprintPure, Category = "MoCap")
    FVector GetLandmarkPosition(int32 LandmarkId) const;

    // Get tracking confidence of a landmark (0-1).
    UFUNCTION(BlueprintPure, Category = "MoCap")
    float GetLandmarkVisibility(int32 LandmarkId) const;

    // ────────────────────────────────────────────
    //  EVENTS (bind in Animation BP if needed)
    // ────────────────────────────────────────────

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoCapConnected);
    UPROPERTY(BlueprintAssignable, Category = "MoCap|Events")
    FOnMoCapConnected OnConnected;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoCapDisconnected);
    UPROPERTY(BlueprintAssignable, Category = "MoCap|Events")
    FOnMoCapDisconnected OnDisconnected;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoCapPoseReceived);
    UPROPERTY(BlueprintAssignable, Category = "MoCap|Events")
    FOnMoCapPoseReceived OnPoseReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoCapIdleChanged);
    UPROPERTY(BlueprintAssignable, Category = "MoCap|Events")
    FOnMoCapIdleChanged OnIdleChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    TSharedPtr<IWebSocket> WebSocket;
    FTimerHandle ReconnectTimerHandle;
    bool bWasIdle = true;

    void OnMessageReceived(const FString& Message);
    void ParsePoseJSON(const FString& JsonString);
    void ScheduleReconnect();
    FVector NormalizedToWorld(float InX, float InY, float InZ) const;
};
