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


import subprocess
from fnmatch import fnmatch
import os
import shutil
import zipfile
import glob
from aloha_log import log
from pathlib import Path
from config import architectures, preferred_architecture_dirname
from paths import build_base_dir, script_dir, aar_full_filename, source_dir
from docker_utils import dockered_main

libs_to_skip = [
    "*.interface.jar",
    "android_support_*.jar",
    "jsr_*_javalib.jar",
    "google_play_services*",
    "hamcrest*",
    "junit*",
    "content_shell*",
    "base_java_test_support.jar",
    "shape_detection_java.jar",
    "error_prone_ant-*.jar",
    "*proguard*",
    "*retrace603*",
    "retrace.jar",
    "java_deobfuscate.jar",
    "asm-util-*.jar",
    "Desugar-runtime.jar",
    "*guava*",
    "auto_service_processor.jar",
    "com_android_support_animated_vector_drawable_java.jar",
    "android_arch_lifecycle_common.jar",
    "android_arch_lifecycle_runtime_java.jar",
    "com_android_support_support_annotations.jar",
    "com_android_support_support_vector_drawable_java.jar",
    "com_android_support_multidex_java.jar",
    "com_android_support_support_compat_java.jar",
    "com_android_support_support_media_compat_java.jar",
    "com_android_support_support_core_utils_java.jar",
    "com_android_support_support_core_ui_java.jar",
    "com_android_support_appcompat_v7_java.jar",
    "com_android_support_support_fragment_java.jar",
    "android_arch_core_common.jar",
    "android_system.jar",
    "android.jar",
    "com_android_support_support_v4_java.jar",
    "*gms*",
    "*robolectric*",
    "asm.jar",
    "asm-util.jar",
    "asm-tree.jar",
    "android-support-v7-recyclerview.jar",
    "com_android_support_recyclerview_v7_java.jar",
    "jni_processor.jar",
    "*_protobuf_*",
    "*_asm_*",
    "*_checkerframework_*",
    "*androidx*",
    "*_findbugs_*",
    "*_material_*",
    "*_viewpager_*",
    "*listenablefuture*",
    "*annotations-16.0.1*",
    "chromium_commands.dex.jar",
    "*kotlin*",
    "com_squareup_javapoet.jar",
    "com_google_auto_auto_common.jar",
    "com_google_auto_service_auto_service_annotations.jar",
    "javax_annotation_jsr250_api.jar",
    "com_google_ar_core_java.jar",
    "com_google_j2objc_j2objc_annotations.jar",
    "org_jetbrains_annotations.jar",
    "com_google_errorprone_error_prone_annotations.jar",
    "com_google_auto_service_auto_service.jar",
    "buildcompat_java.jar",
    "com_github_ben_manes_caffeine_caffeine.jar",
    "com_github_kevinstn_software_andlgorithm_algorithm.jar",
    "com_github_kevinstern_software_and_algorithms.jar"
    "com_google_auto_value_auto_value_annotations.jar",
    "com_google_errorprone_error_prone_annotation.jar",
    "com_google_errorprone_error_prone_check_api.jar",
    "com_google_errorprone_error_prone_core.jar",
    "com_google_errorprone_error_prone_type_annotations.jar",
    "com_google_errorprone_javac.jar",
    "io_github_java_diff_utils_java_diff_utils.jar",
    "org_eclipse_jgit_org_eclipse_jgit.jar",
    "org_pcollections_pcollections.jar",
    "errorprone_plugin_errorprone_plugin.jar",
    "error_prone_java.jar",
    "error_prone_javac_java.jar",
    "components_about_ui_android_aboutui_java.jar",
    "components_webui_about_android_aboutui_java.jar",
    "*aboutui*",
    "core_overrides_java.jar",
]

resources_for_skip = [
    "*androidx*",
]


def _skip_if_match(filename: str, skip_list: list):
    for l in skip_list:
        if fnmatch(filename, l):
            return True
    return False


def skip_library(filename: str):
    return _skip_if_match(filename, libs_to_skip)


def skip_resource(filename: str):
    return _skip_if_match(filename, resources_for_skip)


def prepare_dir(path):
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path, exist_ok=True)


def collect_jars(input_dir: Path, output_dir: Path):
    for root, dirs, files in os.walk(input_dir):
        updated_root = root.replace(f"{input_dir}/", "")
        updated_root = updated_root.replace("/", "_")
        for file in files:
            if os.path.splitext(file)[1] == ".jar" and not skip_library(file):
                lib_name = f"{updated_root}_{file}"
                shutil.copyfile(os.path.join(root, file), output_dir / lib_name)


def _find_apks(docker: bool) -> dict:
    arch_to_apk = dict()
    for arch in architectures.values():
        apk_filename = build_base_dir(docker) / arch / "apks/BromiumWebView.apk"
        if os.path.isfile(apk_filename):
            arch_to_apk[arch] = apk_filename
    return arch_to_apk


def find_java_arch(docker: bool):
    found_archs = _find_apks(docker).keys()
    if len(found_archs) == 0:
        raise RuntimeError("Apks not found")
    if preferred_architecture_dirname in found_archs:
        return preferred_architecture_dirname
    return list(found_archs)[0]


def process_apks(docker: bool, java_arch: str, int_base_dir: Path, output_dir: Path):
    assets_path = output_dir / "assets"
    prepare_dir(assets_path)
    for arch, apk_filename in _find_apks(docker).items():
        with zipfile.ZipFile(apk_filename, 'r') as zip_ref:
            unpacked_path = int_base_dir / f"unpacked_syswebview_{arch}"
            prepare_dir(unpacked_path)
            zip_ref.extractall(unpacked_path)
            # Copy native libs (extracted/lib/arch) -> (intdir/aar/jni/arch)
            native_path = output_dir / f"jni/{arch}"
            prepare_dir(native_path)
            for so in glob.glob(os.path.join(unpacked_path, f"lib/{arch}/*.so")):
                shutil.copy(so, native_path)
            # Copy V8 blobs (extracted/assets) -> (intdir/aar/assets)
            for bin in glob.glob(os.path.join(unpacked_path, f"assets/*.bin")):
                new_file_name = f"{os.path.splitext(os.path.basename(bin))[0]}-{arch}.bin"
                shutil.copyfile(bin, assets_path / new_file_name)

    default_apk_path = int_base_dir / f"unpacked_syswebview_{java_arch}"
    shutil.copy(default_apk_path / "assets/icudtl.dat", assets_path)
    shutil.copy(default_apk_path / "assets/resources.pak", assets_path)
    shutil.copy(default_apk_path / "assets/chrome_100_percent.pak", assets_path)
    shutil.copytree(default_apk_path / "assets/stored-locales", assets_path / "stored-locales", dirs_exist_ok=True)
    return default_apk_path


def process_resource_zip(int_base_dir: Path, zip_name: Path, out_dir: Path):
    tmp_zip_folder = int_base_dir / f"{zip_name.name}_unpacked"
    suffix = zip_name.stem
    prepare_dir(tmp_zip_folder)
    with zipfile.ZipFile(zip_name, 'r') as zip_ref:
        zip_ref.extractall(tmp_zip_folder)
    for root, dirs, files in os.walk(tmp_zip_folder):
        # We can't rename files in folder other than "values" because
        # filename is resource ID
        if "values" in root:
            for file in files:
                if os.path.splitext(file)[1] == ".xml":
                    new_filename = f"{os.path.splitext(file)[0]}_{suffix}.xml"
                    os.rename(os.path.join(root, file), os.path.join(root, new_filename))
    for dirname in glob.glob(str(tmp_zip_folder / "*_res*")):
        if os.path.isdir(dirname):
            shutil.copytree(dirname, out_dir, dirs_exist_ok=True)


def prepare_resources(docker: bool, java_arch: str, int_base_dir: Path, aar_int_base_dir: Path):
    res_dir = aar_int_base_dir / "res"
    prepare_dir(res_dir)

    gen_dir = build_base_dir(docker) / f"{java_arch}/gen"
    shutil.copytree(gen_dir / "components/strings/java/res", res_dir, dirs_exist_ok=True)
    for dirname in glob.glob(str(gen_dir / "**/*strings_grd_grit_output"), recursive=True):
        if not skip_resource(dirname):
            shutil.copytree(dirname, res_dir, dirs_exist_ok=True)

    for filename in glob.glob(str(build_base_dir(docker) / java_arch / "obj/**/*.resources.zip"), recursive=True):
        if not skip_resource(filename):
            process_resource_zip(int_base_dir, Path(filename), res_dir)


def add_from_input_srcjars(docker: bool, root_path, class_path, filename, out: Path):
    full_out_path = out / class_path
    os.makedirs(full_out_path, exist_ok=True)
    src_path = build_base_dir(docker) / root_path / class_path
    shutil.copy(src_path / filename, full_out_path)


def compile_extra_javas(docker: bool, java_arch: str, int_base_dir: Path, jars_dir: Path):
    input_srcjars_path = f"{java_arch}/gen/android_webview/bromium_webview_apk/generated_java/input_srcjars"

    extra_java_dir = int_base_dir / "java"
    prepare_dir(extra_java_dir)

    build_cfg_dir = extra_java_dir / "src"
    prepare_dir(build_cfg_dir)

    # Collect all java files
    add_from_input_srcjars(docker, input_srcjars_path, "gen/base_module", "R.java", build_cfg_dir)

    for root, dirs, files in os.walk(build_base_dir(docker) / input_srcjars_path / "org"):
        class_path = root[root.find("org"):]
        for file in files:
            add_from_input_srcjars(docker, input_srcjars_path, class_path, file, build_cfg_dir)

    # White all copied file names into "sources.txt"
    sources_file_name = extra_java_dir / "sources.txt"

    with open(sources_file_name, "w") as sources_file:
        for root, dirs, files in os.walk(extra_java_dir / "src"):
            for file in files:
                print(os.path.join(root, file), file=sources_file)

    # Compile files and pack jar
    extra_java_out_dir = extra_java_dir / "out"
    prepare_dir(extra_java_out_dir)

    jdk_dir = source_dir(docker) / "third_party/jdk/current/bin"
    subprocess.check_call([jdk_dir / "javac", "-source", "21", "-target", "21", f"@{sources_file_name}", "-d", extra_java_out_dir])
    subprocess.check_call([jdk_dir / "jar", "cf", jars_dir / "aloha-support.jar", "-C", extra_java_out_dir, "."])


def main(args):
    log("Preparing")
    int_base_dir = build_base_dir(args.docker).parent / "intermediates"
    prepare_dir(int_base_dir)

    aar_int_base_dir = int_base_dir / "aar"
    prepare_dir(aar_int_base_dir)

    jars_dir = aar_int_base_dir / "libs"
    prepare_dir(jars_dir)

    java_arch = find_java_arch(args.docker)

    # Copy all generated JAR files (except blacklisted) to intermediates/jar (aka $lib_java_dir)
    lib_java_dir = build_base_dir(args.docker) / f"{java_arch}/lib.java"
    log("Collecting JARs")
    collect_jars(lib_java_dir, jars_dir)

    # Unpack APKs and collect artefacts
    log("Processing APKs")
    process_apks(args.docker, java_arch, int_base_dir, aar_int_base_dir)

    # Processing resources
    log("Processing resources")
    prepare_resources(args.docker, java_arch, int_base_dir, aar_int_base_dir)

    log("Adding extras")

    # Copy manifest
    shutil.copy(script_dir(args.docker).parent / "src/AndroidManifest.xml", aar_int_base_dir)

    # Copy and compile additional java's
    log("Compiling additional java files")
    compile_extra_javas(args.docker, java_arch, int_base_dir, jars_dir)

    # Make AAR
    log("Generating AAR")
    aar_full_fn = aar_full_filename(args.docker)
    prepare_dir(aar_full_fn.parent)
    shutil.make_archive(aar_full_fn, "zip", aar_int_base_dir)
    os.rename(f"{aar_full_fn}.zip", aar_full_fn)

    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, None, main))
