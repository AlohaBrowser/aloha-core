# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
#
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import os
import subprocess
import sys

repo_root = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))


def get_executable():
    third_party_instance = os.path.join(repo_root, "third_party", "lzma_sdk",
                                        "bin", "host_platform", "7za")

    # On Windows, gclient sync checks out an instance of 7za into third_party/lzma_sdk
    if sys.platform == 'win32':
        third_party_instance += '.exe'
        return third_party_instance

    # On Linux, third_party/lzma_sdk will contain 7za only if .gclient has target_os = ["win"]
    if os.path.exists(third_party_instance):
        return third_party_instance

    # Alternatively, system 7za may be available the p7zip-full apt package is installed
    system_instance = "/usr/bin/7za"
    return system_instance


def read_paths(input_file):
    with open(input_file, 'r') as f:
        return [line.strip() for line in f.readlines()]


def rebase_paths(input_paths, build_dir):
    # Create a list of absolute paths based on build_dir
    absolute_paths = [
        os.path.abspath(os.path.join(build_dir, path)) for path in input_paths
    ]

    # Return a list of rebased paths relative to repo_root
    return [os.path.relpath(path, repo_root) for path in absolute_paths]


def compress_paths(paths, output):
    # If an archive already exists, 7za will add new files but not
    # remove old files. Remove existing archive to ensure a clean slate.
    if os.path.exists(os.path.join(repo_root, output)):
        os.remove(os.path.join(repo_root, output))
    # Create the 7z command to add the paths to the output archive
    command = [get_executable(), 'a', output] + paths

    # Execute the 7z command
    try:
        subprocess.run(command,
                       cwd=repo_root,
                       capture_output=True,
                       universal_newlines=True)
    except subprocess.CalledProcessError as err:
        print(err.stderr, file=sys.stderr)
        raise


def main():
    # Parse the command-line arguments
    parser = argparse.ArgumentParser(description='Rebase and archive paths.')
    parser.add_argument('--input-file',
                        required=True,
                        help='Path to the input file, relative to build dir')
    parser.add_argument('--build-dir',
                        required=True,
                        help='Absolute path to the build directory')
    parser.add_argument(
        '--output',
        required=True,
        help='Output path to the archive, relative to build dir')
    args = parser.parse_args()

    inputs = read_paths(args.input_file)
    # compression requires inputs to be relative to repo root, to add resources
    # from outside of the build dir
    inputs_relative_to_root = rebase_paths(inputs, args.build_dir)
    compress_paths(inputs_relative_to_root,
                   os.path.abspath(os.path.join(args.build_dir, args.output)))


if __name__ == '__main__':
    main()
