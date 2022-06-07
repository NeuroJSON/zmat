%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Compilation script for zmat in MATLAB and GNU Octave
%
% author: Qianqian Fang <q.fang at neu.edu>
%
% Dependency (Windows only):
%  1.If you have MATLAB R2017b or later, you may skip this step.
%    To compile mcxlabcl in MATLAB R2017a or earlier on Windows, you must
%    pre-install the MATLAB support for MinGW-w64 compiler
%    https://www.mathworks.com/matlabcentral/fileexchange/52848-matlab-support-for-mingw-w64-c-c-compiler
%
%    Note: it appears that installing the above Add On is no longer working
%    and may give an error at the download stage. In this case, you should
%    install MSYS2 from https://www.msys2.org/. Once you install MSYS2,
%    run MSYS2.0 MinGW 64bit from Start menu, in the popup terminal window,
%    type
%
%       pacman -Syu
%       pacman -S base-devel gcc git mingw-w64-x86_64-opencl-headers
%
%    Then, start MATLAB, and in the command window, run
%
%       setenv('MW_MINGW64_LOC','C:\msys64\usr');
%  2.After installation of MATLAB MinGW support, you must type
%    "mex -setup C" in MATLAB and select "MinGW64 Compiler (C)".
%  3.Once you select the MingW C compiler, you should run "mex -setup C++"
%    again in MATLAB and select "MinGW64 Compiler (C++)" to compile C++.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

filelist = {'lz4/lz4.c', 'lz4/lz4hc.c', 'easylzma/compress.c', 'easylzma/decompress.c', ...
          'easylzma/lzma_header.c', 'easylzma/lzip_header.c', 'easylzma/common_internal.c', ...
          'easylzma/pavlov/LzmaEnc.c', 'easylzma/pavlov/LzmaDec.c', 'easylzma/pavlov/LzmaLib.c' ...
          'easylzma/pavlov/LzFind.c', 'easylzma/pavlov/Bra.c', 'easylzma/pavlov/BraIA64.c' ...
          'easylzma/pavlov/Alloc.c', 'easylzma/pavlov/7zCrc.c', 'zmatlib.c'};

mexfile = 'zmat.cpp';
suffix = '.o';
if (ispc)
    suffix = '.obj';
end
if (~exist('OCTAVE_VERSION', 'builtin'))
    delete(['*', suffix]);
    if (ispc)
        CCFLAG = 'CFLAGS=''-O3 -g -I../include -Ieasylzma -Ieasylzma/pavlov -Ilz4'' -c';
        LINKFLAG = 'CXXLIBS=''$CLIBS -lz'' -output ../zipmat -outdir ../';
    else
        CCFLAG = 'CFLAGS=''-O3 -g -I../include -Ieasylzma -Ieasylzma/pavlov -Ilz4 -fPIC'' -c';
        LINKFLAG = 'CXXLIBS=''\$CLIBS -lz'' -output ../zipmat -outdir ../';
    end
    for i = 1:length(filelist)
        fprintf(1, 'mex %s %s\n', CCFLAG, filelist{i});
        eval(sprintf('mex %s %s', CCFLAG, filelist{i}));
    end
    filelist = dir(['*' suffix]);
    filelist = {filelist.name};
    cmd = sprintf('mex %s -I../include -Ieasylzma %s %s', mexfile, LINKFLAG, sprintf('%s ', filelist{:}));
    fprintf(1, '%s\n', cmd);
    eval(cmd);
else
    delete('*.o');
    CCFLAG = '-O3 -g -c -I../include -Ieasylzma -Ieasylzma/pavlov -Ilz4';
    LINKFLAG = '-o ../zipmat -lz';
    for i = 1:length(filelist)
        fprintf(stdout, 'mex %s %s\n', CCFLAG, filelist{i});
        fflush(stdout);
        eval(sprintf('mex %s %s', CCFLAG, filelist{i}));
    end
    if (ispc)
        filelist = dir(['*.o']);
        filelist = {filelist.name};
    end
    cmd = sprintf('mex %s -I../include -Ieasylzma %s %s', mexfile, LINKFLAG, regexprep(sprintf('%s ', filelist{:}), '\.c[p]*', '\.o'));
    fprintf(stdout, '%s\n', cmd);
    fflush(stdout);
    eval(cmd);
end
