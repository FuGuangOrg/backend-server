# ADR 003: Use git submodule instead of git subtree for fuguang-algo

**Date:** 2026-02-26
**Status:** Accepted

## Context

`fuguang-server` and `fuguang-ui` both depend on `fuguang-algo`. Two common approaches exist:

1. **git submodule** — a pointer to a specific commit in another repo
2. **git subtree** — a copy of the other repo's history merged in

## Decision

Use **git submodule**.

## Reasons

1. **Independent history:** each repo retains its own clean history without cross-contamination
2. **Upstream push is natural:** `cd third_party/fuguang-algo && git push` — no special command
3. **Pointer semantics match our model:** server/UI pin to a known-good algo commit; updating is an explicit, reviewable act
4. **subtree complexity:** `git subtree push --prefix=...` is error-prone and hard to remember; subtree history is larger

## Operations Reference

```bash
# Clone with submodule
git clone --recurse-submodules <repo>

# After initial clone without --recurse
git submodule update --init --recursive

# Update algo to latest main
cd third_party/fuguang-algo && git pull origin main && cd ../..
git add third_party/fuguang-algo
git commit -m "Update fuguang-algo to <version>"
```

## Consequences

**Positive:** clean separation, natural upstream contributions, small repo size

**Negative:** developers must remember `--recurse-submodules` on clone; detached HEAD in submodule confuses newcomers (document in CONTRIBUTING.md)
