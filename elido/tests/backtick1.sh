#!/bin/bash

set -e
set -x

test -n "$ELIDO"

actual=$(seq 1 4 | $ELIDO echo '%readable_num(X, 5, 2)%' | tr "\n" " ")
expected=" 1.00  2.00  3.00  4.00 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --varname=Y echo '%readable_num(Y, 5, 2)%' | tr "\n" " ")
expected=" 1.00  2.00  3.00  4.00 "
test "$actual" = "$expected"

# Errors out saying Z not found.
actual=$(seq 1 4 | $ELIDO --varname=Z echo '%readable_num(Y, 5, 2)%' || echo notok)
expected="notok"
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO echo '"%readable_num(X, 5, 2)%"' | tr "\n" " ")
expected='" 1.00" " 2.00" " 3.00" " 4.00" '
test "$actual" = "$expected"

echo "All done"
