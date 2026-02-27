# Mushkin Project Context

You are working on **Mushkin**, a high-performance C++ desktop application.

## Tech Stack
- **Language:** C++26 (migrating from C++17). See `docs/migration_status.md` for per-module progress.
- **Compiler:** Clang 19+
- **UI:** Qt 6.9.3 Widgets (No QML)
- **Build:** CMake + Ninja. `cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- **Tests:** GoogleTest via `ctest --test-dir build --output-on-failure`

## Safety Principles (Non-Negotiable)
- `std::span` for all buffer passing. C-style arrays forbidden.
- Every raw `new` is a bug. Use Qt parent-child or `std::unique_ptr`.
- Fallible functions return `std::expected<T, E>`. No `bool` + output params.
- Use C++26 `std::meta` for serialization where possible.

## High-Level Instructions
- Always prefer C++26 features (std::expected, std::span, Reflection, Ranges).
- Replace raw pointer ownership with `std::unique_ptr` or Qt's parent system.
- Use functor-based `connect()` with generic lambdas for signal/slot.
- All logic changes require a corresponding GoogleTest case.

## Agent Council
See `AGENTS.md` for the full subagent council. This Gemini instance serves as the **Architect** role — global refactoring planning and large-context analysis.
