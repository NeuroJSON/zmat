#!/bin/bash
set -e

cd /src/python/

# Build wheels for each Python version
for PYBIN in /opt/python/cp3{7,8,9,10,11,12,13,14}*/bin/; do
    if [ -d "$PYBIN" ]; then
        echo "========================================"
        echo "Building for: ${PYBIN}"
        echo "========================================"

        PYVER=$(${PYBIN}/python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
        echo "Python version: ${PYVER}"

        # Clean previous build artifacts
        rm -rf build/ *.egg-info/ tmpwheels/ builddir/

        # Install build dependencies
        "${PYBIN}/pip" install --upgrade pip
        "${PYBIN}/pip" install build setuptools wheel

        # Build wheel
        "${PYBIN}/python" -m build --wheel --outdir tmpwheels/

        # Run unit tests — install only the wheel we just built
        BUILT_WHEEL=$(ls tmpwheels/zmat-*.whl | head -1)
        "${PYBIN}/pip" install "$BUILT_WHEEL"
        "${PYBIN}/python" -m unittest discover -s tests -v
        "${PYBIN}/pip" uninstall -y zmat

        # Move wheel to final collection directory
        mkdir -p allwheels/
        mv "$BUILT_WHEEL" allwheels/
    fi
done

# Repair wheels with auditwheel to get manylinux tags
rm -rf dist/
mkdir -p dist/

for WHEEL in allwheels/*.whl; do
    if [ -f "$WHEEL" ]; then
        echo "Checking: ${WHEEL}"

        NEEDS_REPAIR=$(auditwheel show "${WHEEL}" 2>&1 || true)

        if echo "$NEEDS_REPAIR" | grep -q "is consistent with the following platform tag"; then
            echo "Wheel is self-contained, copying without repair"
            cp "${WHEEL}" dist/
        else
            echo "Repairing wheel (bundling shared libraries)..."
            auditwheel repair "${WHEEL}" -w dist/
        fi
    fi
done

# Cleanup
rm -rf allwheels/ tmpwheels/ build/ *.egg-info/ builddir/

echo "========================================"
echo "Built wheels:"
ls -lh dist/
echo "========================================"