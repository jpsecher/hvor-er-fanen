---
name: dev-loop
description: The project's core development iteration. Use when implementing a task or making any feature, fix, or change to project code — drives small test-first iterations (list tests, red, green, refactor), repeating until the test list is empty.
---

# Development loop

Work in small test-first iterations.  One trip through the loop should add one passing test and the minimal code to satisfy it.  Repeat until the current task's test list is empty.

## The loop

1. **Consider architecture** — decide whether the change needs any structural adjustment before writing code.  Separate pure logic from I/O so logic tests need no mocks; design for testability first, and if a test would need mocking, redesign to minimise it — mocks are a last resort for true infrastructure boundaries only.  Configuration and framework wiring are not logic and have no seam worth extracting for tests — cover them by exercising the behaviour they produce.
2. **List the tests** — enumerate the behaviours the task requires as a checklist of tests in `.claude/TESTS.local.md` (create it if missing), the working list for the active task.  Each entry names something observable at a boundary — a return value, a status code, a persisted record, an emitted message — not an internal value read back.  Prefer the outermost boundary the change is visible at, and reach inward only when an outer test cannot isolate the cause.  Two entries satisfied by the same branch of code are one test, not two — list the branch being exercised, not each input/output pair that flows through it.  A test is automated by default; prefix a check that can only be confirmed by hand with `(manual)`.  The order or exact content is not fixed — re-prioritise the list as the implementation reveals what to tackle next.
3. **Red** — pick the next test to tackle from the list, implement just that test, and run it to watch it fail for the right reason.  Do this for one test at a time — never write or run more than one test-list entry before its implementation is green and ticked off.  If it passes on the first run, the test might be restating behaviour that already exists or the implementation itself: rewrite it to target the behaviour that is still missing, or strike it from the list.  Never edit working code to manufacture a red.  A `(manual)` check has no red/green run — carry it to step 7 and verify it by hand.
4. **Plan** — decide the smallest implementation that will make the test pass.
5. **Green** — write the simplest code that passes the test; nothing more.
6. **Refactor** — clean up the code while keeping the tests green, running them as you go.
7. **Update** — tick off the test in `.claude/TESTS.local.md`.
8. **Repeat** — return to step 3 with the next test; when `.claude/TESTS.local.md` is empty, the task is done.

## Notes

- A failing test means a code defect, not an infrastructure problem.
- After the task's list is empty, do the after-task bookkeeping from `.claude/CLAUDE.md` (update `.claude/PROJECT.md` and `.claude/TASKS.md`, and delete `.claude/TESTS.local.md`).
