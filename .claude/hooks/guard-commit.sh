#!/usr/bin/env bash
# Portable twin of the git wrapper built in claude-contained's dev-env.nix.
# In a full container both guards run; in a harness-only install this is the
# only guard. It matches the command string, so it is best-effort (bypassable);
# the container wrapper shims git itself and is the robust enforcement.
set -euo pipefail
input=$(cat)
command=$(echo "$input" | jq -r '.tool_input.command // empty')
echo "$command" | grep -qE '(^|[ ;|&])git (commit|push)' || exit 0
project_root=$(git rev-parse --show-toplevel 2>/dev/null) || exit 0
policy_file="$project_root/.claude/commit-policy"
if [ ! -f "$policy_file" ]; then
  echo "No .claude/commit-policy found. Create one to allow commits." >&2
  exit 2
fi
branch_prefix=$(grep '^branch_prefix=' "$policy_file" | cut -d= -f2 | tr -d '[:space:]' || true)
if [ -z "$branch_prefix" ]; then
  exit 0
fi
current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null) || exit 0
if [[ "$current_branch" != "$branch_prefix"* ]]; then
  echo "Branch '$current_branch' does not match required prefix '$branch_prefix'." >&2
  exit 2
fi
