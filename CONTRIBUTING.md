# Contributing to fuguang-server

## Branch Strategy

Same as `fuguang-algo`: `feature/<name>` → `develop` → `main`.

## Development Setup

```powershell
git clone --recurse-submodules https://github.com/FuGuangOrg/backend-server.git
cd fuguang-server
# Open fuguang-server.sln in Visual Studio 2022
# Select Debug | x64
```

### Submodule (fuguang-algo)

The algorithm library lives at `third_party/fuguang-algo/`. After `--recurse-submodules` it is automatically checked out. To update:

```powershell
cd third_party/fuguang-algo
git pull origin main
cd ..\..
git add third_party/fuguang-algo
git commit -m "Update fuguang-algo to <commit-hash>"
```

## Testing Without Hardware

The camera and motion control interfaces are pure virtual base classes:
- `interface_camera` (`device_camera/interface_camera.h`)
- `motion_control` (`motion_control/motion_control.h`)

Inject mock implementations for unit/integration tests:

```cpp
class MockCamera : public interface_camera {
public:
    QImage capture() override { return QImage(":/test_images/fiber.png"); }
    // ... stub other methods
};
```

The `--mock-hardware` flag (CLI11) causes the server to instantiate mocks instead of real hardware drivers, enabling:
- End-to-end JSON command/response testing
- Algorithm integration tests
- CI runs without physical equipment

## PR Checklist

- [ ] Build passes: `msbuild fuguang-server.sln /p:Configuration=Release /p:Platform=x64`
- [ ] Protocol changes documented in `docs/PROTOCOL.md`
- [ ] Config changes documented in `docs/CONFIG_REFERENCE.md`
- [ ] Architectural decisions recorded in `docs/adr/`
- [ ] No Qt includes in `third_party/fuguang-algo/` (submodule boundary)
- [ ] `request_id` echoed in all responses

## ADR Process

When making a significant architectural decision, create `docs/adr/NNN-short-title.md` using this template:

```markdown
# ADR NNN: Title

**Date:** YYYY-MM-DD
**Status:** Proposed | Accepted | Superseded by ADR-NNN

## Context
## Decision
## Consequences
```

ADRs are append-only — do not edit accepted ADRs; create a new one to supersede.
