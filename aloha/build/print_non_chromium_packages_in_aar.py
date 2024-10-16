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


import os
import io
import zipfile
from aloha_log import log
from paths import aar_full_filename


def get_non_chromium_package_files_from_jar(files_in_jar: list):
    ignore_starts = ["META-INF/", "org/chromium/", "com/android/webview/chromium", "com/alohamobile/"]
    return [filename
            for filename in files_in_jar
            if all(not filename.startswith(starts) for starts in ignore_starts)]


def _main():
    jars_with_non_chromium_packages = []
    with zipfile.ZipFile(aar_full_filename(docker=False), 'r') as aar_file:
        jar_filenames = [fn
                         for fn in aar_file.namelist()
                         if fn.endswith(".jar")]
        for jar_filename in jar_filenames:
            jar_file_data = aar_file.read(jar_filename)
            with zipfile.ZipFile(io.BytesIO(jar_file_data)) as jar_file:
                package_files = get_non_chromium_package_files_from_jar(jar_file.namelist())
                if len(package_files) > 0:
                    jar_basename = os.path.basename(jar_filename)
                    jars_with_non_chromium_packages.append(jar_basename)
                    log("Jar '%s' contains non chromium files: %s"
                        % (jar_basename, package_files))

    log("Jars with non chromium packages: %s" % jars_with_non_chromium_packages)
    return 0


if __name__ == '__main__':
    exit(_main())
