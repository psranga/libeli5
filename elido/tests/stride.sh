#!/bin/bash

set -e
set -x

test -n "$ELIDO"

actual=$(seq 1 4 | $ELIDO --stride=1 echo X | tr "\n" " ")
expected="1 2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --stride=2 echo X | tr "\n" " ")
expected="1 3 "
test "$actual" = "$expected"

actual=$(seq 1 5 | $ELIDO --stride=2 echo X | tr "\n" " ")
expected="1 3 5 "
test "$actual" = "$expected"

# stride and ignore_first_n
actual=$(seq 1 4 | $ELIDO --ignore_first_n=1 --stride=1 echo X | tr "\n" " ")
expected="2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --ignore_first_n=1 --stride=2 echo X | tr "\n" " ")
expected="2 4 "
test "$actual" = "$expected"

actual=$(seq 1 5 | $ELIDO --ignore_first_n=1 --stride=2 echo X | tr "\n" " ")
expected="2 4 "
test "$actual" = "$expected"

# stride and chunksize
actual=$(seq 1 4 | $ELIDO --ignore_first_n=0 --stride=2 --chunksize=2 echo X0X1 | tr "\n" " ")
expected="13 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --ignore_first_n=1 --stride=2 --chunksize=2 echo X0X1 | tr "\n" " ")
expected="24 "
test "$actual" = "$expected"

# partial chunks not processed.
actual=$(seq 1 4 | $ELIDO --ignore_first_n=1 --stride=2 --chunksize=3 echo X0X1X2 | tr "\n" " ")
expected=""
test "$actual" = "$expected"

actual=$(seq 1 6 | $ELIDO --ignore_first_n=1 --stride=2 --chunksize=3 echo X0X1X2 | tr "\n" " ")
expected="246 "
test "$actual" = "$expected"
