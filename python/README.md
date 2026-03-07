# ZMat - A portable data compression/decompression library for Python

![Build Status](https://github.com/NeuroJSON/zmat/actions/workflows/build_linux_wheel.yml/badge.svg)
![Build Status](https://github.com/NeuroJSON/zmat/actions/workflows/build_macos_wheel.yml/badge.svg)
![Build Status](https://github.com/NeuroJSON/zmat/actions/workflows/build_windows_wheel.yml/badge.svg)
[![PyPI](https://img.shields.io/pypi/v/zmat)](https://pypi.org/project/zmat/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

ZMat is a lightweight Python C extension for fast, in-memory data compression
and decompression. It provides a unified interface to multiple compression
algorithms, all compiled directly into the module with **zero external
dependencies**.

ZMat is part of the [NeuroJSON project](https://neurojson.org) and is supported
by the US National Institute of Health (NIH) grant
[U24-NS124027](https://reporter.nih.gov/project-details/10308329).

- **GitHub**: https://github.com/NeuroJSON/zmat
- **Documentation**: https://neurojson.org/Page/zmat

## Supported Algorithms

| Method | Description | Strength |
|--------|-------------|----------|
| `zlib` | The most widely used algorithm for `.zip` files | Excellent balance of speed and ratio |
| `gzip` | gzip format, compatible with `.gz` files | Same as zlib with gzip header/footer |
| `lzma` | High compression ratio LZMA algorithm | Best compression ratio, slowest |
| `lzip` | LZIP format using LZMA | Similar to lzma with lzip framing |
| `lz4`  | Real-time LZ4 compression | Fastest compression/decompression |
| `lz4hc`| LZ4 High Compression mode | Better ratio than lz4, slower |
| `zstd` | Zstandard compression | Fast with high compression ratio |
| `blosc2blosclz` | Blosc2 meta-compressor with BloscLZ | Optimized for numeric data |
| `blosc2lz4` | Blosc2 with LZ4 backend | Fast numeric data compression |
| `blosc2lz4hc` | Blosc2 with LZ4HC backend | Higher ratio numeric compression |
| `blosc2zlib` | Blosc2 with zlib backend | Balanced numeric compression |
| `blosc2zstd` | Blosc2 with Zstandard backend | High ratio numeric compression |
| `base64` | Base64 encoding/decoding | Not compression; encoding only |

## Installation

### From PyPI

```bash
pip install zmat
```

### From source

```bash
git clone https://github.com/NeuroJSON/zmat.git
cd zmat/python
pip install .
```

All compression libraries (miniz, easylzma, lz4, zstd, blosc2) are embedded
in the source and compiled directly into the module. No system libraries are
required.

## Quick Start

```python
import zmat

# Compress data
data = b"Hello, ZMat! " * 1000
compressed = zmat.compress(data)
print(f"Original: {len(data)} bytes -> Compressed: {len(compressed)} bytes")

# Decompress data
restored = zmat.decompress(compressed)
assert restored == data

# Use different algorithms
fast = zmat.compress(data, method='lz4')       # fastest
small = zmat.compress(data, method='lzma')      # smallest
balanced = zmat.compress(data, method='zstd')   # good balance

# Base64 encoding/decoding
encoded = zmat.encode(data, method='base64')
decoded = zmat.decode(encoded, method='base64')
assert decoded == data
```

## API Reference

### `zmat.compress(data, method='zlib', level=1)`

Compress a bytes-like object.

**Parameters:**
- `data` — `bytes`, `bytearray`, or any object supporting the buffer protocol
- `method` — compression algorithm name (default: `'zlib'`)
- `level` — compression level: `1` for default, higher values (up to 9 or 12
  depending on algorithm) for more compression at the cost of speed

**Returns:** `bytes` — compressed data

### `zmat.decompress(data, method='zlib')`

Decompress a bytes-like object.

**Parameters:**
- `data` — `bytes` or `bytearray` of compressed data
- `method` — compression algorithm used to compress the data (default: `'zlib'`)

**Returns:** `bytes` — decompressed data

### `zmat.encode(data, method='base64')`

Encode data (e.g., base64 encoding).

**Parameters:**
- `data` — `bytes` or `bytearray` to encode
- `method` — encoding method (default: `'base64'`)

**Returns:** `bytes` — encoded data

### `zmat.decode(data, method='base64')`

Decode data (e.g., base64 decoding).

**Parameters:**
- `data` — `bytes` or `bytearray` of encoded data
- `method` — encoding method used (default: `'base64'`)

**Returns:** `bytes` — decoded data

### `zmat.zmat(data, iscompress=1, method='zlib', nthread=1, shuffle=1, typesize=4)`

Low-level compression/decompression interface with full control over all
parameters, including blosc2 multi-threading options.

**Parameters:**
- `data` — `bytes`, `bytearray`, or buffer-protocol object
- `iscompress` — `1` to compress (default level), `0` to decompress,
  negative values to set compression level (e.g., `-9` for maximum)
- `method` — algorithm name
- `nthread` — number of threads for blosc2 (default: `1`)
- `shuffle` — byte shuffle for blosc2: `0` disabled, `1` enabled (default: `1`)
- `typesize` — element byte size for blosc2 shuffle (default: `4`)

**Returns:** `bytes` — compressed or decompressed data

## Interoperability

ZMat's zlib and gzip output is fully compatible with Python's standard library:

```python
import zmat
import zlib
import gzip
import io

data = b"interoperability test " * 100

# zmat -> Python zlib
compressed = zmat.compress(data, method='zlib')
assert zlib.decompress(compressed) == data

# Python zlib -> zmat
compressed = zlib.compress(data)
assert zmat.decompress(compressed, method='zlib') == data

# zmat -> Python gzip
compressed = zmat.compress(data, method='gzip')
with gzip.open(io.BytesIO(compressed), 'rb') as f:
    assert f.read() == data
```

## Working with NumPy Arrays

ZMat accepts any object supporting Python's buffer protocol, including
NumPy arrays:

```python
import numpy as np
import zmat

arr = np.random.rand(1000, 1000)
compressed = zmat.compress(arr.tobytes(), method='lz4')
restored = np.frombuffer(zmat.decompress(compressed, method='lz4'),
                         dtype=arr.dtype).reshape(arr.shape)
assert np.array_equal(arr, restored)
```

For numerical arrays, the blosc2 methods with byte-shuffle can achieve
better compression ratios:

```python
compressed = zmat.zmat(arr.tobytes(), iscompress=1, method='blosc2zstd',
                       typesize=8)  # 8 bytes per float64 element
```

## Environment Variables

The build can be customized via environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `ZMAT_USE_SYSTEM_ZLIB=1` | off | Link against system `-lz` instead of embedded miniz |
| `ZMAT_NO_LZMA=1` | off | Disable lzma/lzip support |
| `ZMAT_NO_LZ4=1` | off | Disable lz4/lz4hc support |
| `ZMAT_NO_ZSTD=1` | off | Disable zstd support |
| `ZMAT_NO_BLOSC2=1` | off | Disable blosc2 support |

Example: build without blosc2 for a smaller binary:

```bash
ZMAT_NO_BLOSC2=1 pip install .
```

## MATLAB/Octave Version

ZMat is also available as a MATLAB/Octave MEX function with an identical
feature set. See the [main README](https://github.com/NeuroJSON/zmat) for
details on the MATLAB/Octave toolbox.

## Acknowledgements

ZMat bundles the following open-source libraries:

- **zlib/miniz** — Jean-loup Gailly, Mark Adler / Rich Geldreich (MIT/Zlib license)
- **easylzma** — Lloyd Hilaiel (public domain)
- **LZMA SDK** — Igor Pavlov (public domain)
- **LZ4** — Yann Collet (BSD 2-Clause)
- **Zstandard** — Meta Platforms, Inc. (BSD 3-Clause)
- **C-Blosc2** — Blosc Development Team (BSD 3-Clause)
- **base64** — Jouni Malinen (BSD license)

## License

ZMat is licensed under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0).

Copyright (C) 2019-2026 Qianqian Fang <<q.fang at neu.edu>>