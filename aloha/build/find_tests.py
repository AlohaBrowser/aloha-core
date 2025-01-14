#!/usr/bin/python3

# Copyright 2024 Aloha Mobile Ltd.

# Permission is hereby granted, free of charge, to any person obtaining 
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import json
import subprocess
from aloha_log import log
from config import architectures, test_types, default_test_arch
from docker_utils import dockered_main
from paths import build_base_dir, tests_json_filename


def find_tests(docker: bool, arch_name: str):
    from run_tests import clear_succeed_tests
    clear_succeed_tests(docker, arch_name)

    build_dir = build_base_dir(docker) / architectures[arch_name]
    log("Getting bromium dependencies...")
    all_bromium_deps_text = subprocess.check_output([
        "gn", "desc",
        "-C", build_dir,
        "//android_webview:bromium_webview_apk",
        "deps", "--all",
    ])
    all_bromium_dep_dirs = {name.split(":")[0]
                            for name in all_bromium_deps_text.decode("utf-8").split('\n')}

    log("Getting all tests...")
    all_tests_text = subprocess.check_output([
        "gn", "ls",
        "-C", build_dir,
        "--testonly=true",
    ])
    all_tests = {name
                 for name in all_tests_text.decode("utf-8").split('\n')
                 if name and len(name) > 0}
    bromium_dep_tests = {name
                         for name in all_tests
                         if name.split(":")[0] in all_bromium_dep_dirs}

    cpp_ignore_tests = {
        # Failed on original chromium build:
        "//third_party/blink/renderer/platform:blink_fuzzer_unittests",
        "//ui/compositor:compositor_unittests",
        "//third_party/flatbuffers:flatbuffers_unittests",
        "//third_party/libaddressinput:libaddressinput_unittests",
        "//third_party/libphonenumber:libphonenumber_unittests",
        "//media/remoting:media_remoting_unittests",
        "//components/metrics:metrics_unittests",
        "//net:net_unittests",
        "//third_party/perfetto:perfetto_unittests",
        "//sandbox/linux:sandbox_linux_unittests",
        "//ui/snapshot:snapshot_unittests",
        "//components/ukm:ukm_unittests",
        "//components/variations:variations_unittests",
        "//cc:cc_unittests",  # failed 144 tests when run with dchecks
    }

    # See https://chromium.googlesource.com/chromium/src/+/refs/heads/main/testing/android/docs/android_test_instructions.md#C
    cpp_tests = {name
                 for name in bromium_dep_tests
                 if (name.endswith("_unittests")
                         or name.endswith("_browsertests")
                         or name.endswith("_browser_tests"))
                     and all(ignore not in name
                             for ignore in cpp_ignore_tests)}

    # See https://chromium.googlesource.com/chromium/src/+/refs/heads/main/testing/android/docs/android_test_instructions.md#java
    java_tests = {name
                  for name in bromium_dep_tests
                  if name.endswith("_junit_tests")}

    apk_ignore_set = {
        "__test_apk",
        "//weblayer/",
        "//chrome/",
        "//build/android/test/nocompile_gn:",
        "//remoting/android:remoting_test_apk",
        "//content/shell/android:content_shell_test_apk",  # crashed emulator in original chromium
        "//android_webview/test:webview_instrumentation_test_apk",  # failed 57 tests in original chromium
        "//components/cronet/android:cronet_sample_test_apk",  # failed 1 test of 3 in original chromium
        "//android_webview/tools/automated_ui_tests:webview_ui_test_app_test_apk",  # failed 1 of 1 test in original chromium
    }
    apk_tests = {name
                 for name in all_tests
                 if name.endswith("_test_apk") and all(ignore not in name
                                                       for ignore in apk_ignore_set)}

    test_json = {
        "cpp": sorted(cpp_tests),
        "java": sorted(java_tests),
        "apk": sorted(apk_tests),
    }
    assert (set(test_json.keys()) == test_types)
    with open(build_dir / tests_json_filename, 'w') as file:
        json.dump(test_json, file, indent=2)

    return 0


def _add_cl_params(parser):
    parser.add_argument("-a", "--archs", nargs="*", default=[default_test_arch],
                        help=f"architectures for generate (allowed={set(architectures.keys())})")


def _main(args):
    for arch_name in args.archs:
        log(f"Finding tests for {arch_name}")
        find_tests(args.docker, arch_name)


if __name__ == "__main__":
    exit(dockered_main(__file__, _add_cl_params, _main))
