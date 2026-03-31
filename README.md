# DT4MOB Twin Viewer

Digital Twin for Mobility — an Unreal Engine 5.6 application for visualizing transportation entities (cars, barriers, infrastructure, etc.) from a [Ditto](https://www.eclipse.org/ditto/) digital twin backend in a 3D world.

---

## Prerequisites

| Requirement | Version / Notes |
|---|---|
| Unreal Engine | 5.6 (install via Epic Games Launcher) |
| Visual Studio | 2022 with **Game Development with C++** workload |
| Ditto backend | Running instance accessible over HTTP and WebSocket |

### Marketplace Plugins

The following plugins must be installed into your UE 5.6 engine via the Epic Games Launcher before opening the project:

- **glTFRuntime** — runtime glTF/glb asset loader
- **Cesium for Unreal** — geospatial world streaming

---

## Setup

### 1. Clone the repository

```bash
git clone <repository-url>
cd dt4mob-twin-viewer
```

### 2. Configure secrets

Copy the secrets template and fill in your Ditto credentials:

```bash
cp Config/Secrets.ini.example Config/Secrets.ini
```

Edit `Config/Secrets.ini`:

```ini
[Ditto]
Username=<your-ditto-username>
Password=<your-ditto-password>
BaseUrl=<http://your-ditto-host/api>
WsUrl=<ws://your-ditto-host/ws/2>
WsAuthHeader=<Basic base64(username:password)>
WsStartMessage=<Ditto START-SEND-EVENTS JSON command>
```

> **`Secrets.ini` is git-ignored — never commit it.**

### 3. Generate project files

Right-click `DT4MOB.uproject` and select **Generate Visual Studio project files**, or run:

```bash
"C:\Work\Apps\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="C:\Work\Project\dt4mob-twin-viewer\DT4MOB.uproject" -game -rocket -progress
```

### 4. Open in Unreal Editor

Double-click `DT4MOB.uproject`, or launch from the command line:

```bash
"C:\Work\Apps\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Work\Project\dt4mob-twin-viewer\DT4MOB.uproject"
```

The editor will compile the C++ module on first launch. This may take a few minutes.

---

## Building (VS Code)

The workspace file `DT4MOB.code-workspace` includes pre-configured build tasks. Open the workspace in VS Code and use **Ctrl+Shift+B** to select a build configuration:

| Configuration | Use case |
|---|---|
| `Win64 Development` | Standard editor development (recommended) |
| `Win64 DebugGame` | Full C++ debugger support in-game |
| `Win64 Debug` | Full debugger support including engine code |
| `Win64 Shipping` | Optimized release build |

Each configuration also has **Rebuild** and **Clean** variants.

---

## Runtime behaviour

- `DittoService` and `WSService` are `UGameInstanceSubsystem`s — they initialize automatically when the game instance starts.
- Both read `Config/Secrets.ini` at startup. Watch the Output Log for warnings about missing or empty fields.
- The WebSocket client reconnects automatically on disconnect (exponential backoff, up to 5 retries).
- Ditto API queries are paginated at 200 items per page.

---

## Generating Doxygen documentation

A `Doxyfile` is included at the project root. Requirements: [Doxygen](https://www.doxygen.nl/) installed and on `PATH`.

```bash
doxygen Doxyfile
```

Output is written to `Docs/Doxygen/html/`. Open `Docs/Doxygen/html/index.html` in a browser to view the docs.

The Doxyfile is configured to:
- Parse all `.h` and `.cpp` files under `Source/DT4MOB/`
- Expand UE macros (`UCLASS`, `USTRUCT`, `UPROPERTY`, etc.) so they do not pollute the output
- Include private members, static members, and local classes
- Generate cross-reference links (referenced-by / references relations)

---

## Project structure

```
dt4mob-twin-viewer/
├── Config/
│   ├── Secrets.ini.example   # Template — copy to Secrets.ini and fill in
│   └── Secrets.ini           # Git-ignored credentials
├── Source/DT4MOB/
│   ├── Public/
│   │   ├── Services/         # DittoService, WSService, CoordinatesConversionService, EntityUpdateDaemon
│   │   ├── Entities/         # Entity factory and actor types
│   │   ├── EntityStructs/    # Data structs for 15+ entity types (Car, Barrier, Sign, …)
│   │   ├── Gameplay/         # Pawn, controller, camera manager, movement components
│   │   ├── Managers/         # SelectionManager, UIManager
│   │   └── UI/               # UMG widget classes
│   └── Private/              # Implementations
├── Plugins/
│   └── glTFRuntime/          # Bundled plugin source
├── Docs/Doxygen/             # Generated API docs (not committed)
├── Doxyfile                  # Doxygen configuration
└── DT4MOB.uproject
```
