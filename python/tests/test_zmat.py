"""
Unit tests for the zmat Python C extension module.

Based on the MATLAB test suite (test/run_zmat_test.m) and demo scripts
(example/demo_zmat_basic.m, example/zmat_speedbench.m).

Usage:
    python -m pytest python/tests/test_zmat.py -v
    python -m unittest python/tests/test_zmat.py -v
"""

import struct
import time
import unittest

import zmat


class TestZmatBasic(unittest.TestCase):
    """Basic compression/decompression round-trip tests (mirrors demo_zmat_basic.m)."""

    def test_compress_decompress_bytes(self):
        """Test zlib round-trip on simple byte data."""
        data = b"\x01\x00\x00\x00\x00" * 5 + b"\x00\x01\x00\x00\x00" * 5  # like eye(5) uint8
        compressed = zmat.compress(data)
        self.assertIsInstance(compressed, bytes)
        self.assertGreater(len(compressed), 0)
        decompressed = zmat.decompress(compressed)
        self.assertEqual(decompressed, data)

    def test_compress_decompress_string(self):
        """Test zlib round-trip on string data (encoded to bytes)."""
        data = b"zmat toolbox test string"
        compressed = zmat.compress(data)
        decompressed = zmat.decompress(compressed)
        self.assertEqual(decompressed, data)

    def test_base64_encode_decode(self):
        """Test base64 encoding/decoding (mirrors demo_zmat_basic.m base64 test)."""
        data = b"zmat toolbox"
        encoded = zmat.encode(data, method="base64")
        self.assertIsInstance(encoded, bytes)
        self.assertGreater(len(encoded), 0)
        decoded = zmat.decode(encoded, method="base64")
        self.assertEqual(decoded, data)

    def test_base64_round_trip_binary(self):
        """Test base64 on binary data with all byte values."""
        data = bytes(range(256))
        encoded = zmat.encode(data, method="base64")
        decoded = zmat.decode(encoded, method="base64")
        self.assertEqual(decoded, data)

    def test_empty_input(self):
        """Test that empty input returns empty output."""
        self.assertEqual(zmat.compress(b""), b"")
        self.assertEqual(zmat.decompress(b""), b"")
        self.assertEqual(zmat.encode(b""), b"")
        self.assertEqual(zmat.decode(b""), b"")

    def test_single_byte(self):
        """Test compression of a single byte."""
        data = b"\x42"
        for method in ["zlib", "gzip", "base64"]:
            compressed = zmat.compress(data, method=method)
            decompressed = zmat.decompress(compressed, method=method)
            self.assertEqual(decompressed, data, f"Failed for method={method}")


class TestZmatMethods(unittest.TestCase):
    """Round-trip tests for all supported compression methods."""

    def setUp(self):
        """Create test data — similar to eye(5) as doubles (mirrors MATLAB tests)."""
        # 5x5 identity matrix as doubles (40 doubles = 320 bytes)
        self.eye5 = b""
        for i in range(5):
            for j in range(5):
                val = 1.0 if i == j else 0.0
                self.eye5 += struct.pack("<d", val)

        # highly compressible data
        self.zeros = b"\x00" * 10000

        # moderately compressible data
        self.text = b"The quick brown fox jumps over the lazy dog. " * 100

        # random-ish data (less compressible)
        self.mixed = bytes([(i * 37 + 13) & 0xFF for i in range(10000)])

    def _round_trip(self, data, method):
        """Helper: compress and decompress, assert equality."""
        compressed = zmat.compress(data, method=method)
        self.assertIsInstance(compressed, bytes)
        self.assertGreater(len(compressed), 0)
        decompressed = zmat.decompress(compressed, method=method)
        self.assertEqual(decompressed, data, f"Round-trip failed for method={method}")
        return compressed

    def test_zlib(self):
        """Test zlib round-trip on all data types."""
        self._round_trip(self.eye5, "zlib")
        self._round_trip(self.zeros, "zlib")
        self._round_trip(self.text, "zlib")
        self._round_trip(self.mixed, "zlib")

    def test_gzip(self):
        """Test gzip round-trip on all data types."""
        self._round_trip(self.eye5, "gzip")
        self._round_trip(self.zeros, "gzip")
        self._round_trip(self.text, "gzip")
        self._round_trip(self.mixed, "gzip")

    def test_lzma(self):
        """Test lzma round-trip on all data types."""
        self._round_trip(self.eye5, "lzma")
        self._round_trip(self.zeros, "lzma")
        self._round_trip(self.text, "lzma")
        self._round_trip(self.mixed, "lzma")

    def test_lzip(self):
        """Test lzip round-trip on all data types."""
        self._round_trip(self.eye5, "lzip")
        self._round_trip(self.zeros, "lzip")
        self._round_trip(self.text, "lzip")
        self._round_trip(self.mixed, "lzip")

    def test_lz4(self):
        """Test lz4 round-trip on all data types."""
        self._round_trip(self.eye5, "lz4")
        self._round_trip(self.zeros, "lz4")
        self._round_trip(self.text, "lz4")
        self._round_trip(self.mixed, "lz4")

    def test_lz4hc(self):
        """Test lz4hc round-trip on all data types."""
        self._round_trip(self.eye5, "lz4hc")
        self._round_trip(self.zeros, "lz4hc")
        self._round_trip(self.text, "lz4hc")
        self._round_trip(self.mixed, "lz4hc")

    def test_base64(self):
        """Test base64 round-trip on all data types."""
        self._round_trip(self.eye5, "base64")
        self._round_trip(self.zeros, "base64")
        self._round_trip(self.text, "base64")
        self._round_trip(self.mixed, "base64")

    def test_zstd(self):
        """Test zstd round-trip on all data types."""
        self._round_trip(self.eye5, "zstd")
        self._round_trip(self.zeros, "zstd")
        self._round_trip(self.text, "zstd")
        self._round_trip(self.mixed, "zstd")

    def test_blosc2blosclz(self):
        """Test blosc2blosclz round-trip on all data types."""
        self._round_trip(self.eye5, "blosc2blosclz")
        self._round_trip(self.zeros, "blosc2blosclz")
        self._round_trip(self.text, "blosc2blosclz")
        self._round_trip(self.mixed, "blosc2blosclz")

    def test_blosc2lz4(self):
        """Test blosc2lz4 round-trip on all data types."""
        self._round_trip(self.eye5, "blosc2lz4")
        self._round_trip(self.zeros, "blosc2lz4")
        self._round_trip(self.text, "blosc2lz4")
        self._round_trip(self.mixed, "blosc2lz4")

    def test_blosc2lz4hc(self):
        """Test blosc2lz4hc round-trip on all data types."""
        self._round_trip(self.eye5, "blosc2lz4hc")
        self._round_trip(self.zeros, "blosc2lz4hc")
        self._round_trip(self.text, "blosc2lz4hc")
        self._round_trip(self.mixed, "blosc2lz4hc")

    def test_blosc2zlib(self):
        """Test blosc2zlib round-trip on all data types."""
        self._round_trip(self.eye5, "blosc2zlib")
        self._round_trip(self.zeros, "blosc2zlib")
        self._round_trip(self.text, "blosc2zlib")
        self._round_trip(self.mixed, "blosc2zlib")

    def test_blosc2zstd(self):
        """Test blosc2zstd round-trip on all data types."""
        self._round_trip(self.eye5, "blosc2zstd")
        self._round_trip(self.zeros, "blosc2zstd")
        self._round_trip(self.text, "blosc2zstd")
        self._round_trip(self.mixed, "blosc2zstd")

    def test_zlib_compresses(self):
        """Test that zlib actually reduces size on compressible data."""
        compressed = zmat.compress(self.zeros, method="zlib")
        self.assertLess(len(compressed), len(self.zeros))

    def test_gzip_compresses(self):
        """Test that gzip actually reduces size on compressible data."""
        compressed = zmat.compress(self.zeros, method="gzip")
        self.assertLess(len(compressed), len(self.zeros))

    def test_lz4_compresses(self):
        """Test that lz4 actually reduces size on compressible data."""
        compressed = zmat.compress(self.zeros, method="lz4")
        self.assertLess(len(compressed), len(self.zeros))

    def test_zstd_compresses(self):
        """Test that zstd actually reduces size on compressible data."""
        compressed = zmat.compress(self.zeros, method="zstd")
        self.assertLess(len(compressed), len(self.zeros))

    def test_blosc2blosclz_compresses(self):
        """Test that blosc2blosclz actually reduces size on compressible data."""
        compressed = zmat.compress(self.zeros, method="blosc2blosclz")
        self.assertLess(len(compressed), len(self.zeros))


class TestZmatLowLevel(unittest.TestCase):
    """Tests for the low-level zmat.zmat() interface."""

    def test_zmat_compress_decompress(self):
        """Test zmat() with iscompress=1 and iscompress=0."""
        data = b"test data for low-level interface" * 50
        compressed = zmat.zmat(data, iscompress=1, method="zlib")
        decompressed = zmat.zmat(compressed, iscompress=0, method="zlib")
        self.assertEqual(decompressed, data)

    def test_zmat_negative_level(self):
        """Test zmat() with negative compression level (higher compression)."""
        data = b"test compression levels" * 100
        c_default = zmat.zmat(data, iscompress=1, method="zlib")
        c_max = zmat.zmat(data, iscompress=-9, method="zlib")
        # both must round-trip correctly
        self.assertEqual(zmat.zmat(c_default, iscompress=0, method="zlib"), data)
        self.assertEqual(zmat.zmat(c_max, iscompress=0, method="zlib"), data)
        # higher compression should produce smaller or equal output
        self.assertLessEqual(len(c_max), len(c_default))

    def test_zmat_method_case_insensitive(self):
        """Test that method names are case-insensitive (mirrors zmat_keylookup)."""
        data = b"case insensitive test"
        c1 = zmat.compress(data, method="zlib")
        c2 = zmat.compress(data, method="ZLIB")
        c3 = zmat.compress(data, method="Zlib")
        # all should decompress to same data
        self.assertEqual(zmat.decompress(c1, method="zlib"), data)
        self.assertEqual(zmat.decompress(c2, method="zlib"), data)
        self.assertEqual(zmat.decompress(c3, method="zlib"), data)


class TestZmatErrors(unittest.TestCase):
    """Error handling tests (mirrors run_zmat_test.m error tests)."""

    def test_unsupported_method(self):
        """Test that unsupported method raises ValueError."""
        with self.assertRaises(ValueError) as ctx:
            zmat.compress(b"test", method="nosuchmethod")
        self.assertIn("unsupported", str(ctx.exception).lower())

    def test_decompress_invalid_zlib(self):
        """Test zlib decompression of invalid data raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            zmat.decompress(b"this is not zlib data", method="zlib")

    def test_decompress_invalid_gzip(self):
        """Test gzip decompression of invalid data raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            zmat.decompress(b"this is not gzip data", method="gzip")

    def test_decompress_invalid_lz4(self):
        """Test lz4 decompression of invalid data raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            zmat.decompress(b"this is not lz4 data at all!!", method="lz4")

    def test_decompress_invalid_lzma(self):
        """Test lzma decompression of invalid data raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            zmat.decompress(b"this is not lzma data", method="lzma")

    def test_decode_invalid_base64(self):
        """Test base64 decoding of invalid data raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            zmat.decode(b"!!!not-base64", method="base64")

    def test_wrong_method_decompress(self):
        """Test decompressing with wrong method raises RuntimeError
        (mirrors run_zmat_test.m wrong format tests)."""
        data = b"test wrong method" * 50
        compressed = zmat.compress(data, method="zlib")
        # try to decompress zlib data as lz4 — should fail
        with self.assertRaises(RuntimeError):
            zmat.decompress(compressed, method="lz4")

    def test_type_error_on_non_bytes(self):
        """Test that passing non-bytes input raises TypeError."""
        with self.assertRaises(TypeError):
            zmat.compress("this is a string, not bytes")
        with self.assertRaises(TypeError):
            zmat.compress(12345)
        with self.assertRaises(TypeError):
            zmat.compress([1, 2, 3])


class TestZmatLargeData(unittest.TestCase):
    """Tests with larger data (mirrors zmat_speedbench.m patterns)."""

    def _make_eye(self, n):
        """Create an n×n identity matrix as packed doubles."""
        rows = []
        for i in range(n):
            row = b"\x00" * (8 * i) + struct.pack("<d", 1.0) + b"\x00" * (8 * (n - i - 1))
            rows.append(row)
        return b"".join(rows)

    def _make_sequential(self, n):
        """Create n sequential uint32 values (like magic(n) data)."""
        return b"".join(struct.pack("<I", i) for i in range(n * n))

    def test_large_zeros(self):
        """Test compression of large zero-filled buffer (like zeros(500))."""
        data = b"\x00" * (500 * 500 * 8)  # 500x500 doubles
        for method in ["zlib", "gzip", "lz4", "lzma", "zstd", "blosc2blosclz"]:
            compressed = zmat.compress(data, method=method)
            self.assertLess(len(compressed), len(data), f"{method} did not compress zeros")
            decompressed = zmat.decompress(compressed, method=method)
            self.assertEqual(decompressed, data, f"{method} round-trip failed on zeros")

    def test_large_eye(self):
        """Test compression of identity matrix (like eye(500), mirrors speedbench)."""
        data = self._make_eye(500)
        for method in ["zlib", "gzip", "lz4", "lz4hc", "zstd", "blosc2lz4"]:
            compressed = zmat.compress(data, method=method)
            decompressed = zmat.decompress(compressed, method=method)
            self.assertEqual(decompressed, data, f"{method} round-trip failed on eye(500)")

    def test_large_sequential(self):
        """Test compression of sequential data (like magic(500))."""
        data = self._make_sequential(500)
        for method in ["zlib", "gzip", "lz4", "lzma", "zstd", "blosc2zstd"]:
            compressed = zmat.compress(data, method=method)
            decompressed = zmat.decompress(compressed, method=method)
            self.assertEqual(decompressed, data, f"{method} round-trip failed on sequential data")


class TestZmatBytearray(unittest.TestCase):
    """Test that bytearray input works (buffer protocol)."""

    def test_bytearray_compress(self):
        """Test compression of bytearray input."""
        data = bytearray(b"bytearray test data" * 50)
        compressed = zmat.compress(data, method="zlib")
        decompressed = zmat.decompress(compressed, method="zlib")
        self.assertEqual(decompressed, bytes(data))

    def test_bytearray_base64(self):
        """Test base64 encoding of bytearray input."""
        data = bytearray(b"bytearray base64 test")
        encoded = zmat.encode(data, method="base64")
        decoded = zmat.decode(encoded, method="base64")
        self.assertEqual(decoded, bytes(data))


class TestZmatInterop(unittest.TestCase):
    """Test interoperability with Python standard library."""

    def test_gzip_interop(self):
        """Test that zmat gzip output can be decompressed by Python's gzip module."""
        import gzip
        import io

        data = b"gzip interoperability test data" * 100
        compressed = zmat.compress(data, method="gzip")

        # decompress with Python's gzip
        with gzip.open(io.BytesIO(compressed), "rb") as f:
            decompressed = f.read()

        self.assertEqual(decompressed, data)

    def test_zlib_interop(self):
        """Test that zmat zlib output can be decompressed by Python's zlib module."""
        import zlib

        data = b"zlib interoperability test data" * 100
        compressed = zmat.compress(data, method="zlib")

        # decompress with Python's zlib
        decompressed = zlib.decompress(compressed)
        self.assertEqual(decompressed, data)

    def test_python_zlib_to_zmat(self):
        """Test that Python zlib compressed data can be decompressed by zmat."""
        import zlib

        data = b"reverse interoperability test" * 100
        compressed = zlib.compress(data)

        decompressed = zmat.decompress(compressed, method="zlib")
        self.assertEqual(decompressed, data)

    def test_python_gzip_to_zmat(self):
        """Test that Python gzip compressed data can be decompressed by zmat."""
        import gzip
        import io

        data = b"reverse gzip interop test" * 100
        buf = io.BytesIO()
        with gzip.open(buf, "wb") as f:
            f.write(data)
        compressed = buf.getvalue()

        decompressed = zmat.decompress(compressed, method="gzip")
        self.assertEqual(decompressed, data)

    def test_base64_interop(self):
        """Test that zmat base64 output matches Python's base64 module."""
        import base64

        data = b"base64 interop test data"
        encoded = zmat.encode(data, method="base64")

        # zmat base64 may include newlines; strip for comparison
        zmat_clean = encoded.replace(b"\n", b"")
        py_encoded = base64.b64encode(data)
        self.assertEqual(zmat_clean, py_encoded)


class TestZmatBenchmark(unittest.TestCase):
    """Simple benchmark tests (mirrors zmat_speedbench.m).
    These verify correctness rather than enforcing timing thresholds."""

    BENCH_SIZE = 200  # smaller than MATLAB's 2000 to keep tests fast

    def _benchmark_method(self, data, method):
        """Compress and decompress, return timing and sizes."""
        t0 = time.perf_counter()
        compressed = zmat.compress(data, method=method)
        t_compress = time.perf_counter() - t0

        t0 = time.perf_counter()
        decompressed = zmat.decompress(compressed, method=method)
        t_decompress = time.perf_counter() - t0

        self.assertEqual(decompressed, data, f"Benchmark round-trip failed for {method}")

        return {
            "method": method,
            "original": len(data),
            "compressed": len(compressed),
            "ratio": len(compressed) / len(data),
            "compress_time": t_compress,
            "decompress_time": t_decompress,
        }

    def test_benchmark_eye(self):
        """Benchmark on identity matrix data (like eye(200))."""
        n = self.BENCH_SIZE
        data = b""
        for i in range(n):
            row = b"\x00" * (8 * i) + struct.pack("<d", 1.0) + b"\x00" * (8 * (n - i - 1))
            data += row

        methods = [
            "zlib",
            "gzip",
            "lzma",
            "lz4",
            "lz4hc",
            "zstd",
            "blosc2blosclz",
            "blosc2lz4",
            "blosc2lz4hc",
            "blosc2zlib",
            "blosc2zstd",
        ]
        print(f"\n{'Method':<16} {'Size':>8} {'Ratio':>8} {'Comp(s)':>10} {'Decomp(s)':>10}")
        print("-" * 56)
        for m in methods:
            r = self._benchmark_method(data, m)
            print(
                f"{r['method']:<16} {r['compressed']:>8} {r['ratio']:>8.4f} "
                f"{r['compress_time']:>10.6f} {r['decompress_time']:>10.6f}"
            )

    def test_benchmark_text(self):
        """Benchmark on text data."""
        data = b"The quick brown fox jumps over the lazy dog. " * 2000

        methods = [
            "zlib",
            "gzip",
            "lzma",
            "lz4",
            "lz4hc",
            "zstd",
            "blosc2blosclz",
            "blosc2lz4",
            "blosc2lz4hc",
            "blosc2zlib",
            "blosc2zstd",
        ]
        print(f"\n{'Method':<16} {'Size':>8} {'Ratio':>8} {'Comp(s)':>10} {'Decomp(s)':>10}")
        print("-" * 56)
        for m in methods:
            r = self._benchmark_method(data, m)
            print(
                f"{r['method']:<16} {r['compressed']:>8} {r['ratio']:>8.4f} "
                f"{r['compress_time']:>10.6f} {r['decompress_time']:>10.6f}"
            )


if __name__ == "__main__":
    unittest.main()
