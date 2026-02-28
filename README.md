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

## Building

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2022+)
- Qt 6.2+ (Core, Gui, Network, Xml components)
- CMake 3.20+
- vcpkg (recommended) or system package manager

### Dependencies

**Required:**
- Qt6 (Core, Gui, Network, Xml)
- OpenCV 4.x
- spdlog

**Optional:**
- asio (for networking)
- nlohmann_json (for JSON parsing)
- pugixml (for XML parsing) 
- CLI11 (for command line parsing)
- libmodbus (for PLC communication)
- ONNXRuntime (for AI inference, if `FUGUANG_ENABLE_INFERENCE=ON`)

### Cross-Platform Build (CMake)

```bash
# Install dependencies via vcpkg (recommended)
vcpkg install asio nlohmann-json spdlog opencv4 pugixml concurrentqueue libmodbus cli11

# Clone with submodules
git clone --recurse-submodules https://github.com/FuGuangOrg/backend-server.git
cd backend-server

# Configure with vcpkg toolchain
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DFUGUANG_ENABLE_INFERENCE=OFF \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run
./build/bin/fiber_end_server
```

### Windows Build (Visual Studio)

```powershell
# Open fuguang-server.sln in Visual Studio 2022
# Select Release | x64 and Build Solution
```

Or via MSBuild:

```powershell
msbuild fuguang-server.sln /p:Configuration=Release /p:Platform=x64
```

### macOS/Linux Build

```bash
# Install dependencies via system package manager
# Ubuntu/Debian:
sudo apt install qtbase6-dev qtbase6-dev-tools libopencv-dev libspdlog-dev

# macOS:
brew install qt6 opencv spdlog

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/bin/fiber_end_server
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
