#!/bin/bash

# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
#
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
# Non-interactive driver for the resolve-chromium-conflicts skill.
#
# Fixes carried over from tools/eyeo/merge_conflicts_solver.sh (MR !1179):
#   - captures stderr, where gn and ninja actually write their errors
#   - one spelling of the Desktop out dir (was Desktop-release vs Desktop-Release)
#   - build attempts capped at 5, not 25 — the cap exists so a hopeless run
#     ends in hours rather than days, not to limit the agent: the rebuilds are
#     incremental, so attempts after the first cost minutes, not the full ~4h
#   - a targeted compile before the full target list
#   - no credentials in gn args
set -uo pipefail

# Run from an immutable snapshot of this script rather than from the copy in the
# work tree. The agent's fixes land in the repo, and the fix for a stale name in
# ANDROID_TARGETS below is an edit to *this file* -- which replaces the inode
# under the running shell. bash reads a script incrementally, so the run carries
# on from the now-deleted inode with the pre-fix text, rebuilds with the stale
# target list, and reports the identical error until MAX_BUILD_ATTEMPTS is spent.
# That is how one wrong target name ate three stage-4 slots: the fix was right
# and committed on the first attempt, and every later attempt still read the old
# text through /proc/<pid>/fd/255. Pinning makes the version a run uses
# deliberate instead of accidental; self_edited() then ends the run rather than
# spending the remaining attempts on variables that can no longer change.
if [ -z "${EYEO_RESOLVE_PINNED:-}" ]; then
  EYEO_RESOLVE_SELF=$(cd "$(dirname "$0")" && pwd)/$(basename "$0")
  # Outside the work tree for the same reason as BUILD_LOG below.
  EYEO_RESOLVE_PINNED=$(mktemp "${TMPDIR:-/tmp}/eyeo_resolve_conflicts.XXXXXX")
  cat "$EYEO_RESOLVE_SELF" > "$EYEO_RESOLVE_PINNED"
  export EYEO_RESOLVE_SELF EYEO_RESOLVE_PINNED
  exec bash "$EYEO_RESOLVE_PINNED" "$@"
fi
trap 'rm -f "$EYEO_RESOLVE_PINNED"' EXIT

# Outside the work tree on purpose: the final `git add -A` would otherwise commit
# a build artifact onto the release branch.
BUILD_LOG="${BUILD_LOG:-${TMPDIR:-/tmp}/eyeo_resolve_build_log.txt}"
TEST_LOG="${TEST_LOG:-${TMPDIR:-/tmp}/eyeo_resolve_test_log.txt}"
MAX_GN_ATTEMPTS=${MAX_GN_ATTEMPTS:-10}
MAX_BUILD_ATTEMPTS=${MAX_BUILD_ATTEMPTS:-5}
MAX_TEST_ATTEMPTS=${MAX_TEST_ATTEMPTS:-5}
DESKTOP_OUT="out/Desktop-Release"
ANDROID_OUT="out/Release"
# acceptEdits covers file writes and mkdir/touch/mv/cp only -- every other shell
# command still needs an --allowedTools entry, so without Bash the agent cannot
# run `git cat-file` (the skill's ownership test), `git add`, or `gn gen`, and
# resolves by reasoning instead of by checking. --permission-prompts none keeps
# an unattended run from waiting on an approval nobody can give.
# Deliberately NOT --bare: it skips skill auto-discovery, which would silently
# drop the ownership rules this whole script depends on.
CLAUDE=(claude --permission-mode acceptEdits --allowedTools "Bash,Read,Edit" --permission-prompts none)

# Where the intermediate pushes go. In CI the checkout is a detached HEAD, so
# the branch name has to come from CI_COMMIT_BRANCH. Pushing is on in CI and off
# for a local run, where pushing someone's work tree to origin unasked would be a
# surprise; set PUSH_AFTER_EACH_STEP=1 to exercise it locally.
PUSH_BRANCH=${PUSH_BRANCH:-${CI_COMMIT_BRANCH:-$(git rev-parse --abbrev-ref HEAD)}}
PUSH_AFTER_EACH_STEP=${PUSH_AFTER_EACH_STEP:-$([ -n "${CI:-}" ] && echo 1 || echo 0)}
# Name the push target outright instead of trusting `origin`. The first attempt
# at this died on "could not read Username for 'https://chromium.googlesource.com'":
# between .set_git_to_use_ssh in before_script and the push, something puts the
# vanilla upstream back on origin. The SSH key is already in the agent from
# before_script, so an explicit gitlab URL just works and cannot be redirected.
if [ -n "${CI_SERVER_HOST:-}" ] && [ -n "${CI_PROJECT_PATH:-}" ]; then
  PUSH_REMOTE=${PUSH_REMOTE:-git@${CI_SERVER_HOST}:${CI_PROJECT_PATH}.git}
else
  PUSH_REMOTE=${PUSH_REMOTE:-origin}
fi
LAST_PUSHED=""

die() { echo "STOP: $*" >&2; exit 1; }

# True once a fix has edited the script this run was started from. Such a fix
# cannot take effect here -- the shell is executing the pinned snapshot, and the
# gn args and target lists are already expanded into variables -- so retrying
# would re-run an identical build and write an identical log.
self_edited() { ! cmp -s "$EYEO_RESOLVE_PINNED" "$EYEO_RESOLVE_SELF"; }

# Commit and push after every step. Without this a late failure throws away every
# earlier stage: the commits live only on the runner's detached HEAD, and the
# hand-off push is the last line of the job, so a `die` means it never runs. That
# is how a run that had resolved seven conflicts and compiled Desktop clean lost
# both. ci.skip keeps these intermediate pushes from starting pipelines; the full
# pipeline is triggered once, at hand-off.
save() {
  local msg=$1 untracked; shift
  # `git add -u`, not `git add -A`. gclient leaves plenty in the work tree that
  # is neither ours nor ignored: DEPS dependencies checked out as nested git
  # repos (harfbuzz-ng, ink_stroke_modeler, libphonenumber, xdg-utils -- present
  # in .gitmodules but untracked here, so -A records them as bare gitlinks) and
  # the reclient .cfg files the configure_reclient hook generates. -A swept all
  # of that into a commit called "AI solved conflicts". Tracked modifications are
  # what the resolver produces; genuinely new eyeo files are the agent's to
  # `git add`, which every prompt already tells it to do.
  git add -u
  untracked=$(git ls-files --others --exclude-standard | head -10)
  [ -z "$untracked" ] || echo "note: untracked and not staged: $(echo "$untracked" | tr '\n' ' ')"
  if git diff --cached --quiet; then
    echo "-- nothing to commit: ${msg}"
  else
    git commit -n -m "$msg" "$@" || true
  fi
  push_head
}

# Separate from save() because the agent commits its own work -- it did so as
# `AI solved conflicts` before save() ever ran. A push conditioned on save()
# having made a commit would have left that on the runner and lost it.
push_head() {
  [ "$PUSH_AFTER_EACH_STEP" = "1" ] || return 0
  local head; head=$(git rev-parse HEAD)
  [ "$head" != "$LAST_PUSHED" ] || return 0
  if ! git push -o ci.skip "$PUSH_REMOTE" "HEAD:refs/heads/${PUSH_BRANCH}"; then
    echo "push to ${PUSH_REMOTE} ${PUSH_BRANCH} failed, retrying once" >&2
    sleep 15
    git push -o ci.skip "$PUSH_REMOTE" "HEAD:refs/heads/${PUSH_BRANCH}" \
      || die "cannot push to ${PUSH_BRANCH} at ${PUSH_REMOTE}. Stop here rather\
 than at hour five: with no push the work cannot survive a failure and the\
 hand-off cannot work either. Remotes are: $(git remote -v | tr '\n' ' ')"
  fi
  LAST_PUSHED=$head
}
ask() { "${CLAUDE[@]}" -p "Use the resolve-chromium-conflicts skill. $1"; }

: "${UPSTREAM_TAG:?set to the vanilla Chromium tag being moved to}"
: "${EYEO_TELEMETRY_AUTH_TOKEN:?set from a masked CI variable}"
: "${EYEO_APPLICATION_NAME:?set from CI config}"
: "${EYEO_APPLICATION_VERSION:?set from CI config}"
EYEO_TELEMETRY_SERVER_URL=${EYEO_TELEMETRY_SERVER_URL:-}

# Siso, not reclient: use_siso=false implies use_reclient=true, and rbe.gni then
# asserts on //buildtools/reclient, which upstream DEPS only downloads for
# checkout_chromeos. Same remote-exec flags as .add_gn_args in .gitlab-ci.yml.
DESKTOP_ARGS="eyeo_intercept_debug_url=true is_debug=false use_remoteexec=true \
use_siso=true use_reclient=false proprietary_codecs=true ffmpeg_branding=\"Chrome\" \
eyeo_application_name=\"${EYEO_APPLICATION_NAME}\" \
eyeo_application_version=\"${EYEO_APPLICATION_VERSION}\" \
disable_fieldtrial_testing_config=true \
eyeo_telemetry_activeping_auth_token=\"${EYEO_TELEMETRY_AUTH_TOKEN}\" \
dcheck_always_on=false eyeo_extend_chrome_devtools_protocol=true"

ANDROID_ARGS="target_os=\"android\" target_cpu=\"x64\" is_debug=false symbol_level=1 \
dcheck_always_on=false disable_fieldtrial_testing_config=true use_siso=true \
use_reclient=false use_remoteexec=true proprietary_codecs=true ffmpeg_branding=\"Chrome\" enable_nacl=false \
is_cfi=false eyeo_intercept_debug_url=true \
eyeo_telemetry_server_url=\"${EYEO_TELEMETRY_SERVER_URL}\" \
eyeo_telemetry_activeping_auth_token=\"${EYEO_TELEMETRY_AUTH_TOKEN}\""

# The stage-4 target lists are data, re-read before every attempt, so a name
# upstream has deleted can be corrected mid-run -- see build_targets.sh. That is
# the one edit self_edited() must NOT end the run for, which is why it lives
# outside this file: sourcing re-reads the whole thing, and the pinned snapshot
# stays untouched.
TARGETS_FILE=tools/eyeo/skills/resolve-chromium-conflicts/scripts/build_targets.sh
[ -f "$TARGETS_FILE" ] || TARGETS_FILE=$(dirname "$EYEO_RESOLVE_SELF")/build_targets.sh
load_targets() { . "$TARGETS_FILE" || die "cannot read target lists from $TARGETS_FILE"; }
load_targets

echo "push target: ${PUSH_REMOTE} -> ${PUSH_BRANCH} (enabled=${PUSH_AFTER_EACH_STEP})"
git remote -v | sed 's/^/remote: /'

START_REF=$(git rev-parse HEAD)

# --- Stage 1: conflicts -----------------------------------------------------
# Locate the core-update commit before reading it, and treat "not found" as an
# error rather than as "nothing to do". With a shallow clone `git log --grep`
# matches nothing, and reading that as "no conflicts" sends the job on to build a
# tree that still has conflict markers in it -- a green-looking run over an
# unresolved update, which is the worst outcome this script can produce.
core_update=$(git log --grep="^Update to Chromium " --format=%H -1)
[ -n "$core_update" ] || die "no 'Update to Chromium' commit reachable from HEAD.\
 A shallow clone (set GIT_DEPTH: 0) and a branch that never came from\
 vanilla-chromium-builder both look like this."

conflicts=$(git log -1 --pretty=format:%B "$core_update" \
  | sed -n '/Pending conflicts:/,/eyeo-parent-commit/p' \
  | grep -vE 'Pending conflicts:|^$|eyeo-parent-commit' | tr -d '\r')

gclient sync -D --force --reset 2>&1 || die "gclient sync failed"

if [ -n "$conflicts" ]; then
  echo "== Stage 1: conflicts =="; echo "$conflicts"
  ask "UPSTREAM_TAG is ${UPSTREAM_TAG}. Resolve every conflict in these files and no others:
${conflicts}
Follow the ownership rules exactly. Never remove eyeo code. Stop and report instead of
guessing if a conflict has no attachment point in the new upstream design.
If you commit, the subject must be exactly 'AI solved conflicts'. Explain your reasoning
in the body." \
    || die "conflict resolution reported a stop condition"
  still=$(git diff --name-only --diff-filter=U)
  [ -z "$still" ] || die "still conflicted: $(echo "$still" | tr '\n' ' ')"
  save "AI solved conflicts" -m "Resolved:" -m "${conflicts}"
else
  # An empty list is legitimate -- a core update can apply cleanly. Check the
  # tree agrees before trusting it: a stray marker here means the list was lost,
  # not that the update was clean. Only runs on this path, so the full-tree grep
  # costs nothing in the common case.
  stray=$(git grep -l -E '^<<<<<<< ' -- . 2>/dev/null || true)
  [ -z "$stray" ] || die "no conflict list on ${core_update} but the tree still \
has conflict markers: $(echo "$stray" | tr '\n' ' ')"
  echo "== Stage 1 skipped: ${core_update} records no pending conflicts =="
fi

# --- Stage 2: gn gen --------------------------------------------------------
run_and_fix_gn() {
  local platform=$1 out_dir=$2 args=$3 attempt=1 output
  while [ "$attempt" -le "$MAX_GN_ATTEMPTS" ]; do
    echo "== Stage 2: $platform gn gen, attempt $attempt/$MAX_GN_ATTEMPTS =="
    if output=$(gn gen --check --args="$args" "$out_dir" 2>&1); then
      echo "$platform gn gen ok"; return 0
    fi
    [ -n "$output" ] || die "$platform gn gen failed with no output — investigate manually"
    ask "gn gen for $platform failed. Fix the BUILD.gn/.gni cause. New files follow eyeo
naming and must be git added.
If you commit, the subject must be exactly 'AI solved gn issues ${platform}'. Explain your
reasoning in the body. Errors:
$output"
    save "AI solved gn issues ${platform}" -m "gn gen, attempt ${attempt}"
    ! self_edited || die "the fix edited ${EYEO_RESOLVE_SELF}, most likely the gn\
 args, which this run cannot pick up -- it is executing a pinned snapshot and\
 \$${platform^^}_ARGS is already expanded. The fix is committed and pushed;\
 restart the job to run gn gen with it."
    attempt=$((attempt + 1))
  done
  die "$platform gn gen still failing after $MAX_GN_ATTEMPTS attempts"
}
run_and_fix_gn "Desktop" "$DESKTOP_OUT" "$DESKTOP_ARGS"
run_and_fix_gn "Android" "$ANDROID_OUT" "$ANDROID_ARGS"
# Deliberately unqualified: the per-attempt saves above name their platform, so
# anything still uncommitted here belongs to neither one in particular.
save "AI solved gn issues"

# --- Stage 3: targeted compile, then Stage 4: full --------------------------
touched=$(git diff --name-only "$START_REF"..HEAD -- '*.cc' '*.h' '*.gn' '*.gni' '*.java' || true)
targeted_for() {
  local out_dir=$1 f t=""
  for f in $touched; do
    [ -e "$f" ] || continue
    # Plain `gn refs` gives the smallest targets that own the file, which is what
    # reference/build-config.md prescribes. `--all` walks the whole reverse
    # dependency closure instead, which on a core update pulls in umbrella
    # targets and blows the output directory past the runner's disk.
    t="$t $(gn refs "$out_dir" "$f" 2>/dev/null | head -20 || true)"
  done
  # gn prints GN labels (//chrome/test:foo); ninja wants them without the leading
  # slashes (chrome/test:foo). Passing the label through verbatim is what made
  # stage 3 die with "ninja: error: unknown target '/:blackbox_fuzzing_targets'".
  # The examples/ demo apps are excluded: they are not shipped and compiling them
  # only costs build time.
  echo "$t" | tr ' ' '\n' | grep -E '^//' | grep -v '^//examples' | sed 's|^//||' \
    | sort -u | head -40 | tr '\n' ' '
}

# Takes the NAME of the variable holding the target list, not its value, and
# re-reads it before every attempt. Passing the value froze the list at call time,
# which is how job 16501332460 spent three stage-4 attempts rebuilding a target
# upstream had deleted while the correction sat committed on disk. A build target
# that has been renamed or removed is a normal finding of a core update -- it is
# most of what stage 4 catches -- so correcting one has to be able to land in the
# run that found it.
run_and_fix_build() {
  local platform=$1 out_dir=$2 targets_var=$3 label=$4 attempt=1 targets
  while [ "$attempt" -le "$MAX_BUILD_ATTEMPTS" ]; do
    # load_targets refreshes the stage-4 lists from build_targets.sh; the stage-3
    # variables are derived from `gn refs` and it does not touch them.
    load_targets
    targets=${!targets_var}
    [ -n "${targets// /}" ] || { echo "$label: nothing to build, skipping"; return 0; }
    echo "== $label: $platform, attempt $attempt/$MAX_BUILD_ATTEMPTS =="
    # -k 0 is ninja's ignore-compile-failures switch: keep going past a failing
    # edge and only stop when nothing else can be built, so one run yields the
    # whole error set instead of just the first. Everything independent of the
    # failure still gets built, which is what makes the next attempt cheap.
    echo "+ autoninja -C $out_dir -k 0 $targets"
    if autoninja -C "$out_dir" -k 0 $targets > "$BUILD_LOG" 2>&1; then
      echo "$platform $label ok"; return 0
    fi
    ask "The $platform build failed ($label). Read ${BUILD_LOG} and fix every error.
Never remove eyeo code. New files follow eyeo naming and must be git added.
If you commit, the subject must be exactly 'AI solved build issues ${platform}'. Explain
your reasoning in the body.
If ninja reports an unknown target rather than a compile error, the build list is
stale: upstream renamed or deleted that target. Correct it in
${TARGETS_FILE} -- that file is re-read before every
attempt, so the fix takes effect here. Editing any other part of the driver
cannot take effect in this run and will end it."
    save "AI solved build issues ${platform}" -m "${label}, attempt ${attempt}"
    ! self_edited || die "the fix edited ${EYEO_RESOLVE_SELF}, most likely the\
 target list, which this run cannot pick up -- it is executing a pinned snapshot\
 and ${label} is building: ${targets}. A stale target name is the exception --\
 correct it in ${TARGETS_FILE} and the next attempt re-reads it. Anything else\
 in the driver cannot take effect, so retrying would\
 rebuild exactly this and write the same log, and the remaining\
 $((MAX_BUILD_ATTEMPTS - attempt)) attempt(s) are spent here rather than on the\
 fix. It is committed and pushed -- restart the job. out/ is preserved, so the\
 restart is incremental."
    attempt=$((attempt + 1))
  done
  die "$platform $label still failing after $MAX_BUILD_ATTEMPTS attempts — see $BUILD_LOG"
}

DESKTOP_STAGE3_TARGETS=$(targeted_for "$DESKTOP_OUT")
ANDROID_STAGE3_TARGETS=$(targeted_for "$ANDROID_OUT")
run_and_fix_build "Desktop" "$DESKTOP_OUT" DESKTOP_STAGE3_TARGETS "Stage 3 targeted"
run_and_fix_build "Android" "$ANDROID_OUT" ANDROID_STAGE3_TARGETS "Stage 3 targeted"
run_and_fix_build "Desktop" "$DESKTOP_OUT" DESKTOP_TARGETS "Stage 4 full"
run_and_fix_build "Android" "$ANDROID_OUT" ANDROID_TARGETS "Stage 4 full"

# --- Stage 5: the eyeo tests ------------------------------------------------
# A branch that compiles is not a branch that works. Chromium 155 broke
# AdblockLocaleFilterListDownloadTest (components/adblock/content/browser/test/
# adblock_filter_list_browsertest.cc) in a way stage 4 could not see, and the
# follow-up pipeline spent 223 minutes finding it. Running the same tests here
# means the run that made the breakage is the run that fixes it.
#
# Same suites, same filter, same runner as build_and_test_desktop_release takes
# on a `default` pipeline -- deliberately, because stage 5 exists to predict that
# job. Only the eyeo tests: the upstream suite is hours long, and a core update
# does not break Chromium's own tests. Desktop only, because the Android tests
# need an emulator the pipeline provides in its own jobs.
EYEO_GTEST_FILTER=${EYEO_GTEST_FILTER:-'*Adblock*:*Eyeo*'}
TEST_SUITES=${TEST_SUITES:-"unit_tests components_unittests browser_tests \
components_browsertests content_browsertests"}

run_and_fix_tests() {
  local out_dir=$1 attempt=1 failed suite
  # browser_tests need the SUID sandbox, and BUILDTYPE is how
  # update-linux-sandbox.sh finds the binary -- it is the out dir's basename, not
  # a build configuration.
  export BUILDTYPE CHROME_DEVEL_SANDBOX
  BUILDTYPE=$(basename "$out_dir")
  CHROME_DEVEL_SANDBOX=/usr/local/sbin/chrome-devel-sandbox
  build/update-linux-sandbox.sh || die "could not install the SUID sandbox; browser_tests cannot run"
  # EyeoDevToolsPrivateApiTest mocks an adb server on 5037 and fails if the port
  # is taken. The pipeline job does the same before its test run.
  fuser -k 5037/tcp || true

  while [ "$attempt" -le "$MAX_TEST_ATTEMPTS" ]; do
    echo "== Stage 5 eyeo tests: attempt $attempt/$MAX_TEST_ATTEMPTS =="
    # A fix from the previous attempt is source, so rebuild before re-running.
    # No-op on the first pass, since stage 4 just built all of these.
    autoninja -C "$out_dir" -k 0 $TEST_SUITES > "$BUILD_LOG" 2>&1 \
      || die "the test binaries stopped building -- see $BUILD_LOG"
    : > "$TEST_LOG"
    failed=""
    for suite in $TEST_SUITES; do
      echo "-- $suite"
      # Not piped through the `grep -v` the pipeline uses on browser_tests: this
      # script sets pipefail, so a pipe would report grep's status, and grep
      # returning 1 on a fully filtered run would invent a failure. The log is
      # read by the fixer, not by a human watching the console.
      python3 .ci-scripts/run_chromium_tests.py "$out_dir" "$suite" -- \
        --gtest_filter="$EYEO_GTEST_FILTER" --test-launcher-jobs=12 >> "$TEST_LOG" 2>&1 \
        || failed="$failed $suite"
    done
    [ -n "$failed" ] || { echo "Stage 5 eyeo tests ok"; return 0; }
    echo "failing suites:$failed"
    grep -E "^\[  FAILED  \]" "$TEST_LOG" | sort -u | head -40 || true
    ask "The eyeo tests failed in:$failed. Read ${TEST_LOG}, find every failing test and
fix the cause. The failures are from this core update, so the fix is normally in the eyeo
code or the test's expectations about upstream behaviour that changed -- not in the test
being deleted or disabled. Never remove eyeo code and never skip a test to make it pass;
if a test is genuinely obsolete because upstream removed what it covered, say so and stop
instead of deleting it. New files follow eyeo naming and must be git added.
If you commit, the subject must be exactly 'AI solved test failures Desktop'. Explain your
reasoning in the body."
    save "AI solved test failures Desktop" -m "attempt ${attempt}, suites:${failed}"
    ! self_edited || die "the fix edited ${EYEO_RESOLVE_SELF}, which this run cannot pick\
 up -- it is executing a pinned snapshot. The fix is committed and pushed; restart the job."
    attempt=$((attempt + 1))
  done
  die "eyeo tests still failing after $MAX_TEST_ATTEMPTS attempts --$failed -- see $TEST_LOG"
}

run_and_fix_tests "$DESKTOP_OUT"

# Unqualified for the same reason as the gn catch-all above.
save "AI solved build issues"
echo "Done. Commits since ${START_REF:0:8}:"
git log --oneline "$START_REF"..HEAD
