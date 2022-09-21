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
%      input: a char, non-complex numeric or logical vector or array
%      iscompress: (optional) if iscompress is 1, zmat compresses/encodes the input,
%             if 0, it decompresses/decodes the input. Default value is 1.
%
%             if iscompress is set to a negative integer, (-iscompress) specifies
%             the compression level. For zlib/gzip, default level is 6 (1-9); for
%             lzma/lzip, default level is 5 (1-9); for lz4hc, default level is 8 (1-16).
%             the default compression level is used if iscompress is set to 1.
%
%             zmat removes the trailing newline when iscompress=2 and method='base64'
%             all newlines are removed when iscompress=3 and method='base64'
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
%             'blosc2zlib:  blosc2 meta-compressor with zlib/zip compression
%             'blosc2zstd':  blosc2 meta-compressor with zstd compression
%             'base64': encode or decode use base64 format
%     options: a series of ('name', value) pairs, supported options include
%             'nthread': followed by an integer specifying number of threads for blosc2 meta-compressors
%             'typesize': followed by an integer specifying the number of bytes per data element (used for shuffle)
%             'shuffle': shuffle methods in blosc2 meta-compressor, 0 disable, 1, byte-shuffle
%
% output:
%      output: a uint8 row vector, storing the compressed or decompressed data;
%             empty when an error is encountered
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
%
% example:
%
%   [ss, info]=zmat(eye(5))
%   orig=zmat(ss,0)
%   orig=zmat(ss,info)
%   ss=char(zmat('zmat test',1,'base64'))
%   orig=char(zmat(ss,0,'base64'))
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

inputinfo=whos('input');
if(~isempty(input))
    typesize=inputinfo.bytes/numel(input);
else
    typesize=0;
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

opt=struct;
if (nargin > 4 && ischar(varargin{4}) && bitand(length(varargin), 1)==1)
    opt=cell2struct(varargin(5:2:end), varargin(4:2:end), 2);
end

shuffle=0;
if(strfind(zipmethod,'blosc2'))
    shuffle=1;
end

nthread=getoption('nthread', 1, opt);
shuffle=getoption('shuffle', shuffle, opt);
typesize=getoption('typesize', typesize, opt);

iscompress = round(iscompress);

if ((strcmp(zipmethod, 'zlib') || strcmp(zipmethod, 'gzip')) && iscompress <= -10)
    iscompress = -9;
end

[varargout{1:max(1, nargout)}] = zipmat(input, iscompress, zipmethod, nthread, shuffle, typesize);

if (strcmp(zipmethod, 'base64') && iscompress > 1)
    varargout{1} = char(varargout{1});
end

if (exist('inputinfo', 'var') && isfield(inputinfo, 'type'))
    if(strcmp(inputinfo.type,'logical'))
        varargout{1} = logical(varargout{1});
    else
        varargout{1} = typecast(varargout{1}, inputinfo.type);
    end
    varargout{1} = reshape(varargout{1}, inputinfo.size);
end

%--------------------------------------------------------------------------------

function value=getoption(key, default, opt)
value=default;
if(isfield(opt, key))
    value=opt.(key);
end