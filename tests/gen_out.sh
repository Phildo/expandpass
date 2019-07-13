#!/bin/bash

n_tests=2

for i in `eval "echo {1..$n_tests}"`; do
  if [ -f $i ]; then
    echo Skip $i...
  else
    echo Gen $i...
    if [ -f args/$i ]; then
      cat args/$i | xargs expandpass -i "in/$i" -o "out/$i" 2> "err/$i"
    else
      expandpass -i "in/$i" -o "out/$i" 2> "err/$i"
    fi
  fi
done

