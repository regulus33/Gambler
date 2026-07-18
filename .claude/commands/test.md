---
name: dev:verify
description: Build, run CMake tests, check JUCE assertions, and auto-fix regressions.
allowed-tools:
  - Bash
  - Read
  - Grep
  - Glob
---

<objective>
Execute a full validation suite: compile the project, run CTest/GoogleTest binaries, analyze regressions from the last commit, and audit JUCE-specific assertion reports.
</objective>

<process>
1. **Build & Execute:** Run the pipeline: `cmake -B build && cmake --build build --target Tests && ./build/Tests`.
   
2. **Regression Analysis:**
   If tests fail:
   - Run `git diff HEAD~1` to isolate recent changes.
   - Compare failing test cases against the diff to determine if the current work introduced the bug.
   
3. **Reactive Fixing:**
   - If a failure is linked to the recent diff, attempt a surgical fix in the source code.
   - Immediately re-run the build/test command to verify the fix.
   
4. **JUCE Audit:**
   Read `tests/jassert_report.md`.
   - **Actionable:** If "actionable count" > 0, extract the file paths and hit counts.
   - **Benign:** If counts are zero or suppressed, simply acknowledge the clean state.

5. **Final Briefing:**
   Provide a structured summary of: Test Status, Regressions Fixed, and JUCE Assertion Health.
</process>

<guardrails>
- **Build Cleanliness:** If `cmake` fails, check for missing dependencies or compiler errors before blaming the test logic.
- **Context Preservation:** Do not delete the `build` directory unless a full clean is required to resolve a linking error.
- **Reporting:** Always provide the specific line number for `jassert` failures to save the human time.
</guardrails>