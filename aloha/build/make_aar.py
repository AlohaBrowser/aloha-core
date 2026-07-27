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


import re
import subprocess
import xml.etree.ElementTree as ET
from fnmatch import fnmatch
import os
import shutil
import zipfile
import glob
from aloha_log import log
from pathlib import Path
from config import architectures, preferred_architecture_dirname
from paths import build_base_dir, script_dir, aar_full_filename, sources_jar_full_filename, source_dir
from docker_utils import dockered_main

libs_to_skip = [
    "*bromium_webview*",
    "*.interface.javac.jar",
    "android_support_*.javac.jar",
    "jsr_*_javalib.javac.jar",
    "google_play_services*",
    "hamcrest*",
    "junit*",
    "content_shell*",
    "base_java_test_support.javac.jar",
    "shape_detection_java.javac.jar",  #Used in BarcodeScanner, FaceDetector, TextDetector
    "error_prone_ant-*.javac.jar",
    "*proguard*",
    "*retrace603*",
    "retrace.javac.jar",
    "java_deobfuscate.javac.jar",
    "asm-util-*.javac.jar",
    "Desugar-runtime.javac.jar",
    "*guava*",
    "auto_service_processor.javac.jar",
    "com_android_support_animated_vector_drawable_java.javac.jar",
    "android_arch_lifecycle_common.javac.jar",
    "android_arch_lifecycle_runtime_java.javac.jar",
    "com_android_support_support_annotations.javac.jar",
    "com_android_support_support_vector_drawable_java.javac.jar",
    "com_android_support_multidex_java.javac.jar",
    "com_android_support_support_compat_java.javac.jar",
    "com_android_support_support_media_compat_java.javac.jar",
    "com_android_support_support_core_utils_java.javac.jar",
    "com_android_support_support_core_ui_java.javac.jar",
    "com_android_support_appcompat_v7_java.javac.jar",
    "com_android_support_support_fragment_java.javac.jar",
    "android_arch_core_common.javac.jar",
    "android_system.javac.jar",
    "android.javac.jar",
    "com_android_support_support_v4_java.javac.jar",
    "*gms*",   #Used in ShapeDetection
    "*robolectric*",
    "asm.javac.jar",
    "asm-util.javac.jar",
    "asm-tree.javac.jar",
    "android-support-v7-recyclerview.javac.jar",
    "com_android_support_recyclerview_v7_java.javac.jar",
    "jni_processor.javac.jar",
    "*_protobuf_*",
    "*_asm_*",
    "*_checkerframework_*",
    "*androidx*",
    "*_findbugs_*",
    "*_material_*",
    "*_viewpager_*",
    "*listenablefuture*",
    "*annotations-16.0.1*",
    "chromium_commands.dex.javac.jar",
    "*kotlin*",
    "com_squareup_javapoet.javac.jar",
    "com_google_auto_auto_common.javac.jar",
    "com_google_auto_service_auto_service_annotations.javac.jar",
    "javax_annotation_jsr250_api.javac.jar",
    "com_google_ar_core_java.javac.jar",
    "com_google_j2objc_j2objc_annotations.javac.jar",
    "org_jetbrains_annotations.javac.jar",
    "com_google_errorprone_error_prone_annotations.javac.jar",
    "com_google_auto_service_auto_service.javac.jar",
    "buildcompat_java.javac.jar",
    "com_github_ben_manes_caffeine_caffeine.javac.jar",
    "com_github_kevinstn_software_andlgorithm_algorithm.javac.jar",
    "com_github_kevinstern_software_and_algorithms.javac.jar"
    "com_google_auto_value_auto_value_annotations.javac.jar",
    "com_google_errorprone_error_prone_annotation.javac.jar",
    "com_google_errorprone_error_prone_check_api.javac.jar",
    "com_google_errorprone_error_prone_core.javac.jar",
    "com_google_errorprone_error_prone_type_annotations.javac.jar",
    "com_google_errorprone_javac.javac.jar",
    "io_github_java_diff_utils_java_diff_utils.javac.jar",
    "org_eclipse_jgit_org_eclipse_jgit.javac.jar",
    "org_pcollections_pcollections.javac.jar",
    "errorprone_plugin_errorprone_plugin.javac.jar",
    "error_prone_java.javac.jar",
    "error_prone_javac_java.javac.jar",
    "components_about_ui_android_aboutui_java.javac.jar",
    "components_webui_about_android_aboutui_java.javac.jar",
    "*aboutui*",
    "core_overrides_java.javac.jar",
    "*jspecify*",
    "*.javac.jar.info",
    "*.javac.jar.md5*",

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
            if ".javac.jar" in os.path.basename(file) and not skip_library(file):
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
    for dirname in glob.glob(str(int_base_dir / "*strings*")):
        if os.path.isdir(dirname) and "generate" not in dirname:
            shutil.copytree(dirname, out_dir, dirs_exist_ok=True)


def prepare_resources(docker: bool, java_arch: str, int_base_dir: Path, aar_int_base_dir: Path):
    res_dir = aar_int_base_dir / "res"
    prepare_dir(res_dir)

    for filename in glob.glob(str(build_base_dir(docker) / java_arch / "obj/**/*.resources.zip"), recursive=True):
        if not skip_resource(filename):
            process_resource_zip(int_base_dir, Path(filename), res_dir)

    generate_res_index(docker, java_arch, res_dir)


# ALOHA (model A remap index): resource types remapped at runtime.
# Must stay in sync with BromiumResources.java TYPES.
res_index_types = ["string", "id", "dimen", "style", "layout", "color", "attr", "drawable",
                   "menu", "integer"]


def _res_field_key(name: str) -> str:
    # aapt2 resource name -> R field name (dots/dashes become underscores).
    return re.sub(r"[^0-9a-zA-Z_]", "_", name)


def _parse_r_java_fields(r_java_path: Path) -> dict:
    """Parse gen/base_module/R.java -> {type: [field names in file order]}."""
    class_re = re.compile(r"public static class (\w+)")
    field_re = re.compile(r"public static int (\w+) =")
    fields = {}
    cur = None
    with open(r_java_path) as f:
        for line in f:
            m = class_re.search(line)
            if m:
                cur = m.group(1)
                fields.setdefault(cur, [])
                continue
            m = field_re.search(line)
            if m and cur is not None:
                fields[cur].append(m.group(1))
    return fields


def _scan_values_file(path: Path, shipped: dict):
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as e:
        log(f"res index: skipping unparsable {path}: {e}")
        return
    for el in root:
        name = el.get("name")
        if name is None:
            continue
        tag = el.get("type") if el.tag == "item" else el.tag
        if el.tag == "declare-styleable":
            # <attr> children (without an android: prefix) define/reference attrs of our package.
            for attr in el:
                a = attr.get("name")
                if attr.tag == "attr" and a and ":" not in a:
                    shipped["attr"].add(a)
            continue
        if tag in shipped and ":" not in name:
            shipped[tag].add(name)


def _scan_shipped_res(res_dir: Path) -> dict:
    """Collect {type: {real resource name}} actually defined by the res/ we ship."""
    shipped = {t: set() for t in res_index_types}
    id_re = re.compile(r"@\+id/([0-9a-zA-Z_.]+)")
    for entry in sorted(os.listdir(res_dir)):
        dir_path = res_dir / entry
        if not dir_path.is_dir():
            continue
        base_type = entry.split("-", 1)[0]
        for root, _, files in os.walk(dir_path):
            for fname in files:
                fpath = Path(root) / fname
                if base_type == "values":
                    if fname.endswith(".xml"):
                        _scan_values_file(fpath, shipped)
                    continue
                if base_type in shipped:
                    name = fname[:-6] if fname.endswith(".9.png") else os.path.splitext(fname)[0]
                    shipped[base_type].add(name)
                if fname.endswith(".xml"):
                    # @+id/foo anywhere (layout, menu, drawable...) defines an id resource.
                    with open(fpath, encoding="utf-8", errors="replace") as f:
                        for m in id_re.finditer(f.read()):
                            shipped["id"].add(m.group(1))
    return shipped


_r_class_re = re.compile(rb"(?:^|/)R\$(\w+)$")


def _class_r_field_refs(data: bytes, wanted_types: set) -> set:
    """Fieldrefs to any R$<type> class found in one .class file's constant pool."""
    if data[:4] != b"\xca\xfe\xba\xbe":
        return set()
    n = int.from_bytes(data[8:10], "big")
    pos = 10
    utf8, classes, nats = {}, {}, {}
    fieldrefs = []
    i = 1
    while i < n:
        tag = data[pos]
        if tag == 1:                     # Utf8
            ln = int.from_bytes(data[pos + 1:pos + 3], "big")
            utf8[i] = data[pos + 3:pos + 3 + ln]
            pos += 3 + ln
        elif tag == 7:                   # Class
            classes[i] = int.from_bytes(data[pos + 1:pos + 3], "big")
            pos += 3
        elif tag in (8, 16, 19, 20):     # String / MethodType / Module / Package
            pos += 3
        elif tag in (3, 4):              # Integer / Float
            pos += 5
        elif tag in (5, 6):              # Long / Double (take two cp slots)
            pos += 9
            i += 1
        elif tag in (9, 10, 11, 12, 17, 18):  # {Field,Method,IfaceMethod}ref / NaT / [Invoke]Dynamic
            a = int.from_bytes(data[pos + 1:pos + 3], "big")
            b = int.from_bytes(data[pos + 3:pos + 5], "big")
            if tag == 9:
                fieldrefs.append((a, b))
            elif tag == 12:
                nats[i] = a
            pos += 5
        elif tag == 15:                  # MethodHandle
            pos += 4
        else:
            return set()                 # unknown tag: malformed/newer format, bail out
        i += 1
    refs = set()
    for class_idx, nat_idx in fieldrefs:
        cname = utf8.get(classes.get(class_idx, -1))
        if cname is None:
            continue
        m = _r_class_re.search(cname)
        if not m:
            continue
        t = m.group(1).decode()
        if t in wanted_types:
            fname = utf8.get(nats.get(nat_idx, -1))
            if fname:
                refs.add((t, fname.decode()))
    return refs


def _scan_jar_r_field_refs(jars_dir: Path) -> set:
    """(type, field) pairs the shipped jars actually read from R classes — the ground truth of
    which R fields can ever be observed at runtime."""
    wanted = set(res_index_types)
    refs = set()
    for jar in sorted(glob.glob(str(jars_dir / "*.jar"))):
        with zipfile.ZipFile(jar) as zf:
            for info in zf.infolist():
                name = info.filename
                if not name.endswith(".class"):
                    continue
                base = name.rsplit("/", 1)[-1]
                # R classes themselves self-reference every own field in <clinit>; skip them.
                if base == "R.class" or base.startswith("R$"):
                    continue
                refs.update(_class_r_field_refs(zf.read(name), wanted))
    return refs


def _validate_res_index(docker: bool, java_arch: str, emitted: list):
    """Warn about emitted names absent from the chromium-built resource table (scan bug guard)."""
    ap_ = build_base_dir(docker) / java_arch / "obj/android_webview/bromium_webview_apk.ap_"
    aapt2 = source_dir(docker) / "third_party/android_build_tools/aapt2/cipd/aapt2"
    if not ap_.is_file() or not aapt2.is_file():
        log(f"res index: validation skipped ({ap_ if not ap_.is_file() else aapt2} not found)")
        return
    dump = subprocess.run([str(aapt2), "dump", "resources", str(ap_)],
                          capture_output=True, text=True)
    apk_names = set(re.findall(r"resource 0x[0-9a-f]{8} (\S+/\S+)", dump.stdout))
    bad = [f"{t}/{real}" for (t, real) in emitted if f"{t}/{real}" not in apk_names]
    if bad:
        log(f"res index: WARNING: {len(bad)} entries not in bromium_webview_apk.ap_: {bad[:20]}")


# ALOHA (model A remap index): generate res/xml/bromium_res_index.xml mapping chromium R field
# keys to @-references to the same resources. The HOST aapt2 resolves each reference into the
# final merged 0x7f id at link time (the same battle-tested mechanism that fixes up every
# @drawable in every layout), so at runtime BromiumResources just bulk-reads the ready ids
# instead of ~2400 getIdentifier lookups.
#
# Why a compiled XML file and not a <array> resource: TypedArray follows alias chains when
# reading (<dimen name="a">@dimen/b</dimen> in an array item yields b's id — or even a
# framework id — instead of a's), while binary-XML attributes are read RAW via
# getAttributeResourceValue, giving the id of the referenced resource itself.
#
# A reference to a name we don't actually ship would fail the HOST build, so the index only
# covers R fields backed by shipped resources. Of the rest, fields that shipped code provably
# reads (constant-pool scan of the collected jars) go into <f> fallback entries and are
# resolved at runtime via getIdentifier — they may exist in the host by name (its own androidx
# copies), which is exactly how model A resolved them before. Unreferenced fields stay 0.
def generate_res_index(docker: bool, java_arch: str, res_dir: Path):
    r_java = (build_base_dir(docker) / java_arch
              / "gen/android_webview/bromium_webview_apk/generated_java/input_srcjars"
              / "gen/base_module/R.java")
    fields = _parse_r_java_fields(r_java)
    shipped = _scan_shipped_res(res_dir)

    jars_dir = res_dir.parent / "libs"
    refs = _scan_jar_r_field_refs(jars_dir)
    if not refs:
        # Degraded mode: without the reference scan every unshipped field must be checked at
        # runtime (slow but exactly the old model-A behavior).
        log("res index: WARNING: no R field refs found in jars; emitting full fallback")

    # (type, R field key) -> real resource name; prefer the exact-match name on key collision.
    real_by_key = {}
    for t in res_index_types:
        for real in sorted(shipped[t]):
            key = (t, _res_field_key(real))
            if key not in real_by_key or real == key[1]:
                real_by_key[key] = real

    names_items, ref_items, fallback_items, emitted = [], [], [], []
    dropped = 0
    for t in res_index_types:
        for field in fields.get(t, []):
            real = real_by_key.get((t, field))
            if real is not None:
                names_items.append(f"{t}/{field}")
                # An attr can't be a plain @-reference; ?attr/ compiles to TYPE_ATTRIBUTE whose
                # data IS the attr resource id (read via peekValue in BromiumResources).
                ref_items.append(f"?attr/{real}" if t == "attr" else f"@{t}/{real}")
                emitted.append((t, real))
            elif not refs or (t, field) in refs:
                fallback_items.append(f"{t}/{field}")
            else:
                dropped += 1

    out = res_dir / "xml" / "bromium_res_index.xml"
    os.makedirs(out.parent, exist_ok=True)
    with open(out, "w") as f:
        f.write('<?xml version="1.0" encoding="utf-8"?>\n')
        f.write('<!-- Generated by make_aar.py (generate_res_index): chromium R -> merged id\n'
                '     index. Host aapt2 resolves the references at link time; BromiumResources\n'
                '     bulk-reads them at startup instead of per-name getIdentifier. -->\n')
        f.write('<index>\n')
        for key, ref in zip(names_items, ref_items):
            f.write(f'    <i n="{key}" r="{ref}"/>\n')
        for key in fallback_items:
            f.write(f'    <f n="{key}"/>\n')
        f.write('</index>\n')

    log(f"Resource index: {len(names_items)} entries, {len(fallback_items)} runtime fallbacks "
        f"(read by code but not shipped), {dropped} unread R fields stay 0")
    _validate_res_index(docker, java_arch, emitted)


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


def build_sources_jar(docker: bool, output_path: Path):
    src_root = source_dir(docker)
    # Public Bromium API + WebView glue layer
    source_dirs = [
        src_root / "aloha/src/java",
        src_root / "android_webview/java/src",
        src_root / "android_webview/glue/java/src",
    ]
    jdk_dir = src_root / "third_party/jdk/current/bin"
    cmd = [str(jdk_dir / "jar"), "cf", str(output_path)]
    for src_dir in source_dirs:
        if src_dir.exists():
            cmd += ["-C", str(src_dir), "."]
    subprocess.check_call(cmd)


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
    lib_java_dir = build_base_dir(args.docker) / f"{java_arch}/obj"
    log("Collecting JARs")
    collect_jars(lib_java_dir, jars_dir)

    # Unpack APKs and collect artefacts
    log("Processing APKs")
    process_apks(args.docker, java_arch, int_base_dir, aar_int_base_dir)

    # ALOHA (model A): ship raw res/ so the host aapt2 merges chromium resources into the app's own
    # 0x7f table. That makes them present in EVERY context by construction (stable across vendors,
    # no per-context AssetManager patching, no dynamic package id). BromiumResources remaps the
    # precompiled R baked IDs to the merged IDs at startup, bulk-reading them from the generated
    # bromium_res_index arrays (see generate_res_index; getIdentifier remains the fallback).
    # See aloha/history/bugfix/resources_remap_investigation.md.
    log("Packaging raw resources")
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

    # Make sources JAR
    log("Generating sources JAR")
    build_sources_jar(args.docker, sources_jar_full_filename(args.docker))

    return 0


if __name__ == "__main__":
    exit(dockered_main(__file__, None, main))
