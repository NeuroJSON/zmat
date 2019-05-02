% addpath('../')

% compression
[dzip,info]=zmat(uint8(eye(5,5)))

% decompression
orig=reshape(zmat(dzip,0),info.size)

% base64 encoding and decoding
base64=zmat('zmat toolbox',1,'base64');
char(base64)

orig=zmat(base64,1,'base64');
char(orig)

