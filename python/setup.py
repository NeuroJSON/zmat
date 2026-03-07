"""
setup.py for the zmat Python C extension module

This file lives in python/ and references C sources in ../src/ and ../include/.
For sdist builds, a custom command copies the required source files into a local
'csrc' directory so the sdist is self-contained.

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
import shutil

from setuptools import Extension, setup
from setuptools.command.sdist import sdist as _sdist

# ---------- paths ----------
here = os.path.dirname(os.path.abspath(__file__))
parent_srcdir = os.path.join(here, "..", "src")
parent_incdir = os.path.join(here, "..", "include")
local_csrc = os.path.join(here, "csrc")


# ---------- custom sdist command ----------
class sdist_with_csrc(_sdist):
    """Custom sdist that copies C sources into csrc/ before packaging."""

    COPY_ITEMS = [
        ("src/zmatlib.c", "src/zmatlib.c"),
        ("src/miniz", "src/miniz"),
        ("src/easylzma", "src/easylzma"),
        ("src/lz4", "src/lz4"),
        ("src/blosc2/internal-complibs/zstd", "src/blosc2/internal-complibs/zstd"),
        ("src/blosc2/include", "src/blosc2/include"),
        ("src/blosc2/blosc", "src/blosc2/blosc"),
        ("src/blosc2/plugins", "src/blosc2/plugins"),
        ("include", "include"),
    ]

    def run(self):
        self._copy_csrc()
        try:
            _sdist.run(self)
        finally:
            self._clean_csrc()

    def _copy_csrc(self):
        parent = os.path.join(here, "..")
        for src_rel, dst_rel in self.COPY_ITEMS:
            src_path = os.path.join(parent, src_rel)
            dst_path = os.path.join(local_csrc, dst_rel)
            if os.path.isfile(src_path):
                os.makedirs(os.path.dirname(dst_path), exist_ok=True)
                shutil.copy2(src_path, dst_path)
            elif os.path.isdir(src_path):
                if os.path.exists(dst_path):
                    shutil.rmtree(dst_path)
                shutil.copytree(
                    src_path, dst_path, ignore=shutil.ignore_patterns("*.o", "*.a", "*.S", "*.s")
                )

    def _clean_csrc(self):
        if os.path.isdir(local_csrc):
            shutil.rmtree(local_csrc)


# ---------- determine source tree location ----------
if os.path.isdir(parent_srcdir):
    srcdir = parent_srcdir
    incdir = parent_incdir
elif os.path.isdir(local_csrc):
    srcdir = os.path.join(local_csrc, "src")
    incdir = os.path.join(local_csrc, "include")
else:
    # during get_requires_for_build_sdist, sources may not be needed yet
    srcdir = parent_srcdir
    incdir = parent_incdir


# ---------- detect CPU architecture ----------
machine = platform.machine().lower()
is_x86 = any(x in machine for x in ["x86_64", "amd64", "i386", "i686"])


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


# ---- zlib / miniz ----
use_system_zlib = os.environ.get("ZMAT_USE_SYSTEM_ZLIB", "0") == "1"

if use_system_zlib:
    libraries.append("z")
else:
    define_macros.append(("NO_ZLIB", None))
    define_macros.append(("_LARGEFILE64_SOURCE", "1"))
    miniz_dir = os.path.join(srcdir, "miniz")
    include_dirs.append(miniz_dir)
    miniz_c = os.path.join(miniz_dir, "miniz.c")
    if os.path.isfile(miniz_c):
        sources.append(miniz_c)


# ---- lzma / easylzma ----
use_lzma = os.environ.get("ZMAT_NO_LZMA", "0") != "1"

if use_lzma:
    easylzma_dir = os.path.join(srcdir, "easylzma")
    pavlov_dir = os.path.join(easylzma_dir, "pavlov")
    include_dirs.extend([easylzma_dir, pavlov_dir])
    for f in ["compress", "decompress", "lzma_header", "lzip_header", "common_internal"]:
        p = os.path.join(easylzma_dir, f + ".c")
        if os.path.isfile(p):
            sources.append(p)
    for f in ["LzmaEnc", "LzmaDec", "LzmaLib", "LzFind", "Bra", "BraIA64", "Alloc", "7zCrc"]:
        p = os.path.join(pavlov_dir, f + ".c")
        if os.path.isfile(p):
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
        if os.path.isfile(p):
            sources.append(p)
else:
    define_macros.append(("NO_LZ4", None))


# ---- zstd ----
use_zstd = os.environ.get("ZMAT_NO_ZSTD", "0") != "1"

if use_zstd:
    zstd_dir = os.path.join(srcdir, "blosc2", "internal-complibs", "zstd")
    include_dirs.append(zstd_dir)

    for subdir in ["common", "compress", "decompress"]:
        pattern = os.path.join(zstd_dir, subdir, "*.c")
        sources.extend(glob.glob(pattern))

    # zstd's huf_decompress.c references assembly symbols from .S files.
    # setuptools' Extension does not support .S files natively, so we
    # always disable assembly and use the pure C fallback.
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
        p = os.path.join(blosc2_dir, f)
        if os.path.isfile(p):
            sources.append(p)

    if is_x86:
        for f in ["shuffle-sse2.c", "bitshuffle-sse2.c"]:
            p = os.path.join(blosc2_dir, f)
            if os.path.isfile(p):
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
    cmdclass={"sdist": sdist_with_csrc},
)
