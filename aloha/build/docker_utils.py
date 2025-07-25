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


import argparse
import json
import os
import sys
import pwd
from pathlib import Path
from aloha_log import log
from paths import docker_home_dir, script_dir, source_dir

_docker_label = "bromium-compile"


def _build_docker():
    import docker
    dock = docker.from_env()
    user_entry = pwd.getpwuid(os.getuid())
    generator = dock.api.build(path=str(script_dir(docker=False) / "docker"),
                               tag=_docker_label,
                               buildargs={
                                   "USER_ID": str(user_entry.pw_uid),
                                   "GROUP_ID": str(user_entry.pw_gid),
                               })
    for output in generator:
        json_output = json.loads(output)
        if 'stream' in json_output:
            log(json_output['stream'].strip('\n'))


def _run_in_docker(commands: list) -> int:
    import docker
    docker_workdir = source_dir(docker=True) / Path.cwd().relative_to(source_dir(docker=False))
    host_ccache_dir = Path.home() / ".ccache"
    host_ccache_dir.mkdir(exist_ok=True)
    host_source_dir_mount = source_dir(docker=False).parent
    volumes = {
        host_ccache_dir:
            {"bind": str(docker_home_dir / ".ccache"), "mode": "rw"},
        host_source_dir_mount:
            {"bind": str(source_dir(docker=True).parent), "mode": "rw"},
    }
    depot_tools = find_depot_tools_path()
    if depot_tools and (depot_tools / ".git").exists() \
            and host_source_dir_mount not in depot_tools.parents:
        volumes[depot_tools] = \
            {"bind": str(source_dir(docker=True).parent / "depot_tools"), "mode": "rw"}

    dock = docker.from_env()
    container = dock.containers.run(image=_docker_label,
                                    command=[str(cmd) for cmd in commands],
                                    detach=True,
                                    stdout=True,
                                    stderr=True,
                                    volumes=volumes,
                                    working_dir=str(docker_workdir))
    for lines in container.logs(stream=True, stdout=True, stderr=True):
        log(lines.decode("UTF-8").strip())
    result = container.wait()
    return result.get("StatusCode", -1)


def find_depot_tools_path() -> Path:
    return next((Path(path)
                 for path in os.environ["PATH"].split(os.pathsep)
                 if "depot_tools" in path),
                None)


def dockered_main(script_filename: str, argument_adder: callable, main: callable) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--docker", action="store_true", help="run in docker")
    parser.add_argument("--script-already-running-in-docker", action="store_true", help=argparse.SUPPRESS)
    if argument_adder:
        argument_adder(parser)
    args = parser.parse_args()

    if not args.docker:
        return main(args)
    elif args.script_already_running_in_docker:
        return main(args)
    else:
        script_name = Path(script_filename).name
        log(f"Running script {script_name} in docker...")
        return _run_in_docker(["python3", script_dir(docker=True) / script_name,
                               "--script-already-running-in-docker"] + sys.argv[1:])


def _main():
    parser = argparse.ArgumentParser()
    actions = parser.add_subparsers(dest="action")
    actions.add_parser("build")
    run_action = actions.add_parser("run")
    run_action.add_argument("commands", nargs='*', help="commands for run in docker")
    args = parser.parse_args()

    if args.action == "build":
        _build_docker()
    elif args.action == "run":
        return _run_in_docker(args.commands)
    else:
        log(f"Invalid action '{args.action}'")
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    exit(_main())
