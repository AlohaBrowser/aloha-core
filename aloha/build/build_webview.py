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
import os
import telebot
from telebot import types
from aloha_log import log
from config import architectures
from paths import source_dir, aloha_js_filename, build_base_dir
from docker_utils import dockered_main
from datetime import datetime
import time

aloha_js_sources = [
    "aloha/src/js/aloha_base.js",
    "aloha/src/js/readability.js",
    "aloha/src/js/apm.js",
    "aloha/src/js/aloha_id.js"
]


def generate_aloha_js(docker: bool):
    lines = []
    for filename in aloha_js_sources:
        with open(source_dir(docker) / filename) as file:
            lines += file.readlines()
    aloha_js = aloha_js_filename(docker)
    if os.path.isfile(aloha_js):
        with open(aloha_js) as file:
            if file.readlines() == lines:
                # Do not rewrite file for not rebuild.
                return
    with open(aloha_js, 'w') as file:
        file.writelines(lines)


def add_cl_params(parser):
    parser.add_argument("-a", "--archs", nargs="*", default=architectures.keys(),
                        help=f"architectures for build (allowed={set(architectures.keys())})")
    parser.add_argument("-j", "--jobs", type=int, default=0, help="jobs count (0 - by cpu count)")


def main(args):
    start_time = datetime.now()

    log(f"*** Generating aloha.js ***")
    generate_aloha_js(args.docker)

    log(f"*** Removing previous APK ***")
    for arch_dir in architectures.values():
        apk_filename = build_base_dir(args.docker) / arch_dir / "apks/BromiumWebView.apk"
        if os.path.isfile(apk_filename):
            os.remove(apk_filename)

    tlg_log_build_result = 0
    tlg_log_arch_count = 0
    for arch in args.archs:
        build_path = build_base_dir(args.docker) / architectures[arch]
        log(f"*** Building {arch} ***")
        params = ["autoninja"]
        if args.jobs > 0:
            params += ['-j', str(args.jobs)]
        params += ["-C", build_path, "bromium_webview_apk"]
        try:
            tlg_log_build_result += subprocess.check_call(params)
            tlg_log_arch_count += 1
        except subprocess.CalledProcessError as e:
            tlg_log_build_result = 1
            break
    
    bot = telebot.TeleBot('ALOHA_PUBLIC:BOT_TOKEN')
    build_result = "success" if tlg_log_build_result == 0 else "FAILED!"
    bot.send_message('-ALOHA_PUBLIC:CHAT_ID', f"Build result: {build_result}  Arch count: {tlg_log_arch_count}")
    log(f"Building duration: {datetime.now() - start_time}")

    return 0



if __name__ == "__main__":
    exit(dockered_main(__file__, add_cl_params, main))
