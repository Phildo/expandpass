#!/bin/bash

# DO NOT RUN THIS PRIOR TO ./run.sh
# that would defeat the whole purpose
# this is a helper function for quickly
# generating tests ::when you are already
# confident in the current build's functionality::
# (and will still need manual verification)

cd `dirname $0`
if [ ! -f ../expandpass ]; then echo "executable expandpass not found (expected `pwd`/../expandpass)" 1>&2; exit 1; fi

#silly way to find out # tests
max_tests=100
n_tests=0
for i in `eval "echo {1..$max_tests}"`; do
  if [ -f $i ] || [ -f in/$i ] || [ -f args/$i ] || [ -f out/$i ]; then
    n_tests=$i
  fi
done

for i in `eval "echo {1..$n_tests}"`; do
  if [ -f $i ]; then
    echo "Skip $i... (custom format)"
  else
    echo Gen $i...
    if [ -f args/$i ]; then
      cat args/$i | xargs ../expandpass -i "in/$i" > "out/$i" 2> "err/$i"
    else
      ../expandpass -i "in/$i" > "out/$i" 2> "err/$i"
    fi
  fi
done

assert_identical()
{
  if ! diff "out/$1" "out/$2"; then
    echo "Genned output $1 and $2 differ"
  fi
}

#hardcoded but whatever
assert_identical 1 2
assert_identical 3 4
assert_identical 4 6

