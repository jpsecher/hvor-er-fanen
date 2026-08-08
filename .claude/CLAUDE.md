# CLAUDE.md — Project Instructions

Generic instructions for Claude Code when working on projects.
See `.claude/PROJECT.md` for project-specific requirements.

## Making changes

- Keep changes small and focused — one concern at a time
- Do not add features beyond what was asked
- Do not add comments unless the logic is genuinely non-obvious
- Avoid empty lines in the middle of sections
- Prefer SOLID, YAGNI & KISS
- Apply DRY only after the rule of three, same intent, not same shape
- Never break a CLI command across lines as it prevents copy-paste
- Write plain, unambiguous English that is easy for a non-native speaker to understand — avoid idioms and figurative phrases, and state things directly rather than creatively
- Markdown rules:
   - Do not break lines
   - add two blanks between sentences
   - Avoid fenced code blocks unless explicit syntax highlighting is needed
- In running text, refer to code identifiers by name only, without trailing punctuation
- After completing a task:
   - Keep the `.claude/TASKS.md` entry to a simple, one-line summary — no implementation detail
   - Record detail worth keeping in `.claude/PROJECT.md`, or a dedicated `.claude/*.md` file when it is its own topic
   - Migrate settled documentation onward into the project's real docs (such as `docs/`) so `.claude` is a staging area, not its permanent home
   - Run the `markdown-cleanup` skill over the markdown the task changed
   - Order `.claude/TASKS.md` with pending entries at the top and completed ones below, newest first
   - Prune `.claude/TASKS.md` to the last week of entries (but at least 3), trimming the oldest from the bottom so pending work is never cut
   - Link to new all-caps files `.claude/*.md`

## Memory

- Long-term decisions belong in committed repo files — not in the auto-memory system
- Prefer small iterations and put future tasks that can be defered in `.claude/TASKS.md`
- `.claude/TASKS.md` is committed and shared — it is where deferred work the whole team needs to see belongs
- `.claude/TASKS.local.md` is personal — git-ignored, never committed, and free of the ordering and pruning rules above
- Subtasks that come up while working on the current task go in `.claude/TASKS.local.md`, not `.claude/TASKS.md`
- The `dev-loop` skill keeps its per-task test checklist in `.claude/TESTS.local.md`, which is also personal and git-ignored
- Only use auto-memory (`~/.claude/projects/.../memory/`) for short-term session context

## Session start

At the beginning of every session, read these files to get full context:
1. `.claude/PROJECT.md` — authoritative description of the current end state
2. `.claude/TASKS.md` — history of completed tasks and pending work, plus `.claude/TASKS.local.md` when present
3. `.claude/DEV-ENV.md` — Nix/Docker dev container workflow; read when present and the session touches it
4. `.claude/PLUGINS.md` — where plugins and skills are installed across environments; read when the session touches plugins or skills
5. `.claude/ACCOUNTS.md` — which Claude account and config directory the project uses; read when the session touches accounts or credentials
6. Other all-caps `.claude/*.md` files are topic-specific docs; read when the session touches that topic

## Development loop

- Feature, fix, and change work follows the `dev-loop` skill — small test-first iterations, one test at a time

## Specifications

- Allium specs live in `.claude/<topic>.allium`, read at session start when one exists
- Update the spec when starting a task and finishing a task, when behaviour changes

## Permissions

- Always start tasks using planning mode before making changes regardless of how small the change appears
- File edits are pre-approved — no confirmation needed
- Git commit and push is pre-approved when user explicitly asks for it

## Debugging

- Investigate one step at a time to check each assumption before the next
- See `.claude/DEBUGGING.md` for infrastructure debugging mode

### Workflow split

- User: spec, design, guide, review, commits, push
- Claude: produce and run code, test, tools
