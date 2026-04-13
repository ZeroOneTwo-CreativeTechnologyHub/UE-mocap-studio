# MoCap Bridge

The WebSocket relay that sits between the browser capture app and Unreal Engine. It receives pose data from the browser on one port and forwards it to all connected Unreal Engine clients on another.

```
Browser (port 8080)  -->  MoCap Bridge  -->  Unreal Engine (port 8081)
```

---

## Run the Bridge

### Step 1: Install Node.js (one-time)

Check if Node.js is already installed by opening a terminal and running:

```
node --version
```

If you see a version number like `v20.11.0` or higher, skip to Step 2.

If you see "command not found" or similar:

- Go to [nodejs.org](https://nodejs.org/)
- Download the **LTS** version (the button on the left)
- Run the installer and accept the defaults
- Close and reopen your terminal, then run `node --version` again to confirm

On macOS with Homebrew you can also run `brew install node`.

### Step 2: Install dependencies (one-time)

Open a terminal and navigate to the `bridge` folder inside the project. The exact path depends on where you put the files. For example:

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

This downloads the required packages into a `node_modules` folder. You only need to run this command once. You may see warnings about deprecated packages or vulnerabilities during the install. These are safe to ignore and do not affect the bridge.

### Step 3: Start the bridge

From the same folder, run:

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

Next time you want to start the bridge, navigate to the folder and run `npm start`. You do not need to run `npm install` again.

### Step 4: Verify the connection

1. Open `mocap-studio.html` in Chrome (double-click the file or drag it into a browser window).
2. Click **Start Capture** and allow camera access.
3. In the sidebar, the URL should already say `ws://localhost:8080`. Click **Connect**.
4. The bridge terminal should update to:

```
  [13:35:xx] Browser connected from ::1
  Status: Browser: Connected | Unreal: Waiting...
```

When you later press Play in Unreal, you will see:

```
  [13:36:xx] UE client connected from ::1 (1 total)
  Status: Browser: Connected | Unreal: 1 connected
  Relaying at ~30 msg/s to 1 UE client(s)
```

---

## Custom Ports

The default ports are 8080 (browser) and 8081 (Unreal). If these conflict with other software on your machine:

```
node mocap-bridge.js --browser-port 9090 --ue-port 9091
```

If you change the ports, update the URL in the browser app sidebar and the `MoCapWebSocket` actor in your Unreal level to match.

---

## Build Standalone Executables (Optional)

If you want to create a standalone binary that runs without Node.js installed (useful for distributing to team members who do not have Node), you can build one from source.

From the `bridge` folder:

```
npm install
npm run build
```

This creates executables for all platforms in the `bridge/dist/` folder:

| Output file | Platform |
|---|---|
| `mocap-bridge-win.exe` | Windows (64-bit) |
| `mocap-bridge-mac-arm64` | macOS (Apple Silicon: M1, M2, M3, M4) |
| `mocap-bridge-mac-x64` | macOS (Intel) |
| `mocap-bridge-linux` | Linux (64-bit) |

To build for a single platform only:

```
npm run build:win         # Windows
npm run build:mac-arm     # macOS Apple Silicon
npm run build:mac-intel   # macOS Intel
npm run build:linux       # Linux
```

**Not sure which macOS version?** Click the Apple menu, then **About This Mac**. If it says "Apple M1" (or M2, M3, M4) under Chip, build with `build:mac-arm`. If it says "Intel", use `build:mac-intel`.

### Running the standalone executable

**Windows:** Double-click `mocap-bridge-win.exe`. Windows may show a SmartScreen warning because the app is not code-signed. Click **More info**, then **Run anyway**.

**macOS:** Open Terminal, navigate to the `dist` folder, make it executable, and run:

```
chmod +x mocap-bridge-mac-arm64
./mocap-bridge-mac-arm64
```

macOS may block it with "cannot be opened because the developer cannot be verified." If this happens, go to **System Settings > Privacy & Security**, scroll down, and click **Allow Anyway** next to the mocap-bridge entry. Then run the command again.

**Linux:**

```
chmod +x mocap-bridge-linux
./mocap-bridge-linux
```

### Publishing to GitHub Releases

To make the executables available for download on the repository's Releases page:

1. Build all platforms with `npm run build`.
2. Go to your repository on GitHub.
3. Click **Releases** on the right side of the page.
4. Click **Draft a new release**.
5. Create a tag (e.g. `v1.0.0`), add release notes, and attach the four files from `bridge/dist/`.
6. Click **Publish release**.

Users can then download the correct executable from the Releases page without needing Node.js.

---

## Troubleshooting

**"Port XXXX is already in use"**
Another instance of the bridge (or another app) is using that port. Close it, or run with different ports using `--browser-port 9090 --ue-port 9091`. Update the URLs in the browser app and Unreal to match.

**Bridge starts but nothing connects**
Make sure the browser app URL matches the bridge's browser port (default `ws://localhost:8080`) and the Unreal URL matches the UE port (default `ws://localhost:8081`).

**npm install fails**
Make sure you are in the `bridge` folder (the one containing `package.json`). Run `ls` (macOS/Linux) or `dir` (Windows) to check. You should see `mocap-bridge.js` and `package.json` listed.

**npm start says "Cannot find module 'ws'"**
Run `npm install` first. The `ws` package needs to be downloaded before the bridge can start.
