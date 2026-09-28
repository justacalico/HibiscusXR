#!/bin/bash
# Push the cte tag to the GitHub mirror and watch the cte.yml release run,
# so a failed GitHub build fails this pipeline - same contract as
# system/dist's trigger-github-build.sh.
set -euo pipefail

REPO="justacalico/HibiscusXR"
WORKFLOW="cte.yml"
TAG="${1:?usage: trigger-cte-release.sh <tag>}"

git remote add github "git@github.com:$REPO.git" 2>/dev/null || true
git remote update github
git push -f github "$TAG"

echo "Watching for cte.yml run on $TAG..."
TS=$(( $(date +%s) - 60 ))
RUN_ID=""
for i in $(seq 60); do
  sleep 5
  RUN_ID=$(gh api "repos/$REPO/actions/workflows/$WORKFLOW/runs?per_page=5" \
    --jq ".workflow_runs | map(select(.head_branch == \"$TAG\" and (.created_at|fromdateiso8601) > $TS)) | .[0].id" \
    2>/dev/null || true)
  [ -n "$RUN_ID" ] && [ "$RUN_ID" != "null" ] && break
done

if [ -z "$RUN_ID" ] || [ "$RUN_ID" = "null" ]; then
  echo "Could not find GitHub cte run for $TAG" >&2
  exit 1
fi

echo "Watching GitHub run $RUN_ID..."
CONCLUSION="failure"
for attempt in 1 2 3; do
  if gh run watch "$RUN_ID" -R "$REPO" --exit-status 2>&1; then
    CONCLUSION="success"
    break
  fi
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
