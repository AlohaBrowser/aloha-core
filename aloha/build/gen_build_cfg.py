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


import subprocess
from pathlib import Path
from aloha_log import log
from config import architectures
from paths import aloha_js_filename, script_dir, build_base_dir
from docker_utils import dockered_main

build_types = {"release", "dcheck"}


def make_args_gn(docker: bool, arch_name: str, build_type: str, ccache: bool, output_path: Path):
    lines = [
        '########',
        f'### From {Path(__file__).name}:',
        f'target_cpu="{arch_name}"',
        f'v8_embed_script="{aloha_js_filename(docker)}"',
    ]
    if ccache:
        lines.append('cc_wrapper = "ccache"')

    def from_file(filename):
        lines = [
            '',
            '########',
            f'### From {filename}:',
        ]
        with open(script_dir(docker) / filename) as file:
            lines += file.readlines()
        return lines

    lines += from_file("args.gn.common.templ")
    lines += from_file(f"args.gn.{build_type}.templ")

    with open(output_path, 'w') as out_file:
        out_file.writelines([line if line.endswith('\n') else f"{line}\n"
                             for line in lines])


def _parse_ccache(args) -> bool:
    ccache = (args.ccache or "").lower()
    if ccache in {"no", "false", "off"}:
        return False
    if ccache in {"", "yes", "true", "on"}:
        return True
    raise RuntimeError(f"Invalid ccache value '{args.ccache}'")


def add_cl_params(parser, ccache_default="yes"):
    parser.add_argument("-b", "--build-type", type=str, default="release", choices=build_types,
                        help="build type")
    parser.add_argument("-c", "--ccache", type=str, default=ccache_default, nargs='?', help="use ccache")


def main(args):
    ccache = _parse_ccache(args)
    log(f"ccache is {'on' if ccache else 'off'}")
    for arch_name, arch_dirname in architectures.items():
        arch_dir = build_base_dir(args.docker) / arch_dirname
        arch_dir.mkdir(parents=True, exist_ok=True)
        make_args_gn(args.docker, arch_name, args.build_type, ccache, arch_dir / "args.gn")
        log(f"Generating {arch_name} configuration")
        subprocess.check_call(["gn", "gen", arch_dir])
    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, add_cl_params, main))
