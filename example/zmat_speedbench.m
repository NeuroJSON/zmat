function zmat_speedbench(varargin)
%
% Usage: 
%     zmat_speedbench
%     zmat_speedbench('nthread', 8, 'shuffle', 1, 'typesize', 8)
%
% Author: Qianqian Fang <q.fang at neu.edu>
%

codecs={'zlib', 'gzip', 'lzma', 'lz4', 'lz4hc', 'zstd', ...
        'blosc2blosclz', 'blosc2lz4', 'blosc2lz4hc', ...
        'blosc2zlib', 'blosc2zstd'};

runbench('1. eye(2000)', eye(2000), codecs, varargin{:});
runbench('2. rand(2000)', rand(2000), codecs, varargin{:});
runbench('3. magic(2000)', uint32(magic(2000)), codecs, varargin{:});
runbench('4. peaks(2000)', single(peaks(2000)), codecs, varargin{:});

%----------------------------------------------------------
function runbench(name, mat, codecs, varargin)
disp(name)
res=cellfun(@(x) benchmark(x, mat, varargin{:}), codecs, 'UniformOutput', false);
if(exist('OCTAVE_VERSION','builtin'))
  disp(res)
else
  res=sortrows(struct2table(cell2mat(res)),'total')
end

%----------------------------------------------------------
function res=benchmark(codec, x, varargin)
tic;
[a,info]=zmat(x, 1, codec, varargin{:});
res.codec=codec;
res.save=toc;
res.size=uint32(numel(a));
tic;
b=zmat(a, info);
res.load=toc;
res.total=res.load+res.save;
res.sum=sum(b(:));
