#!/usr/bin/env python3
# Copyright 2024 Aloha Mobile Ltd.
#
# Formats C/C++ files changed in the current branch (git diff HEAD).
# Uses clang-format from the project's buildtools.
#
# Usage:
#   python3 aloha/build/clang_format.py            # apply formatting
#   python3 aloha/build/clang_format.py --check    # check only, no changes

import argparse
import subprocess
import sys
from pathlib import Path

CC_EXTENSIONS = {'.cc', '.cpp', '.c', '.h', '.mm'}

SOURCE_DIR = Path(__file__).parent.parent.parent
CLANG_FORMAT = SOURCE_DIR / 'buildtools/linux64-format/clang-format'


def get_changed_files() -> list[Path]:
    """Returns C/C++ files from git diff HEAD (staged + unstaged)."""
    result = subprocess.run(
        ['git', 'diff', '--name-only', '--diff-filter=ACMR', 'HEAD'],
        cwd=SOURCE_DIR,
        capture_output=True,
        text=True,
        check=True,
    )
    files = []
    for line in result.stdout.splitlines():
        path = Path(line.strip())
        if path.suffix in CC_EXTENSIONS:
            files.append(SOURCE_DIR / path)
    return files


def main():
    parser = argparse.ArgumentParser(
        description='Runs clang-format on files from git diff HEAD.')
    parser.add_argument(
        '--check',
        action='store_true',
        help='Check formatting only, do not modify files.')
    args = parser.parse_args()

    if not CLANG_FORMAT.exists():
        print(f'Error: clang-format not found: {CLANG_FORMAT}', file=sys.stderr)
        sys.exit(1)

    files = get_changed_files()
    if not files:
        print('No changed C/C++ files.')
        return

    print(f'Files to format: {len(files)}')
    for f in files:
        print(f'  {f.relative_to(SOURCE_DIR)}')

    if args.check:
        cmd = [str(CLANG_FORMAT), '--style=file', '--dry-run', '--Werror']
    else:
        cmd = [str(CLANG_FORMAT), '--style=file', '-i']

    result = subprocess.run(cmd + [str(f) for f in files], cwd=SOURCE_DIR)

    if result.returncode != 0:
        print('\nFormatting does not match .clang-format.', file=sys.stderr)
        print('Run without --check to fix.', file=sys.stderr)
        sys.exit(1)
    elif not args.check:
        print('Done.')


if __name__ == '__main__':
    main()
