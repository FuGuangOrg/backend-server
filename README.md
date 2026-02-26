# fuguang-server

Windows backend service for the fiber optic connector end-face inspection system. Runs as a standalone process and accepts commands from `fuguang-ui` over TCP port 5555.

## Architecture

```
fuguang-server.exe
  ├── fiber_end_server  (QTcpServer, port 5555)
  │     ├── thread_algorithm   — detection pipeline orchestration
  │     ├── thread_misc        — camera + auto-focus + image delivery
  │     ├── thread_motion_control — axis movement commands
  │     └── thread_device_enum — camera enumeration
  └── third_party/fuguang-algo (submodule)
```

Image delivery to client:
- **Same machine (127.0.0.1):** shared memory (`trigger_image`, `detect_image`)
- **Remote IP:** TCP port 5556, JPEG-compressed stream

## Building (Windows)

### Prerequisites

- Visual Studio 2022 (v143 toolset)
- Qt 6.9.2 (MSVC 2022 64-bit)
- vcpkg with manifest mode
- `fuguang-algo` submodule (pulled automatically)

### Steps

```powershell
git clone --recurse-submodules https://github.com/FuGuangOrg/backend-server.git
cd fuguang-server
# Open fuguang-server.sln in Visual Studio 2022
# Select Release | x64 and Build Solution
```

Or via MSBuild:

```powershell
msbuild fuguang-server.sln /p:Configuration=Release /p:Platform=x64
```

## Running

```
fuguang-server.exe [options]

Options:
  --config <path>     Path to config.xml (default: ./config.xml)
  --port <port>       TCP listen port (default: 5555)
  --log-level <lvl>   Log level: trace|debug|info|warn|error (default: info)
  --mock-hardware     Start with simulated camera and motion control
```

## Configuration

See [docs/CONFIG_REFERENCE.md](docs/CONFIG_REFERENCE.md) for all config.xml fields.

## TCP Protocol

See [docs/PROTOCOL.md](docs/PROTOCOL.md) for the full command reference.

## Documentation

- [TCP Protocol](docs/PROTOCOL.md)
- [Hardware Setup](docs/HARDWARE_SETUP.md)
- [Config Reference](docs/CONFIG_REFERENCE.md)
- [Architecture Decisions](docs/adr/)
- [Project Overview](docs/01_PROJECT_OVERVIEW.md)
- [Learning Guide](docs/03_LEARNING_GUIDE.md)

## Repository Relationship

```
fuguang-server
  └── third_party/fuguang-algo  (git submodule → FuGuangOrg/inspection-algo)
```

Update the algorithm submodule:

```powershell
cd third_party/fuguang-algo
git pull origin main
cd ../..
git add third_party/fuguang-algo
git commit -m "Update fuguang-algo to latest"
```
