#!/bin/python3

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

# This script generates a graft file from git history. A graft file is a list
# of commits that connect disjoint branches in the git history.
#
# Example graft file contents:
# 1993fb58c5732068109ce4178ee09bced4973a8a 83a714deb7aeff970e55ef11174b03aa02970d29
# 3cc64cebef5c028d87495de797630d2e3d56d522 a71c84f53e4e2254f6a34ed50e37ecf5d4362eb9
# 85b8322d2c1c97e7adfbe2162e7ade16fcd1e019 f2299e697fb99f2a693b405a2043d9622ebfe68b
# ...
#
# When git blame traverses history and reaches a commit 1993fb58c5732068109ce4178ee09bced4973a8a
# it will skip over it and go to 83a714deb7aeff970e55ef11174b03aa02970d29, then resume
# traversing history from there. If it then reaches 3cc64cebef5c028d87495de797630d2e3d56d522
# it will skip over to a71c84f53e4e2254f6a34ed50e37ecf5d4362eb9 and so on.
#
# In eyeo, we squash our changes when we update to a new Chromium version. This
# means that we lose the history of our changes after every upgrade. Thankfully,
# every squashed commit has a "eyeo-parent-commit" line in its commit message
# which leads us to its pre-squash history. This script traverses
# the git history and generates a graft file that contains pairs of:
# hash-of-squash-commit hash-of-its-eyeo-parent-commit

import argparse
import re
import subprocess


def get_commit_message(commit_hash):
  try:
    # Try to get the commit message
    return subprocess.check_output(
        ['git', 'show', '-s', '--format=%B', commit_hash]).decode('utf-8')
  except subprocess.CalledProcessError:
    # This can happen if the commit is a bad object.
    # For example, if the branch was deleted after a merge request was merged.
    return None


def get_parent_commits(commit_hash):
  # Get the parent commits of the current commit
  return subprocess.check_output(
      ['git', 'rev-list', '-1', '--parents',
       commit_hash]).decode('utf-8').strip().split()[1:]


def read_existing_grafts(graft_file):
  # Read the existing grafts from the graft file
  with open(graft_file, 'r') as f:
    return [tuple(line.strip().split()) for line in f]


def get_grafts_from_history(start_commit, max_commits, existing_grafts):
  # Recurse into git history to find grafts.
  grafts = []
  # Since branches can have multiple parents, we need to recurse through
  # all possible paths to find the next "eyeo-parent-commit" line (ie. "mainline").
  # Instead of using recursion, we use a stack onto which we push the next
  # commit to visit and its depth in the tree.
  stack = [(start_commit, 0)]
  while stack:
    commit_hash, depth = stack.pop()
    if depth >= max_commits:
      continue
    commit_message = get_commit_message(commit_hash)
    if commit_message is None:
      continue  # Skip this branch, it has a bad object (deleted MR branch?)
    match = re.search(r'eyeo-parent-commit:([a-f0-9]{40})', commit_message)
    if match:
      # We found a graft! Add it to the list.
      parent_commit = match.group(1)
      graft = (commit_hash, parent_commit)
      if graft in existing_grafts:
        # Since this graft is already in existing_grafts, the rest of history
        # is already grafted. We can stop traversing. This will happen if
        # a partially populated graft file already exists and we're only
        # updating it with the latest grafts.
        break
      grafts.append(graft)
      # Since we found a graft, we know we're on "mainline" and we can
      # prune unvisited branches. They will not contain any grafts.
      stack = [(parent_commit, depth + 1)]
    else:
      # We didn't find a graft. Keep traversing history, checking all parents.
      parent_commits = get_parent_commits(commit_hash)
      for parent_commit in parent_commits:
        stack.append((parent_commit, depth + 1))
  return grafts


def append_graft_file(grafts, graft_file):
  # Append the new grafts to the graft file
  with open(graft_file, 'a') as f:
    for graft in grafts:
      f.write(f"{graft[0]} {graft[1]}\n")


def main():
  parser = argparse.ArgumentParser(
      description='Generate a graft file from git history.')
  parser.add_argument('--start-commit',
                      default='HEAD',
                      help='The commit to start traversing from.')
  parser.add_argument('--max-commits',
                      type=int,
                      default=float('inf'),
                      help='The maximum number of commits to traverse.')
  parser.add_argument('--graft-file',
                      default='git-grafts.txt',
                      help='The file to append grafts to.')
  args = parser.parse_args()

  # Read the existing grafts
  existing_grafts = read_existing_grafts(args.graft_file)
  # Get the new grafts from the git history
  grafts = get_grafts_from_history(args.start_commit, args.max_commits,
                                   existing_grafts)
  # Append the new grafts to the graft file
  append_graft_file(grafts, args.graft_file)


if __name__ == '__main__':
  main()
