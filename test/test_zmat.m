function test_zmat(testname, method, input, expected, varargin)
opt=struct('level',1);

if(length(varargin)>1 && rem(length(varargin),2)==0 && ischar(varargin{1}))
    for i=1:2:length(varargin)
        opt.(varargin{i})=varargin{i+1};
    end
end

try
    [res, info] = zmat(input, opt.level, method);
catch ME
    if(~isempty(strfind(ME.message, expected)))
        fprintf(1, 'Testing exception %s: ok\n\toutput:''%s''\n', testname, ME.message);
    else
        warning('Test exception %s: failed: expected ''%s'', obtained ''%s''', testname, expected, ME.message);
    end
    return;
end

if(isfield(opt,'info'))
    res=info.(opt.info);
end

if(isfield(opt,'status'))
    if(info.status~=opt.status)
        warning('Test %s: failed: expected ''%s'', obtained ''%s''', testname, mat2str(expected), mat2str(res));
    else
        fprintf(1, 'Testing %s error: ok\n\tstatus:''%d''\n', testname, info.status);
    end
    return;
end

if (~isequal(res, expected))
    warning('Test %s: failed: expected ''%s'', obtained ''%s''', testname, mat2str(expected), mat2str(res));
else
    if(ischar(res))
        fprintf(1, 'Testing %s: ok\n\toutput:''%s''\n', testname, res);
    else
        fprintf(1, 'Testing %s: ok\n\toutput:''%s''\n', testname, mat2str(res));
    end
    if(isfield(opt,'info') || opt.level == 0)
        return;
    end
    newres = zmat(res, info);
    if (exist('isequaln'))
        try
            if (isequaln(newres, input))
                fprintf(1, '\t%s successfully restored the input\n', method);
            end
        catch
        end
    else
        try
            if (newres == input)
                fprintf(1, '\t%s successfully restored the input\n', method);
            end
        catch
        end
    end
end
end
