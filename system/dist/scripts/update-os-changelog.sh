#!/usr/bin/env bash
# Keep CHANGELOG.md's "## Unreleased" block in sync with the MRs merged
# since the last os-v* tag. Runs on every push to main (the only automatic
# cog job - version bumps stay manual, see os-bump.sh). Commits the file
# back only when it actually changed, with ci.skip so the push is quiet.
set -euo pipefail

git fetch origin main "+refs/tags/*:refs/tags/*"
git checkout -B main origin/main

before=$(git rev-parse HEAD:CHANGELOG.md 2>/dev/null || echo none)
python3 system/dist/scripts/os_changelog.py unreleased
after=$(git hash-object CHANGELOG.md)

if [ "$before" = "$after" ]; then
  echo "changelog already up to date"
  exit 0
fi

git add CHANGELOG.md
git commit -m "docs: 更新系统 changelog"

push() {
  git push -o ci.skip origin HEAD:main 2>/tmp/push.err && return 0
  grep -q "push options" /tmp/push.err && git push origin HEAD:main
}
if ! push; then
  echo "main moved, rebasing"
  git fetch origin main
  git rebase origin/main
  python3 system/dist/scripts/os_changelog.py unreleased
  git add CHANGELOG.md
  git commit --amend --no-edit
  git push origin HEAD:main
fi
echo "changelog updated"
