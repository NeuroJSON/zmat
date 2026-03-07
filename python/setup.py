"""
setup.py for the zmat Python C extension module

This file lives in python/ and references C sources in ../src/ and ../include/.
When building an sdist, MANIFEST.in must include those directories so that
the source tree is complete inside the distribution.

Build:
    pip install .
    pip install -e .          # editable/development install
    python setup.py build_ext --inplace   # build in-place for testing

Release:
    python -m build           # creates sdist + wheel in dist/
    twine upload dist/*       # upload to PyPI
"""

import glob
import os
import platform

from setuptools import Extension, setup

# ---------- paths relative to this setup.py (inside python/) ----------
here = os.path.dirname(os.path.abspath(__file__))
srcdir = os.path.join(here, "..", "src")
incdir = os.path.join(here, "..", "include")

# when building from sdist, ../src won't exist — sources are copied flat
# into the sdist under src/ and include/ via MANIFEST.in
if not os.path.isdir(srcdir):
    srcdir = os.path.join(here, "src")
    incdir = os.path.join(here, "include")

# ---------- collect source files ----------
sources = [
    os.path.join(here, "pyzmat.c"),
    os.path.join(srcdir, "zmatlib.c"),
]

include_dirs = [srcdir, incdir]

define_macros = []
libraries = []
library_dirs = []
extra_compile_args = ["-O2"]
extra_link_args = []

# detect CPU architecture
machine = platform.machine().lower()
is_x86 = any(x in machine for x in ["x86_64", "amd64", "i386", "i686"])

# ---- zlib / miniz ----
# By default, use the embedded miniz (no system zlib dependency).
# Set environment variable ZMAT_USE_SYSTEM_ZLIB=1 to link against -lz instead.
use_system_zlib = os.environ.get("ZMAT_USE_SYSTEM_ZLIB", "0") == "1"

if use_system_zlib:
    libraries.append("z")
else:
    define_macros.append(("NO_ZLIB", None))
    define_macros.append(("_LARGEFILE64_SOURCE", "1"))
    miniz_dir = os.path.join(srcdir, "miniz")
    include_dirs.append(miniz_dir)
    sources.append(os.path.join(miniz_dir, "miniz.c"))

# ---- lzma / easylzma ----
use_lzma = os.environ.get("ZMAT_NO_LZMA", "0") != "1"

if use_lzma:
    easylzma_dir = os.path.join(srcdir, "easylzma")
    pavlov_dir = os.path.join(easylzma_dir, "pavlov")
    include_dirs.extend([easylzma_dir, pavlov_dir])
    for f in ["compress", "decompress", "lzma_header", "lzip_header", "common_internal"]:
        sources.append(os.path.join(easylzma_dir, f + ".c"))
    for f in ["LzmaEnc", "LzmaDec", "LzmaLib", "LzFind", "Bra", "BraIA64", "Alloc", "7zCrc"]:
        sources.append(os.path.join(pavlov_dir, f + ".c"))
else:
    define_macros.append(("NO_LZMA", None))

# ---- lz4 ----
use_lz4 = os.environ.get("ZMAT_NO_LZ4", "0") != "1"

if use_lz4:
    lz4_dir = os.path.join(srcdir, "lz4")
    include_dirs.append(lz4_dir)
    sources.append(os.path.join(lz4_dir, "lz4.c"))
    sources.append(os.path.join(lz4_dir, "lz4hc.c"))
else:
    define_macros.append(("NO_LZ4", None))

# ---- zstd ----
# Embed zstd source files directly (no external library dependency)
use_zstd = os.environ.get("ZMAT_NO_ZSTD", "0") != "1"

if use_zstd:
    zstd_dir = os.path.join(srcdir, "blosc2", "internal-complibs", "zstd")
    include_dirs.append(zstd_dir)

    # collect all zstd .c files from subdirectories
    for subdir in ["common", "compress", "decompress"]:
        pattern = os.path.join(zstd_dir, subdir, "*.c")
        sources.extend(glob.glob(pattern))

    # zstd assembly requires GNU as and GNU ld; disable on non-Linux
    # platforms (macOS linker doesn't support zstd's x86_64 asm, and
    # ARM/other architectures don't have it at all)
    if platform.system() != "Linux" or not is_x86:
        define_macros.append(("ZSTD_DISABLE_ASM", "1"))
else:
    define_macros.append(("NO_ZSTD", None))

# ---- blosc2 ----
# Compile blosc2 source files directly (same as miniz, lz4, zstd)
use_blosc2 = os.environ.get("ZMAT_NO_BLOSC2", "0") != "1"

if use_blosc2:
    blosc2_dir = os.path.join(srcdir, "blosc2", "blosc")
    blosc2_inc = os.path.join(srcdir, "blosc2", "include")
    include_dirs.append(blosc2_inc)
    include_dirs.append(blosc2_dir)

    blosc2_srcs = [
        "blosc2.c",
        "blosc2-stdio.c",
        "blosclz.c",
        "delta.c",
        "directories.c",
        "fastcopy.c",
        "frame.c",
        "schunk.c",
        "sframe.c",
        "shuffle.c",
        "shuffle-generic.c",
        "bitshuffle-generic.c",
        "stune.c",
        "timestamp.c",
        "trunc-prec.c",
    ]
    for f in blosc2_srcs:
        sources.append(os.path.join(blosc2_dir, f))

    # add SSE2 shuffle on x86 only
    if is_x86:
        sources.append(os.path.join(blosc2_dir, "shuffle-sse2.c"))
        sources.append(os.path.join(blosc2_dir, "bitshuffle-sse2.c"))

    # blosc2 needs its internal lz4 if we already have lz4 in include path
    # and needs zlib/zstd headers — already added above

    # blosc2 needs pthread on Unix
    if platform.system() != "Windows":
        if "pthread" not in libraries:
            libraries.append("pthread")
else:
    define_macros.append(("NO_BLOSC2", None))

# ---- platform-specific flags ----
if platform.system() == "Windows":
    extra_compile_args = ["/O2"]
else:
    extra_compile_args.append("-fPIC")
    if platform.system() == "Darwin":
        extra_link_args.extend(["-undefined", "dynamic_lookup"])
    else:
        libraries.extend(["pthread", "m"])

# ---------- verify all source files exist ----------
missing = [s for s in sources if not os.path.isfile(s)]
if missing:
    print("WARNING: Missing source files (sdist may be incomplete):")
    for m in missing:
        print(f"  {m}")

# ---------- define the extension ----------
zmat_ext = Extension(
    name="zmat",
    sources=sources,
    include_dirs=include_dirs,
    define_macros=define_macros,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    language="c",
)

setup(
    ext_modules=[zmat_ext],
)
