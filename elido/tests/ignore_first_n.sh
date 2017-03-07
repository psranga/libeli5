#!/bin/bash

set -e
set -x

test -n "$ELIDO"

actual=$(seq 1 4 | $ELIDO --ignore_first_n=1 echo X | tr "\n" " ")
expected="2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --ignore_first_n=0 echo X | tr "\n" " ")
expected="1 2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --ignore_first_n=-1 echo X | tr "\n" " ")
expected="1 2 3 4 "
test "$actual" = "$expected"

echo "All done."
