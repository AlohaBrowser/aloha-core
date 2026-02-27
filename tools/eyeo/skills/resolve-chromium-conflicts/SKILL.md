---
name: resolve-chromium-conflicts
description: Resolve the merge conflicts and follow-on build breakage produced when eyeo's customisations are replayed onto a new Chromium version. Use after `vanilla-chromium-builder` pushes a core-update branch carrying a "Pending conflicts:" commit, or whenever a cherry-pick of the squashed eyeo commit onto a new vanilla tag leaves conflict markers. Reads the conflict list from the commit message, resolves each file under the eyeo-vs-Chromium ownership rules, then escalates through `gn gen` and a targeted compile before the full build. Non-interactive and CI-safe; it commits its own work in labelled stages and stops with a report rather than guessing.
---

# Resolving a Chromium core update

When eyeo moves to a new Chromium version, `vanilla-chromium-builder` squashes every eyeo
change into one commit and cherry-picks it onto the new vanilla tag. Whatever Chromium
restructured in the meantime comes out as a conflict.

This skill resolves those conflicts and the build breakage that follows, in five stages,
committing each stage separately so a reviewer can see what was decided where.

Derived from `tools/eyeo/merge_conflicts_solver.sh` (MR !1179, DPD-3454), with its
resolution heuristics preserved and the mechanics around them rewritten.

## Where each stage runs — read this before wiring anything

All five stages run in **one `chromium-sdk` CI job**, on the runners the project already
uses (OM-1472 phase 1). The branch arrives from `vanilla-chromium-builder` carrying
`PIPELINE_TYPE=core-update`, which selects a pipeline containing only this job.

The five stages still split on one question — **does this stage need a Chromium build?** —
because that decides what one failed attempt costs, and where a stage can be reproduced:

| Stage | Needs a build environment | Cost of one attempt |
|---|---|---|
| 1 — conflicts | **No.** File reads and edits only | seconds |
| 2 — `gn gen` | Yes: checkout + `depot_tools` | minutes |
| 3 — targeted compile | Yes: + remote-exec | minutes to an hour |
| 4 — full build | Yes: + hours | 3.5–5.5 hours |
| 5 — eyeo tests | Yes: + the built binaries and the SUID sandbox | minutes to an hour |

**Stage 1 needs no build environment, and it is worth keeping it that way.** It needs the
conflict list, the conflicted file bodies, and somewhere to write a commit — not a worktree,
a `gclient` sync, or a runner. That is what lets someone reproduce or redo a single
resolution decision without a 150GB checkout, and it keeps the option open to run stage 1
elsewhere later. Do not add a git or build dependency to it.

**Stages 2-5 must stay in one long-lived job that keeps its `out/` directory.** ninja is
incremental: after a fix, only the touched translation unit, its dependents and the link
rebuild — minutes. A fresh workspace re-does `gclient sync`, `gn gen` and the full link, so
handing each attempt to a fresh job costs a full build every time. Three attempts that way
exceed a 12-hour pipeline timeout; the same three cost about 4.3 hours in one job. This is
the single most important constraint on how this is wired.

## The one rule that matters

**During the cherry-pick, `HEAD` is the new Chromium and the incoming side is eyeo.**

```
<<<<<<< HEAD
    the new vanilla Chromium code          <- structurally authoritative
=======
    the eyeo change being replayed         <- must survive, may need adapting
>>>>>>> Update to Chromium <tag>
```

Get this backwards and you will delete either the Chromium upgrade or the entire product.

## Deciding who owns a file

Resolution depends on whether the file is Chromium's with eyeo patches applied, or eyeo's
own. Don't guess from the path — ask the vanilla tag:

```bash
# Does this file exist in upstream Chromium at all?
git cat-file -e "${UPSTREAM_TAG}:${file}" 2>/dev/null && echo "chromium-owned" || echo "eyeo-owned"
```

| Ownership | Resolution |
|---|---|
| **Chromium-owned** (exists upstream) | Keep the HEAD side as the structure. Re-apply the eyeo modifications on top of it, adapting them to the new upstream shape. Never resolve by taking the eyeo side wholesale — that reverts the Chromium upgrade for that file. |
| **eyeo-owned** (absent upstream) | Take the incoming eyeo side. Chromium has no opinion on these files, so a conflict here is almost always a rename or move upstream that dragged the file into the diff. |

Paths that are eyeo-owned in practice: `components/adblock/**`, `components/adblock_comparison/**`,
`components/eyeo_testing/**`, `tools/eyeo/**`, `chrome/browser/adblock/**`. Treat the list
as a hint and the `git cat-file` test as the answer.

## When Chromium removed what eyeo hooked into

The hard case, and the one worth slowing down for. If the upstream code the eyeo change
attached to is gone or reshaped, do not force the old eyeo diff back in and do not drop it.

1. Read how the new upstream design does the same job — find the replacement API, observer,
   factory or registration point.
2. Re-attach the eyeo behaviour to the new design.
3. If you cannot find a plausible attachment point, **stop and report that file**. A wrong
   answer here compiles and then misbehaves at runtime, which is worse than a red pipeline.

**Never delete eyeo code to make a conflict go away.** If eyeo functionality genuinely cannot
survive the upgrade, that is a product decision and belongs in the report, not in a commit.

## Stage 1 — Conflicts

The conflict list is recorded by the builder in the core-update commit message, between
`Pending conflicts:` and `eyeo-parent-commit:`. Read it from there rather than from
`git diff`, so it survives the commit the builder already made:

```bash
git log -1 --pretty=format:%B --grep="Update to Chromium" \
  | sed -n '/Pending conflicts:/,/eyeo-parent-commit/p' \
  | grep -vE 'Pending conflicts:|^$|eyeo-parent-commit' | tr -d '\r'
```

Resolve every file before compiling anything — one build attempt per resolution pass, never
one per file. Work through related files together: a renamed upstream symbol usually
conflicts in several places at once, and resolving them in isolation produces inconsistent
answers.

Any new file you create must follow the eyeo naming convention and be `git add`ed — an
untracked file breaks the build for everyone but you.

Commit as `AI solved conflicts`, listing the files resolved and, for each, one line on what
was decided. That message is the reviewer's only window into the reasoning.

### Commit subjects

Every stage commit uses one of exactly three subjects, and the build and `gn` ones name the
platform they belong to:

| Stage | Subject |
|---|---|
| 1 | `AI solved conflicts` |
| 2 | `AI solved gn issues Desktop` / `AI solved gn issues Android` |
| 3, 4 | `AI solved build issues Desktop` / `AI solved build issues Android` |
| 5 | `AI solved test failures Desktop` |

The platform matters because a core update produces several of each, and a reviewer reading
`git log --oneline` on the release branch otherwise sees four identical `AI solved build
issues` lines with no way to tell which platform broke without opening each one. Keep the
reasoning in the body — subject for identification, body for the argument.

## Stage 2 — `gn gen`

Cheap (minutes) and catches most `BUILD.gn` and `.gni` breakage before any compilation.
Run it for both platforms; args and targets are in `reference/build-config.md`.

Capture **stderr** — gn writes its errors there, and the original script's failure to do so
is why it used to exit claiming "no ERROR lines detected".

Cap at 10 attempts. `gn gen` failures that survive 10 rounds are structural.

## Stage 3 — Targeted compile

**Do not go straight to the full target list.** A full Chromium build is 3.5–5.5 hours, so a
try-fail-fix loop over it gets two to four attempts per day.

Compile only the targets owning the files you touched, and with `-k 0` so one run surfaces
every error rather than the first. Get that green first. This is the difference between a job
that can iterate and one that cannot.

## Stage 4 — Full build

Only after stage 3 is green. The cap is **5 attempts**, not 25: it exists to bound a hopeless
run, not to limit you. Rebuilds are incremental, so attempts after the first cost minutes.

### A target that no longer exists is a normal finding

`ninja: error: unknown target 'X', did you mean 'Y'?` is not a compile error. ninja validates
the whole list before running a single edge, so the build aborts having compiled **nothing** —
and `-k 0` cannot widen a failure that happens before the first edge. The log will be four
lines. Upstream renames and deletes build targets between milestones, and catching that is
most of what stage 4 is for.

Fix it in `scripts/build_targets.sh`, and nowhere else. That file holds the two full target
lists and is re-read before every build attempt, so the correction takes effect in the run
that found it. Editing `scripts/resolve_conflicts.sh` instead cannot work: the run executes a
pinned snapshot of it, `self_edited()` will detect the edit and end the run, and its variables
were expanded hours earlier.

Pick the replacement from `.gitlab-ci.yml` rather than guessing at the nearest upstream
equivalent — stage 4 exists to prove the branch builds what the follow-up `default` pipeline
builds, so the pipeline's own list is the authority. Check every name at once, in seconds,
before spending a build:

```bash
for t in $ANDROID_TARGETS; do
  third_party/ninja/ninja -C out/Release -t query "$t" >/dev/null 2>&1 || echo "MISSING $t"
done
```

**Always build with `-k 0`.** ninja's default is `-k 1` — stop after the first failing job.
That returns one compile error per four-hour build, which is the worst possible ratio. With
`-k 0` ninja keeps going and reports **every** independent failure in one run, so a single
build yields the whole error set and one fix pass can address all of it:

```bash
autoninja -C "$OUT_DIR" -k 0 $TARGETS > "$BUILD_LOG" 2>&1
```

The trade is that a doomed build now runs to completion instead of aborting early, so a
failing build costs the full duration. That is the right way round: the expensive thing is
the number of build *attempts*, not the length of one.

Redirect both streams to the log — `> "$BUILD_LOG" 2>&1`. Keep that log **outside**
the work tree: the final `git add -A` would otherwise commit a build artifact onto
the release branch. Compiler errors go to stderr, and
without the redirect the log the fixer reads is empty of the very errors it needs.

Fix every error the log reports before rebuilding. Errors that share a cause (a renamed
upstream symbol used in ten places) must be fixed together, or the next build returns the
same nine.

**Before fixing anything, check the error is still real.** A log that reports an error the
tree no longer has is the one case where fixing it makes things worse, and it happens for a
mundane reason: if the previous attempt's fix was an edit to `resolve_conflicts.sh` itself,
the running shell is still reading the pre-fix text from a now-deleted inode, so the same
build runs again and writes the same log. Two checks settle it in seconds:

```bash
git log -1 --stat                              # did a previous attempt already fix this?
ls -l /proc/$(pgrep -f resolve_conflicts.sh)/fd/255   # "(deleted)" => the run is stale
```

If the fix is already committed, do not fix it again — a second, different guess at an error
that was right the first time is how a healthy tree acquires a speculative change. Verify the
committed fix instead (for a target name, `ninja -t query`), and report that the run needs a
restart to pick it up.

Commit as `AI solved build issues <Platform>` — see **Commit subjects** under Stage 1.

## Stage 5 — The eyeo tests

A branch that compiles is not a branch that works. Chromium 155 left
`AdblockLocaleFilterListDownloadTest` timing out at 30s on every parameter, which no amount
of stage 4 would have caught, and the follow-up pipeline spent 223 minutes finding it.
Running the tests here means the run that caused the breakage is the run that fixes it.

Run exactly what `build_and_test_desktop_release` runs on a `default` pipeline — same
suites, same filter, same runner script — because predicting that job is the entire point:

```bash
python3 .ci-scripts/run_chromium_tests.py "$OUT_DIR" "$suite" -- \
  --gtest_filter='*Adblock*:*Eyeo*' --test-launcher-jobs=12
```

over `unit_tests`, `components_unittests`, `browser_tests`, `components_browsertests` and
`content_browsertests`.

**Only the eyeo tests.** The full upstream suite runs for hours, and a core update does not
break Chromium's own tests — if it did, the conflict resolution is wrong and that is a stage
1 problem. **Desktop only**: the Android tests need an emulator, which the pipeline provides
in its own jobs.

Two prerequisites, both of which the pipeline job also does and neither of which is
optional. `browser_tests` need the SUID sandbox, so export `BUILDTYPE` — the out dir's
*basename*, not a build configuration — and run `build/update-linux-sandbox.sh`. And
`EyeoDevToolsPrivateApiTest` mocks an adb server on port 5037, so free it first with
`fuser -k 5037/tcp`.

Run every suite before fixing anything, the same reasoning as `-k 0`: one pass then yields
the whole failure set instead of the first one. Rebuild between attempts — a fix is source,
and the binary on disk is the one that failed.

**Never make a test pass by deleting or disabling it.** A failing eyeo test after a core
update means upstream changed something the eyeo code depended on; the fix belongs in that
code or in what the test expects of upstream. If a test is genuinely obsolete because
upstream removed the thing it covered, that is a product decision — say so and stop, the
way you would for a conflict with no attachment point.

Commit as `AI solved test failures Desktop`.

## Builds belong in CI

Stages 2-5 run on a `chromium-sdk` runner, in one job, with `gclient` state already present.
Do not build on a laptop for a release: it needs remote-exec credentials, takes hours, and a
local result proves nothing about the runner the release is actually built on.

If a change needs a build to verify, the path is: commit it, push the branch, let the
pipeline run, read the result. Never split the build stages across jobs - see the incremental
argument above.

## Stopping and reporting

Stop — don't keep trying — when any of these is true:

- a conflict has no plausible attachment point in the new upstream design
- resolving it would require deleting eyeo functionality
- `gn gen` still fails after 10 attempts
- the full build still fails after 2 attempts
- a fix would touch a file that was not in the conflict list and is not eyeo-owned

The report names the file, the upstream change that caused it, what was tried, and what
decision a human needs to make. A clear stop is a good outcome; a silent bad resolution is
the failure mode this skill exists to avoid, because everything downstream reads a green
pipeline as a shippable release.

## Entry points

`scripts/resolve_conflicts.sh` is the non-interactive entry point, and the one the CI job
runs. It drives all five stages, commits each separately, and exits non-zero with the report
on any stop condition. It takes gn args from the environment — **never hardcode a telemetry
token or any other credential into gn args**, in this repo above all: it is public.

When the job finishes cleanly it pushes its commits back to the branch with
`-o ci.variable="PIPELINE_TYPE=default"`, which starts the normal build-and-test pipeline.
**A plain push starts nothing:** `workflow.rules` in `.gitlab-ci.yml` is an allowlist and
matches no push that carries no `PIPELINE_TYPE`. That push option is also the loop guard —
the job pushes `default`, never `core-update`, so it cannot re-trigger itself.

**Stage 1 by hand** (reproducing a decision, or redoing one file) needs neither that script
nor a built tree. It needs only:

- the two commit messages, to derive the work set (see Stage 1)
- the conflicted file bodies at that ref
- a way to write a commit

The resolution rules above are the contract; the transport is the caller's business. Keep
this section true if the wiring changes — a reader deciding where to run a stage is the
person most likely to be misled by a stale answer here.

## Using this skill in a Claude Code session

This skill is versioned at `tools/eyeo/skills/resolve-chromium-conflicts/` because
`tools/eyeo/**` is eyeo-owned and survives the core-update squash. Claude Code discovers
skills under `.claude/skills/`, which upstream Chromium gitignores and documents as
local-install-only (`.claude/README.md`), so link it in rather than committing there:

```bash
ln -s ../../tools/eyeo/skills/resolve-chromium-conflicts \
      .claude/skills/resolve-chromium-conflicts
```

**The CI job needs this link too**, and creates it itself before running the script. The
script's prompts say *"Use the resolve-chromium-conflicts skill"*, so without the link
`claude` runs with the ownership rules missing — which is the one part of this that must
never be improvised.
