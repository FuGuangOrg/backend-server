# ADR 005: Replace Boost with standalone Asio for serial port and TCP

**Date:** 2026-02-26
**Status:** Accepted

## Context

`motion_control_plc` and `motion_control_port` used Boost.Asio for serial port I/O and async TCP reading. Boost was installed as a monolithic package (130+ components), but only `asio` (2 headers, no compilation required) was actually used.

## Decision

Replace `boost/asio.hpp` with standalone [Asio](https://think-async.com/Asio/) (header-only, MIT license).

**Code change — 3 lines per affected file:**

```cpp
// Before
#include "boost/asio.hpp"
#include "boost/asio/serial_port.hpp"
#include "boost/asio/deadline_timer.hpp"  // deprecated

// After
#include <asio.hpp>
#include <asio/serial_port.hpp>
#include <asio/steady_timer.hpp>          // modern replacement
```

All API types (`asio::io_context`, `asio::serial_port`, `asio::error_code`) are identical. `deadline_timer` → `steady_timer` is a direct API-compatible replacement.

## Consequences

**Positive:**
- Eliminates 130-component Boost installation (~800 MB source)
- Standalone Asio is header-only → no compile step, vcpkg `"asio"` package
- `steady_timer` over `deadline_timer` is the modern standard (deadline_timer deprecated in Asio 1.11)

**Negative:** none — zero API breakage, confirmed by visual diff of affected files
