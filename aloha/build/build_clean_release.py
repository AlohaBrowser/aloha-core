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


import shutil
from aloha_log import log
from docker_utils import dockered_main
from paths import build_base_dir
import sync
import gen_build_cfg
import build_webview
import make_aar
import unsafe_publish_aar


def _add_cl_params(parser):
    gen_build_cfg.add_cl_params(parser, "no")
    build_webview.add_cl_params(parser)
    unsafe_publish_aar.add_cl_params(parser)


def _main(args):
    log("=== Removing previous build dir...")
    shutil.rmtree(build_base_dir(args.docker).parent)

    log("=== Synchronizing repositories...")
    sync.main(args)

    log("=== Generating build configuration...")
    gen_build_cfg.main(args)

    log("=== Building...")
    build_webview.main(args)

    log("=== Making AAR...")
    make_aar.main(args)

    log("=== Publishing AAR...")
    unsafe_publish_aar.main(args)
    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, _add_cl_params, _main))
