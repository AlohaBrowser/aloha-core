# Build configuration

Taken verbatim from `tools/eyeo/merge_conflicts_solver.sh` (MR !1179) **except** that every
credential is now an environment variable. The original hardcoded a live
`eyeo_telemetry_activeping_auth_token` value into the Desktop args, in a public repo.

The live pipeline already does this correctly — `.gitlab-ci.yml` on the dev branch appends
the token from a CI variable, and `components/adblock/features.gni` defaults it to `""`.
Follow that pattern; do not reintroduce a literal.

## Required environment

| Variable | Source |
|---|---|
| `EYEO_TELEMETRY_AUTH_TOKEN` | masked CI variable |
| `EYEO_APPLICATION_NAME` | CI config |
| `EYEO_APPLICATION_VERSION` | CI config |
| `EYEO_TELEMETRY_SERVER_URL` | CI config (Android) |
| `UPSTREAM_TAG` | the vanilla Chromium tag being moved to |

## Remote execution: Siso, not reclient

Both platforms use `use_siso=true use_reclient=false`, the same as `.add_gn_args` in
`.gitlab-ci.yml`. `use_siso=false` makes `use_reclient` default to true, and
`//build/toolchain/rbe.gni` then asserts that `//buildtools/reclient/rewrapper` exists.
Upstream DEPS only downloads it when `download_reclient` is set, and that defaults to
`checkout_chromeos`, so a Linux/Android checkout never has it. gn then fails before it
reads a single eyeo `BUILD.gn`.

## Desktop

Out dir: `out/Desktop-Release`

> Note the casing. The original script ran `gn gen` into `out/Desktop-release` and then built
> from `out/Desktop-Release/` — two different directories on a case-sensitive filesystem, so
> the build ran somewhere `gn gen` had never configured. Use one spelling everywhere.

```
eyeo_intercept_debug_url=true
is_debug=false
use_remoteexec=true
use_siso=true
use_reclient=false
proprietary_codecs=true
ffmpeg_branding="Chrome"
eyeo_application_name="${EYEO_APPLICATION_NAME}"
eyeo_application_version="${EYEO_APPLICATION_VERSION}"
disable_fieldtrial_testing_config=true
eyeo_telemetry_activeping_auth_token="${EYEO_TELEMETRY_AUTH_TOKEN}"
dcheck_always_on=false
eyeo_extend_chrome_devtools_protocol=true
```

Full targets:

```
chrome unit_tests components_unittests browser_tests chrome_sandbox content_shell
components_browsertests content_browsertests components_perftests
eyeo_test_servers.7z archive_eyeo_benchmarks
```

## Android

Out dir: `out/Release`

```
target_os="android"
target_cpu="x64"
is_debug=false
symbol_level=1
dcheck_always_on=false
disable_fieldtrial_testing_config=true
use_siso=true
use_reclient=false
use_remoteexec=true
proprietary_codecs=true
ffmpeg_branding="Chrome"
enable_nacl=false
is_cfi=false
eyeo_intercept_debug_url=true
eyeo_telemetry_server_url="${EYEO_TELEMETRY_SERVER_URL}"
eyeo_telemetry_activeping_auth_token="${EYEO_TELEMETRY_AUTH_TOKEN}"
```

Full targets:

```
chrome_public_apk unit_tests components_unittests chrome_public_test_apk
components_perftests system_webview_apk system_webview_shell_apk content_shell_apk
adblock_shell_apk verify_flatbuffer_adblocking chrome_public_bundle
webview_instrumentation_test_apk
```

> This list must stay in step with the Android `BUILD_TARGETS` in `.gitlab-ci.yml`
> (`build_and_test_x64_release`). Stage 4's whole purpose is to prove the branch builds
> what the follow-up `default` pipeline builds, so a target here that the release pipeline
> does not build — or a release target missing here — makes a green stage 4 mean less than
> it appears to.
>
> The inherited list said `monochrome_public_bundle`. Upstream deleted the Monochrome
> bundle targets (Trichrome superseded them; only the `libmonochrome` shared library keeps
> the name), while `.gitlab-ci.yml` had been building `chrome_public_bundle` all along.
> ninja validates the whole list before running an edge, so a name like that aborts the
> build having compiled nothing — one wasted stage-4 attempt per occurrence.

**Check the target names resolve before spending a build on them.** ninja validates the
target list up front and aborts before running a single edge, so `-k 0` buys nothing here:
one stale name returns one error and zero compile information, for a full stage-4 slot.
The bundled ninja answers this in seconds per name — note it is `third_party/ninja/ninja`,
because `depot_tools/ninja` is a wrapper that refuses to run under `use_remoteexec=true`:

```bash
for t in $ANDROID_TARGETS; do
  ./third_party/ninja/ninja -C out/Release -t query "$t" >/dev/null 2>&1 \
    || echo "MISSING $t"
done
```

## Always build with `-k 0`

ninja defaults to `-k 1`: stop after the first failing job. On a four-hour Chromium build
that returns exactly one compile error per attempt. `-k 0` keeps going and reports every
independent failure, so one build gives the whole error set:

```bash
autoninja -C out/Desktop-Release -k 0 <targets> > build_log.txt 2>&1
```

A doomed build then runs to completion rather than aborting early. That is the correct
trade — the cost driver is the number of attempts, not the length of one.

## Stage 3 — picking targeted targets

Map touched files to the smallest owning targets rather than building the lists above:

```bash
gn refs out/Desktop-Release <file>                # smallest targets owning this file
gn refs out/Desktop-Release <file> --type=executable
```

Build those. Escalate to the full list only once they pass.

Use plain `gn refs`, not `gn refs --all`. `--all` walks the entire reverse
dependency closure, so a core update that touches a widely-included header
resolves to umbrella targets and effectively rebuilds the world. `targeted_for()`
in `scripts/resolve_conflicts.sh` follows this rule.

## Disk

One checkout carrying both output directories does not fit on a 194G runner:
source ~60G (43G of it `third_party`), `out/Release` (Android) ~74G,
`out/Desktop-Release` ~39G. Android stage 3 has run out of space on that volume.
Either give the runner a larger volume, or build one platform per job.
