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


from pathlib import Path

_host_script_dir = Path(__file__).parent
_host_source_dir = _host_script_dir.parent.parent

docker_home_dir = Path("/home/user")

tests_json_filename = "tests-for-bromium-dependencies.json"

# Warning: When file import in docker, right works only functions with parameter 'docker=True'.


def source_dir(docker: bool) -> Path:
    return docker_home_dir / "bromium/src" if docker else _host_source_dir


def script_dir(docker: bool) -> Path:
    if docker:
        return source_dir(docker) / _host_script_dir.relative_to(_host_source_dir)
    else:
        return _host_script_dir


def build_base_dir(docker: bool) -> Path:
    return source_dir(docker) / "out/bromium/build"


def aar_full_filename(docker: bool) -> Path:
    return build_base_dir(docker).parent / "aar" / "aloha-bromium-localbuild.aar"


def aloha_js_filename(docker: bool) -> Path:
    return build_base_dir(docker).parent / "aloha.js"
