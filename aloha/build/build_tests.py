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
from paths import build_base_dir, tests_json_filename
from docker_utils import dockered_main


def _add_cl_params(parser):
    parser.add_argument("-t", "--tests", nargs="*", default=test_types,
                        help=f"test types for build (allowed={test_types})")
    parser.add_argument("-a", "--archs", nargs="*", default=[default_test_arch],
                        help=f"architectures for build (allowed={set(architectures.keys())})")
    parser.add_argument("-j", "--jobs", type=int, default=0, help="jobs count (0 - by cpu count)")


def build_tests(docker: bool, arch: str, tests: list, jobs: int):
    from run_tests import clear_succeed_tests
    clear_succeed_tests(docker, arch)
    
    build_path = build_base_dir(docker) / architectures[arch]
    with open(build_path / tests_json_filename) as file:
        tests_json = json.load(file)

    log(f"Building tests {tests} for {arch}...")
    test_targets = [name
                    for test_type in tests
                    for name in tests_json[test_type]]
    assert (all(name.startswith("//") for name in test_targets))
    build_targets = [name[2:]
                     for name in test_targets]
    params = ["autoninja"]
    if jobs > 0:
        params += ['-j', str(jobs)]
    params += ["-C", build_path] + build_targets
    subprocess.check_call(params)


def _main(args):
    from build_webview import generate_aloha_js
    generate_aloha_js(args.docker)

    for arch in args.archs:
        build_tests(args.docker, arch, args.tests, args.jobs)
    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, _add_cl_params, _main))
