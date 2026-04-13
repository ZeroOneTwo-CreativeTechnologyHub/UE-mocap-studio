# Unreal Engine Setup Guide

A complete walkthrough for creating an Unreal Engine project that receives live pose data from MoCap Studio and drives a rigged avatar in real time.

This guide assumes you have Unreal Engine 5.x installed via the Epic Games Launcher and have no prior UE project set up for this. Every step is covered.

---

## Table of Contents

1. [Create the Project](#1-create-the-project)
2. [Enable Required Plugins](#2-enable-required-plugins)
3. [Import or Add an Avatar](#3-import-or-add-an-avatar)
4. [Start the MoCap Bridge](#4-start-the-mocap-bridge)
5. [Create the WebSocket Receiver](#5-create-the-websocket-receiver)
6. [Create the Animation Blueprint](#6-create-the-animation-blueprint)
7. [Build the Idle State](#7-build-the-idle-state)
8. [Build the MoCap State](#8-build-the-mocap-state)
9. [Map Bones from MediaPipe to Your Skeleton](#9-map-bones-from-mediapipe-to-your-skeleton)
10. [Place the Avatar in Your Level](#10-place-the-avatar-in-your-level)
11. [Connect and Test](#11-connect-and-test)
12. [Troubleshooting](#12-troubleshooting)

---

## 1. Create the Project

1. Open the **Epic Games Launcher** and click **Launch** next to Unreal Engine 5.x.
2. In the Project Browser, select **Games** on the left panel.
3. Choose the **Third Person** template. This gives you a level with lighting, a floor, and a mannequin already set up, which is useful for testing.
4. Set the following options:
   - **Project Name:** `MoCapAvatar`
   - **Location:** Wherever you keep your UE projects
   - **Blueprint** (not C++, unless you want to use the C++ receiver from the integration guide)
   - **Starter Content:** Enabled (gives you materials and meshes to work with)
5. Click **Create**.
6. Wait for the editor to open. You should see the default Third Person level.

---

## 2. Enable Required Plugins

Before doing anything else, enable the plugins that handle WebSocket communication.

1. Go to **Edit > Plugins** in the top menu bar.
2. In the search box, type **WebSocket**.
3. Find **WebSockets** and make sure the checkbox is **enabled** (it usually is by default in UE5).
4. Search for **WebSocket Networking**. Enable it if it is not already on.
5. Optionally search for **LiveLink** and enable it. LiveLink is an alternative data pipeline that you might use later, but it is not required for this guide.
6. Search for **Json Blueprint Utilities** and enable it. This gives you Blueprint nodes for parsing JSON strings, which is how the pose data arrives.
7. Click **Restart Now** when prompted. The editor will close and reopen your project.

---

## 3. Import or Add an Avatar

You need a rigged skeletal mesh. There are several ways to get one.

### Option A: Use the UE5 Mannequin (Fastest)

The Third Person template already includes `SK_Mannequin` or the newer `SKM_Manny` / `SKM_Quinn`. These are fully rigged and ready to go. If you chose the Third Person template, skip to Step 5.

To verify the mannequin is available:

1. Open the **Content Browser** (bottom panel).
2. Navigate to `Content > Characters > Mannequins > Meshes`.
3. You should see `SKM_Manny` and/or `SKM_Quinn`. These are your avatars.

### Option B: Download a Free Avatar from the Marketplace

1. In the Epic Games Launcher, go to the **Marketplace** tab.
2. Search for free character packs. Some popular options:
   - **Paragon Characters** (free, high quality, fully rigged)
   - **City of Brass: Characters** (free)
3. Click **Add to Project** and select your `MoCapAvatar` project.
4. The character assets will appear in your Content Browser.

### Option C: Import from Mixamo

Mixamo (mixamo.com) provides free rigged characters that work well with UE.

1. Go to [mixamo.com](https://www.mixamo.com) and sign in with an Adobe account (free).
2. Click **Characters** and choose any character you like.
3. Click **Download** with these settings:
   - **Format:** FBX
   - **Pose:** T-Pose
4. In Unreal, go to the Content Browser, right-click in an empty area, and choose **Import**.
5. Browse to your downloaded `.fbx` file and click **Open**.
6. In the import dialog:
   - Make sure **Skeletal Mesh** is checked.
   - Under **Mesh**, check **Import Mesh**.
   - Under **Animation**, uncheck **Import Animations** (we will drive animation from live data, not from a file).
   - Click **Import All**.
7. You should now see your character's skeletal mesh and skeleton in the Content Browser.

### Option D: Import a Custom Character (Blender, Maya, etc.)

If you have your own rigged character:

1. Export it from your 3D software as `.fbx` with these settings:
   - Apply all transforms
   - Include the armature/skeleton
   - Set scale to match UE (1 unit = 1 cm)
   - Export in T-pose or A-pose
2. Import into UE using the same process as Option C above.

**Important:** Whatever character you use, take note of its **Skeleton** asset name. You will need this when creating the Animation Blueprint.

---

## 4. Start the MoCap Bridge

The bridge is a small app that relays data from the browser capture app to Unreal Engine. It runs separately from both.

### 4.1 Install Node.js (One-Time Setup)

The bridge runs on Node.js. Check if you already have it by opening a terminal and running:

```
node --version
```

If you see a version number like `v20.11.0` or higher, skip to step 4.2.

If you see "command not found" or similar, install Node.js:

- Go to [nodejs.org](https://nodejs.org/)
- Download the **LTS** version (the button on the left)
- Run the installer and accept the defaults
- Close and reopen your terminal, then run `node --version` again to confirm

On macOS with Homebrew, you can also run `brew install node`.

### 4.2 Install Dependencies (One-Time Setup)

Open a terminal and navigate to the `bridge` folder inside the project. The exact command depends on where you put the project files. For example:

```
cd ~/Downloads/ue-mocap-studio/bridge
```

Or if you cloned with git:

```
cd ue-mocap-studio/bridge
```

Then install the dependencies:

```
npm install
```

This downloads the required packages into a `node_modules` folder. You only need to run this once. You may see warnings about deprecated packages or vulnerabilities during the install. These are safe to ignore and do not affect the bridge.

### 4.3 Start the Bridge

From the same `bridge` folder, run:

```
npm start
```

You should see:

```
  ╔══════════════════════════════════════════╗
  ║        MoCap Studio - Bridge v1.0        ║
  ╠══════════════════════════════════════════╣
  ║  Browser  ->  ws://localhost:8080        ║
  ║  Unreal   ->  ws://localhost:8081        ║
  ╚══════════════════════════════════════════╝

  Status: Browser: Waiting... | Unreal: Waiting...

  Press Ctrl+C to stop.
```

Leave this terminal window open. The bridge must stay running the entire time you are using MoCap Studio.

Next time you want to start the bridge, you only need to navigate to the folder and run `npm start`. You do not need to run `npm install` again.

### 4.4 Test With the Capture App

Before moving on to Unreal, verify that the bridge and capture app can talk to each other:

1. Open the `mocap-studio.html` file in Chrome. You can double-click it in Finder / File Explorer, or drag it into an open Chrome window.
2. Click **Start Capture**. Chrome asks for camera permission. Click **Allow**.
3. You should see your webcam feed with a skeleton overlay tracking your body.
4. In the sidebar on the right, the WebSocket URL should already say `ws://localhost:8080`. Click **Connect**.
5. Check the bridge terminal. It should now show:

```
  [13:35:xx] Browser connected from ::1
  Status: Browser: Connected | Unreal: Waiting...
```

If you see "Browser connected", the capture side is working. Move around in front of the camera and watch the skeleton track your body. The bridge is relaying this data and waiting for Unreal to connect on the other side.

If the browser does not connect, check that the bridge is still running and that the URL in the sidebar matches `ws://localhost:8080`.

### 4.5 Custom Ports (Optional)

The default ports are 8080 (browser) and 8081 (Unreal). If these conflict with other software on your machine, you can start the bridge with different ports:

```
node mocap-bridge.js --browser-port 9090 --ue-port 9091
```

If you change the ports, update the URL in the browser app sidebar and the `MoCapWebSocket` actor in your Unreal level to match.

### 4.6 Alternative: Standalone Executables (No Node.js)

If you prefer not to install Node.js, you can build a standalone executable from the bridge source, or check the [Releases](https://github.com/ZeroOneTwo-CreativeTechnologyHub/ue-mocap-studio/releases) page to see if pre-built binaries have been published. See [`bridge/README.md`](bridge/README.md) for build instructions and platform-specific details.

---

## 5. Create the WebSocket Receiver

This section adds a C++ class to your project that handles all WebSocket communication and JSON parsing internally. You do not need to write any C++ yourself. The files are provided in the `unreal/` folder of this repository. You just copy them in, add three words to a config file, and compile. After that, you drag the actor into your level and it works.

Unreal's WebSocket module is not exposed to Blueprints by default. The provided C++ class wraps it and exposes all pose data as Blueprint-readable properties. No third-party plugins or marketplace downloads required.

### 5.1 Install a C++ Compiler (One-Time Setup)

Unreal needs a C++ compiler to build the class. If you have already compiled C++ in UE before, skip to step 5.2.

#### Windows

You need **Visual Studio** with the C++ game development workload.

1. Go to [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) and download **Visual Studio Community** (free).
2. Run the installer.
3. When the installer shows a list of **Workloads**, check these two:
   - **Desktop development with C++**
   - **Game development with C++**
4. On the right side panel under "Installation details", confirm these individual components are selected:
   - **MSVC v143 C++ build tools** (or whatever the latest version is)
   - **Windows 10/11 SDK**
5. Click **Install**. This download is several GB and may take a while.
6. Once installed, you do not need to open Visual Studio manually. Unreal will use it automatically in the background.

#### macOS

You need **Xcode**.

1. Open the **App Store** and search for **Xcode**. Install it (it is free but large, around 12 GB).
2. After installation, open Xcode once to accept the license agreement, then close it.
3. Open Terminal and run this command to install the command-line tools:

```
xcode-select --install
```

4. Once complete, Unreal will use Xcode automatically in the background.

#### Linux

Install the build essentials:

```
sudo apt update
sudo apt install build-essential clang
```

### 5.2 Add C++ Support to Your Project

If you selected "C++" when you created the project in Step 1, skip to step 5.3.

If you selected "Blueprint" when creating the project, you need to add C++ support now:

1. In the Unreal Editor, go to **Tools > New C++ Class** in the top menu bar.
2. A wizard appears. Under "Choose Parent Class", select **Actor**.
3. Click **Next**.
4. On the next screen:
   - **Name:** Type `MoCapWebSocket`
   - **Path:** Leave the default (it will be inside your project's `Source` folder)
   - **Class Type:** Public
5. Click **Create Class**.

What happens next:
- Unreal generates two files: `MoCapWebSocket.h` and `MoCapWebSocket.cpp` inside your project's `Source` folder.
- Unreal may open Visual Studio or Xcode automatically. If it does not, that is fine.
- A dialog may appear asking to reload the project or saying "Successfully added class." Click **OK** or **Yes**.
- The Unreal Editor may close and reopen. This is normal for a first-time C++ addition.

**If you see an error** saying no compiler was found, go back to step 5.1 and make sure Visual Studio (Windows) or Xcode (macOS) is fully installed.

### 5.3 Locate Your Project Files

You need to edit three files. Here is where to find them on disk.

Open your project folder in a file explorer (Finder on macOS, File Explorer on Windows). The default locations are:

- **Windows:** `C:\Users\YourName\Documents\Unreal Projects\MoCapAvatar\`
- **macOS:** `/Users/YourName/Documents/Unreal Projects/MoCapAvatar/`

Inside the project folder you will see a `Source` directory with this structure:

```
MoCapAvatar/
├── Source/
│   └── MoCapAvatar/
│       ├── MoCapAvatar.Build.cs        <-- Edit this (step 5.4)
│       ├── MoCapAvatar.h
│       ├── MoCapAvatar.cpp
│       ├── MoCapWebSocket.h            <-- Copy from unreal/ folder (step 5.5)
│       └── MoCapWebSocket.cpp          <-- Copy from unreal/ folder (step 5.5)
```

If you skipped step 5.2 (because your project was already C++), you may not have `MoCapWebSocket.h` and `MoCapWebSocket.cpp` yet. In that case, create both files manually in the `Source/MoCapAvatar/` folder.

You can edit these files with any text editor. Some good free options: **Visual Studio Code** (all platforms), **Notepad++** (Windows), or **TextEdit** (macOS, switch to plain text mode first via Format > Make Plain Text).

### 5.4 Edit the Build File

Open `Source/MoCapAvatar/MoCapAvatar.Build.cs` in your text editor.

Look for a line that contains `PublicDependencyModuleNames`. It will look something like this:

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
```

Replace that entire line with:

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "WebSockets", "Json", "JsonUtilities" });
```

The three additions are `WebSockets`, `Json`, and `JsonUtilities`. These tell Unreal to include those modules when compiling your project.

Save the file.

**Note:** If your project is named something other than `MoCapAvatar`, the folder and file names will match whatever name you chose. The structure is the same.

### 5.5 Copy the C++ Files

The two C++ files are provided in the `unreal/` folder of this repository. Instead of typing them out, copy them directly into your project:

1. Open the `unreal/` folder from the MoCap Studio repository.
2. Copy both files into your project's source folder:
   - Copy `MoCapWebSocket.h` to `Source/MoCapAvatar/MoCapWebSocket.h`
   - Copy `MoCapWebSocket.cpp` to `Source/MoCapAvatar/MoCapWebSocket.cpp`
3. If the files already exist from step 5.2 (when Unreal generated placeholder files), overwrite them.

**Note:** If your project is named something other than `MoCapAvatar`, replace `MoCapAvatar` with your project name in the paths above.

### 5.6 Compile

Go back to the Unreal Editor and compile the new code.

1. Click the **Compile** button in the bottom-right corner of the toolbar (it looks like a stack of blocks).
2. Wait for the progress bar to finish.
3. If successful, you will see a green checkmark and "Compile Complete".

**If compilation fails**, check the Output Log (Window > Developer Tools > Output Log) for error messages. The most common issues are:

- **"Cannot open include file 'IWebSocket.h'"**: The `WebSockets` module was not added to the Build.cs file. Go back to step 5.4.
- **"Unrecognized type 'AMoCapWebSocket'"**: The file names do not match. Make sure they are called `MoCapWebSocket.h` and `MoCapWebSocket.cpp` exactly.
- **General build errors on first compile**: Close Unreal, delete the `Binaries` and `Intermediate` folders from your project directory, reopen the project, and compile again.

### 5.7 Verify the Class Exists

After successful compilation:

1. Click the **Place Actors** panel (or go to Window > Place Actors).
2. Search for `MoCapWebSocket`.
3. It should appear in the list. If it does, the compile worked.

### 5.8 Drop It Into Your Level

1. From the Place Actors panel, drag **MoCapWebSocket** into your level viewport. Place it anywhere, its position does not matter.
2. With the actor selected, look at the **Details** panel on the right. Under the **MoCap | Connection** section you will see:
   - **Server URL**: `ws://localhost:8081` (matches the bridge default, no change needed)
   - **Auto Connect**: checked (the actor connects to the bridge automatically when you press Play)
   - **Reconnect Delay**: 2.0 seconds (if the connection drops, it retries automatically)
3. That is it. No Blueprint wiring needed.

The actor handles everything internally: connecting to the bridge, parsing the JSON, converting coordinates to Unreal world space, tracking idle state, and storing all 33 landmarks plus pre-computed bone angles. The Animation Blueprint reads directly from this actor.

**What the actor exposes to your Animation Blueprint:**

| Property | Type | What It Contains |
|---|---|---|
| `bIsIdle` | Boolean | True when no person is detected. Use this to switch states. |
| `bIsConnected` | Boolean | True when connected to the bridge. |
| `Landmarks` | Array of FMoCapLandmark | All 33 tracked points with raw coords, visibility, and world-space positions. |
| `BoneAngles` | FMoCapBoneAngles | Pre-computed angles: spine tilt, head tilt, elbow, knee, shoulder, hip. |
| `CaptureFPS` | Integer | Frame rate reported by the browser app. |

You can also call `GetLandmarkPosition(Id)` and `GetLandmarkVisibility(Id)` from any Blueprint to get data for a specific joint.

---

## 6. Create the Animation Blueprint

The Animation Blueprint (ABP) is where pose data gets translated into actual bone movements on the avatar.

1. In the Content Browser, right-click and select **Animation > Animation Blueprint**.
2. In the dialog:
   - **Parent Class:** AnimInstance
   - **Target Skeleton:** Select the skeleton that matches your avatar. For the UE5 mannequin, this is `SK_Mannequin` or the Manny/Quinn skeleton.
3. Name it `ABP_MoCap`.
4. Double-click to open it.

### 6.1 Add Variables to the Animation Blueprint

In the **My Blueprint** panel, add these variables:

| Variable Name | Type | How to Set It | Default Value |
|---|---|---|---|
| `MoCapSocket` | MoCapWebSocket (Object Reference) | Type `MoCapWebSocket` in the type dropdown, select **Object Reference** | None |
| `bIsIdle` | Boolean | Select **Boolean** | True |
| `BoneAngles` | FMoCapBoneAngles | Type `MoCapBoneAngles` in the type dropdown | (leave default) |

The `MoCapWebSocket` and `FMoCapBoneAngles` types appear in the dropdown because you compiled the C++ class in step 5.

### 6.2 Update Variables Every Frame

Open the **Event Graph** tab in the Animation Blueprint.

From **Event Blueprint Update Animation:**

1. Drag from the white execution pin on **Event Blueprint Update Animation** and release in empty space. A search menu appears. Type `Get All Actors of Class` and select it. This is a global function, so you must search from empty space or an execution pin, not from another node's data output.
2. On the **Get All Actors of Class** node, click the **Actor Class** dropdown and type `MoCapWebSocket`. Select it.
3. Drag from the **Out Actors** pin and type `Get`. Select **Get (a copy)**. Leave the index at 0. This gets the first (and usually only) MoCapWebSocket actor in the level.
4. Drag your `MoCapSocket` variable from the **My Blueprint** panel into the graph and select **Set**. Connect the blue output pin from the Get node into the blue input pin on the Set node. Unreal will accept the connection automatically. Connect the white execution pin from **Get All Actors of Class** to the **Set MoCapSocket** node.
5. Now read the pose data. Drag `MoCapSocket` into the graph again (select **Get** this time). Drag off it and type `bIsIdle`. Select **Get bIsIdle**. Wire this into a **Set bIsIdle** node for your local variable.
6. Drag off `MoCapSocket` again and type `BoneAngles`. Select **Get BoneAngles**. Wire this into a **Set BoneAngles** node for your local variable.
7. Chain all the white execution pins together in order.

The complete chain looks like this:

```
Event Blueprint Update Animation
  > Get All Actors of Class (MoCapWebSocket)
  > Set MoCapSocket (from Get[0])
  > Set bIsIdle (from MoCapSocket.bIsIdle)
  > Set BoneAngles (from MoCapSocket.BoneAngles)
```

**If the blue wire won't connect** between the Get node and the Set MoCapSocket node, it means the variable type does not match. Select your `MoCapSocket` variable in the My Blueprint panel, check the Details panel, and confirm the Variable Type is set to `MoCap Web Socket` with **Object Reference** (not Class Reference). If it was set to a different type, change it, recompile, and try wiring again.

This runs every tick, keeping the animation data in sync with the latest pose from the camera.

---

## 7. Build the Idle State

Open the **AnimGraph** tab in the Animation Blueprint.

### 8.1 Create the State Machine

1. Right-click in the AnimGraph and select **Add New State Machine**. Name it `MoCap State Machine`.
2. Connect its output to the **Output Pose** node.
3. Double-click the state machine to open it.

### 8.2 Add the Idle State

1. Right-click inside the state machine and select **Add State**. Name it `Idle`.
2. Draw a transition from the **Entry** node to the **Idle** state (drag from Entry to Idle).
3. Double-click the **Idle** state to open it.
4. Inside, add a **Play Animation** node.
5. Set the animation to an idle or breathing animation:
   - For the UE5 mannequin: use `MM_Idle` or any idle animation from the Mannequins Animation pack.
   - For Mixamo characters: go back to mixamo.com, search for "Breathing Idle" or "Standing Idle", download as FBX, and import into your project (this time with Import Animations checked).
6. Set **Loop Animation** to **True**.
7. Connect the output to the state's **Result** node.

---

## 8. Build the MoCap State

### 9.1 Add the MoCap State

1. Go back to the state machine view.
2. Right-click and **Add State**. Name it `MoCapActive`.
3. Create transitions:
   - **Idle to MoCapActive:** Double-click the transition arrow and set the condition to `bIsIdle == False` (drag in the variable, use a NOT Boolean node).
   - **MoCapActive to Idle:** Double-click this transition arrow and set the condition to `bIsIdle == True`.

4. On both transitions, in the Details panel:
   - Set **Blend Duration** to `0.3` seconds. This creates a smooth transition between idle and live tracking.

### 9.2 Build the MoCap Pose Logic

Double-click the **MoCapActive** state to open it.

This is where bones get rotated based on the incoming data. You will use **Transform (Modify) Bone** nodes.

1. Right-click and add a **Modify Bone** node. You will need one for each bone you want to control.

To access the bone angles, drag your `BoneAngles` variable into the graph (Get), then drag off it and select **Break MoCapBoneAngles**. This splits the struct into individual float pins that you can wire directly into Make Rotator nodes.

For each bone, the setup is:

**Spine / Torso:**

1. Add a **Transform (Modify) Bone** node.
2. In the Details panel, set **Bone to Modify** to `spine_03` (or your skeleton's upper spine bone).
3. Set **Rotation Mode** to **Add to Existing**.
4. Set **Rotation Space** to **Component Space**.
5. From the broken-out BoneAngles, take the **Spine Tilt** pin.
6. Use a **Make Rotator** node: put the spine tilt value into the **Pitch** (forward/back lean).
7. Connect to the Rotation input on the Modify Bone node.

**Head:**

1. Add another **Transform (Modify) Bone** node, chained after the spine.
2. Set **Bone to Modify** to `head`.
3. Use the **Head Tilt** pin from BoneAngles.
4. Feed it into the **Roll** of a Make Rotator (lateral head tilt).

**Left Upper Arm:**

1. Add a **Transform (Modify) Bone** for `upperarm_l`.
2. Use the **Left Shoulder Angle** pin.
3. Calculate the rotation: the raw angle represents the angle between the torso and the upper arm. Convert this to a bone rotation by subtracting the rest pose angle (typically around 90 degrees for a T-pose).
4. Feed the result into the **Pitch** or **Yaw** depending on your skeleton's orientation.

**Left Forearm:**

1. Add a **Transform (Modify) Bone** for `lowerarm_l`.
2. Use the **Left Elbow Angle** pin.
3. The elbow is a hinge joint, so the rotation goes into a single axis. For most UE skeletons this is **Roll** or **Yaw** on the forearm bone.
4. Subtract the rest angle (180 = fully straight) and negate if needed.

**Repeat for the right side** using **Right Shoulder Angle** and **Right Elbow Angle**.

**Left Thigh:**

1. Add a **Transform (Modify) Bone** for `thigh_l`.
2. Use the **Left Hip Angle** pin.
3. Feed into the appropriate rotation axis.

**Left Calf:**

1. Add a **Transform (Modify) Bone** for `calf_l`.
2. Use the **Left Knee Angle** pin.

**Repeat for the right leg** using **Right Hip Angle** and **Right Knee Angle**.

### 9.3 Chain the Nodes Together

Each Transform (Modify) Bone node has a pose input and a pose output. Chain them in sequence:

```
[Start] > Spine > Head > L.UpperArm > L.Forearm > R.UpperArm > R.Forearm > L.Thigh > L.Calf > R.Thigh > R.Calf > [Result]
```

The first node in the chain can take a base pose. Use a **Play Animation** node with your idle animation (not looping) as the base, or use a **Mesh Space Ref Pose** node to start from the default skeleton pose.

### 9.4 Add Smoothing (Important)

To prevent jittery movement, add interpolation to each rotation:

1. Between the map lookup and the Make Rotator, add a **RInterp To** (Rotator Interpolation) node.
2. Set **Current** to the previous frame's rotator (store it in a variable).
3. Set **Target** to the new calculated rotator.
4. Set **Interp Speed** to a value between 5.0 and 15.0. Lower = smoother but slower response. Higher = more responsive but potentially jittery.
5. Use **Delta Time** from the animation update event.

### 9.5 Compile and Save

Click **Compile** and fix any warnings. Common issues at this stage:
- Bone names do not match your skeleton. Open the skeleton asset and verify the exact names.
- Rotation axes are wrong. You may need to experiment with which axis (Pitch, Yaw, Roll) produces the correct movement for each bone.

---

## 9. Map Bones from MediaPipe to Your Skeleton

The bone names differ between skeletons. Here is the mapping for the UE5 Mannequin. If you are using a different character, open your Skeleton asset, expand the bone hierarchy, and find the equivalent bones.

| Body Part | MediaPipe Joint | UE5 Mannequin Bone | Modify Bone Axis |
|---|---|---|---|
| Spine | spine_tilt (derived) | `spine_03` | Pitch |
| Head | head_tilt (derived) | `head` | Roll |
| Left Shoulder | left_shoulder (ID 11) | `upperarm_l` | Pitch + Yaw |
| Left Elbow | left_elbow (ID 13) | `lowerarm_l` | Roll |
| Left Wrist | left_wrist (ID 15) | `hand_l` | Pitch |
| Right Shoulder | right_shoulder (ID 12) | `upperarm_r` | Pitch + Yaw |
| Right Elbow | right_elbow (ID 14) | `lowerarm_r` | Roll |
| Right Wrist | right_wrist (ID 16) | `hand_r` | Pitch |
| Left Hip | left_hip (ID 23) | `thigh_l` | Pitch |
| Left Knee | left_knee (ID 25) | `calf_l` | Pitch |
| Left Ankle | left_ankle (ID 27) | `foot_l` | Pitch |
| Right Hip | right_hip (ID 24) | `thigh_r` | Pitch |
| Right Knee | right_knee (ID 26) | `calf_r` | Pitch |
| Right Ankle | right_ankle (ID 28) | `foot_r` | Pitch |

**Note on rotation axes:** The "Modify Bone Axis" column above is a starting point. Depending on how your skeleton is authored, you may need to swap axes or negate values. The quickest way to figure this out is to temporarily set a constant rotation value (like 45 degrees) on each axis one at a time and observe which direction the bone moves.

---

## 10. Place the Avatar in Your Level

1. Go back to the main level editor.
2. Delete or move the existing Third Person character if you do not need it.
3. From the Content Browser, drag your **Skeletal Mesh** (e.g. `SKM_Manny`) into the viewport. Position it where you want the avatar to stand.
4. With the skeletal mesh selected, look at the **Details** panel on the right.
5. Under **Animation**, set:
   - **Animation Mode:** Use Animation Blueprint
   - **Anim Class:** Select `ABP_MoCap` (the Animation Blueprint you created)
6. The `MoCapWebSocket` actor should already be in your level from step 5.8. If not, drag it in from the Place Actors panel now.
7. With the MoCapWebSocket actor selected, check the Details panel:
   - **Server URL** should be `ws://localhost:8081` (or whatever UE port the bridge is using).
8. Save the level.

---

## 11. Connect and Test

If you followed Step 4 earlier, you already have the bridge running and the browser capture app connected. If not, go back and complete Step 4 first.

Follow this exact order:

### Step 1: Confirm the Bridge and Browser Are Running

Check the bridge terminal window. It should show:

```
  Status: Browser: Connected | Unreal: Waiting...
```

If it says "Browser: Waiting...", open `mocap-studio.html` in Chrome, click Start Capture, allow the camera, and click Connect in the sidebar. See Step 4.4 for details.

### Step 2: Play in Unreal

Back in the UE editor, click **Play** (the green Play button in the toolbar, or press Alt+P).

The `MoCapWebSocket` actor connects to the bridge automatically on BeginPlay. Check two places to confirm:

1. **Unreal Output Log** (Window > Developer Tools > Output Log): Look for `MoCap: Connected to ws://localhost:8081`.
2. **Bridge terminal**: It should update to show:

```
  [13:36:xx] UE client connected from ::1 (1 total)
  Status: Browser: Connected | Unreal: 1 connected
  Relaying at ~30 msg/s to 1 UE client(s)
```

If the Output Log shows "MoCap: Connection error" instead, see the Troubleshooting section below.

### Step 3: Move

Stand in front of your webcam. You should see:
- The skeleton overlay in the browser app tracking your body.
- The avatar in Unreal mirroring your movements.

When you step out of frame, the browser sends an idle signal, and the avatar should transition back to the idle breathing animation.

### Step 4: Iterate

At this point you will almost certainly need to adjust:
- **Rotation axes** on specific bones (swap Pitch/Yaw/Roll).
- **Rotation direction** (negate some values if a bone rotates the wrong way).
- **Scale multipliers** on angles to make movements more or less pronounced.
- **Interpolation speed** to balance smoothness vs. responsiveness.

This tuning is normal and expected. Make changes in the Animation Blueprint, recompile, and test again. You do not need to restart the bridge or browser app between changes. Just stop Play in UE, make your edits, and press Play again.

---

## 12. Troubleshooting

### "MoCap: Connection error" in the Output Log

- Confirm the bridge app is running and shows the startup banner.
- Check that the URL in your `MoCapWebSocket` actor in the level matches the bridge's UE port (default: `ws://localhost:8081`).
- Check your firewall is not blocking local WebSocket connections.

### Avatar does not move at all

- Open the Output Log and check for JSON parsing errors.
- Check the Output Log for JSON parsing errors. The `MoCapWebSocket` actor logs connection events automatically.
- Make sure the Animation Blueprint's `MoCapSocket` variable is finding the actor (the Get All Actors of Class node is returning a result).
- Verify the Animation Blueprint is assigned to the skeletal mesh component in the level.

### Avatar moves but in the wrong directions

- Open your Skeleton asset and examine the bone orientations. Different skeletons have different axis conventions.
- Change which axis you feed each angle into on the Transform (Modify) Bone nodes.
- Try negating the angle value.

### Movement is very jittery

- Increase the **Smoothing** slider in the browser app (0.6-0.8 works well).
- Increase the **Interp Speed** denominator in the Animation Blueprint's RInterp To nodes (lower interp speed = smoother).
- Reduce the **Send Rate** in the browser app to 15-20 fps. Higher is not always better if the data is noisy.

### Skeleton overlay works in browser but no data reaches Unreal

- Make sure you connected the browser to port 8080 and UE to port 8081 (not the same port).
- Check the bridge terminal window. You should see "Browser connected" when the browser connects and "UE client connected" when Unreal connects.
- If the bridge shows both connected but Unreal receives nothing, try stopping Play in UE and pressing Play again.

### Limbs bend beyond realistic angles

- Add **Clamp Float** nodes in the Animation Blueprint to restrict rotation ranges. For example, clamp elbow angles between 0 and 160 degrees.

### Performance is poor

- Reduce the model complexity in `mocap-studio.html`. Open the file in a text editor and change `modelComplexity: 1` to `modelComplexity: 0` (faster but less accurate).
- Reduce the send rate to 15 fps.
- Make sure you are running Chrome with hardware acceleration enabled (Settings > System > Use hardware acceleration).

---

## What to Do Next

Once basic tracking is working, consider these enhancements:

**Inverse Kinematics (IK):** Add a Two Bone IK node for each leg in the AnimGraph so that the feet plant realistically on the ground instead of floating.

**Root Motion:** Use the `hip_center` data from the JSON bones object to drive the avatar's world position, allowing the character to walk around the scene.

**Facial Animation:** The roadmap for MoCap Studio includes face mesh support. When available, you can drive blend shapes / morph targets on the avatar's face for expressions and lip sync.

**Multiple Cameras:** For better depth estimation, consider running two browser instances with cameras at different angles and averaging the landmark positions in the receiver Blueprint.

**Recording and Playback:** Use the browser app's record feature to capture a session as JSON, then write a Blueprint that reads the JSON file and plays it back through the same Animation Blueprint. Useful for reviewing performances without a live camera.
