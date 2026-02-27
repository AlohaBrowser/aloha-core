# Copyright 2026 Aloha Mobile Ltd.

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

"""Measures the size of a built Bromium AAR and compares two Bromium versions.

A measurement is a small text table (snapshot): AAR entries, ELF sections of
libwebviewchromium.so and its size attributed to source directories (via
//tools/binary_size/supersize). Snapshots are saved per version, so a later
comparison does not need the old build tree any more.

Usage:
  # Measure this checkout and the one we are upgrading from, print the diff:
  python3 aloha/build/size_report.py --old /path/to/old/bromium/src

  # Compare against the latest saved snapshot of an older version:
  python3 aloha/build/size_report.py

  # Only measure this checkout:
  python3 aloha/build/size_report.py --snapshot-only

Both trees must have a finished release build (build_webview.py + make_aar.py).
"""

import argparse
import collections
import multiprocessing
import subprocess
import sys
import tempfile
import zipfile
import zlib
from pathlib import Path

_THIS_SRC = Path(__file__).resolve().parent.parent.parent
_AAR_REL = Path("out/bromium/aar/aloha-bromium-localbuild.aar")
_BUILD_REL = Path("out/bromium/build")
_SNAPSHOT_DIR = _THIS_SRC / "aloha/history/size"
_LIB_NAME = "libwebviewchromium.so"

# Source directories are stored up to this depth.
_STORE_DEPTH = 4
# Directories smaller than this are folded into their parent.
_FOLD_BYTES = 4096

_NO_PATH = "{no path}"


def _log(message):
    print(message, file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------
# Snapshot model: rows of (kind, scope, name) -> (size, size2).
#   meta    -       <key>            value is stored in |name| as "key=value"
#   aar     -       <zip entry>      size = compressed, size2 = uncompressed
#   section <abi>   <ELF section>    size
#   src     <abi>   <source dir>     size (pss, bytes)
# ---------------------------------------------------------------------------


class Snapshot:
    def __init__(self):
        self.meta = {}
        self.aar = {}  # name -> (compressed, uncompressed)
        self.sections = collections.defaultdict(dict)  # abi -> {name: size}
        self.src = collections.defaultdict(dict)  # abi -> {dir: size}

    @property
    def version(self):
        return self.meta.get("version", "?")

    def save(self, path):
        lines = ["# Bromium size snapshot, see aloha/build/size_report.py"]
        for key, value in self.meta.items():
            lines.append(f"meta\t-\t{key}={value}")
        for name, (comp, raw) in sorted(self.aar.items()):
            lines.append(f"aar\t-\t{name}\t{comp}\t{raw}")
        for abi in sorted(self.sections):
            for name, size in self.sections[abi].items():
                lines.append(f"section\t{abi}\t{name}\t{size}")
        for abi in sorted(self.src):
            for name, size in sorted(self.src[abi].items()):
                lines.append(f"src\t{abi}\t{name}\t{size}")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(lines) + "\n")

    @staticmethod
    def load(path):
        snap = Snapshot()
        for line in path.read_text().splitlines():
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            kind, scope, name = parts[0], parts[1], parts[2]
            if kind == "meta":
                key, _, value = name.partition("=")
                snap.meta[key] = value
            elif kind == "aar":
                snap.aar[name] = (int(parts[3]), int(parts[4]))
            elif kind == "section":
                snap.sections[scope][name] = int(parts[3])
            elif kind == "src":
                snap.src[scope][name] = int(parts[3])
        return snap


# ---------------------------------------------------------------------------
# Measuring
# ---------------------------------------------------------------------------


def _read_version(src_dir):
    values = {}
    for line in (src_dir / "chrome/VERSION").read_text().splitlines():
        key, _, value = line.partition("=")
        values[key.strip()] = value.strip()
    return "{MAJOR}.{MINOR}.{BUILD}.{PATCH}".format(**values)


def _git_head(src_dir):
    try:
        return subprocess.check_output(
            ["git", "-C", str(src_dir), "rev-parse", "--short=12", "HEAD"],
            text=True, stderr=subprocess.DEVNULL).strip()
    except (subprocess.CalledProcessError, OSError):
        return "unknown"


def _aar_entry_name(name):
    """Keeps big entries as is, collapses the thousands of res/ files."""
    top = name.split("/", 1)[0]
    if top == "res":
        # res/<type-qualifier>/<file> -> res/<type>, e.g. res/drawable.
        parts = name.split("/")
        if len(parts) > 2:
            return "res/" + parts[1].split("-", 1)[0]
    return name


def _measure_aar(aar_path, snap):
    """Returns {abi: (uncompressed size, crc32)} of the bundled native libs."""
    libs = {}
    with zipfile.ZipFile(aar_path) as aar:
        for info in aar.infolist():
            if info.is_dir():
                continue
            name = _aar_entry_name(info.filename)
            comp, raw = snap.aar.get(name, (0, 0))
            snap.aar[name] = (comp + info.compress_size, raw + info.file_size)
            parts = info.filename.split("/")
            if parts[0] == "jni" and parts[-1] == _LIB_NAME:
                libs[parts[1]] = (info.file_size, info.CRC)
    snap.meta["aar_file_bytes"] = str(aar_path.stat().st_size)
    snap.meta["abis"] = ",".join(sorted(libs))
    return libs


def _file_crc32(path):
    crc = 0
    with open(path, "rb") as f:
        while chunk := f.read(1 << 24):
            crc = zlib.crc32(chunk, crc)
    return crc


def _find_unstripped(build_dir, size, crc):
    """Finds the build output that was packed into the AAR.

    The out directory holds several libwebviewchromium.so (per toolchain, plus
    stale ones from older builds), so the library is matched by size and CRC
    against the AAR entry, not by a hardcoded path.
    Returns (unstripped ELF, GN output directory) or None.
    """
    for stripped in sorted(build_dir.glob(f"*/**/{_LIB_NAME}")):
        if stripped.parent.name == "lib.unstripped":
            continue
        unstripped = stripped.parent / "lib.unstripped" / _LIB_NAME
        map_file = Path(str(unstripped) + ".map.gz")
        if not unstripped.is_file() or not map_file.is_file():
            continue
        if stripped.stat().st_size != size or _file_crc32(stripped) != crc:
            continue
        out_dir = stripped.parent
        while out_dir != build_dir and not (out_dir / "args.gn").is_file():
            out_dir = out_dir.parent
        if not (out_dir / "args.gn").is_file():
            continue
        return unstripped, out_dir
    return None


def _import_supersize():
    """Imports libsupersize of *this* checkout with two local workarounds."""
    lib_dir = _THIS_SRC / "tools/binary_size/libsupersize"
    if str(lib_dir) not in sys.path:
        sys.path.insert(0, str(lib_dir))
    import file_format  # pylint: disable=import-error
    import main as supersize_main  # pylint: disable=import-error
    import native  # pylint: disable=import-error
    # The linker map parser does not pick up .comment, which makes the
    # ELF-vs-map section check fail. The section is ~600 bytes of metadata.
    if ".comment" not in native._SECTION_SIZE_BLOCKLIST:
        native._SECTION_SIZE_BLOCKLIST.append(".comment")
    return supersize_main, file_format


def _source_dir_of(symbol):
    path = symbol.source_path or symbol.object_path
    if not path:
        return _NO_PATH
    parts = [p for p in path.split("/") if p not in ("..", ".", "")]
    # Drop the file name, keep at most _STORE_DEPTH directories.
    parts = parts[:-1][:_STORE_DEPTH]
    return "/".join(parts) if parts else _NO_PATH


def _fold_small_dirs(sizes):
    """Folds directories below _FOLD_BYTES into their parents."""
    sizes = dict(sizes)
    for depth in range(_STORE_DEPTH, 1, -1):
        for name in [n for n in sizes if n.count("/") == depth - 1]:
            if abs(sizes[name]) < _FOLD_BYTES:
                parent = name.rsplit("/", 1)[0]
                sizes[parent] = sizes.get(parent, 0) + sizes.pop(name)
    return sizes


def _measure_native(abi, unstripped, out_dir, snap):
    supersize_main, file_format = _import_supersize()
    with tempfile.TemporaryDirectory(prefix="bromium_size_") as tmp:
        size_file = Path(tmp) / f"{abi}.size"
        _log(f"  supersize archive ({abi}): {unstripped}")
        saved_argv = sys.argv
        sys.argv = ["supersize", "archive", str(size_file),
                    "--elf-file", str(unstripped),
                    "--output-directory", str(out_dir)]
        try:
            supersize_main.main()
        finally:
            sys.argv = saved_argv
        size_info = file_format.LoadSizeInfo(str(size_file))

    sections = collections.Counter()
    for container in size_info.containers:
        sections.update(container.section_sizes)
    snap.sections[abi] = dict(
        sorted(sections.items(), key=lambda kv: -kv[1]))

    dirs = collections.Counter()
    for symbol in size_info.raw_symbols:
        dirs[_source_dir_of(symbol)] += symbol.pss
    snap.src[abi] = {
        name: int(round(size))
        for name, size in _fold_small_dirs(dirs).items() if round(size)
    }


def measure(src_dir):
    src_dir = src_dir.resolve()
    aar_path = src_dir / _AAR_REL
    if not aar_path.is_file():
        sys.exit(f"error: no AAR at {aar_path}; run build_webview.py and "
                 "make_aar.py there first")
    snap = Snapshot()
    snap.meta["version"] = _read_version(src_dir)
    snap.meta["commit"] = _git_head(src_dir)
    _log(f"Measuring {snap.version} at {src_dir}")
    libs = _measure_aar(aar_path, snap)
    for abi, (size, crc) in sorted(libs.items()):
        found = _find_unstripped(src_dir / _BUILD_REL, size, crc)
        if not found:
            _log(f"  warning: {abi}: no build output matches the library in "
                 "the AAR (unstripped ELF + .map.gz); native breakdown "
                 "skipped. Rebuild the AAR from the current build.")
            continue
        _measure_native(abi, *found, snap)
    return snap


# ---------------------------------------------------------------------------
# Comparing
# ---------------------------------------------------------------------------


def _fmt_bytes(value, signed=False):
    sign = ""
    if signed:
        sign = "+" if value > 0 else ("-" if value < 0 else " ")
    value = abs(value)
    if value >= 1 << 20:
        return f"{sign}{value / (1 << 20):.2f} MB"
    if value >= 1 << 10:
        return f"{sign}{value / (1 << 10):.1f} KB"
    return f"{sign}{value} B"


def _fmt_pct(old, new):
    if not old:
        return "new" if new else ""
    return f"{(new - old) * 100.0 / old:+.1f}%"


def _print_table(title, rows, top=None, min_delta=1):
    """rows: iterable of (name, old, new)."""
    rows = [r for r in rows if abs(r[2] - r[1]) >= min_delta]
    rows.sort(key=lambda r: -abs(r[2] - r[1]))
    hidden = 0
    if top is not None and len(rows) > top:
        hidden = len(rows) - top
        rows = rows[:top]
    print(f"\n{title}")
    if not rows:
        print("  (no changes)")
        return
    width = min(max(len(r[0]) for r in rows), 70)
    for name, old, new in rows:
        print(f"  {name:<{width}}  {_fmt_bytes(old):>10} -> "
              f"{_fmt_bytes(new):>10}  {_fmt_bytes(new - old, True):>11}  "
              f"{_fmt_pct(old, new):>7}")
    if hidden:
        print(f"  ... {hidden} more rows with smaller deltas")


def _joined(old, new):
    for name in sorted(set(old) | set(new)):
        yield name, old.get(name, 0), new.get(name, 0)


def _aar_group(name):
    parts = name.split("/")
    if parts[0] == "jni":
        return "/".join(parts[:2])
    return parts[0] if len(parts) > 1 else "(root files)"


def _aggregate(sizes, depth):
    result = collections.Counter()
    for name, size in sizes.items():
        result["/".join(name.split("/")[:depth])] += size
    return result


def compare(old, new, top, depth):
    print(f"Bromium size: {old.version} ({old.meta.get('commit')}) -> "
          f"{new.version} ({new.meta.get('commit')})")

    old_abis = set(filter(None, old.meta.get("abis", "").split(",")))
    new_abis = set(filter(None, new.meta.get("abis", "").split(",")))
    if old_abis != new_abis:
        print(f"\nWARNING: AARs bundle different ABIs ({sorted(old_abis)} vs "
              f"{sorted(new_abis)}); AAR totals are not comparable.")

    old_total = int(old.meta.get("aar_file_bytes", 0))
    new_total = int(new.meta.get("aar_file_bytes", 0))
    _print_table("AAR file", [("aloha-bromium.aar", old_total, new_total)],
                 min_delta=0)

    old_comp = {k: v[0] for k, v in old.aar.items()}
    new_comp = {k: v[0] for k, v in new.aar.items()}
    groups_old, groups_new = collections.Counter(), collections.Counter()
    for name, size in old_comp.items():
        groups_old[_aar_group(name)] += size
    for name, size in new_comp.items():
        groups_new[_aar_group(name)] += size
    _print_table("AAR by group (compressed)", _joined(groups_old, groups_new),
                 min_delta=0)
    _print_table("AAR entries (compressed), biggest changes",
                 _joined(old_comp, new_comp), top=top, min_delta=1024)

    for abi in sorted(set(old.src) | set(new.src)):
        if abi not in old.src or abi not in new.src:
            print(f"\n{abi}: native breakdown is missing in "
                  f"{old.version if abi not in old.src else new.version}, "
                  "skipped")
            continue
        _print_table(f"{abi}: {_LIB_NAME} sections (uncompressed)",
                     _joined(old.sections[abi], new.sections[abi]),
                     top=top, min_delta=1024)
        for d in range(1, depth + 1):
            _print_table(
                f"{abi}: source directories, depth {d} (uncompressed)",
                _joined(_aggregate(old.src[abi], d),
                        _aggregate(new.src[abi], d)),
                top=top, min_delta=1024)


# ---------------------------------------------------------------------------


def _version_key(version):
    return tuple(int(p) for p in version.split(".") if p.isdigit())


def _snapshot_path(version):
    return _SNAPSHOT_DIR / f"{version}.tsv"


def _latest_older_snapshot(version):
    candidates = [
        p for p in _SNAPSHOT_DIR.glob("*.tsv")
        if _version_key(p.stem) < _version_key(version)
    ]
    return max(candidates, key=lambda p: _version_key(p.stem), default=None)


def _load_or_measure(arg, save):
    path = Path(arg).expanduser()
    if path.is_file():
        return Snapshot.load(path)
    if not (path / "chrome/VERSION").is_file():
        sys.exit(f"error: {path} is neither a snapshot file nor a Chromium "
                 "src directory")
    snap = measure(path)
    if save:
        snap.save(_snapshot_path(snap.version))
        _log(f"Saved {_snapshot_path(snap.version)}")
    return snap


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--old",
        help="Bromium we are upgrading from: path to its src directory (it "
        "is measured and its snapshot saved) or to a saved snapshot. "
        "Default: the latest older snapshot in aloha/history/size.")
    parser.add_argument(
        "--new", default=str(_THIS_SRC),
        help="src directory or snapshot of the new Bromium "
        "(default: this checkout).")
    parser.add_argument("--snapshot-only", action="store_true",
                        help="Only measure --new and save its snapshot.")
    parser.add_argument("--no-save", action="store_true",
                        help="Do not write snapshots to aloha/history/size.")
    parser.add_argument("--top", type=int, default=25,
                        help="Rows per table (default: %(default)s).")
    parser.add_argument("--depth", type=int, default=3,
                        choices=range(1, _STORE_DEPTH + 1),
                        help="Deepest source directory level to print "
                        "(default: %(default)s).")
    args = parser.parse_args()

    new = _load_or_measure(args.new, save=not args.no_save)
    if args.snapshot_only:
        return

    if args.old:
        old = _load_or_measure(args.old, save=not args.no_save)
    else:
        old_path = _latest_older_snapshot(new.version)
        if not old_path:
            sys.exit("error: no older snapshot in aloha/history/size; pass "
                     "--old <path to the old Bromium src>")
        _log(f"Comparing against saved snapshot {old_path}")
        old = Snapshot.load(old_path)

    compare(old, new, args.top, args.depth)


if __name__ == "__main__":
    # libsupersize relies on fork() semantics for its worker pool.
    multiprocessing.set_start_method("fork")
    main()
