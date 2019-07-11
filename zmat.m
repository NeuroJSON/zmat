function varargout=zmat(varargin)
%
% output=zmat(input)
%    or
% [output, info]=zmat(input, iscompress, method)
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
%             if one defines iscompress as the info struct (2nd output of
%             zmat) during encoding, zmat will perform a
%             decoding/decompression operation and recover the original
%             input using the info stored in the info structure.
%      method: (optional) compression method, currently, zmat supports the below methods
%             'zlib': zlib/zip based data compression (default)
%             'gzip': gzip formatted data compression
%             'lzip': lzip formatted data compression
%             'lzma': lzma formatted data compression
%             'base64': encode or decode use base64 format
%
% output:
%      output: a uint8 row vector, storing the compressed or decompressed data
%      info: (optional) a struct storing additional info regarding the input data, may have
%            'type': the class of the input array
%            'size': the dimensions of the input array
%            'byte': the number of bytes per element in the input array
%            'status': the zlib function return value, including potential error codes (<0)
%
% example:
%
%   [ss, info]=zmat(eye(5))
%   orig=zmat(ss,0)
%   orig=zmat(ss,info)
%   ss=char(zmat('zmat test',1,'base64'))
%   orig=char(zmat(ss,0,'base64'))
%
% -- this function is part of the ZMAT toolbox (http://github.com/fangq/zmat)
%

if(exist('zipmat')~=3 && exist('zipmat')~=2)
    error('zipmat mex file is not found. you must download the mex file or recompile');
end

if(nargin==0)
    fprintf(1,'Usage:\n\t[output,info]=zmat(input,iscompress,method);\nPlease run "help zmat" for more details.\n');
    return;
end

input=varargin{1};
iscompress=1;
zipmethod='zlib';

if(~(ischar(input) || islogical(input) || (isnumeric(input) && isreal(input))))
    error('input must be a char, non-complex numeric or logical vector or N-D array');
end

if(ischar(input))
    input=uint8(input);
end

if(nargin>1)
    iscompress=varargin{2};
    if(isstruct(varargin{2}))
        inputinfo=varargin{2};
        iscompress=0;
        zipmethod=inputinfo.method;
    end
end

if(nargin>2)
    zipmethod=varargin{3};
end

[varargout{1:nargout}]=zipmat(input,iscompress,zipmethod);

if(exist('inputinfo','var') && isfield(inputinfo,'type'))
        varargout{1}=typecast(varargout{1},inputinfo.type);
        varargout{1}=reshape(varargout{1},inputinfo.size);
end