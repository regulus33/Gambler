Run the project tests, analyze results, and fix any issues.

## Steps

1. **Build and run tests:**
   Run the full test pipeline:
   ```
   cmake -B build && cmake --build build --target Tests && ./build/Tests
   ```

2. **Analyze test results:**
   - If any tests fail, identify the failing test(s) and root cause.
   - Check `git diff HEAD~1` to see what changed in the last commit — determine if the failure was introduced by the most recent change.

3. **Fix bugs if introduced by last change:**
   - If a test failure was introduced by the last commit, fix the bug in the source code.
   - Re-run the tests to confirm the fix works.
   - Report what was broken and how it was fixed.

4. **Check JUCE assertion report:**
   Read `tests/jassert_report.md` and review its contents:
   - If there are **actionable assertions** (non-zero actionable count), report each unique location, hit count, and propose a fix if the cause is apparent.
   - If all assertions are **suppressed/known-benign**, briefly confirm that no actionable assertions were found.

5. **Report summary to user:**
   Provide a concise summary:
   - Test pass/fail status
   - Any bugs found and fixed
   - JUCE assertion report highlights
