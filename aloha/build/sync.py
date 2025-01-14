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
from paths import source_dir
from docker_utils import dockered_main, find_depot_tools_path


_gclient_data = """
solutions = [
  {
    "name": "src",
    "url": "ALOHA_PUBLIC://aloha.invalid.link",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {},
  },
]
target_os = ["android"]
"""


def main(args):
    ccache_conf = Path.home() / ".ccache/ccache.conf"
    if not ccache_conf.exists():
        with open(ccache_conf, 'w') as file:
            file.write("max_size = 30G\n")

    root_dir = source_dir(args.docker).parent
    config_filename = root_dir / ".gclient"
    if not config_filename.exists():
        with open(config_filename, 'w') as file:
            file.write(_gclient_data)

    depot_tools_dir = find_depot_tools_path()
    if depot_tools_dir is None:
        raise RuntimeError("Failed to find depot_tools in PATH")
    if not (depot_tools_dir / ".gitignore").exists():
        subprocess.check_call(["git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools.git"],
                              cwd=root_dir)

    subprocess.check_call(["gclient", "sync", "--force", "--delete_unversioned_trees"])

    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, None, main))
