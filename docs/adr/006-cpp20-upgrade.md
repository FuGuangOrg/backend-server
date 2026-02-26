# ADR 006: Upgrade from C++17 to C++20

**Date:** 2026-02-26
**Status:** Accepted

## Context

The project used C++17. C++20 is fully supported by MSVC 2022 v143 and Qt 6.9.2, and provides features directly applicable to this codebase.

## Decision

Set `<LanguageStandard>stdcpp20</LanguageStandard>` in all `.vcxproj` files; `set(CMAKE_CXX_STANDARD 20)` in `fuguang-algo/CMakeLists.txt`.

## Applicable C++20 Features

| Feature | Application in this project |
|---|---|
| `std::jthread` | Replace `std::thread` in worker threads — auto-joins on destruction, no dangling threads |
| `std::span<float>` | Replace `float* data + size` in predictor/shape_match hot paths |
| `std::ranges::sort` | Replace `std::sort` in NMS result sorting |
| `[[nodiscard]]` | Mark `detect_objects()`, `get_shape_match_result()`, etc. |
| Designated initializers | Cleaner `st_fiber_end_algorithm_parameter{.m_score_thresh = 0.7}` construction |
| `std::filesystem` | Replace `QFile`/`QDir` where already started |
| `std::format` (partial) | Structured log message formatting with spdlog |

## Compatibility Assessment

- MSVC 2022 v143: full C++20 support (confirmed)
- Qt 6.9.2: internal code uses C++17; public API is C++17-compatible; using C++20 in client code is documented as supported
- No `import` modules (CMake support incomplete); no `std::coroutines` (not needed)

## Consequences

**Positive:** safer threading (jthread), cleaner API surface (nodiscard), modern algorithms (ranges)

**Negative:** minimal — one-line change per project file; no library compatibility issues identified
