function run_zmat_test(tests)
%
% run_zmat_test
%   or
% run_zmat_test(tests)
% run_zmat_test({'c','d','err'})
%
% Unit testing for ZMat toolbox
%
% authors:Qianqian Fang (q.fang <at> neu.edu)
% date: 2022/08/19
%
% input:
%      tests: is a cell array of strings, possible elements include
%         'c':  compression
%         'd':  decompression
%         'err':  error handling
%
% license:
%     BSD or GPL version 3, see LICENSE_{BSD,GPLv3}.txt files for details
%
% -- this function is part of ZMat toolbox (https://github.com/NeuroJSON/zmat)
%

if (nargin == 0)
    tests = {'c', 'd', 'err'};
end

%%
if (ismember('c', tests))
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));
    fprintf('Test compression\n');
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));

    test_zmat('zlib (empty)', 'zlib', [], zeros(1,0));
    test_zmat('gzip (empty)', 'gzip', '', zeros(1,0));
    test_zmat('lzma (empty)', 'lzma', zeros(0,0), zeros(1,0));
    test_zmat('lzip (empty)', 'lzip', [], zeros(1,0));
    test_zmat('lz4 (empty)', 'lz4', '', zeros(1,0));
    test_zmat('lz4hc (empty)', 'lz4hc', zeros(0,0), zeros(1,0));
    test_zmat('base64 (empty)', 'base64', [], zeros(1,0));

    isminiz=zmat('0',1,'gzip');
    isminiz=(isminiz(10)==255);
    if(isminiz)
        test_zmat('zlib (scalar)', 'zlib', pi, [120 1 1 8 0 247 255 24 45 68 84 251 33 9 64 9 224 2 67]);
        test_zmat('gzip (scalar)', 'gzip', 'test gzip', [31 139 8 0 0 0 0 0 0 0 1 9 0 246 255 116 101 115 116 32 103 122 105 112 35 1 18 68 9 0 0 0]);
    else
        test_zmat('zlib (scalar)', 'zlib', pi, [120 156 147 208 117 9 249 173 200 233 0 0 9 224 2 67]);
        test_zmat('gzip (scalar)', 'gzip', 'test gzip', [31 139 8 0 0 0 0 0 0 0 43 73 45 46 81 72 175 202 44 0 0 35 1 18 68 9 0 0 0]);
    end
    test_zmat('lzma (scalar)', 'lzma', uint32(1902), [93 0 0 16 0 4 0 0 0 0 0 0 0 0 55 1 188 0 10 215 98 63 255 251 13 160 0]);
    test_zmat('lzip (scalar)', 'lzip', single(89.8901), [76 90 73 80 0 20 0 93 177 210 100 7 58 15 255 255 252 63 0 0 133 75 237 40 4 0 0 0 0 0 0 0]);
    test_zmat('lz4 (scalar)', 'lz4', 2.71828, [128 144 247 170 149 9 191 5 64]);
    test_zmat('lz4hc (scalar)', 'lz4hc', 0.0, [128 0 0 0 0 0 0 0 0]);
    test_zmat('zstd (scalar)', 'base64', zmat(uint8(198),1,'zstd'), 'KLUv/SABCQAAxg==', 'level', 2);
    test_zmat('blosc2blosclz (scalar)', 'base64', zmat(uint8(201),1,'blosc2blosclz'), 'BQEHAQEAAAABAAAAIQAAAAAAAAAAAAAAAAAAAAAAAADJ', 'level', 2);
    test_zmat('blosc2lz4 (scalar)', 'base64', zmat(single(202),1,'blosc2lz4'), 'BQEHBAQAAAAEAAAAJAAAAAAAAAAAAQEAAAAAAAAAAAAAAEpD', 'level', 2);
    test_zmat('blosc2lz4hc (scalar)', 'base64', zmat(uint32(58392),1,'blosc2lz4hc'), 'BQEHBAQAAAAEAAAAJAAAAAAAAAAAAQIAAAAAAAAAAAAY5AAA', 'level', 2);
    if(~isminiz)
        test_zmat('blosc2zlib (scalar)', 'base64', zmat(2.2,1,'blosc2zlib'), 'BQEHCAgAAAAIAAAAKAAAAAAAAAAAAQQAAAAAAAAAAACamZmZmZkBQA==', 'level', 2);
    end
    test_zmat('blosc2zstd (scalar)', 'base64', zmat(logical(0.1),1,'blosc2zstd'), 'BQEHAQEAAAABAAAAIQAAAAAAAAAAAAUAAAAAAAAAAAAB', 'level', 2);
    test_zmat('base64 (scalar)', 'base64', uint8(100), [90 65 61 61]);

    if(isminiz)
        test_zmat('zlib (array)', 'zlib', uint8([1,2,3]), [120 1 1 3 0 252 255 1 2 3 0 13 0 7]);
        test_zmat('gzip (array)', 'gzip', single([pi;exp(1)]), [31 139 8 0 0 0 0 0 0 0 1 8 0 247 255 219 15 73 64 84 248 45 64 197 103 247 17 8 0 0 0]);
    else
        test_zmat('zlib (array)', 'zlib', uint8([1,2,3]), [120 156 99 100 98 6 0 0 13 0 7]);
        test_zmat('gzip (array)', 'gzip', single([pi;exp(1)]), [31 139 8 0 0 0 0 0 0 0 187 205 239 233 16 242 67 215 1 0 197 103 247 17 8 0 0 0]);
    end
    test_zmat('lzma (array)', 'lzma', uint8(magic(3)), [93 0 0 16 0 9 0 0 0 0 0 0 0 0 4 0 207 17 232 198 252 139 53 45 235 13 99 255 249 133 192 0]);
    test_zmat('lzip (array)', 'lzip', uint8(reshape(1:(2*3*4), [3,2,4])), [76 90 73 80 0 20 0 0 128 157 97 211 13 93 174 25 62 219 132 40 29 52 41 93 234 35 61 128 60 72 152 87 41 88 255 253 203 224 0 163 16 142 146 24 0 0 0 0 0 0 0]);
    test_zmat('lz4 (array)', 'lz4', [1], [128 0 0 0 0 0 0 240 63]);
    test_zmat('lz4hc (array)', 'lz4hc', 'test zmat', [144 116 101 115 116 32 122 109 97 116]);
    test_zmat('zstd (array)', 'base64', zmat(uint8(magic(5)),1,'zstd'), 'KLUv/SAZyQAAERcECgsYBQYMEgEHDRMZCA4UFQIPEBYDCQ==');
    test_zmat('blosc2blosclz (array)', 'base64', zmat(uint8(magic(4)),1,'blosc2blosclz'), 'BQEHARAAAAAQAAAAMAAAAAAAAAAAAAAAAAAAAAAAAAAQBQkEAgsHDgMKBg8NCAwB');
    test_zmat('blosc2lz4 (array)', 'base64', zmat(uint16(magic(3)),1,'blosc2lz4'), 'BQEHAhIAAAASAAAAMgAAAAAAAAAAAQEAAAAAAAAAAAAIAAMABAABAAUACQAGAAcAAgA=');
    test_zmat('blosc2lz4hc (array)', 'base64', zmat([1.1,2.1,3.1],1,'blosc2lz4hc'), 'BQEHCBgAAAAYAAAAOAAAAAAAAAAAAQIAAAAAAAAAAACamZmZmZnxP83MzMzMzABAzczMzMzMCEA=');
    if(~isminiz)
        test_zmat('blosc2zlib (array)', 'base64', zmat(uint8(reshape(1:(2*3*4), [3,2,4])),1,'blosc2zlib'), 'BQEHARgAAAAYAAAAOAAAAAAAAAAAAAQAAAAAAAAAAAABAgMEBQYHCAkKCwwNDg8QERITFBUWFxg=');
    end
    test_zmat('blosc2zstd (array)', 'base64', zmat(uint8(ones(2,3,4)),1,'blosc2zstd'), 'BQEHARgAAAAYAAAAOAAAAAAAAAAAAAUAAAAAAAAAAAABAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=');
    test_zmat('base64 (array)', 'base64', ['test';'zmat'], [100 72 112 108 98 88 78 104 100 72 81 61]);

    if(isminiz)
        test_zmat('zlib (level=9)', 'zlib', 55, [120 1 1 8 0 247 255 0 0 0 0 0 128 75 64 2 94 1 12], 'level', -9);
        test_zmat('zlib (level=2.6)', 'zlib', 55, [120 1 1 8 0 247 255 0 0 0 0 0 128 75 64 2 94 1 12], 'level', -2.6);
        test_zmat('gzip (level)', 'gzip', 'level 9', [31 139 8 0 0 0 0 0 0 0 1 7 0 248 255 108 101 118 101 108 32 57 182 235 101 120 7 0 0 0], 'level', -9);
    else
        test_zmat('zlib (level=9)', 'zlib', 55, [120 218 99 96 0 130 6 111 7 0 2 94 1 12], 'level', -9);
        test_zmat('zlib (level=2.6)', 'zlib', 55, [120 94 99 96 0 130 6 111 7 0 2 94 1 12], 'level', -2.6);
        test_zmat('gzip (level)', 'gzip', 'level 9', [31 139 8 0 0 0 0 0 2 0 203 73 45 75 205 81 176 4 0 182 235 101 120 7 0 0 0], 'level', -9);
    end
    test_zmat('lzma (level)', 'lzma', uint8([1,2,3,4]), [93 0 0 16 0 4 0 0 0 0 0 0 0 0 0 128 157 97 229 167 24 31 255 247 52 128 0], 'level', -9);
    test_zmat('lzip (level)', 'lzip', logical([1,2,3,4]), [76 90 73 80 0 20 0 0 232 190 92 247 255 255 224 0 128 0 153 211 38 246 4 0 0 0 0 0 0 0],'level', -9);
    test_zmat('lz4 (level)', 'lz4', 'random data', [176 114 97 110 100 111 109 32 100 97 116 97],'level', -9);
    test_zmat('lz4hc (level)', 'lz4hc', 1.2, [128 51 51 51 51 51 51 243 63],'level', -9);
    test_zmat('zstd (level=1)', 'base64', zmat(eye(10),-1,'zstd'), 'KLUv/WAgAp0AAEgAAAAAAADwPwACAL+2UAGZwBE=');
    test_zmat('zstd (level=3)', 'base64', zmat(eye(10),-3,'zstd'), 'KLUv/WAgAo0AACgAAPA/AAMAv7ZQAQEzLIAF');
    test_zmat('zstd (level=9)', 'base64', zmat(eye(10),-9,'zstd'), 'KLUv/WAgAo0AACgAAPA/AAMAv7ZQAQEzLIAF');
    test_zmat('zstd (level=19)', 'base64', zmat(eye(10),-19,'zstd'), 'KLUv/WAgAp0AAEgAAAAAAADwPwACAMW2UAFGwRE=');
    test_zmat('blosc2blosclz (typesize=2)', 'base64', zmat(uint32(magic(4)),1,'blosc2blosclz','typesize',2), 'BQEFAkAAAABAAAAATAAAAAAAAAAAAQAAAAAAAAAAAAAkAAAAIAAAABAABQAJAAQAAgALAAcADgADAAoABgAPAA0ACAAMAAEAAAAAAA==');
    test_zmat('blosc2blosclz (typesize=4)', 'base64', zmat(uint32(magic(4)),1,'blosc2blosclz','typesize',4), 'BQEXBEAAAABAAAAAYAAAAAAAAAAAAQAAAAAAAAAAAAAQAAAABQAAAAkAAAAEAAAAAgAAAAsAAAAHAAAADgAAAAMAAAAKAAAABgAAAA8AAAANAAAACAAAAAwAAAABAAAA');
    test_zmat('blosc2blosclz (typesize=8)', 'base64', zmat(uint32(magic(4)),1,'blosc2blosclz','typesize',8), 'BQEXCEAAAABAAAAAYAAAAAAAAAAAAQAAAAAAAAAAAAAQAAAABQAAAAkAAAAEAAAAAgAAAAsAAAAHAAAADgAAAAMAAAAKAAAABgAAAA8AAAANAAAACAAAAAwAAAABAAAA');
    test_zmat('blosc2zstd (typesize=2)', 'base64', zmat(single(magic(4)),1,'blosc2zstd','typesize',2), 'BQGXAkAAAABAAAAAYAAAAAAAAAAAAQUAAAAAAAAAAAAAAIBBAACgQAAAEEEAAIBAAAAAQAAAMEEAAOBAAABgQQAAQEAAACBBAADAQAAAcEEAAFBBAAAAQQAAQEEAAIA/');
    test_zmat('blosc2zstd (typesize=4)', 'base64', zmat(single(magic(4)),1,'blosc2zstd','typesize',4), 'BQGVBEAAAABAAAAAVQAAAAAAAAAAAQUAAAAAAAAAAAAkAAAALQAAACi1L/0gQCUBAOAAAICgEIAAMOBgQCDAcFAAQIBBQEFAQEFBQUE/AgByQxMBFg==');
    test_zmat('blosc2zstd (typesize=8)', 'base64', zmat(single(magic(4)),1,'blosc2zstd','typesize',8), 'BQGXCEAAAABAAAAAYAAAAAAAAAAAAQUAAAAAAAAAAAAAAIBBAACgQAAAEEEAAIBAAAAAQAAAMEEAAOBAAABgQQAAQEAAACBBAADAQAAAcEEAAFBBAAAAQQAAQEEAAIA/');
    test_zmat('base64 (no newline)', 'base64', uint8(100), sprintf('ZA==\n'), 'level', 3);
    test_zmat('base64 (trailing newline)', 'base64', ones(7,1), sprintf('AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAAAAAA\n8D8='), 'level', 2);
    test_zmat('base64 (all newline)', 'base64', ones(7,1), sprintf('AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAAAAAA\n8D8=\n'), 'level', 3);
end
%%
if (ismember('d', tests))
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));
    fprintf('Test decompression\n');
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));

    test_zmat('zlib (scalar)', 'zlib', uint8([120 156 147 208 117 9 249 173 200 233 0 0 9 224 2 67]), typecast(pi, 'uint8'), 'level', 0);
end
%%
if (ismember('err', tests))
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));
    fprintf('Test error messages\n');
    fprintf(sprintf('%s\n', char(ones(1, 79) * 61)));

    test_zmat('empty method', '', [], 'the ''method'' field must be a non-empty string');
    test_zmat('unsupported method', 'ppp', [], 'the specified compression method is not supported');
    test_zmat('unsupported input (cell)', 'zlib', {}, 'input must be a char, non-complex numeric or logical vector or N-D array');
    test_zmat('unsupported input (handle)', 'zlib', @sin, 'input must be a char, non-complex numeric or logical vector or N-D array');
    test_zmat('unsupported input (complex)', 'gzip', 1+3i, 'input must be a char, non-complex numeric or logical vector or N-D array');
    if (exist('string'))
        if(~ischar(string('test')))
            test_zmat('unsupported input (string)', 'zlib', string(sprintf('zmat\ntest')), 'input must be a char, non-complex numeric or logical vector or N-D array');
        end
    end
    test_zmat('zlib wrong input format', 'zlib', [1, 2, 3, 4], [], 'level', 0, 'status', -3);
    test_zmat('blosc2zstd wrong input format', 'blosc2zstd', [1, 2, 3, 4], [], 'level', 0, 'status', -11);
    test_zmat('zstd wrong input format', 'zstd', [1, 2, 3, 4], [], 'level', 0, 'status', -9);
end

