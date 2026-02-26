# ADR 002: Remove Qt from the algorithm layer

**Date:** 2026-02-26
**Status:** Accepted

## Context

`basic_algorithm` and related headers contained Qt types:
- `QString m_area_name` in `st_detect_area` (a pure data struct)
- `QFile`, `QTextStream`, `QDomDocument` in `fiber_end_algorithm.h`
- `QJsonArray` in algorithm result processing

This made the algorithm DLL dependent on QtCore and QtXml, preventing compilation on non-Qt toolchains.

## Decision

Remove all Qt types from the algorithm layer boundary:

1. `QString` → `std::string` everywhere in data structs (UTF-8 with `/utf-8` MSVC flag)
2. `QDomDocument` XML config loading moved to `fuguang-server/config_loader` (uses pugixml)
3. `QJsonArray` result serialization moved to server layer (uses nlohmann/json)
4. `QFile`/`QDir` → `std::filesystem`

**Boundary rule:** `projects/basic_algorithm/` must compile with zero Qt includes.

## Consequences

**Positive:**
- `fuguang-algo` builds on macOS/Linux with standard toolchain
- Algorithm headers no longer pull Qt into consumer code

**Negative:**
- One-time migration of 4 files with moderate effort
- `std::string` requires explicit UTF-8 handling for Chinese strings (use `u8"..."` literals)
