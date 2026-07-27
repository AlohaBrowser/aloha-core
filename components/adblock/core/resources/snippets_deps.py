#!/usr/bin/env python3

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
import os.path
import shutil


class SnippetsUtils:

    def __init__(self, args):
        self.deps_input_file = args.deps
        self.deps_output_file = args.output + '/snippets-xpath3-dep.jst'
        self.script_input_file = args.lib
        self.script_output_file = args.output + '/snippets.jst'

    # Reads the dependencies.jst file distributed with snippets library
    # and extracts from it the code which is required to support xpath3 snippet filters.
    # The extracted code is copied into the location pointed out by output_file argument.
    def prepare_deps(self):
        # Create file so it exists for sure
        with open(self.deps_output_file, 'w', encoding='utf8') as out_file:
            lines_to_write = []
            start_tag = '/**! Start hide-if-matches-xpath3 dependency !**/'
            end_tag = '/**! End hide-if-matches-xpath3 dependency !**/'
            between_tags = False
            with open(self.deps_input_file, 'r', encoding='utf8') as in_file:
                for line in in_file:
                    if line.strip() == start_tag:
                        between_tags = True
                        continue
                    if line.strip() == end_tag:
                        between_tags = False
                        continue
                    if between_tags:
                        lines_to_write.append(line)
            out_file.writelines(lines_to_write)

    def prepare_script(self):
        shutil.copy(self.script_input_file, self.script_output_file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Prepares snippets library for BAS')
    parser.add_argument('--deps',
                        type=str,
                        help='snippet library dependencies')
    parser.add_argument('--lib',
                        type=str,
                        help='main script for snippet library')
    parser.add_argument('--output',
                        type=str,
                        help='output folder for final content')
    main = SnippetsUtils(parser.parse_args())
    main.prepare_deps()
    main.prepare_script()
