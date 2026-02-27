#!/bin/bash

# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	echo "
  This script generates interdiffs between eyeo Chromium SDK releases, git diff
  file will be saved in `eyeo_modules/` directory, created if needed.
  No upstream changes included in diff file.
  Module interdiffs supported since version 115.

  Usage:
    generate_interdiffs.sh <eyeo-release-tag> <eyeo-release-tag> <options>

  Options:
    --full-sdk          Generates one diff includes whole eyeo Chromium SDK implementation (include testing & CI files)
    --all-modules       Generates diff files for each module: base, chrome, webview, android-api, android-settings and extension-api
    --base              Generates one diff for base module changes
    --chrome            Generates one diff for Chorme Integration module changes
    --webview           Generates one diff for Android Webview Integration module changes
    --android-api       Generates one diff for Android API module changes
    --android-settings  Generates one diff for Android UI module changes
    --extension-api     Generates one diff for Extension API module changes
"
  exit
fi

OLD_TAG=$1
NEW_TAG=$2
OPTIONS=${@:3}
OPTIONS="${OPTIONS/--all-modules/ -- base --chrome --webview --android-api --android-settings --extension-api}"

if [ -z "$OLD_TAG" ] || [ -z "$NEW_TAG" ]; then
	echo "
  Missing arguments!
  Usage:
    generate_interdiffs.sh <eyeo-release-tag> <eyeo-release-tag> <options>
  or see help:
    generate_interdiffs.sh --help
"
	exit
fi

# git has no diff support for binary files
# also remove any internal CI files
EXCLUDE_FILES=("components/resources/adblocking/easylist*"
               "components/resources/adblocking/exceptionrules*"
               "components/resources/adblocking/anticv*"
               ".ci-scripts/*"
               ".gitlab-ci.yml"
)

for module in $OPTIONS
do
  if [ "$module" = "--base" ]; then
    SUFFIX="-base-module-branch"
  elif [ "$module" = "--chrome" ]; then
    SUFFIX="-base-chrome-module-branch"
  elif [ "$module" = "--android-api" ]; then
    SUFFIX="-base-chrome-android-api-module-branch"
  elif [ "$module" = "--android-settings" ]; then
    SUFFIX="-base-chrome-android-settings-module-branch"
  elif [ "$module" = "--extension-api" ]; then
    SUFFIX="-base-chrome-desktop-module-branch"
  elif [ "$module" = "--webview" ]; then
    SUFFIX="-base-webview-module-branch"
  elif [ "$module" = "--full-sdk" ]; then
    SUFFIX=""
  else
    echo "
  Unknow option: $module
  Avaiable options:
    --full-sdk
    --all-modules
    --base
    --chrome
    --webview
    --android-api
    --android-settings
    --extension-api
"
    exit
  fi

  repo_dir=$(git rev-parse --show-toplevel)
  mkdir -p $repo_dir/eyeo_modules/interdiffs

  OLD_TAG_HEAD=$(git rev-parse --verify origin/${OLD_TAG}${SUFFIX}  2>/dev/null)
  OLD_TAG_PREV=$(git rev-parse --verify origin/${OLD_TAG}${SUFFIX}~1  2>/dev/null)
  NEW_TAG_HEAD=$(git rev-parse --verify origin/${NEW_TAG}${SUFFIX}  2>/dev/null)
  NEW_TAG_PREV=$(git rev-parse --verify origin/${NEW_TAG}${SUFFIX}~1  2>/dev/null)

  if [ "$module" = "--full-sdk" ]; then
    OLD_TAG_HEAD=$(git ls-remote --tags origin | grep -o -e ${OLD_TAG}$)
    NEW_TAG_HEAD=$(git ls-remote --tags origin | grep -o -e ${NEW_TAG}$)
  else
    OLD_TAG_HEAD=$(git rev-parse --verify origin/${OLD_TAG}${SUFFIX}  2>/dev/null)
    NEW_TAG_HEAD=$(git rev-parse --verify origin/${NEW_TAG}${SUFFIX}  2>/dev/null)
  fi

  if [ -z "$OLD_TAG_HEAD" ]; then
    echo "Interdiff modules are not supported for version ${OLD_TAG}"
  fi
  if [ -z "$NEW_TAG_HEAD" ]; then
   echo "Interdiff modules are not supported for version ${NEW_TAG}"
  fi
  if [ -z "$OLD_TAG_HEAD" ] || [ -z "$NEW_TAG_HEAD" ]; then
	  echo "Interdiff are supported since version 115"
	  exit
  fi

  if [ "$module" = "--full-sdk" ]; then
    # Find lastest upstream commit for given version.
    OLD_VERSION=$(echo ${OLD_TAG} | cut -d "-" -f3)
    NEW_VERSION=$(echo ${NEW_TAG} | cut -d "-" -f3)
    OLD_TAG_PREV=$(git log --no-decorate --all --grep="Publish DEPS for ${OLD_VERSION}" -n 1 --pretty=format:"%h")
    NEW_TAG_PREV=$(git log --no-decorate --all --grep="Publish DEPS for ${NEW_VERSION}" -n 1 --pretty=format:"%h")
  else
    # Take advantage of modules test branches design,
    # top commit always contain module changes only.
    OLD_TAG_PREV=$(git rev-parse --verify origin/${OLD_TAG}${SUFFIX}~1  2>/dev/null)
    NEW_TAG_PREV=$(git rev-parse --verify origin/${NEW_TAG}${SUFFIX}~1  2>/dev/null)
  fi

  # Create a list of changed files, it's important to create it for each version,
  # some of files may renamed.
  OLD_TAG_FILES=$(git diff --name-only $OLD_TAG_HEAD..$OLD_TAG_PREV)
  NEW_TAG_FILES=$(git diff --name-only $NEW_TAG_HEAD..$NEW_TAG_PREV)
  # Combine and uniquify.
  COMBINED_FILES=(`for R in "${OLD_TAG_FILES[@]}" "${NEW_TAG_FILES[@]}" ; do echo "$R" ; done | sort -du`)
  # Remove files not supported by git diff
  for del in ${EXCLUDE_FILES[@]}
  do
    COMBINED_FILES=("${COMBINED_FILES[@]/$del}")
  done

  #Create diff in eyeo_modules/interdiffs/
  git diff ${OLD_TAG_HEAD}..${NEW_TAG_HEAD} -- ${COMBINED_FILES[@]} > $repo_dir/eyeo_modules/interdiffs/${module:2}-$OLD_TAG-$NEW_TAG.diff

done
