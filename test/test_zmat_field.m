function test_zmat_field(testname, info, fieldname, expected)
%
% test_zmat_field(testname, info, fieldname, expected)
%
% Verify that info.(fieldname) exists and equals expected.
%
% input:
%      testname: descriptive name for this test
%      info: the info struct returned by zmat
%      fieldname: field name to check
%      expected: expected value
%

if (~isfield(info, fieldname))
    warning('Test %s: failed: info.%s does not exist', testname, fieldname);
    return
end

val = info.(fieldname);

if (ischar(expected) && ischar(val))
    passed = strcmp(val, expected);
elseif (isnumeric(expected) && isnumeric(val))
    passed = isequal(val, expected);
else
    passed = isequal(val, expected);
end

if (passed)
    if (ischar(val))
        fprintf(1, 'Testing %s: ok\n\tvalue: ''%s''\n', testname, val);
    else
        fprintf(1, 'Testing %s: ok\n\tvalue: %s\n', testname, mat2str(val));
    end
else
    if (ischar(val))
        warning('Test %s: failed: expected ''%s'', got ''%s''', testname, expected, val);
    else
        warning('Test %s: failed: expected %s, got %s', testname, mat2str(expected), mat2str(val));
    end
end
