<div align="center">

# UE MoCap Studio

**Browser-based motion capture that drives Unreal Engine avatars in real time.**

Capture full-body pose data from a standard webcam using MediaPipe, stream it over WebSocket, and map it onto any UE5 skeletal mesh. No expensive mocap hardware required.

[![License: MIT](https://img.shields.io/badge/License-MIT-00e5ff.svg)](#license)
[![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.x-0E1128.svg?logo=unrealengine)](https://www.unrealengine.com/)
[![MediaPipe](https://img.shields.io/badge/MediaPipe-Pose-ff6f00.svg)](https://google.github.io/mediapipe/solutions/pose.html)
[![Node.js](https://img.shields.io/badge/Node.js-18+-339933.svg?logo=node.js&logoColor=white)](https://nodejs.org)

<br>

<img src="docs/banner.png" alt="UE MoCap Studio demo" width="720">

<br>

[Quick Start](#quick-start) · [How It Works](#how-it-works) · [Unreal Setup](#unreal-engine-setup) · [JSON Protocol](#json-protocol) · [Configuration](#configuration) · [Contributing](#contributing)

</div>

---

## Features

- **33-point skeleton tracking** / Full body, face, and hand landmark detection via MediaPipe Pose
- **Real-time WebSocket streaming** / Sub-frame latency pose data at configurable frame rates (5-60 fps)
- **Pre-computed bone angles** / Elbow, knee, shoulder, hip, and spine angles ready for direct bone rotation in UE
- **Automatic idle detection** / When no person is visible, sends an idle signal so your avatar falls back to a breathing/idle animation
- **Adjustable smoothing** / Temporal landmark smoothing to eliminate jitter on your avatar
- **Confidence filtering** / Per-joint visibility thresholds to discard unreliable tracking data
- **Session recording** / Record and export full capture sessions as JSON for offline replay or retargeting
- **Mirror mode** / Toggle mirrored/natural view for performer comfort
- **Joint angle overlay** / Optional real-time angle display on elbows, knees, and shoulders
- **Zero hardware cost** / Works with any USB webcam or laptop camera

---

## Quick Start

### Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| **Chrome / Edge** | Latest | Runs the capture app (requires WebRTC) |
| **Node.js** | 18+ | Runs the WebSocket bridge ([download here](https://nodejs.org/)) |
| **Unreal Engine** | 5.x | Avatar rendering and animation |

**Check if Node.js is installed:** Open a terminal and run `node --version`. If you see a version number (e.g. `v20.11.0`), you are good. If not, download the LTS version from [nodejs.org](https://nodejs.org/) and install it. On macOS you can also use `brew install node` if you have Homebrew.

### 1. Download the Project

Clone the repository, or download and extract the ZIP from GitHub:

```bash
git clone https://github.com/ZeroOneTwo-CreativeTechnologyHub/ue-mocap-studio.git
```

### 2. Start the Bridge

Open a terminal and navigate to the `bridge` folder inside the project:

```bash
cd ue-mocap-studio/bridge
npm install
npm start
```

`npm install` downloads the required dependencies (you only need to run it once). `npm start` launches the bridge.

You should see:

```
  ╔══════════════════════════════════════════╗
  ║        MoCap Studio - Bridge v1.0        ║
  ╠══════════════════════════════════════════╣
  ║  Browser  ->  ws://localhost:8080        ║
  ║  Unreal   ->  ws://localhost:8081        ║
  ╚══════════════════════════════════════════╝

  Status: Browser: Waiting... | Unreal: Waiting...
```

Leave this terminal window open. The bridge must stay running.

You may see warnings about deprecated packages or vulnerabilities during `npm install`. These are safe to ignore and do not affect the bridge.

### 3. Open the Capture App

Open the `mocap-studio.html` file in Chrome. You can double-click it in your file explorer, or drag it into an open Chrome window.

1. Click **Start Capture**.
2. Chrome will ask for camera permission. Click **Allow**.
3. You should see your webcam feed with a skeleton overlay appearing on your body.
4. In the sidebar on the right, the WebSocket URL should already say `ws://localhost:8080`. Click **Connect**.

The bridge terminal should now update to show:

```
  [13:35:xx] Browser connected from ::1
  Status: Browser: Connected | Unreal: Waiting...
```

At this point the capture side is fully working. Move around in front of the camera and you will see the skeleton tracking your body in real time.

### 4. Connect Unreal Engine

Follow the [`UNREAL-SETUP-GUIDE.md`](UNREAL-SETUP-GUIDE.md) to set up your UE project with an avatar. When you press Play in the editor, the avatar connects to the bridge on `ws://localhost:8081` and begins mirroring your movements.

The bridge terminal updates to:

```
  [13:36:xx] UE client connected from ::1 (1 total)
  Status: Browser: Connected | Unreal: 1 connected
  Relaying at ~30 msg/s to 1 UE client(s)
```

When you step away from the camera, the avatar transitions to an idle animation. Step back in and tracking resumes.

---

## How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│                        DATA PIPELINE                            │
│                                                                 │
│  ┌────────┐    ┌───────────┐    ┌────────────┐    ┌─────────┐  │
│  │ Webcam │───>│ MediaPipe │───>│  WebSocket │───>│ Unreal  │  │
│  │  Feed  │    │   Pose    │    │   Bridge   │    │ Engine  │  │
│  └────────┘    └───────────┘    └────────────┘    └─────────┘  │
│                                                                 │
│  30 fps         33 landmarks     JSON stream      Anim Blueprint│
│  720p           + bone angles    ws://8080>8081   > Skeletal    │
│                 + idle state                        Mesh        │
└─────────────────────────────────────────────────────────────────┘
```

**Stage 1: Capture.** The browser app accesses the webcam via WebRTC and feeds each frame to MediaPipe Pose, which returns 33 body landmarks in normalized 3D coordinates.

**Stage 2: Process.** Landmarks are temporally smoothed, filtered by confidence, and enriched with derived bone angles (elbow flexion, knee bend, spine tilt, etc.). The app also determines whether a person is present.

**Stage 3: Transport.** Processed pose data is serialized as JSON and pushed over WebSocket to the bridge relay at a configurable rate.

**Stage 4: Animate.** Unreal Engine receives the data, maps landmarks to the avatar skeleton, and applies bone rotations. When an idle signal arrives, the avatar transitions to a resting animation.

---

## Project Structure

```
ue-mocap-studio/
├── mocap-studio.html           # Browser capture app (single-file, no build step)
├── UNREAL-INTEGRATION-GUIDE.md # Technical reference for the UE integration
├── UNREAL-SETUP-GUIDE.md       # Step-by-step beginner walkthrough for UE
├── README.md
├── LICENSE
│
├── bridge/                     # Standalone WebSocket relay
│   ├── mocap-bridge.js         # Bridge source
│   ├── package.json            # Dependencies and build scripts
│   └── README.md               # Download, run, and build instructions
│
├── unreal/                     # Unreal Engine C++ source files
│   ├── MoCapWebSocket.h        # WebSocket wrapper - header
│   └── MoCapWebSocket.cpp      # WebSocket wrapper - implementation
│
└── docs/
    ├── banner.png
    ├── joint-map.png           # Visual landmark ID reference
    └── blueprint-example.png   # Example AnimBP screenshot
```

---

## Unreal Engine Setup

> **New to Unreal?** Follow the full step-by-step walkthrough in [`UNREAL-SETUP-GUIDE.md`](UNREAL-SETUP-GUIDE.md). It covers everything from creating the project to getting your avatar moving.
>
> For a technical reference (C++ receiver, data structures, Blueprint pseudocode), see [`UNREAL-INTEGRATION-GUIDE.md`](UNREAL-INTEGRATION-GUIDE.md).

### Summary

1. **Enable plugins:** WebSockets (built-in), optionally LiveLink
2. **Add the MoCapReceiver actor** to your level. It opens a WebSocket server on the specified port.
3. **Create an Animation Blueprint** with two states:

| State | Trigger | Behaviour |
|-------|---------|-----------|
| **Idle** | `IsIdle == true` | Plays a looping idle/breathing animation |
| **MoCap** | `IsIdle == false` | Reads landmark data and applies bone rotations via Transform (Modify) Bone nodes |

4. **Map bones** using the table below

### Bone Mapping: MediaPipe to UE5 Mannequin

| MediaPipe Landmark | ID | UE5 Bone |
|---|---|---|
| nose | 0 | `head` |
| left_shoulder | 11 | `clavicle_l` |
| right_shoulder | 12 | `clavicle_r` |
| left_elbow | 13 | `lowerarm_l` |
| right_elbow | 14 | `lowerarm_r` |
| left_wrist | 15 | `hand_l` |
| right_wrist | 16 | `hand_r` |
| left_hip | 23 | `thigh_l` |
| right_hip | 24 | `thigh_r` |
| left_knee | 25 | `calf_l` |
| right_knee | 26 | `calf_r` |
| left_ankle | 27 | `foot_l` |
| right_ankle | 28 | `foot_r` |

---

## JSON Protocol

### Pose Packet (person detected)

```jsonc
{
  "type": "pose",
  "idle": false,
  "timestamp": 1714000000000,
  "fps": 30,
  "landmarks": [
    {
      "id": 0,
      "name": "nose",
      "x": 0.51234,       // Normalized [0-1], left to right
      "y": 0.30156,       // Normalized [0-1], top to bottom
      "z": -0.05012,      // Depth relative to hip midpoint
      "visibility": 0.993 // Confidence [0-1]
    }
    // ... 33 total landmarks
  ],
  "bones": {
    "spine_tilt": 2.3,           // Degrees, 0 = upright
    "head_tilt": -0.5,           // Lateral offset from torso center
    "left_elbow_angle": 145.2,   // Degrees, 180 = straight
    "right_elbow_angle": 150.8,
    "left_knee_angle": 170.1,
    "right_knee_angle": 168.5,
    "left_shoulder_angle": 45.3,
    "right_shoulder_angle": 42.1,
    "left_hip_angle": 170.0,
    "right_hip_angle": 172.3,
    "shoulder_width": 0.25,      // Normalized distance
    "hip_width": 0.18,
    "torso_center": { "x": 0.5, "y": 0.45, "z": -0.06 },
    "hip_center":   { "x": 0.5, "y": 0.65, "z": -0.04 }
  }
}
```

### Idle Packet (no person detected)

```jsonc
{
  "type": "pose",
  "idle": true,
  "timestamp": 1714000000000
}
```

---

## Configuration

### Browser App Settings

| Setting | Range | Default | Description |
|---|---|---|---|
| **Min Confidence** | 0.1 - 0.95 | 0.5 | Landmarks below this threshold are hidden and excluded |
| **Send Rate** | 5 - 60 fps | 30 | WebSocket transmission frequency |
| **Smoothing** | 0.0 - 0.9 | 0.5 | Temporal smoothing factor. Higher = smoother but more latent |
| **Show Skeleton** | on/off | on | Render bone connections on the video feed |
| **Show Joints** | on/off | on | Render joint markers on the video feed |
| **Show Angles** | on/off | off | Display computed angles at key joints |
| **Mirror View** | on/off | on | Horizontally flip the camera feed |

### Bridge

| Setting | Default | Description |
|---|---|---|
| `--browser-port` | `8080` | Port the browser app connects to |
| `--ue-port` | `8081` | Port Unreal Engine connects to |

Pass these as command-line flags when running the bridge, or edit the defaults in `bridge/mocap-bridge.js`.

---

## Tips for Best Results

- **Lighting** / Even, front-facing light reduces tracking noise significantly
- **Distance** / Stand 1.5-3 metres from the camera for reliable full-body capture
- **Background** / A plain, contrasting background helps MediaPipe isolate your body
- **Clothing** / Fitted clothing gives cleaner joint detection than loose or flowing garments
- **Smoothing** / Start at 0.5, increase to 0.7 if you see jitter on the avatar, decrease for faster response
- **Performance** / Close other GPU-intensive applications. MediaPipe benefits from hardware acceleration
- **Multiple monitors** / Run the browser app on one screen and UE on another for a clean workflow

---

## Roadmap

- [ ] **Hand tracking** / Finger-level landmark support via MediaPipe Hands
- [ ] **Face mesh** / Blend shape and morph target data for facial animation
- [ ] **Multi-person** / Track and assign multiple performers to different avatars
- [ ] **LiveLink plugin** / Native UE LiveLink source as an alternative to raw WebSocket
- [ ] **Recording playback** / Timeline scrubbing and replay inside the browser app
- [ ] **BVH export** / Industry-standard motion capture format export
- [ ] **Calibration wizard** / Guided setup to scale tracking space to avatar proportions

---

## Contributing

Contributions are welcome. Please open an issue first to discuss what you'd like to change.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/hand-tracking`)
3. Commit your changes (`git commit -m 'Add hand tracking support'`)
4. Push to the branch (`git push origin feature/hand-tracking`)
5. Open a Pull Request

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

MediaPipe is licensed under the [Apache 2.0 License](https://github.com/google/mediapipe/blob/master/LICENSE) by Google.

---

<div align="center">

Built with [MediaPipe](https://mediapipe.dev) · [Unreal Engine](https://unrealengine.com) · [Node.js](https://nodejs.org)

**If this project helped you, consider giving it a star.**

</div>
