function test_zmat_roundtrip(testname, input, tol, method)
%
% test_zmat_roundtrip(testname, input)
% test_zmat_roundtrip(testname, input, tol)
% test_zmat_roundtrip(testname, input, tol, method)
%
% Compress and decompress input via zmat, verify the round-trip matches.
% Handles sparse, diagonal, permutation, range, and dense arrays.
%
% input:
%      testname: descriptive name for this test
%      input: the original data to compress and restore
%      tol: (optional) tolerance for floating-point comparison, default 0 (exact)
%      method: (optional) compression method, default 'zlib'
%

if (nargin < 3 || isempty(tol))
    tol = 0;
end

if (nargin < 4 || isempty(method))
    method = 'zlib';
end

try
    [compressed, info] = zmat(input, 1, method);
    restored = zmat(compressed, info);
catch ME
    warning('Test %s: failed with error: %s', testname, ME.message);
    return
end

%% compare original and restored
passed = false;

if (issparse(input))
    if (issparse(restored) && isequal(size(input), size(restored)))
        if (tol == 0)
            passed = (nnz(input - restored) == 0);
        else
            passed = (max(max(abs(input - restored))) <= tol);
        end
    end
else
    origfull = full(input);
    restfull = full(restored);
    if (isequal(size(origfull), size(restfull)))
        if (tol == 0)
            if (exist('isequaln', 'builtin') || exist('isequaln'))
                passed = isequaln(origfull, restfull);
            else
                passed = isequal(origfull, restfull);
            end
        else
            passed = (max(abs(origfull(:) - restfull(:))) <= tol);
        end
    end
end

if (passed)
    fprintf(1, 'Testing %s: ok\n\tcompressed: %d bytes, info.method: %s\n', ...
            testname, numel(compressed), info.method);
else
    warning('Test %s: failed: round-trip mismatch (sizes: [%s] vs [%s])', ...
            testname, mat2str(size(input)), mat2str(size(restored)));
end
