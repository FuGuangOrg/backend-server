# ADR 007: Adaptive image transport (shared memory vs TCP stream)

**Date:** 2026-02-26
**Status:** Accepted

## Context

The original design had a critical inconsistency: the UI connection dialog allowed entering a remote IP address, but images were always delivered via `QSharedMemory` — an OS-level IPC mechanism that only works between processes on the **same machine**. A client connecting from a remote IP would receive image metadata (via TCP) but the shared memory read would silently fail.

## Decision

Introduce an `ImageTransport` abstract interface with two implementations:

```cpp
class ImageTransport {
public:
    virtual ~ImageTransport() = default;
    virtual bool send_image(const cv::Mat& img, const std::string& type) = 0;
};

class SharedMemoryTransport : public ImageTransport { ... };
class TcpImageTransport : public ImageTransport { ... };
```

**Selection logic (server side):**
- Client connects from `127.0.0.1` or `::1` → `SharedMemoryTransport`
- Client connects from any other IP → `TcpImageTransport`

(Optionally: UI connection dialog includes an explicit "Local" / "Remote" toggle.)

### SharedMemoryTransport

Wraps existing `image_shared_memory` implementation (triple-buffered `QSharedMemory`). No change to the same-machine code path — zero performance regression.

### TcpImageTransport

New implementation:
- Opens a second TCP listener on port **5556**
- Images compressed with `cv::imencode(".jpg", img, buf, {IMWRITE_JPEG_QUALITY, 85})`  → ~10:1 compression ratio
- Framing: `[4-byte BE length][JSON header][JPEG bytes]`
- JSON header: `{"frame_id": N, "type": "trigger"|"annotated", "width": W, "height": H, "jpeg_quality": 85}`
- Port 5556 is independent of port 5555 to avoid large image transfers blocking command responses

## Consequences

**Positive:**
- Fixes the silent cross-machine failure
- Clean abstraction: `thread_misc` calls `m_transport->send_image()` regardless of mode
- Same-machine performance unchanged

**Negative:**
- ~600 KB/frame over TCP (JPEG 85) vs zero-copy shared memory — acceptable for 30fps streams, needs profiling at higher frame rates
- Additional port (5556) to document and configure in firewall

## Why not ZeroMQ / gRPC

- ZeroMQ: adds a new dependency; existing TCP infrastructure is sufficient
- gRPC: adds protobuf dependency; overkill for binary blob transfer
- Custom TCP: simplest, already have framing logic in `send_process_result()`
