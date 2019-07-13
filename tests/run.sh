#!/bin/bash

n_tests=2

# runs files named "1" through "$n_tests"
# if no file exists, assumes there's a corresponding
# seed file in "in", and an
# args fine in "args", and an
# expected out file in "out"
# expected err file in "err"

cd `dirname $0`
for i in `eval "echo {1..$n_tests}"`; do
  echo -n Test $i...
  if [ -f $i ]; then
    if [ ./$i ]; then echo Passed
    else echo Failed
    fi
  else
    if [ -f args/$i ]; then
      cat args/$i | xargs expandpass -i "in/$i" -o "test_stdout" 2> test_stderr
    else
      expandpass -i "in/$i" -o "test_stdout" 2> test_stderr
    fi
      if ! diff test_stdout out/$i; then echo "Failed (stdout)"
    elif ! diff test_stderr err/$i; then echo "Failed (stderr)"
    else echo Passed
    fi
    rm test_stdout
    rm test_stderr
  fi
done

