# Mushkin C++26 Migration Roadmap

## Per-Module Status

| Module | Dir | Status | unique_ptr | span | expected | print | Reflection | Raw `new` Count |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| World | `src/world/` | 🟢 Complete | Yes (line buffer, MXP, logfile, notepad QPointer) | Yes (LoadImageMemory, compression) | Yes (WorldError for registry ops + notepad ops) | No | No | 0 (~11 Qt parent-child; MXP maps now use unique_ptr — no qDeleteAll) |
| Automation | `src/automation/` | 🟢 Complete | Yes (plugin, variable, regexp) | N/A (no buffers) | Yes (compileRegexp, SaveState, LoadState) | No | No | 0 |
| Text | `src/text/` | 🟢 Complete | Yes (line.h) | Yes (Line::text() returns std::span) | N/A (no fallible functions) | No | No | 0 |
| Network | `src/network/` | 🟢 Complete | Yes (remote_client) | Yes (WorldSocket, AnsiFormatter) | Yes (StartError, send/receive) | No | No | 0 (3 Qt parent-child) |
| Storage | `src/storage/` | 🟢 Complete | Yes (Meyer's singletons) | Yes (geometry lambdas) | Yes (StorageError) | No | No | 0 |
| UI | `src/ui/` | 🟢 Complete | Yes (WorldWidget in openWorld, Trigger/Timer in edit dialogs) | N/A (no buffer parameters) | Yes (loadFromFile, saveToFile, savePluginXml) | No | No | 0 (~60 Qt parent-child, QMimeData clipboard ownership, QTableWidgetItem/QListWidgetItem widget ownership) |
| Utils | `src/utils/` | 🟢 Complete | N/A (no ownership) | N/A (no buffers) | N/A (no fallible functions) | No | No | 0 |
| Third-Party | `src/third-party/` | N/A (C code) | — | — | — | — | — | — |

## Signal/Slot Connections
- **UI module:** ~255 `connect()` calls across 10+ files (all functor-based, no SIGNAL/SLOT macros)
- **Current focus:** Converting to C++26 generic lambdas where beneficial

## Priority Queue
1. ~~**Storage** — DONE. Meyer's singletons, `std::expected<void, StorageError>` for all fallible ops, `std::span` for geometry lambdas. Reviewed by `/reviewer`.~~
2. ~~**Network** — DONE. `std::span<const char>` for socket send/receive, `std::span<const unsigned char>` for SendPacket, `std::expected<void, StartError>` for server start, `std::expected<qint64, QString>` for socket I/O. 3 raw `new` calls documented as Qt parent-child. Reviewed by Sonnet reviewer.~~
3. ~~**World** — DONE. `std::unique_ptr<Line>` for line buffer, `std::unique_ptr<MXPArgument/ElementItem>` for MXP types, `std::unique_ptr<QFile>` for logfile. `std::vector<unsigned char>` for zlib compression buffers. `std::span<const unsigned char>` for LoadImageMemory. `std::expected<void, WorldError>` for trigger/alias/timer registry ops, InitZlib, notepad ops. `std::array` for all fixed-size arrays (colors, macros, keypad, IAC tracking, sound buffers). `LuaDatabase` destructor added for SQLite handle cleanup. `[[nodiscard]]` on all registry ops. `CleanupMXP()` called in destructor. ~11 raw `new` calls documented as Qt parent-child. MXP top-level maps (`AtomicElementMap`, `CustomElementMap`, `MXPEntityMap`, `ActiveTagList`) now use `std::unique_ptr` values — all `qDeleteAll`/`delete` calls removed. Re-verified: UAF in plugin/world-level replacement paths fixed (bExecutingScript guard), NotepadWidget WA_DeleteOnClose + QPointer<QMdiSubWindow>, notepad bool→expected conversion. Reviewed by Sonnet reviewer + Gemini architect (OpenRouter).~~
4. ~~**Text** — DONE. `Line::text()` returns `std::span<const char>` (read-only; mutation uses `textBuffer.data()` directly). No fallible functions in module. No raw `new`. Mutable span overload removed per review (off-by-one with null terminator). Reviewed by Sonnet reviewer + Gemini architect (OpenRouter).~~
5. ~~**Utils** — DONE. Module already compliant — no raw `new`, no fallible functions needing `std::expected`, no buffer parameters needing `std::span`. Pure utility functions (path resolution, logging categories, name generation, font scaling). Only raw pointer is `QRandomGenerator::global()` (Qt singleton, not owned).~~
6. ~~**UI** — DONE. `std::array` for all C-style arrays (colors_page, output_page, world_properties_dialog). `std::expected<void, QString>` for WorldWidget::loadFromFile/saveToFile and PluginWizard::savePluginXml. `std::make_unique` for WorldWidget in openWorld(), Trigger in saveTrigger(), Timer in saveTimer(). `[[nodiscard]]` on all expected-returning functions. compileRegexp() errors now surfaced to user (was silently discarded). ~60 raw `new` calls documented as Qt parent-child (QMdiArea, layouts, widgets). QMimeData clipboard ownership transfer documented. QTableWidgetItem/QListWidgetItem widget ownership documented. Reviewed by Sonnet reviewer.~~
7. ~~**Automation** — DONE. `QRegularExpression*` → `std::unique_ptr<QRegularExpression>` in Alias and Trigger. `compileRegexp()` → `std::expected<void, QString>`. `SaveState()`/`LoadState()` → `std::expected<void, QString>`. RAII scope guard for `m_bSavingStateNow`. `[[nodiscard]]` on all `std::expected` functions. `ExecutePluginScript()` kept as `bool` (event propagation signal). Non-owning `WorldDocument*`/`Plugin*` observers documented. Reviewed by Sonnet reviewer.~~

## Acceptance Criteria
A module is **not** marked 🟢 Complete until:
- All raw `new` replaced with `std::unique_ptr` or Qt parent-child (documented which)
- All fallible functions return `std::expected<T, E>`
- All buffer parameters use `std::span` instead of pointer + size
- Reviewed by `/reviewer` for memory safety sign-off
