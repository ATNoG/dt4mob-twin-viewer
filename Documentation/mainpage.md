# DT4MOB Twin Viewer {#mainpage}

The DT4MOB Twin Viewer is an Unreal Engine 5.6 application that shows the DT4MOB mobility
digital twin on a 3D globe. It reads digital twin "things" (vehicles, toll plazas, road
assets, sensors, weather stations, wildfire ignition points) from an
[Eclipse Ditto](https://www.eclipse.org/ditto/) backend, places them on a
[Cesium for Unreal](https://cesium.com/platforms/unreal/) globe, and keeps them up to date
through Ditto's WebSocket event stream.

Developed by the ATNoG group of Instituto de Telecomunicacoes (Aveiro) for the DT4MOB
project. Source: https://github.com/ATNoG/dt4mob-twin-viewer

## Running the application

The packaged application starts on the **Login** map (`/Game/Scenes/Login/Login`). The user
enters the backend host, username and password in ULoginWidget. UBackendConnectionService
authenticates against the backend, and UCredentialStoreService can store the credentials for
the current OS user (Windows DPAPI or the macOS Keychain) so the next launch logs in without
showing the form. After a successful login, ALoginPlayerController opens the **Scene** map
(`/Game/Scenes/WorldMap/Scene`), where ADT4MOBGamemode streams entities onto the globe.
Logging out from the viewer goes back to the Login map.

## Architecture

The code lives in one runtime module, `DT4MOB`, split into these areas (each one is a
Doxygen group, see the *Topics* page):

| Area | Group | Main types |
|---|---|---|
| Services | @ref services | UBackendConnectionService, UWSService, UEntityUpdateDaemon, UCoordinatesConversionService, UCredentialStoreService, UGlbModelService, UActorRegistryService |
| Backend abstraction | @ref backend | IBackendRestClient, IBackendEventSubscription, IBackendEnvelopeParser and their Ditto implementations |
| Entities | @ref entities | ATempUIActor, UDT4MOBEntityFactory, RegisterAllEntityTypes() |
| Entity data structs | @ref entitystructs | One USTRUCT tree per Ditto thing type, for example FCarData and FTollData |
| Entity type extensions | @ref entitydeps | UEntityTypeExtension, UEntityBehaviorComponent and the per-type subclasses |
| Gameplay | @ref gameplay | ADT4MOBGamemode, AUnifiedPawn, AUnifiedController, the login game mode |
| Managers | @ref managers | USelectionManager, UUIManager, UPlacementManager |
| User interface | @ref ui | URootHUDWidget, UEntityWindowWidget, the outline, toolbar, options and theme classes |
| Sign benchmark | @ref signbenchmark | A separate harness for measuring sign spawning strategies |

### Services

The services are `UGameInstanceSubsystem`s, except UCoordinatesConversionService, which is a
plain rooted singleton. Unreal creates the subsystems with the game instance and they survive
level changes, so the login state carries over from the Login map to the Scene map.

- **UBackendConnectionService** is the entry point for REST calls. It loads the connection
  settings from the `DA_DittoSecrets` data asset, runs the login, and exposes thing queries
  (all things, things in a set of map tiles, a single thing). The Ditto specifics live in
  `UDittoRestClient` behind the IBackendRestClient interface.
- **UWSService** owns the WebSocket to Ditto (`/ws/2`). It sends the start message and the
  event filter, and reconnects with exponential back-off when the connection drops.
- **UEntityUpdateDaemon** parses every WebSocket message into a thing id, a JSON path and a
  value, and forwards it to the actor registered for that thing.
- **UCoordinatesConversionService** converts WGS-84 latitude, longitude and height to Unreal
  world positions through the Cesium georeference. All geographic placement goes through it.
- **UGlbModelService** downloads and caches glTF/GLB models at runtime (through glTFRuntime),
  for example the road sign and lighting models served from the backend's S3 bucket.

### Entities

Each Ditto thing becomes one ATempUIActor. The actor keeps the thing's raw JSON and a
reflected copy of it in the struct registered for that thing type. UDT4MOBEntityFactory picks
the struct by matching registered keys against the thing id (for example `traci` for
vehicles or `tolls:camera` for toll cameras) and spawns the actor.

Type-specific behaviour is not written into ATempUIActor. It comes from a UEntityTypeExtension
subclass registered with the type. It sets the badge label and colour, the default fields
shown in the info tab, and how live updates are handled (staleness timeout, whether a
WebSocket event may spawn a new actor). Larger behaviours (fire simulation cones, toll camera
animation, geometry meshes) are UEntityBehaviorComponent subclasses; the extension names the
component class and the actor creates it. The default model for a type is the mesh path given
when the type is registered.

### Live updates

```
UWSService  ->  UEntityUpdateDaemon::HandleSocketMessage()
            ->  parse envelope into (thingId, path, value)
            ->  ATempUIActor::HandleEntityUpdate()
                  apply the patch to the raw JSON and the struct
                  move the actor if its coordinates changed
                  broadcast OnEntityDataChanged so open UI refreshes
```

A patch can replace the whole thing (`/`), an attribute subtree (`/attributes/...`) or a
feature property (`/features/<name>/properties/...`).

Vehicles are a special case. The simulation stamps the whole fleet with one shared expiry
time, so the viewer ignores it and removes a vehicle only when that vehicle has not sent an
update for 60 seconds (see UVehicleExtension).

### Tile streaming

ADT4MOBGamemode keeps the number of actors bounded by loading only what is near the camera.
Every 0.5 s it turns the camera altitude into a map zoom level and works out the 3 x 3 block
of map tiles around the camera. When that set of tiles changes and then stays the same for
0.75 s, it refreshes:

- the WebSocket event filter is narrowed to the new tiles, so only nearby updates arrive;
- if the zoom level changed, all actors are removed and every tile is fetched again;
- otherwise only the tiles that left the block are unloaded and only the new ones fetched.

Actors that are on screen or have an open window are kept until that is no longer true. A zoom level
change has to persist for 2 s before it counts, which stops the tiles from reloading when the
camera sits on a zoom boundary.

Below zoom level 7 the tile filter is dropped and all things load, except simulated vehicles,
which are too small to see at that distance.

### Camera

AUnifiedPawn has two modes (ECameraMode):

- **RTS**: a top-down view on a spring arm. The arm length is the zoom. The pawn is aligned to
  the globe's local north and east every tick.
- **FreeFly**: the arm is collapsed and the camera moves freely, kept upright relative to the
  globe surface.

UUnifiedMovementComponent turns input into movement for both modes.

### Selection and UI

USelectionManager (a local player subsystem) tracks the hovered and selected actor and
broadcasts when either changes. Actors react by turning their custom-depth outline on or off.
URootHUDWidget listens to the selection and opens a UEntityWindowWidget for the selected
thing, with tabs for the main fields, the raw JSON, the model layers and the associated things. Colours come from the theme data assets
(dark and light) selected in UUIThemeSettings.

## Adding a new entity type

1. Add a header in `Public/EntityStructs/` with USTRUCTs that mirror the thing's JSON. Field
   names must match the JSON keys exactly, because FJsonObjectConverter fills them by name.
2. Optionally add a UEntityTypeExtension subclass in `Public/EntityDependencies/<Type>/` for
   the badge, default info fields or live update behaviour.
3. Add one `Factory.RegisterType(...)` line to RegisterAllEntityTypes() in
   `Private/Entities/EntityTypeRegistrations.cpp`, with the thing id key, the struct, the
   display name, an optional default mesh path and the extension class.

## Configuration

| Setting | Where | Notes |
|---|---|---|
| Backend connection | `Content/Data/DA_DittoSecrets` (UDittoSecretsAsset) | Default host for the login form, HTTPS on or off, OAuth or Basic auth, OAuth client id, WebSocket start message. Holds no credentials. |
| Credentials | Entered at runtime | Stored per OS user by UCredentialStoreService when the user asks to be remembered. |
| UI themes | Project Settings, UUIThemeSettings | Dark and light UUIThemeData assets. |
| Transparency material | Project Settings, UMeshVisualSettings | Material used when a model layer is made translucent. |

## Network access

All connections are outgoing on TCP 443 (HTTPS and secure WebSocket):

- the DT4MOB backend host (Ditto REST API `/api/2/...`, WebSocket `/ws/2`, Keycloak
  `/auth/realms/dt4mob/...` and model files under `/s3/...`)
- Cesium ion (`api.cesium.com`, `assets.ion.cesium.com`) for the globe, terrain and 3D tiles

## Building this documentation

Run `doxygen Doxyfile` in the repository root. The HTML goes to `Docs/Doxygen/html/` and
warnings go to `Docs/Doxygen/warnings.log`. The Doxyfile does not extract undocumented
symbols, so anything new that lacks a comment shows up as a warning in that log.
