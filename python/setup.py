"""
setup.py for the zmat Python C extension module

This file lives in python/ and references C sources in ../src/ and ../include/.
Since setuptools requires all paths to be within the setup.py directory,
sources are copied into a local 'csrc' directory before building.

Build:
    pip install .
    pip install -e .          # editable/development install
    python setup.py build_ext --inplace   # build in-place for testing

Release:
    python -m build           # creates sdist + wheel in dist/
    twine upload dist/*       # upload to PyPI
"""

import os
import shutil
import platform
import glob
import atexit
from setuptools import setup, Extension

# ---------- paths ----------
here          = os.path.dirname(os.path.abspath(__file__))
parent_srcdir = os.path.join(here, "..", "src")
parent_incdir = os.path.join(here, "..", "include")
csrc_dir      = os.path.join(here, "csrc")

# items to copy from parent repo into csrc/
COPY_ITEMS = [
    ("src/zmatlib.c",                          "src/zmatlib.c"),
    ("src/miniz",                              "src/miniz"),
    ("src/easylzma",                           "src/easylzma"),
    ("src/lz4",                                "src/lz4"),
    ("src/blosc2/internal-complibs/zstd",      "src/blosc2/internal-complibs/zstd"),
    ("src/blosc2/include",                     "src/blosc2/include"),
    ("src/blosc2/blosc",                       "src/blosc2/blosc"),
    ("src/blosc2/plugins",                     "src/blosc2/plugins"),
    ("include",                                "include"),
]


def ensure_csrc():
    """Copy C sources from parent directory into csrc/ if needed."""
    csrc_src = os.path.join(csrc_dir, "src")
    if os.path.isdir(csrc_src) and os.path.isfile(os.path.join(csrc_src, "zmatlib.c")):
        return  # already populated

    if not os.path.isdir(parent_srcdir):
        return  # no parent sources available (pure sdist build, csrc should be in tree)

    parent = os.path.join(here, "..")
    for src_rel, dst_rel in COPY_ITEMS:
        src_path = os.path.join(parent, src_rel)
        dst_path = os.path.join(csrc_dir, dst_rel)
        if os.path.isfile(src_path):
            os.makedirs(os.path.dirname(dst_path), exist_ok=True)
            shutil.copy2(src_path, dst_path)
        elif os.path.isdir(src_path):
            if os.path.exists(dst_path):
                shutil.rmtree(dst_path)
            shutil.copytree(src_path, dst_path,
                            ignore=shutil.ignore_patterns('*.o', '*.a', '*.S', '*.s'))


# copy sources into csrc/ so all paths are within setup.py's directory
# must change to setup.py's directory first so relative paths work
_orig_cwd = os.getcwd()
os.chdir(here)
ensure_csrc()
os.chdir(_orig_cwd)

# verify csrc exists
csrc_src = os.path.join(csrc_dir, "src")
if not os.path.isdir(csrc_src):
    import warnings
    warnings.warn("csrc/src/ not found - C sources may be missing. "
                  "Build from a full git checkout or a complete sdist.")

# all paths are relative to here, inside csrc/
srcdir = os.path.join("csrc", "src")
incdir = os.path.join("csrc", "include")


# ---------- detect CPU architecture ----------
machine = platform.machine().lower()
is_x86 = any(x in machine for x in ["x86_64", "amd64", "i386", "i686"])


# helper: check if file exists relative to here
def _exists(relpath):
    return os.path.isfile(os.path.join(here, relpath))


# ---------- collect source files ----------
sources = ["pyzmat.c"]

zmatlib = os.path.join(srcdir, "zmatlib.c")
if _exists(zmatlib):
    sources.append(zmatlib)

include_dirs = [srcdir, incdir]

define_macros = []
libraries = []
library_dirs = []
extra_compile_args = ["-O2"]
extra_link_args = []


# ---- zlib / miniz ----
use_system_zlib = os.environ.get("ZMAT_USE_SYSTEM_ZLIB", "0") == "1"

if use_system_zlib:
    libraries.append("z")
else:
    define_macros.append(("NO_ZLIB", None))
    define_macros.append(("_LARGEFILE64_SOURCE", "1"))
    miniz_dir = os.path.join(srcdir, "miniz")
    include_dirs.append(miniz_dir)
    p = os.path.join(miniz_dir, "miniz.c")
    if _exists(p):
        sources.append(p)


# ---- lzma / easylzma ----
use_lzma = os.environ.get("ZMAT_NO_LZMA", "0") != "1"

if use_lzma:
    easylzma_dir = os.path.join(srcdir, "easylzma")
    pavlov_dir = os.path.join(easylzma_dir, "pavlov")
    include_dirs.extend([easylzma_dir, pavlov_dir])
    for f in ["compress", "decompress", "lzma_header", "lzip_header", "common_internal"]:
        p = os.path.join(easylzma_dir, f + ".c")
        if _exists(p):
            sources.append(p)
    for f in ["LzmaEnc", "LzmaDec", "LzmaLib", "LzFind", "Bra", "BraIA64", "Alloc", "7zCrc"]:
        p = os.path.join(pavlov_dir, f + ".c")
        if _exists(p):
            sources.append(p)
else:
    define_macros.append(("NO_LZMA", None))


# ---- lz4 ----
use_lz4 = os.environ.get("ZMAT_NO_LZ4", "0") != "1"

if use_lz4:
    lz4_dir = os.path.join(srcdir, "lz4")
    include_dirs.append(lz4_dir)
    for f in ["lz4.c", "lz4hc.c"]:
        p = os.path.join(lz4_dir, f)
        if _exists(p):
            sources.append(p)
else:
    define_macros.append(("NO_LZ4", None))


# ---- zstd ----
use_zstd = os.environ.get("ZMAT_NO_ZSTD", "0") != "1"

if use_zstd:
    zstd_dir = os.path.join(srcdir, "blosc2", "internal-complibs", "zstd")
    include_dirs.append(zstd_dir)
    include_dirs.append(os.path.join(zstd_dir, "common"))

    for subdir in ["common", "compress", "decompress", "dictBuilder"]:
        abs_pattern = os.path.join(here, zstd_dir, subdir, "*.c")
        for abspath in glob.glob(abs_pattern):
            sources.append(os.path.relpath(abspath, here))

    # zstd asm not supported via setuptools Extension; use pure C fallback
    define_macros.append(("ZSTD_DISABLE_ASM", "1"))
else:
    define_macros.append(("NO_ZSTD", None))


# ---- blosc2 ----
use_blosc2 = os.environ.get("ZMAT_NO_BLOSC2", "0") != "1"

if use_blosc2:
    blosc2_dir = os.path.join(srcdir, "blosc2", "blosc")
    blosc2_inc = os.path.join(srcdir, "blosc2", "include")
    include_dirs.append(blosc2_inc)
    include_dirs.append(blosc2_dir)

    # tell blosc2 which codecs are available
    # blosc2zlib needs zlib API — miniz provides this even when NO_ZLIB is set
    define_macros.append(("HAVE_ZLIB", "1"))
    if use_zstd:
        define_macros.append(("HAVE_ZSTD", "1"))
    if use_lz4:
        define_macros.append(("HAVE_LZ4", "1"))

    blosc2_srcs = [
        "blosc2.c", "blosc2-stdio.c", "blosclz.c", "delta.c", "directories.c",
        "fastcopy.c", "frame.c", "schunk.c", "sframe.c", "shuffle.c",
        "shuffle-generic.c", "bitshuffle-generic.c", "stune.c",
        "timestamp.c", "trunc-prec.c",
    ]
    for f in blosc2_srcs:
        p = os.path.join(blosc2_dir, f)
        if _exists(p):
            sources.append(p)

    if is_x86:
        for f in ["shuffle-sse2.c", "bitshuffle-sse2.c"]:
            p = os.path.join(blosc2_dir, f)
            if _exists(p):
                sources.append(p)

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
        if "pthread" not in libraries:
            libraries.append("pthread")
        if "m" not in libraries:
            libraries.append("m")


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