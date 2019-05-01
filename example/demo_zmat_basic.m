% addpath('../')

% compression
[dzip,info]=zmat(uint8(eye(5,5)))

% decompression
orig=reshape(zmat(dzip,0),info.ArraySize)
