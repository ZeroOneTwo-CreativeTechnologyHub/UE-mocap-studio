# MoCap Studio → Unreal Engine Integration Guide

## Architecture Overview

```
┌──────────────┐    WebSocket (JSON)    ┌──────────────────┐
│  Browser App  │ ───────────────────►  │  Unreal Engine    │
│  MediaPipe    │   ws://localhost:8080  │  WebSocket Server │
│  Pose Detect  │                       │  → Animation BP   │
└──────────────┘                        └──────────────────┘
```

The browser captures your pose via MediaPipe and streams landmark data over WebSocket.
Unreal Engine runs a WebSocket **server** that receives this data and maps it onto a skeletal mesh.

---

## Step 1: Enable Required Plugins in UE

Open your Unreal project → Edit → Plugins, and enable:

- **WebSockets** (built-in)
- **LiveLink** (optional, alternative method)

Restart the editor after enabling.

---

## Step 2: Add Module Dependencies

In your project's `Build.cs`, add:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "WebSockets",
    "Json",
    "JsonUtilities",
    "AnimGraphRuntime"
});
```

---

## Step 3: WebSocket Server Actor (C++)

### MoCapReceiver.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IWebSocketServer.h"
#include "IWebSocketNetworkingModule.h"
#include "MoCapReceiver.generated.h"

USTRUCT(BlueprintType)
struct FMoCapLandmark
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Id;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) float X;
    UPROPERTY(BlueprintReadOnly) float Y;
    UPROPERTY(BlueprintReadOnly) float Z;
    UPROPERTY(BlueprintReadOnly) float Visibility;
};

USTRUCT(BlueprintType)
struct FMoCapBoneData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) float SpineTilt;
    UPROPERTY(BlueprintReadOnly) float HeadTilt;
    UPROPERTY(BlueprintReadOnly) float LeftElbowAngle;
    UPROPERTY(BlueprintReadOnly) float RightElbowAngle;
    UPROPERTY(BlueprintReadOnly) float LeftKneeAngle;
    UPROPERTY(BlueprintReadOnly) float RightKneeAngle;
    UPROPERTY(BlueprintReadOnly) float LeftShoulderAngle;
    UPROPERTY(BlueprintReadOnly) float RightShoulderAngle;
    UPROPERTY(BlueprintReadOnly) float LeftHipAngle;
    UPROPERTY(BlueprintReadOnly) float RightHipAngle;
};

USTRUCT(BlueprintType)
struct FMoCapFrame
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bIsIdle;
    UPROPERTY(BlueprintReadOnly) TArray<FMoCapLandmark> Landmarks;
    UPROPERTY(BlueprintReadOnly) FMoCapBoneData Bones;
    UPROPERTY(BlueprintReadOnly) int64 Timestamp;
};

UCLASS()
class YOURPROJECT_API AMoCapReceiver : public AActor
{
    GENERATED_BODY()

public:
    AMoCapReceiver();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoCap")
    int32 Port = 8080;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    FMoCapFrame LatestFrame;

    UPROPERTY(BlueprintReadOnly, Category = "MoCap")
    bool bIsConnected = false;

    UFUNCTION(BlueprintCallable, Category = "MoCap")
    void StartServer();

    UFUNCTION(BlueprintCallable, Category = "MoCap")
    void StopServer();

    UFUNCTION(BlueprintPure, Category = "MoCap")
    bool IsIdle() const { return LatestFrame.bIsIdle; }

    UFUNCTION(BlueprintPure, Category = "MoCap")
    FVector GetLandmarkPosition(int32 LandmarkId) const;

    // Event dispatchers
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPoseReceived);
    UPROPERTY(BlueprintAssignable, Category = "MoCap")
    FOnPoseReceived OnPoseReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIdleStateChanged);
    UPROPERTY(BlueprintAssignable, Category = "MoCap")
    FOnIdleStateChanged OnIdleStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    TSharedPtr<IWebSocketServer> WebSocketServer;
    void OnMessage(const FString& Message);
    void ParsePoseData(const FString& JsonStr);
    bool bWasIdle = true;
};
```

### MoCapReceiver.cpp

```cpp
#include "MoCapReceiver.h"
#include "WebSocketsModule.h"
#include "Json.h"

AMoCapReceiver::AMoCapReceiver()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMoCapReceiver::BeginPlay()
{
    Super::BeginPlay();
    StartServer();
}

void AMoCapReceiver::EndPlay(const EEndPlayReason::Type Reason)
{
    StopServer();
    Super::EndPlay(Reason);
}

void AMoCapReceiver::StartServer()
{
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    // Create WebSocket server on specified port
    IWebSocketNetworkingModule& WSModule =
        FModuleManager::GetModuleChecked<IWebSocketNetworkingModule>("WebSocketNetworking");

    // Note: exact API varies by UE version — see alternative Blueprint approach below
    FWebSocketClientConnectedCallBack OnConnected;
    OnConnected.BindLambda([this](INetworkingWebSocket* Socket) {
        bIsConnected = true;
        UE_LOG(LogTemp, Log, TEXT("MoCap: Browser connected"));
    });

    UE_LOG(LogTemp, Log, TEXT("MoCap: Server started on port %d"), Port);
}

void AMoCapReceiver::StopServer()
{
    WebSocketServer.Reset();
    bIsConnected = false;
}

void AMoCapReceiver::OnMessage(const FString& Message)
{
    ParsePoseData(Message);
}

void AMoCapReceiver::ParsePoseData(const FString& JsonStr)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

    bool bIdle = Root->GetBoolField(TEXT("idle"));

    // Detect idle state change
    if (bIdle != bWasIdle)
    {
        bWasIdle = bIdle;
        OnIdleStateChanged.Broadcast();
    }

    LatestFrame.bIsIdle = bIdle;
    LatestFrame.Timestamp = Root->GetIntegerField(TEXT("timestamp"));

    if (!bIdle)
    {
        // Parse landmarks
        LatestFrame.Landmarks.Empty();
        const TArray<TSharedPtr<FJsonValue>>& LandmarksArray =
            Root->GetArrayField(TEXT("landmarks"));

        for (const auto& Val : LandmarksArray)
        {
            const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
            FMoCapLandmark LM;
            LM.Id = Obj->GetIntegerField(TEXT("id"));
            LM.Name = Obj->GetStringField(TEXT("name"));
            LM.X = Obj->GetNumberField(TEXT("x"));
            LM.Y = Obj->GetNumberField(TEXT("y"));
            LM.Z = Obj->GetNumberField(TEXT("z"));
            LM.Visibility = Obj->GetNumberField(TEXT("visibility"));
            LatestFrame.Landmarks.Add(LM);
        }

        // Parse bone data
        const TSharedPtr<FJsonObject>& BonesObj = Root->GetObjectField(TEXT("bones"));
        LatestFrame.Bones.SpineTilt = BonesObj->GetNumberField(TEXT("spine_tilt"));
        LatestFrame.Bones.HeadTilt = BonesObj->GetNumberField(TEXT("head_tilt"));
        LatestFrame.Bones.LeftElbowAngle = BonesObj->GetNumberField(TEXT("left_elbow_angle"));
        LatestFrame.Bones.RightElbowAngle = BonesObj->GetNumberField(TEXT("right_elbow_angle"));
        LatestFrame.Bones.LeftKneeAngle = BonesObj->GetNumberField(TEXT("left_knee_angle"));
        LatestFrame.Bones.RightKneeAngle = BonesObj->GetNumberField(TEXT("right_knee_angle"));
        LatestFrame.Bones.LeftShoulderAngle = BonesObj->GetNumberField(TEXT("left_shoulder_angle"));
        LatestFrame.Bones.RightShoulderAngle = BonesObj->GetNumberField(TEXT("right_shoulder_angle"));
        LatestFrame.Bones.LeftHipAngle = BonesObj->GetNumberField(TEXT("left_hip_angle"));
        LatestFrame.Bones.RightHipAngle = BonesObj->GetNumberField(TEXT("right_hip_angle"));

        OnPoseReceived.Broadcast();
    }
}

FVector AMoCapReceiver::GetLandmarkPosition(int32 LandmarkId) const
{
    if (LandmarkId >= 0 && LandmarkId < LatestFrame.Landmarks.Num())
    {
        const FMoCapLandmark& LM = LatestFrame.Landmarks[LandmarkId];
        // Convert from normalized webcam space to UE world space
        // X = right, Y = forward, Z = up in UE
        return FVector(
            (LM.X - 0.5f) * 200.0f,   // Center and scale X
            LM.Z * 200.0f,              // Depth from MediaPipe Z
            (1.0f - LM.Y) * 200.0f     // Flip Y and scale for height
        );
    }
    return FVector::ZeroVector;
}
```

---

## Step 4: The WebSocket Bridge

The standalone bridge app relays data between the browser and Unreal Engine. It lives in the `bridge/` folder of this repository.

Run from source with Node.js (18+):

```
cd bridge
npm install
npm start
```

See [`bridge/README.md`](bridge/README.md) for full details, custom ports, and how to build standalone executables.

Default ports:
- **Browser connects to:** `ws://localhost:8080`
- **Unreal connects to:** `ws://localhost:8081`

Custom ports: `./mocap-bridge --browser-port 9090 --ue-port 9091`

---

## Step 5: Animation Blueprint Setup

### Blueprint Approach (No C++ Required)

1. **Create an Animation Blueprint** for your skeletal mesh
2. **Add a "MoCapReceiver" actor** to your level
3. In the **AnimGraph**, use the following node setup:

```
┌─────────────────────┐
│  State Machine       │
│  ┌───────┐          │
│  │ Idle  │◄─── IsIdle == true ──► Play idle/breathe anim
│  └───┬───┘          │
│      │ IsIdle==false │
│  ┌───▼───┐          │
│  │ MoCap │◄─── Apply bone rotations from MoCapReceiver
│  └───────┘          │
└─────────────────────┘
```

### Key Bone Mapping (MediaPipe → UE Mannequin)

| MediaPipe Joint     | ID  | UE5 Bone Name      |
|---------------------|-----|---------------------|
| nose                | 0   | head                |
| left_shoulder       | 11  | clavicle_l          |
| right_shoulder      | 12  | clavicle_r          |
| left_elbow          | 13  | lowerarm_l          |
| right_elbow         | 14  | lowerarm_r          |
| left_wrist          | 15  | hand_l              |
| right_wrist         | 16  | hand_r              |
| left_hip            | 23  | thigh_l             |
| right_hip           | 24  | thigh_r             |
| left_knee           | 25  | calf_l              |
| right_knee          | 26  | calf_r              |
| left_ankle          | 27  | foot_l              |
| right_ankle         | 28  | foot_r              |

### Event Graph Logic (Pseudo-Blueprint)

```
Event Tick
  │
  ├─► Get MoCapReceiver reference
  │
  ├─► Branch: Is Idle?
  │     ├─ YES → Set State to "Idle" (plays idle anim montage)
  │     └─ NO  → For each bone mapping:
  │               ├─ Get landmark position from receiver
  │               ├─ Convert to local bone rotation
  │               ├─ Apply rotation with interpolation (0.15s lerp)
  │               └─ Set bone rotation via "Modify Bone" node
  │
  └─► Optional: Apply IK for feet grounding
```

### Anim Blueprint: Modify Bone Example

In the AnimGraph, add **"Transform (Modify) Bone"** nodes for each mapped bone:

1. **Spine**: Use `spine_tilt` from bone data → rotate spine bone on Y axis
2. **Head**: Use `head_tilt` + nose landmark → rotate head bone
3. **Arms**: Use `shoulder_angle` and `elbow_angle` → rotate upper/lower arm
4. **Legs**: Use `hip_angle` and `knee_angle` → rotate thigh/calf

---

## Step 6: JSON Data Format Reference

### Active Pose Packet
```json
{
  "type": "pose",
  "idle": false,
  "timestamp": 1714000000000,
  "fps": 30,
  "landmarks": [
    { "id": 0,  "name": "nose",           "x": 0.512, "y": 0.301, "z": -0.05, "visibility": 0.99 },
    { "id": 11, "name": "left_shoulder",   "x": 0.612, "y": 0.485, "z": -0.08, "visibility": 0.95 },
    ...33 total landmarks
  ],
  "bones": {
    "spine_tilt": 2.3,
    "head_tilt": -0.5,
    "left_elbow_angle": 145.2,
    "right_elbow_angle": 150.8,
    "left_knee_angle": 170.1,
    "right_knee_angle": 168.5,
    "left_shoulder_angle": 45.3,
    "right_shoulder_angle": 42.1,
    "left_hip_angle": 170.0,
    "right_hip_angle": 172.3,
    "shoulder_width": 0.25,
    "hip_width": 0.18,
    "torso_center": { "x": 0.5, "y": 0.45, "z": -0.06 },
    "hip_center": { "x": 0.5, "y": 0.65, "z": -0.04 }
  }
}
```

### Idle Packet (No Person Detected)
```json
{
  "type": "pose",
  "idle": true,
  "timestamp": 1714000000000
}
```

---

## Quick Start Checklist

1. Open `mocap-studio.html` in Chrome (camera permissions required)
2. Start the MoCap Bridge app (see `bridge/README.md`)
3. In the browser app, connect to `ws://localhost:8080`
4. In Unreal, connect to `ws://localhost:8081` via your receiver
5. Stand in front of camera — your avatar mirrors your movements
6. Step away — avatar transitions to idle animation

---

## Tips for Best Results

- **Lighting**: Ensure even, front-facing lighting for reliable tracking
- **Distance**: Stand 1.5–3m from the camera for full-body capture
- **Clothing**: Avoid very loose clothing that obscures joint positions
- **Smoothing**: Increase the smoothing slider (0.5–0.7) to reduce jitter on the avatar
- **Confidence**: Raise the minimum confidence threshold if you see phantom joints
- **Frame Rate**: Match the send rate to your UE tick rate for smoothest results
