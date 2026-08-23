# CLAUDE.md — Project Instructions

Generic instructions for Claude Code when working on projects.
See `.claude/PROJECT.md` for project-specific requirements.

## File conventions

- Suffix-free `.claude/*.md` files (`PROJECT.md`, `TASKS.md`, `TOOLS.md`) are project-owned: seeded once by the harness, then never touched again — edit them freely
- `.claude/*.managed.md` files (`ACCOUNTS.managed.md`, `CLAUDE.managed.md`, `DEBUGGING.managed.md`, `DEV-ENV.managed.md`, `PLUGINS.managed.md`) are harness-owned: a harness upgrade overwrites them, so hand edits are lost — propose changes upstream instead
- `.claude/*.local.md` files (`TASKS.local.md`, `TESTS.local.md`) are personal: git-ignored, never committed, free of the ordering and pruning rules that apply to their shared counterpart
- The root-level `CLAUDE.md` (outside `.claude/`) is the one file Claude Code requires by exact name.  It only ever holds the one-line `@.claude/CLAUDE.managed.md` import — the real guidance lives in the suffixed file you are reading now

## Making changes

- Keep changes small and focused — one concern at a time
- Do not add features beyond what was asked
- Do not add comments unless the logic is genuinely non-obvious
- Avoid empty lines in the middle of sections
- Prefer SOLID, YAGNI & KISS
- Apply DRY only after the rule of three, same intent, not same shape
- Never break a CLI command across lines as it prevents copy-paste
- Write plain, unambiguous English that is easy for a non-native speaker to understand — avoid idioms and figurative phrases, and state things directly rather than creatively
- Documentation describes the system as it is now, not how it got there — what changed and when belongs in git history and `.claude/TASKS.md`.  A part that is planned but not yet built gets a short separate note, not a blend into the current-state description
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

## Tools

- `.claude/TOOLS.md` lists tools as `name: what it's for`, one per line — for example `imagemagick: image manipulation`, `jq: JSON parsing`
- Check `.claude/TOOLS.md` before picking a CLI tool for a task the project has not needed before
- Suggest to the user if some new tool should be added

## Session start

At the beginning of every session, read these files to get full context:
1. `.claude/PROJECT.md` — authoritative description of the current end state
2. `.claude/TASKS.md` — history of completed tasks and pending work, plus `.claude/TASKS.local.md` when present
3. `.claude/DEV-ENV.managed.md` — Nix/Docker dev container workflow; read when present and the session touches it
4. `.claude/PLUGINS.managed.md` — where plugins and skills are installed across environments; read when the session touches plugins or skills
5. `.claude/ACCOUNTS.managed.md` — which Claude account and config directory the project uses; read when the session touches accounts or credentials
6. `.claude/TOOLS.md` — which tool to use for a kind of task; read before picking a CLI tool for a task the project has not needed before
7. Other all-caps `.claude/*.md` files are topic-specific docs; read when the session touches that topic

## Development loop

- Feature, fix, and change work follows the `dev-loop` skill — small test-first iterations, one test at a time

## Specifications

- Allium specs live in `.claude/<topic>.allium`, read at session start when one exists
- Update the spec when starting a task and finishing a task, when behaviour changes

## Permissions

- Always start tasks using planning mode before making changes, or before running external infrastructure commands, regardless of how small the change or the query appears
- File edits are pre-approved — no confirmation needed
- Git commit and push is pre-approved when user explicitly asks for it

## Live changes

- A command that changes state outside the repository is different in kind from a file edit: git holds no record of it and there is no undo.  Name the resource and the value you intend to change, and why, before running the command
- The same pause applies before a command that only reads external state, such as a discovery or listing call through a cloud CLI.  State what you are trying to find and what you assume is already available and accessible — access, tools, resource names — then wait for the user to confirm before making the call, rather than finding out through repeated API calls
- This covers cloud and hosting CLIs, deployments, databases, DNS and package registries, for both changes and read-only queries.  It does not cover the local dev container, which the project rebuilds from its own files
- When such a command fails, returns something unexpected, or needs a value you inferred rather than read from an authoritative source, stop and state the assumption plainly, then ask the user to confirm before continuing — do not retry with a guess
- Never restart, stop, deallocate, or scale a production service.  Ask first, every time, immediately before that exact command, and wait for an explicit yes.  This holds even when the step was already written down as approved in a plan file, and even when a different command earlier in the same sequence was already confirmed.  Stating the intent in a text update is not the same as asking, and does not count as approval

## Debugging

- Investigate one step at a time to check each assumption before the next
- See `.claude/DEBUGGING.managed.md` for infrastructure debugging mode

### Workflow split

- User: spec, design, guide, review, commits, push
- Claude: produce and run code, test, tools
