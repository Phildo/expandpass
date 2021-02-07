#!/bin/bash

# runs files named "1" through "$n_tests"
# if no file exists, assumes there's a corresponding
# seed file in "in", and an
# args fine in "args", and an
# expected out file in "out"
# expected err file in "err"

cd `dirname $0`
if [ ! -f ../expandpass ]; then echo "executable expandpass not found (expected `pwd`/../expandpass)" 1>&2; exit 1; fi

../expandpass --version

#silly way to find out # tests
max_tests=100
n_tests=0
for i in `eval "echo {1..$max_tests}"`; do
  if [ -f $i ] || [ -f in/$i ] || [ -f args/$i ] || [ -f out/$i ]; then
    n_tests=$i
  fi
done

for i in `eval "echo {1..$n_tests}"`; do
  echo -n Test $i...
  touch "in/$i"
  touch "out/$i"
  touch "err/$i"
  if [ -f $i ]; then
    if [ ./$i ]; then echo Passed
    else echo Failed
    fi
  else
    if [ -f args/$i ]; then
      cat args/$i | xargs ../expandpass -i "in/$i" > "test_stdout" 2> test_stderr
    else
      ../expandpass -i "in/$i" > "test_stdout" 2> test_stderr
    fi
      if ! diff test_stdout out/$i; then echo "Failed (stdout)"; exit 1
    elif ! diff test_stderr err/$i; then echo "Failed (stderr)"; exit 1
    else echo Passed
    fi
    rm test_stdout
    rm test_stderr
  fi
done

