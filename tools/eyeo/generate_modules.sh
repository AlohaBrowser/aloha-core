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

if [ $# -eq 0 ]; then
  echo "Please provide an eyeo tag argument." \
       "It is needed to find vanila branch to generate module diffs for."
  exit 1
fi

EYEO_RELEASE_TAG=$1

CHROMIUM_TAG=$(echo ${EYEO_RELEASE_TAG} | cut -d "-" -f3)
TOP_VANILLA_COMMIT=$(git rev-parse --short $(git log --grep='Copy eyeo build requirements from' --pretty=format:"%h")~1)

if [[ $(git cat-file -t ${TOP_VANILLA_COMMIT} 2> /dev/null) != "commit" ]]; then
  echo "Failed to find vanila branch."
  exit 1
fi

INITIAL_BRANCH=$(git rev-parse --abbrev-ref HEAD)
INITIAL_HASH=$(git rev-parse HEAD)

### Remove CI & testing ###
CI_AND_TESTING_FILES="
.ci-scripts/
.gitignore
.gitlab-ci.yml
.pre-commit-config.yaml
.vpython
android_webview/tools/adblock_shell/
build/install-build-deps.py
components/eyeo_testing/
gclient/
tools/perf/
"

for path in $CI_AND_TESTING_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

CI_AND_TESTING_FILES_TO_CROP="
components/adblock/core/BUILD.gn
BUILD.gn
DEPS
"

for path in $CI_AND_TESTING_FILES_TO_CROP
do
  sed -i '/CI & Testing module start/,/CI & Testing module end/d' $path
  git add $path
done

git commit -n -m "Remove CI requirements"
NEGATIVE_CI_TESTING=$(git rev-parse --short HEAD)

### Remove Content Shell
ANDROID_CONTENT_SHELL_FILES="
content/shell/android/
components/adblock/android/
"

for path in $ANDROID_CONTENT_SHELL_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

ANDROID_CONTENT_SHELL_FILES_TO_CROP="
content/shell/BUILD.gn
"

for path in $ANDROID_CONTENT_SHELL_FILES_TO_CROP
do
  sed -i '/Android Content Shell module start/,/Android Content Shell module end/d' $path
  git add $path
done

git commit -n -m "Negative Android Content Shell module"
NEGATIVE_ANDROID_CONTENT_SHELL_MODULE_HASH=$(git rev-parse --short HEAD)

### Restore components/adblock/android/ as this is shared part between Webview, Android API and Android Content Shell
git restore --source=HEAD~1 --staged --worktree -- components/adblock/android/
git commit -n -m "Restore jni_headers"

### Android UI removal ###
ANDROID_UI_FILES="
chrome/android/java/res/xml/main_preferences.xml
chrome/android/java/res/xml/main_preferences_legacy.xml
chrome/browser/adblock/android/adblock_strings.grd
chrome/browser/adblock/android/java/
chrome/browser/adblock/android/javatests/src/org/chromium/chrome/browser/adblock/AdblockFilterFragmentTest.java
chrome/browser/adblock/android/README.md
chrome/browser/adblock/android/translations/
"

ANDROID_UI_SHARED_FILES="
chrome/browser/adblock/android/BUILD.gn
chrome/android/BUILD.gn
"

for path in $ANDROID_UI_SHARED_FILES
do
  sed -i '/Android UI module start/,/Android UI module end/d' $path
  git add $path
done

for path in $ANDROID_UI_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

git commit -n -m "Negative Android UI module"
NEGATIVE_ANDROID_UI_MODULE_HASH=$(git rev-parse --short HEAD)

### Android Webview removal ###
WEBVIEW_FILES="
android_webview/
components/adblock/android/
"

for path in $WEBVIEW_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

git commit -n -m "Remove Android Webview integration"
NEGATIVE_WEBVIEW_INTEGRATION=$(git rev-parse --short HEAD)

### Restore components/adblock/android/ as this is shared part between Webview and Android API
git restore --source=HEAD~1 --staged --worktree -- components/adblock/android/
git commit -n -m "Restore jni_headers"

### Android API removal ###
ANDROID_API_FILES="
components/adblock/android/
chrome/browser/android/adblock/
chrome/android/
"

ANDROID_API_SHARED_FILES="
chrome/browser/BUILD.gn
chrome/browser/ui/BUILD.gn
"

for path in $ANDROID_API_SHARED_FILES
do
  sed -i '/Android API module start/,/Android API module end/d' $path
  git add $path
done

for path in $ANDROID_API_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

git commit -n -m "Negative Android API module"
NEGATIVE_ANDROID_API_MODULE_HASH=$(git rev-parse --short HEAD)

#### Extension API removal ###

EXTENSION_API_FILES="
chrome/browser/extensions/api/
chrome/browser/extensions/extension_function_registration_test.cc
chrome/common/extensions/api/
chrome/common/extensions/permissions/
chrome/test/data/extensions/api_test/
extensions/browser/extension_event_histogram_value.h
extensions/browser/extension_function_histogram_value.h
extensions/common/mojom/api_permission_id.mojom
tools/metrics/histograms/enums.xml
tools/typescript/definitions/adblock_private.d.ts
tools/typescript/definitions/eyeo_filtering_private.d.ts
"

EXTENSION_API_SHARED_FILES="
chrome/browser/extensions/BUILD.gn
chrome/test/BUILD.gn
"

for path in $EXTENSION_API_SHARED_FILES
do
  sed -i '/Extensions API module start/,/Extensions API module end/d' $path
  git add $path
done

for path in $EXTENSION_API_FILES
do
  git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- $path
done

git commit -n -m "Negative Extension API module"
NEGATIVE_EXTENSION_API_MODULE_HASH=$(git rev-parse --short HEAD)

### Chrome removal ###
git restore --source=$TOP_VANILLA_COMMIT --staged --worktree -- chrome

git commit -n -m "Remove Chrome integration"
NEGATIVE_CHROME_INTEGRATION=$(git rev-parse --short HEAD)

### Create Base Module
git reset $TOP_VANILLA_COMMIT --soft
git commit -n -m "eyeo Browser Ad filtering Solution: Base Module" -m "Based on Chromium ${CHROMIUM_TAG}"
BASE_MODULE_HASH=$(git rev-parse --short HEAD)

### Generate modules
repo_dir=$(git rev-parse --show-toplevel)
mkdir -p $repo_dir/eyeo_modules

git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/base.patch

git revert -n $NEGATIVE_CHROME_INTEGRATION
git commit -n -m "eyeo Browser Ad filtering Solution: Chrome Integration Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/chrome_integration.patch

git revert -n $NEGATIVE_WEBVIEW_INTEGRATION
git commit -n -m "eyeo Browser Ad filtering Solution: Android Weview Integration Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/webview_integration.patch

### Remove webview integration commit to aviod conflicts on shared files from Webview Integration and Android API
git reset HEAD~1 --hard

git revert -n $NEGATIVE_ANDROID_CONTENT_SHELL_MODULE_HASH
git commit -n -m "eyeo Browser Ad filtering Solution: Content Shell Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/content_shell.patch

### Remove content shell integration commit to aviod conflicts on shared files from ContentShell Integration and Android API
git reset HEAD~1 --hard

git revert -n $NEGATIVE_ANDROID_API_MODULE_HASH
git commit -n -m "eyeo Browser Ad filtering Solution: Android API Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/android_api.patch

git revert -n $NEGATIVE_ANDROID_UI_MODULE_HASH
git commit -n -m "eyeo Browser Ad filtering Solution: Android Settings UI Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module and Android API Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/android_settings.patch

git revert -n $NEGATIVE_EXTENSION_API_MODULE_HASH
git commit -n -m "eyeo Browser Ad filtering Solution: Extension API Module" -m "Based on Chromium ${CHROMIUM_TAG}" -m "Pre-requisites: eyeo Browser Ad filtering Solution: Base Module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/extension_api.patch

git revert -n $NEGATIVE_CI_TESTING
git commit -n -m "eyeo Browser Ad filtering Solution: CI & testing module"
git format-patch -1 HEAD --stdout > $repo_dir/eyeo_modules/ci_and_testing.patch

### Recover HEAD & branch after script - usefull for local runs
#git checkout $INITIAL_HASH
#git branch -D $INITIAL_BRANCH
#git checkout -b $INITIAL_BRANCH
