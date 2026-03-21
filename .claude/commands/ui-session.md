---
name: dev:ui-session
description: Start a UI development session with visual snapshot baseline and iterative change verification.
allowed-tools:
  - Bash
  - Read
  - Write
  - Edit
  - Grep
  - Glob
---

<objective>
Establish a visual baseline of the current UI, then guide iterative development — implementing changes, generating new snapshots after each one, and reading the images to verify the result looks correct.
</objective>

<process>
1. **Build & Baseline:**
   - Build: `cmake --build build --target Tests`
   - Capture current UI state as golden references: `UPDATE_SNAPSHOTS=1 ./build/Tests "[visual]"`
   - Read every file in `tests/snapshots/` that does NOT end in `_ACTUAL.png` or `_DIFF.png`.
   - Describe what you see in each image: layout, colours, visible components, anything notable.

2. **Session Briefing:**
   Tell the user:
   - Which components now have golden snapshots
   - A brief visual description of the current UI state
   - That you're ready to work — ask what they'd like to change

3. **Development Loop** (repeat for each change):
   a. Implement the requested UI change in source.
   b. Build: `cmake --build build --target Tests`
   c. Compare against goldens: `./build/Tests "[visual]"` (no UPDATE_SNAPSHOTS)
   d. Read all `_ACTUAL.png` files that were written (these show the current render).
      Also read `_DIFF.png` files if present to see exactly what changed.
   e. Report findings: describe what changed visually, flag anything that looks wrong or unintended.
   f. Ask the user: does this look correct, or should we adjust?

4. **Promote a Change:**
   When the user confirms a change looks correct, lock it in as the new golden:
   `UPDATE_SNAPSHOTS=1 ./build/Tests "[visual]"`
   The new state is now the baseline for subsequent changes.
</process>

<guardrails>
- **Never run UPDATE_SNAPSHOTS without explicit user confirmation** that the change looks correct.
- **Always read the _ACTUAL images** after each test run — this is how you see the current UI without a running plugin.
- **If the build fails**, fix compilation errors before attempting any snapshot capture.
- **If a component renders as a blank/empty image**, note it — the component likely has zero bounds and needs a setSize() call in the test.
- **Scope**: only touch files the user asks about. Do not refactor surrounding code.
</guardrails>
