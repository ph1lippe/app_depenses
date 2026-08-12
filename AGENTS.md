# AGENTS.md

## Project context
- This project is a Qt/C++ desktop application built with CMake.
- The build should use the repository's configured CMake presets and existing build tasks rather than ad hoc compiler commands.
- Keep UI code in `src/ui`, domain logic in `src/domain`, and persistence code in `src/persistence`.
- Do not modify generated files or build artifacts under `build/` or other generated folders.

## Coding rules
- Follow the existing project structure and naming conventions already used in the codebase.
- Keep business logic separate from the GUI layer.
- Prefer Qt idioms such as `QObject`, signals/slots, layouts, and widget ownership patterns already used in the app.
- Keep changes minimal and targeted to the task at hand.
- Use RAII, const-correctness, and clear ownership semantics.
- Favor readability and maintainability over clever shortcuts.
- Avoid broad refactors unless the task explicitly requires them.

## Testing and validation
- Do not run the full CMake build after every small code modification.
- Only rebuild when the change is substantial, affects compilation, or is required to validate the fix.
- Prefer targeted validation and focused checks over repeated full project rebuilds.
- Run focused tests when behavior is changed or a bug fix is made.
- For user-visible behavior changes, prefer real behavior checks over mock-only assertions.
- Do not claim success until the relevant build/test command completes successfully.

## Implementation expectations
- Make the minimum necessary change to fix the root cause.
- Reuse existing helpers and models before introducing parallel implementations.
- Preserve existing behavior unless the task explicitly changes it.
- If a UI or startup behavior is updated, verify the app still launches normally.
- If an assumption is needed, document it clearly in the code or commit description.

## Quality bar
- Respect the current app design and user experience.
- Keep functions small, focused, and understandable.
- Keep comments useful and sparse.
- Prefer code that matches the style and structure already present in the repository.
