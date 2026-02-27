# Mushkin C++26 Migration Roadmap

## Per-Module Status

| Module | Dir | Status | unique_ptr | span | expected | print | Reflection | Raw `new` Count |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| World | `src/world/` | 🟡 Partial | Yes (13 in header) | No | No | No | No | ~20 |
| Automation | `src/automation/` | 🟡 Partial | Yes (plugin, variable) | No | No | No | No | ~2 |
| Text | `src/text/` | 🟡 Partial | Yes (line.h) | No | No | No | No | 0 |
| Network | `src/network/` | 🟡 Partial | Yes (remote_client) | No | No | No | No | ~2 |
| Storage | `src/storage/` | 🟢 Complete | Yes (Meyer's singletons) | Yes (geometry lambdas) | Yes (StorageError) | No | No | 0 |
| UI | `src/ui/` | 🔴 Legacy | No | No | No | No | No | ~66 |
| Utils | `src/utils/` | 🔴 Legacy | No | No | No | No | No | 0 |
| Third-Party | `src/third-party/` | N/A (C code) | — | — | — | — | — | — |

## Signal/Slot Connections
- **UI module:** ~255 `connect()` calls across 10+ files (all functor-based, no SIGNAL/SLOT macros)
- **Current focus:** Converting to C++26 generic lambdas where beneficial

## Priority Queue
1. ~~**Storage** — DONE. Meyer's singletons, `std::expected<void, StorageError>` for all fallible ops, `std::span` for geometry lambdas. Reviewed by `/reviewer`.~~
2. **Network** — Safety-critical. Replace raw buffer passing with `std::span`, error returns with `std::expected`.
3. **World** — Largest module. Already has partial `unique_ptr` adoption; needs `std::expected` for Lua API error paths.
4. **UI** — 66+ raw `new` calls, but most are Qt parent-child (safe). Focus on the non-parented allocations.
5. **Automation** — Mostly done for ownership. Needs `std::expected` for plugin load/eval failures.

## Acceptance Criteria
A module is **not** marked 🟢 Complete until:
- All raw `new` replaced with `std::unique_ptr` or Qt parent-child (documented which)
- All fallible functions return `std::expected<T, E>`
- All buffer parameters use `std::span` instead of pointer + size
- Reviewed by `/reviewer` for memory safety sign-off
