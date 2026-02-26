# ADR 001: Split monorepo into three repositories

**Date:** 2026-02-26
**Status:** Accepted

## Context

The original `IndustrialInspection` monorepo contained algorithm, hardware control, and Qt GUI code in a single Visual Studio solution. This caused:

1. Algorithm engineers could not develop on macOS/Linux — Qt and Windows camera SDKs were hard transitive dependencies of everything
2. Coupling made partial builds impossible — changing a UI string required recompiling the algorithm DLL
3. No cross-platform CI was feasible
4. Reviewers could not give targeted feedback (algorithm PR containing UI noise)

## Decision

Split into three repositories:

| Repo | Content | Platform |
|---|---|---|
| `FuGuangOrg/inspection-algo` | Pure C++ algorithm library | Win / Mac / Linux |
| `FuGuangOrg/backend-server` | Hardware control + backend service | Windows x64 |
| `FuGuangOrg/inspection-gui` | Qt 6.9 GUI client | Windows x64 |

`fuguang-server` and `fuguang-ui` reference `fuguang-algo` as a git submodule.

## Consequences

**Positive:**
- Algorithm engineers develop on any OS without installing Qt or camera SDKs
- PRs can be clearly scoped to one layer
- Cross-platform CI for `fuguang-algo` is now straightforward
- Future: `fuguang-algo` can be shipped as a vcpkg port

**Negative:**
- Submodule management requires discipline (updating the pointer after algo changes)
- Initial migration effort (this migration)
- Two Windows repos still require MSVC/Qt for builds
