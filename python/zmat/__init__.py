"""
zmat — A portable data compression/decompression library.

Re-exports the C backend (_zmat) and adds NumPy-aware helpers that
capture and restore array dtype, shape, and memory order alongside
compressed data, mirroring the ``[output, info] = zmat(input)`` /
``output = zmat(input, info)`` pattern from the MATLAB/Octave toolbox.

Basic API (bytes in, bytes out — no NumPy dependency):
    zmat.compress(data, method='zlib', level=1)
    zmat.decompress(data, method='zlib')
    zmat.encode(data, method='base64')
    zmat.decode(data, method='base64')
    zmat.zmat(data, iscompress=1, method='zlib', ...)   # low-level

NumPy-aware API:
    compressed, info = zmat.compress(arr, info=True)
    restored_arr     = zmat.decompress(compressed, info=info)

    compressed, info = zmat.zmat(arr, info=True)          # low-level with info
    restored_arr     = zmat.zmat(compressed, info=info)   # low-level restore
"""

from _zmat import compress as _compress
from _zmat import decode
from _zmat import decompress as _decompress
from _zmat import encode
from _zmat import zmat as _zmat_c

__all__ = ["compress", "decompress", "encode", "decode", "zmat"]

__version__ = "1.1.0"

def _byte_shuffle(data_bytes, typesize):
    """Regroup bytes by position within each element (byte-shuffle filter).

    Transforms [b0_e1 b1_e1 ... b0_e2 b1_e2 ...] into
    [b0_e1 b0_e2 ... b1_e1 b1_e2 ...], improving compressibility of
    structured numerical arrays with any codec.
    """
    import numpy as np
    arr = np.frombuffer(data_bytes, dtype=np.uint8).reshape(-1, typesize)
    return arr.flatten(order='F').tobytes()


def _byte_unshuffle(data_bytes, typesize):
    """Reverse of _byte_shuffle."""
    import numpy as np
    nelems = len(data_bytes) // typesize
    arr = np.frombuffer(data_bytes, dtype=np.uint8).reshape(typesize, nelems)
    return arr.flatten(order='F').tobytes()

def compress(data, method="zlib", level=1, info=False, shuffle=0):
    """Compress *data* using the requested algorithm.

    Parameters
    ----------
    data : bytes, bytearray, memoryview, or numpy.ndarray
        Input to compress.  Any object that supports the buffer protocol
        is accepted.  When *data* is a :class:`numpy.ndarray` **and**
        *info=True*, the array metadata (dtype, shape, memory order) is
        captured so that :func:`decompress` can reconstruct the original
        array exactly.
    method : str, optional
        Compression algorithm.  One of ``'zlib'`` (default), ``'gzip'``,
        ``'lzma'``, ``'lzip'``, ``'lz4'``, ``'lz4hc'``, ``'zstd'``,
        ``'base64'``, ``'blosc2blosclz'``, ``'blosc2lz4'``,
        ``'blosc2lz4hc'``, ``'blosc2zlib'``, ``'blosc2zstd'``.
    level : int, optional
        Compression level: ``1`` = library default, higher values give
        better compression at the cost of speed.
    info : bool, optional
        When *True* and *data* is a :class:`numpy.ndarray`, return a
        ``(compressed_bytes, info_dict)`` tuple instead of plain bytes.
        The *info_dict* mirrors the MATLAB ``info`` struct and contains:

        - ``'type'``   — NumPy dtype string (e.g. ``'float64'``)
        - ``'shape'``  — tuple of array dimensions
        - ``'byte'``   — bytes per element (``data.itemsize``)
        - ``'method'`` — the compression method used
        - ``'order'``  — ``'F'`` for Fortran-contiguous, ``'C'`` otherwise
        - ``'shuffle'``— byte-shuffle level applied (0 = none, 1 = byte)
        - ``'typesize'``— element size in bytes used for shuffle

        When *info=True* but *data* is not an ndarray, the tuple
        ``(compressed_bytes, None)`` is returned so callers can always
        unpack two values.
    shuffle : int, optional
        Byte-shuffle level for non-blosc2 codecs (default ``0`` = disabled,
        ``1`` = byte-shuffle).  Requires *info=True* so that the shuffle
        state can be recorded and reversed on decompression.  For blosc2
        methods the shuffle is handled by the C layer and this parameter
        is ignored.  Has no effect when *data* is not a
        :class:`numpy.ndarray`.

    Returns
    -------
    bytes
        Compressed data (when *info=False*).
    tuple[bytes, dict | None]
        ``(compressed_bytes, info_dict)`` when *info=True*.

    Examples
    --------
    Basic bytes round-trip::

        compressed = zmat.compress(b"hello " * 1000)
        original   = zmat.decompress(compressed)

    NumPy array round-trip with byte-shuffle::

        import numpy as np
        arr = np.random.rand(100, 100)
        compressed, info = zmat.compress(arr, method='lz4', info=True, shuffle=1)
        restored = zmat.decompress(compressed, info=info)
        assert np.array_equal(restored, arr)
    """
    _use_shuffle = (shuffle > 0 and "blosc2" not in method and method != "base64")

    if info:
        try:
            import numpy as np

            if isinstance(data, np.ndarray):
                order = "F" if np.isfortran(data) else "C"
                ts = data.itemsize
                apply_shuffle = _use_shuffle and ts > 1
                arr_info = {
                    "type": str(data.dtype),
                    "shape": tuple(data.shape),
                    "byte": ts,
                    "method": method,
                    "order": order,
                    "shuffle": shuffle if apply_shuffle else 0,
                    "typesize": ts,
                }
                flat = np.ascontiguousarray(data).tobytes()
                if apply_shuffle:
                    flat = _byte_shuffle(flat, ts)
                compressed = _compress(flat, method=method, level=level)
                return compressed, arr_info
        except ImportError:
            pass

        # non-ndarray with info=True: compress normally, return (bytes, None)
        return _compress(data, method=method, level=level), None

    return _compress(data, method=method, level=level)


def decompress(data, method="zlib", info=None):
    """Decompress *data*.

    Parameters
    ----------
    data : bytes or bytearray
        Compressed input.
    method : str, optional
        Compression algorithm (default ``'zlib'``).  Ignored when *info*
        is provided — the method stored in ``info['method']`` is used
        instead, matching the MATLAB toolbox behaviour.
    info : dict or None, optional
        Info dict returned by :func:`compress` with ``info=True``.
        When provided, the raw decompressed bytes are reinterpreted as a
        :class:`numpy.ndarray` with the original dtype, shape, and memory
        order.  If NumPy is not installed the raw ``bytes`` are returned.

    Returns
    -------
    bytes
        Decompressed data (when *info* is ``None`` or NumPy is absent).
    numpy.ndarray
        Restored array with original dtype and shape (when *info* is given
        and NumPy is available).

    Examples
    --------
    ::

        import numpy as np
        arr = np.eye(50, dtype=np.float32)
        compressed, info = zmat.compress(arr, method='lz4', info=True)
        restored = zmat.decompress(compressed, info=info)
        assert restored.dtype == arr.dtype
        assert restored.shape == arr.shape
        assert np.array_equal(restored, arr)
    """
    if info is not None:
        actual_method = info.get("method", method)
        raw = _decompress(data, method=actual_method)

        # unshuffle if compression applied wrapper-level byte shuffle
        shuf = info.get("shuffle", 0)
        ts   = info.get("typesize", 1)
        if shuf > 0 and ts > 1 and "blosc2" not in actual_method:
            raw = _byte_unshuffle(raw, ts)

        try:
            import numpy as np

            dtype = np.dtype(info["type"])
            shape = tuple(info["shape"])
            order = info.get("order", "C")
            # frombuffer on immutable bytes is read-only; .copy() makes it writable
            arr = np.frombuffer(raw, dtype=dtype).copy().reshape(shape)
            if order == "F":
                arr = np.asfortranarray(arr)
            return arr
        except ImportError:
            return raw

    return _decompress(data, method=method)


def zmat(data, iscompress=1, method="zlib", nthread=1, shuffle=1, typesize=4, info=False):
    """Low-level compression/decompression interface with full parameter control.

    Mirrors the MATLAB ``[ss, info] = zmat(arr)`` / ``zmat(ss, info)`` pattern
    when *info* is used.

    Parameters
    ----------
    data : bytes, bytearray, buffer-protocol object, or numpy.ndarray
        Input data.
    iscompress : int
        ``1`` to compress at the default level (default), ``0`` to
        decompress, or a negative integer to set the compression level
        (e.g. ``-9`` for maximum compression).  Ignored when *info* is a
        dict — decompression is performed automatically in that case.
    method : str
        Compression algorithm (default ``'zlib'``).
    nthread : int
        Thread count for blosc2 codecs (default ``1``).
    shuffle : int
        Byte-shuffle flag for blosc2: ``0`` = disabled, ``1`` = enabled
        (default ``1``).
    typesize : int
        Element byte size used by the blosc2 byte-shuffle filter
        (default ``4``).
    info : bool or dict, optional
        * ``False`` (default) — plain bytes in, bytes out.
        * ``True`` — when *data* is a :class:`numpy.ndarray`, capture its
          dtype, shape, and memory order and return ``(compressed_bytes,
          info_dict)``.  When *data* is not an ndarray, returns
          ``(compressed_bytes, None)``.
        * ``dict`` — treat as the info dict previously returned by this
          function or by :func:`compress`.  Decompresses *data* and
          reconstructs the original :class:`numpy.ndarray` using the
          stored metadata.  The method is taken from ``info['method']``;
          the *method* argument is used only as a fallback.

    Returns
    -------
    bytes
        Compressed or decompressed data when *info* is ``False``.
    tuple[bytes, dict | None]
        ``(compressed_bytes, info_dict)`` when *info=True*.
    numpy.ndarray
        Reconstructed array when *info* is a dict and NumPy is available.

    Examples
    --------
    NumPy array round-trip::

        import numpy as np
        arr = np.random.rand(100, 100).astype(np.float32)
        compressed, info = zmat.zmat(arr, info=True)
        restored = zmat.zmat(compressed, info=info)
        assert np.array_equal(restored, arr)

    blosc2 with multi-threading::

        out = zmat.zmat(data, iscompress=1, method='blosc2zstd',
                        nthread=4, shuffle=1, typesize=8)
    """
    _use_shuffle = (shuffle > 0 and "blosc2" not in method and method != "base64")

    # info dict supplied → decompress and reconstruct numpy array
    if isinstance(info, dict):
        actual_method = info.get("method", method)
        # blosc2 shuffle is handled by the C layer; pass it through unchanged
        c_shuffle = shuffle if "blosc2" in actual_method else 0
        raw = _zmat_c(data, iscompress=0, method=actual_method,
                      nthread=nthread, shuffle=c_shuffle, typesize=typesize)

        # unshuffle if wrapper-level shuffle was recorded in info
        shuf = info.get("shuffle", 0)
        ts   = info.get("typesize", 1)
        if shuf > 0 and ts > 1 and "blosc2" not in actual_method:
            raw = _byte_unshuffle(raw, ts)

        try:
            import numpy as np

            dtype = np.dtype(info["type"])
            shape = tuple(info["shape"])
            order = info.get("order", "C")
            arr = np.frombuffer(raw, dtype=dtype).copy().reshape(shape)
            if order == "F":
                arr = np.asfortranarray(arr)
            return arr
        except ImportError:
            return raw

    # info=True → capture numpy array metadata before compressing
    if info is True:
        try:
            import numpy as np

            if isinstance(data, np.ndarray):
                order = "F" if np.isfortran(data) else "C"
                ts = data.itemsize
                apply_shuffle = _use_shuffle and ts > 1
                arr_info = {
                    "type": str(data.dtype),
                    "shape": tuple(data.shape),
                    "byte": ts,
                    "method": method,
                    "order": order,
                    "shuffle": shuffle if apply_shuffle else 0,
                    "typesize": ts,
                }
                flat = np.ascontiguousarray(data).tobytes()
                if apply_shuffle:
                    flat = _byte_shuffle(flat, ts)
                # for blosc2, pass shuffle/typesize to C; for others, already done
                c_shuffle  = shuffle if "blosc2" in method else 0
                c_typesize = typesize if "blosc2" in method else 1
                compressed = _zmat_c(flat, iscompress=iscompress, method=method,
                                     nthread=nthread, shuffle=c_shuffle, typesize=c_typesize)
                return compressed, arr_info
        except ImportError:
            pass

        # non-ndarray with info=True: compress normally, return (bytes, None)
        return _zmat_c(data, iscompress=iscompress, method=method,
                       nthread=nthread, shuffle=shuffle, typesize=typesize), None

    return _zmat_c(
        data,
        iscompress=iscompress,
        method=method,
        nthread=nthread,
        shuffle=shuffle,
        typesize=typesize,
    )
