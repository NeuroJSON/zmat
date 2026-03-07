#################################################################
#  Makefile for building the Python zmat C extension module
#  Qianqian Fang <q.fang at neu.edu>
#  2025
#
#  Usage:
#      make python          # build C extension in-place
#      make test            # run round-trip tests
#      make wheel           # build sdist + wheel
#      make upload          # upload to PyPI
#      make upload-test     # upload to TestPyPI
#      make lint            # format and lint
#      make coverage        # run tests with coverage
#      make clean           # remove build artifacts
#################################################################

PYTHON     ?= python3
PIP        ?= pip3
PYCONFIG   := $(PYTHON) -c

# query Python for build flags
PYINC      := $(shell $(PYCONFIG) "import sysconfig; print(sysconfig.get_path('include'))")
PYLIB      := $(shell $(PYCONFIG) "import sysconfig; print(sysconfig.get_config_var('LDLIBRARY') or '')")
PYLIBDIR   := $(shell $(PYCONFIG) "import sysconfig; print(sysconfig.get_config_var('LIBDIR') or '')")
PYEXT      := $(shell $(PYCONFIG) "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

# fall back if EXT_SUFFIX is empty (Python < 3.5)
ifeq ($(PYEXT),)
  PYEXT    := .so
endif

ROOTDIR    ?= ..
SRCDIR     ?= $(ROOTDIR)/src
ZMATDIR    ?= $(ROOTDIR)
LIBDIR     ?= $(ROOTDIR)/lib

CC         ?= gcc
MKDIR      := mkdir -p

HAVE_ZLIB  ?= miniz
HAVE_LZMA  ?= yes
HAVE_LZ4   ?= yes
HAVE_ZSTD  ?= yes
HAVE_BLOSC2?= yes

CFLAGS     += -O2 -fPIC -Wall
INCLUDEDIRS = -I$(SRCDIR) -I$(ROOTDIR)/include -I$(PYINC)

LDFLAGS    += -shared
LIBS       :=

# zlib / miniz
ifneq ($(HAVE_ZLIB),yes)
  CFLAGS   += -DNO_ZLIB -D_LARGEFILE64_SOURCE=1
  INCLUDEDIRS += -I$(SRCDIR)/miniz
else
  # system zlib linked at link time
endif

# lzma
ifeq ($(HAVE_LZMA),no)
  CFLAGS   += -DNO_LZMA
else
  INCLUDEDIRS += -I$(SRCDIR)/easylzma -I$(SRCDIR)/easylzma/pavlov
endif

# lz4
ifeq ($(HAVE_LZ4),no)
  CFLAGS   += -DNO_LZ4
else
  INCLUDEDIRS += -I$(SRCDIR)/lz4
endif

# blosc2
ifeq ($(HAVE_BLOSC2),no)
  CFLAGS   += -DNO_BLOSC2
else
  INCLUDEDIRS += -I$(SRCDIR)/blosc2/include
endif

# zstd
ifeq ($(HAVE_ZSTD),no)
  CFLAGS   += -DNO_ZSTD
else
  INCLUDEDIRS += -I$(SRCDIR)/blosc2/internal-complibs/zstd
endif

# ---------- static library path ----------
LIBZMAT    := $(LIBDIR)/libzmat.a

# core sources — only pyzmat.c, everything else comes from libzmat.a
SRCS       := pyzmat.c

# object files
OBJS       := $(SRCS:.c=.o)

# output binary
TARGET     := zmat$(PYEXT)

# link against static libzmat.a — all codecs (miniz, easylzma, lz4, zstd,
# blosc2) are already compiled into it, so we only need system libs for
# threading and math
LIBS       := $(LIBZMAT)

# platform-specific system libraries needed at link time
PLATFORM   := $(shell uname -s)
ifeq ($(findstring Darwin,$(PLATFORM)),Darwin)
  LDFLAGS  += -undefined dynamic_lookup
else ifeq ($(findstring _NT-,$(PLATFORM)),_NT-)
  # Windows — no extra libs needed
else
  # Linux / other Unix
  LIBS     += -lpthread -lm
endif

ifeq ($(HAVE_ZLIB),yes)
  LIBS     += -lz
endif

# platform-specific linker flags
ifneq ($(PYLIBDIR),)
  LDFLAGS  += -L$(PYLIBDIR)
endif

# Windows extension suffix
ifeq ($(findstring _NT-,$(PLATFORM)),_NT-)
  PYEXT    := .pyd
  TARGET   := zmat$(PYEXT)
  PYLDLIB  := $(shell $(PYCONFIG) "import sysconfig; print(sysconfig.get_config_var('LDLIBRARY') or '')")
  ifneq ($(PYLDLIB),)
    LIBS   += -l$(basename $(PYLDLIB))
  endif
endif

# ================================================================
#  Targets
# ================================================================

.PHONY: all python lib clean install test \
        wheel sdist upload upload-test \
        lint format check \
        coverage \
        unittest pytest \
        dev-install dev-uninstall \
        help

all: python

# ---------------- build ----------------

python: lib $(TARGET)

lib:
	$(MAKE) -C $(SRCDIR) lib CC=$(CC)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# build extension in-place via setuptools (alternative to raw make)
build-ext:
	$(PYTHON) setup.py build_ext --inplace

# ---------------- packaging ----------------

wheel:
	$(PYTHON) -m build

sdist:
	$(PYTHON) -m build --sdist

upload: wheel
	$(PYTHON) -m twine upload dist/*

upload-test: wheel
	$(PYTHON) -m twine upload --repository testpypi dist/*

# ---------------- install ----------------

install: $(TARGET)
	$(PYTHON) -c "import site; print(site.getsitepackages()[0])" | xargs -I{} cp $(TARGET) {}

# install in editable/development mode via pip
dev-install:
	$(PIP) install -e .

dev-uninstall:
	$(PIP) uninstall -y zmat

# ---------------- testing ----------------

test: $(TARGET)
	$(PYTHON) -c "\
import zmat; \
data = b'Hello ZMat! ' * 100; \
print('Original:', len(data), 'bytes'); \
c = zmat.compress(data); \
print('Compressed (zlib):', len(c), 'bytes'); \
d = zmat.decompress(c); \
assert d == data, 'zlib round-trip failed'; \
print('Decompressed: OK'); \
e = zmat.encode(data, method='base64'); \
print('Base64 encoded:', len(e), 'bytes'); \
f = zmat.decode(e, method='base64'); \
assert f == data, 'base64 round-trip failed'; \
print('Base64 decoded: OK'); \
c2 = zmat.compress(data, method='lz4'); \
d2 = zmat.decompress(c2, method='lz4'); \
assert d2 == data, 'lz4 round-trip failed'; \
print('LZ4 round-trip: OK'); \
c3 = zmat.compress(data, method='lzma'); \
d3 = zmat.decompress(c3, method='lzma'); \
assert d3 == data, 'lzma round-trip failed'; \
print('LZMA round-trip: OK'); \
c4 = zmat.compress(data, method='gzip'); \
d4 = zmat.decompress(c4, method='gzip'); \
assert d4 == data, 'gzip round-trip failed'; \
print('Gzip round-trip: OK'); \
print('All tests passed!'); \
"

# run tests via unittest
unittest:
	PYTHONPATH=$(CURDIR) $(PYTHON) -m unittest discover -s tests -v

pytest: unittest

# ---------------- coverage ----------------

coverage:
	PYTHONPATH=$(CURDIR) $(PYTHON) -m coverage run -m unittest discover -s tests -v
	$(PYTHON) -m coverage report --show-missing
	$(PYTHON) -m coverage html -d htmlcov

coverage-open: coverage
	@if [ "$(PLATFORM)" = "Darwin" ]; then \
	    open htmlcov/index.html; \
	elif command -v xdg-open >/dev/null 2>&1; then \
	    xdg-open htmlcov/index.html; \
	else \
	    echo "Coverage report: htmlcov/index.html"; \
	fi

# ---------------- linting / formatting ----------------

lint: format check

format:
	$(PYTHON) -m black . --line-length 100
	$(PYTHON) -m isort . --profile black --line-length 100

check:
	$(PYTHON) -m black . --check --line-length 100
	$(PYTHON) -m isort . --check --profile black --line-length 100
	$(PYTHON) -m flake8 . --max-line-length 100 --ignore=E203,W503

# ---------------- clean ----------------

clean:
	-rm -f $(OBJS) $(TARGET)
	-rm -rf build dist *.egg-info
	-rm -rf htmlcov .coverage
	-find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
	-find . -name "*.pyc" -delete 2>/dev/null || true

# ---------------- dependency installation helpers ----------------

deps-build:
	$(PIP) install build twine

deps-dev:
	$(PIP) install build twine coverage \
	    black isort flake8

# ---------------- help ----------------

help:
	@echo "ZMat Python module build targets:"
	@echo ""
	@echo "  Build:"
	@echo "    make python       - build C extension in-place (default)"
	@echo "    make build-ext    - build via setuptools (alternative)"
	@echo ""
	@echo "  Package:"
	@echo "    make wheel        - build sdist + wheel in dist/"
	@echo "    make sdist        - build source distribution only"
	@echo "    make upload       - upload to PyPI"
	@echo "    make upload-test  - upload to TestPyPI"
	@echo ""
	@echo "  Install:"
	@echo "    make install      - copy .so to site-packages"
	@echo "    make dev-install  - pip install -e . (editable)"
	@echo "    make dev-uninstall- pip uninstall zmat"
	@echo ""
	@echo "  Test:"
	@echo "    make test         - quick inline round-trip test"
	@echo "    make pytest       - run pytest test suite"
	@echo "    make unittest     - same as pytest"
	@echo "    make coverage     - pytest with coverage report"
	@echo "    make coverage-open- coverage + open HTML report"
	@echo ""
	@echo "  Lint:"
	@echo "    make format       - auto-format with black + isort"
	@echo "    make check        - check formatting (no changes)"
	@echo "    make lint         - format + check"
	@echo ""
	@echo "  Misc:"
	@echo "    make deps-build   - install build + twine"
	@echo "    make deps-dev     - install all dev dependencies"
	@echo "    make clean        - remove all build artifacts"
	@echo "    make help         - show this message"