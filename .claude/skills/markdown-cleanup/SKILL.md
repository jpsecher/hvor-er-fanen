---
name: markdown-cleanup
description: Check markdown written during a task against the project's markdown rules. Use as after-task bookkeeping, once the work is done but before updating `.claude/TASKS.md` — scoped to the files the task actually touched.
---

# Markdown cleanup

Run the markdown changed by the current task through the rules in `.claude/CLAUDE.md`.  Prose moves away from those rules while attention is on the work itself, so this is a deliberate pass at the end rather than something to trust to memory.

## Scope

Only files the task touched.  Never a repo-wide rewrite — untouched files stay as they are, even when they violate a rule.

Collect them with `git diff --name-only`, `git diff --cached --name-only` and `git ls-files --others --exclude-standard`, keeping the `*.md` entries.

## Mechanical checks

Run each against the collected files.  Every hit is a candidate, not a confirmed problem — read the line before changing it.

Two blanks between sentences:

    sed -E 's/^[[:space:]]*[0-9]+\.[[:space:]]//' FILE | grep -nE '[a-z0-9)`"]\. [A-Z]' | grep -vE '\b(e\.g|i\.e|etc|vs|cf)\. ' | grep -vE '^[0-9]+:(name|description):'

Each stage suppresses a false positive the bare pattern reports: the `sed` strips numbered list markers, which otherwise match as "1. Starts"; the first `grep -v` clears abbreviations like "e.g. Github"; the second clears skill frontmatter, where a single-line `description` is not prose.  The `sed` does not remove lines, so the reported line numbers stay correct.

Identifiers by name only, no trailing punctuation:

    grep -nE '`[A-Za-z_][A-Za-z0-9_.]*\(\)`' FILE

Fenced code blocks, kept only where syntax highlighting genuinely helps:

    grep -n '^```' FILE

CLI commands split across lines, which breaks copy-paste:

    grep -nE '\\$' FILE

Idioms and figurative phrases, which the plain-English rule excludes:

    grep -niE '\b(under the hood|out of the box|behind the scenes|rabbit hole|silver bullet|baked in|first-class|drop-in|gotcha|sanity check|on the fly|in the wild|heavy lifting|boils down to|moving parts|kick off|the trap|the catch|hand-rolled|glue|magic)\b' FILE

The wordlist holds the phrases that recur in this repo's prose.  It is a starting list, not a definition of the rule, so a clean result does not mean the file reads plainly.

## Judgment checks

No reliable pattern exists for these, so read the diff for them:

- Prose is plain and direct: no idiom or figure of speech beyond the wordlist, no creative phrasing where a direct statement would do, and no wording a non-native speaker would have to decode
- Lines are unbroken — a paragraph is one line, however long
- No empty lines in the middle of a section
- Detail sits at the right altitude: a one-line summary in `.claude/TASKS.md`, detail in `.claude/PROJECT.md` or its own `.claude/*.md`

## Finish

Fix what the pass found, then re-run the greps to confirm they come back clean.
