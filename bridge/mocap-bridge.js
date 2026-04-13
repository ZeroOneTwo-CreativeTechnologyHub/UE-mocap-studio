#!/usr/bin/env node

// ============================================================
//  MoCap Studio - WebSocket Bridge
//  Relays pose data from the browser capture app to Unreal Engine.
//
//  Usage:
//    node mocap-bridge.js
//    node mocap-bridge.js --browser-port 8080 --ue-port 8081
//
//  Or run the packaged executable:
//    ./mocap-bridge            (macOS / Linux)
//    mocap-bridge.exe          (Windows)
//
//  Part of UE MoCap Studio (open source, MIT license).
// ============================================================

const { WebSocketServer } = require("ws");

// ── Configuration ──────────────────────────────────────────
const args = parseArgs(process.argv.slice(2));
const BROWSER_PORT = args["browser-port"] || 8080;
const UE_PORT = args["ue-port"] || 8081;
// ───────────────────────────────────────────────────────────

// Track connected clients
const ueClients = new Set();
let browserSocket = null;
let messageCount = 0;
let lastStatsTime = Date.now();

// ── Browser-facing server (receives pose data) ────────────
const browserServer = new WebSocketServer({ port: BROWSER_PORT });

browserServer.on("listening", () => {
  log(`Browser server listening on ws://localhost:${BROWSER_PORT}`);
});

browserServer.on("connection", (ws, req) => {
  const addr = req.socket.remoteAddress;
  browserSocket = ws;
  log(`Browser connected from ${addr}`);
  printStatus();

  ws.on("message", (data) => {
    messageCount++;
    const msg = data.toString();

    // Relay to all connected UE clients
    for (const client of ueClients) {
      if (client.readyState === 1) {
        client.send(msg);
      }
    }
  });

  ws.on("close", () => {
    browserSocket = null;
    log("Browser disconnected");
    printStatus();
  });

  ws.on("error", (err) => {
    log(`Browser socket error: ${err.message}`);
  });
});

browserServer.on("error", (err) => {
  if (err.code === "EADDRINUSE") {
    error(`Port ${BROWSER_PORT} is already in use. Close any other MoCap Bridge instances or choose a different port with --browser-port.`);
    process.exit(1);
  }
  error(`Browser server error: ${err.message}`);
});

// ── UE-facing server (sends pose data to Unreal) ─────────
const ueServer = new WebSocketServer({ port: UE_PORT });

ueServer.on("listening", () => {
  log(`Unreal Engine server listening on ws://localhost:${UE_PORT}`);
});

ueServer.on("connection", (ws, req) => {
  const addr = req.socket.remoteAddress;
  ueClients.add(ws);
  log(`UE client connected from ${addr} (${ueClients.size} total)`);
  printStatus();

  ws.on("close", () => {
    ueClients.delete(ws);
    log(`UE client disconnected (${ueClients.size} remaining)`);
    printStatus();
  });

  ws.on("error", (err) => {
    log(`UE socket error: ${err.message}`);
  });
});

ueServer.on("error", (err) => {
  if (err.code === "EADDRINUSE") {
    error(`Port ${UE_PORT} is already in use. Close any other MoCap Bridge instances or choose a different port with --ue-port.`);
    process.exit(1);
  }
  error(`UE server error: ${err.message}`);
});

// ── Stats ticker ──────────────────────────────────────────
setInterval(() => {
  const now = Date.now();
  const elapsed = (now - lastStatsTime) / 1000;
  const fps = Math.round(messageCount / elapsed);
  messageCount = 0;
  lastStatsTime = now;

  if (browserSocket && ueClients.size > 0) {
    process.stdout.write(`\r  Relaying at ~${fps} msg/s to ${ueClients.size} UE client(s)    `);
  }
}, 3000);

// ── Startup banner ────────────────────────────────────────
console.log("");
console.log("  ╔══════════════════════════════════════════╗");
console.log("  ║        MoCap Studio - Bridge v1.0        ║");
console.log("  ╠══════════════════════════════════════════╣");
console.log(`  ║  Browser  ->  ws://localhost:${String(BROWSER_PORT).padEnd(13)}║`);
console.log(`  ║  Unreal   ->  ws://localhost:${String(UE_PORT).padEnd(13)}║`);
console.log("  ╚══════════════════════════════════════════╝");
console.log("");
printStatus();
console.log("");
console.log("  Press Ctrl+C to stop.");
console.log("");

// ── Graceful shutdown ─────────────────────────────────────
process.on("SIGINT", () => {
  console.log("\n\n  Shutting down...");
  browserServer.close();
  ueServer.close();
  for (const client of ueClients) {
    client.close();
  }
  if (browserSocket) {
    browserSocket.close();
  }
  process.exit(0);
});

// ── Helpers ───────────────────────────────────────────────
function log(msg) {
  const time = new Date().toLocaleTimeString();
  console.log(`\n  [${time}] ${msg}`);
}

function error(msg) {
  console.error(`\n  ERROR: ${msg}`);
}

function printStatus() {
  const browser = browserSocket ? "Connected" : "Waiting...";
  const ue = ueClients.size > 0 ? `${ueClients.size} connected` : "Waiting...";
  console.log(`  Status: Browser: ${browser} | Unreal: ${ue}`);
}

function parseArgs(argv) {
  const result = {};
  for (let i = 0; i < argv.length; i++) {
    if (argv[i].startsWith("--") && i + 1 < argv.length) {
      const key = argv[i].slice(2);
      const val = argv[i + 1];
      result[key] = isNaN(val) ? val : parseInt(val, 10);
      i++;
    }
  }
  return result;
}
