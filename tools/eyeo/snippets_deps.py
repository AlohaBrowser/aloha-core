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


# This class reads the dependencies.jst file distributed with snippets library
# and extracts from it the code which is required to support xpath3 snippet filters.
# The extracted code is copied into the location pointed out by output_file argument.
class PrepareXpath3Dep:
  def __init__(self, args):
    self.input_file = args.input_dir + "/dependencies.jst"
    self.output_file = args.output_file

  def run(self):
    # Create file so it exists for sure
    with open(self.output_file, 'w', encoding="utf8") as out_file:
      out_file.writelines([])
      if os.path.exists(self.input_file):
        lines_to_write = []
        start_tag = "/**! Start hide-if-matches-xpath3 dependency !**/"
        end_tag = "/**! End hide-if-matches-xpath3 dependency !**/"
        between_tags = False
        with open(self.input_file, encoding="utf8") as in_file:
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
        in_file.close()
        out_file.close()


if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='Prepares snippets xpath3 dependency')

  parser.add_argument('input_dir',
                      help='directory where dependencies.jst can be found')
  parser.add_argument(
      'output_file',
      help='output file where to copy content parsed from dependencies.jst')

  main = PrepareXpath3Dep(parser.parse_args())
  main.run()
