function varargout = zmat(varargin)
%
% output=zmat(input)
%    or
% [output, info]=zmat(input, iscompress, method)
% [output, info]=zmat(input, iscompress, method, options ...)
% output=zmat(input, info)
%
% A portable data compression/decompression toolbox for MATLAB/GNU Octave
%
% author: Qianqian Fang <q.fang at neu.edu>
% initial version created on 04/30/2019
%
% input:
%      input: a char, non-complex numeric or logical vector or array.
%             Sparse matrices, and Octave diagonal, permutation, and range
%             types are also accepted and stored in a compact representation.
%      iscompress: (optional) if iscompress is 1, zmat compresses/encodes the input,
%             if 0, it decompresses/decodes the input. Default value is 1.
%
%             if iscompress is set to a negative integer, (-iscompress) specifies
%             the compression level. For zlib/gzip, default level is 6 (1-9); for
%             lzma/lzip, default level is 5 (1-9); for lz4hc, default level is 8 (1-16).
%             the default compression level is used if iscompress is set to 1.
%
%             zmat removes the trailing newline when iscompress=2 and method='base64'
%             all newlines are kept when iscompress=3 and method='base64'
%
%             if one defines iscompress as the info struct (2nd output of zmat), zmat
%             will perform a decoding/decompression operation and recover the original
%             input using the info stored in the info structure.
%      method: (optional) compression method, currently, zmat supports the below methods
%             'zlib': zlib/zip based data compression (default)
%             'gzip': gzip formatted data compression
%             'lzip': lzip formatted data compression
%             'lzma': lzma formatted data compression
%             'lz4':  lz4 formatted data compression
%             'lz4hc':lz4hc (LZ4 with high-compression ratio) formatted data compression
%             'zstd':  zstd formatted data compression
%             'blosc2blosclz':  blosc2 meta-compressor with blosclz compression
%             'blosc2lz4':  blosc2 meta-compressor with lz4 compression
%             'blosc2lz4hc':  blosc2 meta-compressor with lz4hc compression
%             'blosc2zlib':  blosc2 meta-compressor with zlib/zip compression
%             'blosc2zstd':  blosc2 meta-compressor with zstd compression
%             'base64': encode or decode use base64 format
%     options: a series of ('name', value) pairs, supported options include
%             'nthread': followed by an integer specifying number of threads for blosc2 meta-compressors
%             'typesize': followed by an integer specifying the number of bytes per data element (used for shuffle)
%             'shuffle': shuffle methods in blosc2 meta-compressor, 0 disable, 1, byte-shuffle
%
% output:
%      output: a uint8 row vector, storing the compressed or decompressed data;
%             empty when an error is encountered. When decompressing with the info
%             struct, the original data type and dimensions are restored, including
%             special matrix types.
%      info: (optional) a struct storing additional info regarding the input data, may have
%            'type': the class of the input array
%            'size': the dimensions of the input array
%            'byte': the number of bytes per element in the input array
%            'method': a copy of the 3rd input indicating the encoding method
%            'status': the zlib/lzma/lz4 compression/decompression function return value,
%                    including potential error codes; see documentation of the respective
%                    libraries for details
%            'level': a copy of the iscompress flag; if non-zero, specifying compression
%                    level, see above
%            'matrixtype': (optional) one of 'diagonal', 'permutation', 'sparse', or
%                    'range' for special matrix types. Absent for regular dense arrays.
%                    - 'diagonal': Octave diagonal matrix (e.g. eye(N), diag(v)); only
%                      the diagonal elements are stored.
%                    - 'permutation': Octave permutation matrix; only the permutation
%                      index vector is stored as uint32.
%                    - 'range': Octave range object (e.g. 1:1000); stored as 3 doubles
%                      [start, step, numel] regardless of range length.
%                    - 'sparse': sparse matrix (both MATLAB and Octave); row/column
%                      indices stored as uint32 and values in the original type.
%            'matrixclass': (optional) original element class for diagonal and sparse types
%            'matrixsize': (optional) original [rows, cols] dimensions
%            'sparsecount': (optional) number of nonzero elements for sparse type
%
% example:
%
%   [ss, info]=zmat(eye(5))
%   orig=zmat(ss,0)
%   orig=zmat(ss,info)
%   ss=char(zmat('zmat test',1,'base64'))
%   orig=char(zmat(ss,0,'base64'))
%
%   % sparse matrix round-trip
%   A=sparse(eye(100));
%   [ss, info]=zmat(A);
%   A2=zmat(ss, info);
%   assert(nnz(A-A2)==0);
%
% -- this function is part of the ZMAT toolbox (https://github.com/NeuroJSON/zmat)
%

if (exist('zipmat') ~= 3 && exist('zipmat') ~= 2)
    error('zipmat mex file is not found. you must download the mex file or recompile');
end

if (nargin == 0)
    fprintf(1, 'Usage:\n\t[output,info]=zmat(input,iscompress,method);\nPlease run "help zmat" for more details.\n');
    return
end

input = varargin{1};
iscompress = 1;
zipmethod = 'zlib';

if (~(ischar(input) || islogical(input) || (isnumeric(input) && isreal(input))))
    error('input must be a char, non-complex numeric or logical vector or N-D array');
end

inputinfo = whos('input');
if (~isempty(input))
    typesize = inputinfo.bytes / numel(input);
else
    typesize = 0;
end

%% detect and handle special matrix types (Octave diagonal, permutation, range; sparse)
specialtype = '';
isoctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;

if (issparse(input))
    specialtype = 'sparse';
    [sprow, spcol, spval] = find(input);
    spsize = size(input);
elseif (isoctave)
    tinfo = typeinfo(input);
    if (any(strcmp(tinfo, {'diagonal matrix', 'complex diagonal matrix'})))
        specialtype = 'diagonal';
    elseif (strcmp(tinfo, 'permutation matrix'))
        specialtype = 'permutation';
    elseif (strcmp(tinfo, 'range'))
        specialtype = 'range';
    end
end

%% for special types, convert to a compact representation before compression
if (strcmp(specialtype, 'diagonal'))
    % store only the diagonal vector — N elements instead of N*N
    origsize = size(input);
    origclass = class(input);
    input = diag(input);  % extract diagonal as a vector
    inputinfo = whos('input');
    if (~isempty(input))
        typesize = inputinfo.bytes / numel(input);
    end
elseif (strcmp(specialtype, 'permutation'))
    % store only the permutation index vector (1-based uint32 indices)
    origsize = size(input);
    input = full(input);           % convert to dense first for portability
    [~, pidxvec] = max(input, [], 2);  % extract permutation vector
    input = uint32(pidxvec);
    inputinfo = whos('input');
    if (~isempty(input))
        typesize = inputinfo.bytes / numel(input);
    end
elseif (strcmp(specialtype, 'range'))
    % store as [start, step, numel] — 3 doubles regardless of range length
    origsize = size(input);
    rstart = input(1);
    if (numel(input) > 1)
        rstep = input(2) - input(1);
    else
        rstep = 1;
    end
    rcount = numel(input);
    input = [rstart, rstep, rcount];
    inputinfo = whos('input');
    if (~isempty(input))
        typesize = inputinfo.bytes / numel(input);
    end
elseif (strcmp(specialtype, 'sparse'))
    % store [rows, cols, vals] compactly: indices as uint32, vals in original type
    origsize = spsize;
    origclass = class(spval);
    input = uint8([typecast(uint32(sprow(:)'), 'uint8'), ...
                   typecast(uint32(spcol(:)'), 'uint8'), ...
                   typecast(spval(:)', 'uint8')]);
    inputinfo = whos('input');
    typesize = 1;
end

if (ischar(input))
    input = uint8(input);
end

if (nargin > 1)
    iscompress = varargin{2};
    if (isstruct(varargin{2}))
        inputinfo = varargin{2};
        iscompress = 0;
        zipmethod = inputinfo.method;
    end
end

if (nargin > 2)
    zipmethod = varargin{3};
end

opt = struct;
if (nargin > 4 && ischar(varargin{4}) && bitand(length(varargin), 1) == 1)
    opt = cell2struct(varargin(5:2:end), varargin(4:2:end), 2);
end

shuffle = 0;
if (strfind(zipmethod, 'blosc2'))
    shuffle = 1;
end

nthread = getoption('nthread', 1, opt);
shuffle = getoption('shuffle', shuffle, opt);
typesize = getoption('typesize', typesize, opt);

iscompress = round(iscompress);

if ((strcmp(zipmethod, 'zlib') || strcmp(zipmethod, 'gzip')) && iscompress <= -10)
    iscompress = -9;
end

[varargout{1:max(1, nargout)}] = zipmat(input, iscompress, zipmethod, nthread, shuffle, typesize);

%% store special matrix type info in the output info struct
if (nargout > 1 && ~isempty(specialtype))
    varargout{2}.matrixtype = specialtype;
    if (strcmp(specialtype, 'diagonal') || strcmp(specialtype, 'sparse'))
        varargout{2}.matrixclass = origclass;
        varargout{2}.matrixsize = origsize;
    elseif (strcmp(specialtype, 'permutation'))
        varargout{2}.matrixsize = origsize;
    elseif (strcmp(specialtype, 'range'))
        varargout{2}.matrixsize = origsize;
    end
    if (strcmp(specialtype, 'sparse'))
        varargout{2}.sparsecount = length(sprow);
    end
end

if (strcmp(zipmethod, 'base64') && iscompress > 1)
    varargout{1} = char(varargout{1});
end

if (exist('inputinfo', 'var') && isfield(inputinfo, 'type'))
    %% fix for Octave 10+ where mxGetClassName returns empty for special types
    if (isempty(inputinfo.type) && isfield(inputinfo, 'class'))
        inputinfo.type = inputinfo.class;
    end

    %% restore special matrix types
    if (isfield(inputinfo, 'matrixtype'))
        if (strcmp(inputinfo.matrixtype, 'diagonal'))
            dvec = typecast(varargout{1}, inputinfo.matrixclass);
            varargout{1} = diag(dvec);
            ms = inputinfo.matrixsize;
            if (size(varargout{1}, 1) ~= ms(1) || size(varargout{1}, 2) ~= ms(2))
                tmp = zeros(ms, inputinfo.matrixclass);
                n = min(length(dvec), min(ms));
                for k = 1:n
                    tmp(k, k) = dvec(k);
                end
                varargout{1} = tmp;
            end
            return
        elseif (strcmp(inputinfo.matrixtype, 'permutation'))
            pidx = double(typecast(varargout{1}, 'uint32'));
            n = inputinfo.matrixsize(1);
            varargout{1} = zeros(n, n);
            for k = 1:n
                varargout{1}(k, pidx(k)) = 1;
            end
            return
        elseif (strcmp(inputinfo.matrixtype, 'range'))
            rparams = typecast(varargout{1}, 'double');
            rstart = rparams(1);
            rstep  = rparams(2);
            rcount = round(rparams(3));
            varargout{1} = rstart:rstep:(rstart + rstep * (rcount - 1));
            if (isfield(inputinfo, 'matrixsize') && ~isequal(size(varargout{1}), inputinfo.matrixsize))
                varargout{1} = reshape(varargout{1}, inputinfo.matrixsize);
            end
            return
        elseif (strcmp(inputinfo.matrixtype, 'sparse'))
            rawbytes = varargout{1};
            nnzcount = inputinfo.sparsecount;
            idx_bytes = nnzcount * 4;  % uint32 per index
            ridx = typecast(rawbytes(1:idx_bytes), 'uint32');
            cidx = typecast(rawbytes(idx_bytes + 1:2 * idx_bytes), 'uint32');
            valbytes = rawbytes(2 * idx_bytes + 1:end);
            vals = typecast(valbytes, inputinfo.matrixclass);
            varargout{1} = sparse(double(ridx), double(cidx), vals, ...
                                  inputinfo.matrixsize(1), inputinfo.matrixsize(2));
            return
        end
    end

    if (strcmp(inputinfo.type, 'logical'))
        varargout{1} = logical(varargout{1});
    else
        varargout{1} = typecast(varargout{1}, inputinfo.type);
    end
    varargout{1} = reshape(varargout{1}, inputinfo.size);
end

% --------------------------------------------------------------------------------

function value = getoption(key, default, opt)
value = default;
if (isfield(opt, key))
    value = opt.(key);
end
