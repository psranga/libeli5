#!/bin/bash

set -e
set -x

test -n "$ELIDO"

actual=$(seq 1 4 | $ELIDO echo X | tr "\n" " ")
expected="1 2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --varname=Y echo Y | tr "\n" " ")
expected="1 2 3 4 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --varname=Z echo Y | tr "\n" " ")
expected="Y Y Y Y "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO echo '"Y"' | tr "\n" " ")
expected='"Y" "Y" "Y" "Y" '
test "$actual" = "$expected"

echo "All done."
