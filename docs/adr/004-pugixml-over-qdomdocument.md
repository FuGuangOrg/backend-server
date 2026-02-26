# ADR 004: Use pugixml for XML config parsing

**Date:** 2026-02-26
**Status:** Accepted

## Context

Config files (`config.xml`, `parameter.xml`) were parsed with Qt's `QDomDocument`. Moving parsing to the server layer (per ADR 002) requires a non-Qt XML library.

## Candidates

| Library | Size | Speed vs tinyxml2 | vcpkg | XPath |
|---|---|---|---|---|
| tinyxml2 | small | 1× | ✓ | ✗ |
| pugixml | small | 2-3× faster | ✓ | ✓ |
| libxml2 | large | faster | ✓ | ✓ |
| RapidXML | header-only | fastest | manual | ✗ |

## Decision

**pugixml.**

Reasons:
- 2-3× faster than tinyxml2 (measured)
- XPath support (useful for parameter queries)
- Single-header + single-source, zero external deps
- First-class vcpkg support (`vcpkg install pugixml`)
- Simple, modern API

libxml2 was rejected (too large, C API). RapidXML was rejected (no vcpkg, modifies input string in-place, unmaintained).

## Usage Pattern

```cpp
#include <pugixml.hpp>

pugi::xml_document doc;
doc.load_file("config.xml");
auto root = doc.child("config");
int speed = root.child("move_speed").text().as_int(3000);
```

## Consequences

- Config loading code lives in `fuguang-server/config_loader.cpp` (not in `fuguang-algo`)
- vcpkg `fuguang-server/vcpkg.json` includes `"pugixml"`
- Algorithm struct (`st_fiber_end_algorithm_parameter`) is populated by the server and passed to the algo init call
