#!/bin/bash
# Trigger the GitHub build workflow and block while streaming its output, so
# the GitLab job duration tracks the GitHub run and the build log shows up in
# the job trace as if it were a native runner.
set -euo pipefail

REPO="justacalico/HibiscusXR"
WORKFLOW="build.yml"

REF="${1:-main}"
PUSH_REF="${2:-}"
CHANNEL="${CHANNEL:-release}"
DEVICE="${DEVICE:-neo2}"

if [ -n "$PUSH_REF" ]; then
  echo "Pushing $PUSH_REF to GitHub branch $REF..."
  # resolve to a sha now: `git remote update` below rewrites FETCH_HEAD, so
  # pushing the symbolic ref afterwards would push the mirror's own stale
  # main back to it and silently no-op
  PUSH_SHA=$(git rev-parse "$PUSH_REF")
  git remote add github "git@github.com:$REPO.git" 2>/dev/null || true
  git remote update github
  git push -f github "$PUSH_SHA:refs/heads/$REF"
  REMOTE_SHA=$(git ls-remote github "refs/heads/$REF" | cut -f1)
  [ "$REMOTE_SHA" = "$PUSH_SHA" ] || {
    echo "ERROR: github mirror $REF is at $REMOTE_SHA, expected $PUSH_SHA"
    exit 1
  }
fi

TS=$(( $(date +%s) - 60 ))
echo "Triggering $WORKFLOW @ $REF (channel: $CHANNEL, device: $DEVICE)"
INPUTS=(-f channel="$CHANNEL" -f device="$DEVICE")
# a cog-tagged release pins the version so the GitHub release tag, the
# image's ro.hibiscus.version and the changelog section all agree
[ -n "${RELEASE_VERSION:-}" ] && INPUTS+=(-f version="$RELEASE_VERSION")
gh workflow run "$WORKFLOW" -R "$REPO" --ref "$REF" "${INPUTS[@]}"

echo "Looking for run ID..."
RUN_ID=""
for i in $(seq 30); do
  sleep 5
  RUN_ID=$(gh api "repos/$REPO/actions/runs?branch=$REF&event=workflow_dispatch&per_page=5" \
    --jq ".workflow_runs | map(select((.created_at|fromdateiso8601) > $TS)) | .[0].id" \
    2>/dev/null || true)
  [ -n "$RUN_ID" ] && [ "$RUN_ID" != "null" ] && break
done

if [ -z "$RUN_ID" ] || [ "$RUN_ID" = "null" ]; then
  echo "Could not find GitHub run for $REF" >&2
  exit 1
fi

echo "Watching GitHub run $RUN_ID..."
CONCLUSION="failure"
for attempt in 1 2 3; do
  if gh run watch "$RUN_ID" -R "$REPO" --exit-status 2>&1; then
    CONCLUSION="success"
    break
  fi

  # watch can drop on transient API errors; check the real conclusion before
  # giving up
  C=$(gh api "repos/$REPO/actions/runs/$RUN_ID" -q '.conclusion' 2>/dev/null || true)
  case "$C" in
    success)
      CONCLUSION="success"; break ;;
    failure|cancelled|timed_out|startup_failure|action_required|stale)
      CONCLUSION="$C"; break ;;
    *)
      echo "gh run watch lost connection (attempt $attempt), retrying..."
      sleep 10 ;;
  esac
done

echo "GitHub run finished: $CONCLUSION"
[ "$CONCLUSION" = "success" ]
