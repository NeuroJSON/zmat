filelist={'lz4/lz4.c','lz4/lz4hc.c','easylzma/compress.c','easylzma/decompress.c', ...
    'easylzma/lzma_header.c', 'easylzma/lzip_header.c', 'easylzma/common_internal.c', ...
    'easylzma/pavlov/LzmaEnc.c', 'easylzma/pavlov/LzmaDec.c', 'easylzma/pavlov/LzmaLib.c' ...
    'easylzma/pavlov/LzFind.c', 'easylzma/pavlov/Bra.c', 'easylzma/pavlov/BraIA64.c' ...
    'easylzma/pavlov/Alloc.c', 'easylzma/pavlov/7zCrc.c','zmatlib.c'};

mexfile='zmat.cpp';
suffix='.o';
if(ispc)
    suffix='.obj';
end
if(~exist('OCTAVE_VERSION','builtin'))
    CCFLAG='CFLAGS=''-O3 -g -I../include -Ieasylzma -Ieasylzma/pavlov -Ilz4 -fPIC'' -c';
    LINKFLAG='CXXLIBS=''\$CLIBS -lz'' -output ../zipmat -outdir ../';
    for i=1:length(filelist)
        fprintf(1,'mex %s %s\n', CCFLAG, filelist{i});
        eval(sprintf('mex %s %s', CCFLAG, filelist{i}));
    end
    filelist=dir(['*' suffix]);
    filelist={filelist.name};
    cmd=sprintf('mex %s -Ieasylzma %s %s',mexfile, LINKFLAG, sprintf('%s ' ,filelist{:}));
    fprintf(1,'%s\n',cmd);
    eval(cmd)
else
    CCFLAG='-O3 -g -c -Ieasylzma -Ieasylzma/pavlov -Ilz4';
    LINKFLAG='-o ../zipmat -lz';
    for i=1:length(filelist)
        fprintf(stdout,'mex %s %s\n', CCFLAG, filelist{i});
        fflush(stdout);
        eval(sprintf('mex %s %s', CCFLAG, filelist{i}));
    end
    if(ispc)
        filelist=dir('*.obj');
        filelist={filelist.name};
    end
    cmd=sprintf('mex %s -Ieasylzma %s %s',mexfile, LINKFLAG, regexprep(sprintf('%s ' ,filelist{:}),'\.c[p]*','\.o'));
    fprintf(stdout,'%s\n',cmd);fflush(stdout);
    eval(cmd)
end

