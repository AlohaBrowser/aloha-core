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
import os
import subprocess
import glob
from pathlib import Path
from aloha_log import log
from config import architectures, test_types, default_test_arch
from paths import build_base_dir
from docker_utils import dockered_main

test_templates = {
    "cpp": "run_*_unittests",
    "java": "run_*_junit_tests",
    "apk": "run_*_test_apk",
}
assert(set(test_templates.keys()) == test_types)


def _add_cl_params(parser):
    parser.add_argument("-t", "--tests", nargs="*", default=test_types,
                        help=f"test types for build (allowed={test_types})")
    parser.add_argument("-r", "--run-all-tests", action="store_true", help="run all tests without succeed check")
    parser.add_argument("-a", "--archs", nargs="*", default=[default_test_arch],
                        help=f"architectures for build (allowed={set(architectures.keys())})")
    parser.add_argument("-e", "--device", type=str, default="", help="adb device name for run tests")


def _succeed_tests_filename(docker: bool, arch: str) -> Path:
    return build_base_dir(docker) / architectures[arch] / "succeed-tests-for-bromium.json"


def clear_succeed_tests(docker: bool, arch: str):
    filename = _succeed_tests_filename(docker, arch)
    if os.path.exists(filename):
        os.remove(filename)


def _add_succeed_tests(docker: bool, arch: str, test_type: str, test_name: str):
    filename = _succeed_tests_filename(docker, arch)
    succeed_tests = {}
    if os.path.exists(filename):
        with open(filename) as file:
            succeed_tests = json.load(file)
    if test_type not in succeed_tests:
        succeed_tests[test_type] = []
    if test_name not in succeed_tests[test_type]:
        succeed_tests[test_type].append(test_name)
    with open(filename, 'w') as file:
        json.dump(succeed_tests, file, indent=2)


def _was_test_succeed(docker: bool, arch: str, test_type: str, test_name: str):
    filename = _succeed_tests_filename(docker, arch)
    succeed_tests = {}
    if os.path.exists(filename):
        with open(filename) as file:
            succeed_tests = json.load(file)
    return test_type in succeed_tests \
        and test_name in succeed_tests[test_type]


def run_tests(docker: bool, arch: str, tests: list, device: str):
    build_path = build_base_dir(docker) / architectures[arch]

    ignore_tests = {
        # excluded in find_test.py:
        "run_remoting_test_apk",
        "run_cc_unittests",
        "run_content_shell_test_apk",
        "run_cronet_sample_test_apk",
        "run_webview_instrumentation_test_apk",
        "run_webview_ui_test_app_test_apk",
    }
    tests_for_repair = {
        "run_blink_unittests",
        "run_gin_unittests",
        "run_google_apis_unittests",

        "run_system_webview_shell_layout_test_apk",
        "run_webview_js_sandbox_test_app_test_apk",
    }

    bin_path = build_path / "bin"
    for test_type in tests:
        log(f"\n\n=== Running tests '{test_type}' for {arch}...")
        test_bins = sorted(glob.glob(str(bin_path / test_templates[test_type])))
        for test_bin in test_bins:
            test_name = Path(test_bin).name
            if test_name in ignore_tests:
                pass
            elif test_name in tests_for_repair:
                log(f"=== Test '{test_name}' needs to be repaired, skipped")
            elif _was_test_succeed(docker, arch, test_type, test_name):
                log(f"=== Test '{test_name}' was succeed, skipped")
            else:
                log(f"\n\n=== Running test '{test_name}'...")
                params = [test_bin]
                if device and len(device) > 0 and not test_name.endswith("_junit_tests"):
                    params += ["--device", device]
                subprocess.check_call(params)
                _add_succeed_tests(docker, arch, test_type, test_name)


def _main(args):
    for arch in args.archs:
        if args.run_all_tests:
            clear_succeed_tests(args.docker, arch)
        run_tests(args.docker, arch, args.tests, args.device)
    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, _add_cl_params, _main))
