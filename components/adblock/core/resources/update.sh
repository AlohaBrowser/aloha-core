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

# This script downloads filter lists that get bundled with the browser binary.
# Bundled filter lists allow out-of-the-box ad-filtering and serve as backup in
# case the browser needs to navigate to a website without having downloaded
# the desired filter list from the Internet yet.
#
# The browser will replace these bundled filter lists by ones downloaded from
# the Internet as soon as possible.

# We use minified lists to reduce resource bundle size and speed up startup.
# We don't care about perfect ad-filtering quality as we expect these to be
# replaced very soon.
wget https://easylist-downloads.adblockplus.org/easylist-minified.txt -O easylist.txt
gzip -f easylist.txt

wget https://easylist-downloads.adblockplus.org/exceptionrules-minimal.txt -O exceptionrules.txt
gzip -f exceptionrules.txt

wget https://easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt -O anticv.txt
gzip -f anticv.txt
