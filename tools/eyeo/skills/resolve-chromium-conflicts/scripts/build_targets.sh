# Stage 4 target lists for the resolve-chromium-conflicts skill.
#
# Data, not code, and kept in their own file because resolve_conflicts.sh
# re-sources this before every build attempt. A target name that upstream has
# deleted is a normal finding of a core update -- it is most of what stage 4
# catches -- and when the agent corrects one it has to take effect on the next
# attempt. Assigning these in the driver made that impossible: the variable is
# expanded when the script starts, so job 16501332460 spent every remaining
# stage-4 attempt re-running a target list the agent had already fixed on disk.
#
# These track .gitlab-ci.yml's own lists. Stage 4 exists to prove the branch
# builds what the follow-up `default` pipeline builds, so when the two disagree
# the pipeline is the authority. `ninja -t query <target>` against a generated
# out dir checks every name in seconds -- see reference/build-config.md.

DESKTOP_TARGETS="chrome unit_tests components_unittests browser_tests chrome_sandbox \
content_shell components_browsertests content_browsertests components_perftests \
eyeo_test_servers.7z archive_eyeo_benchmarks"

# chrome_public_bundle, not monochrome_public_bundle: upstream deleted the
# Monochrome bundle targets (Trichrome superseded them), so the inherited name
# made ninja abort at target resolution before compiling anything.
ANDROID_TARGETS="chrome_public_apk unit_tests components_unittests chrome_public_test_apk \
components_perftests system_webview_apk system_webview_shell_apk content_shell_apk \
adblock_shell_apk verify_flatbuffer_adblocking chrome_public_bundle \
webview_instrumentation_test_apk"
