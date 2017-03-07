#!/bin/bash

set -e
set -x

test -n "$ELIDO"

# A bare varname always sees the input as is.
actual=$(seq 1 4 | sed -e 's/$/.500/g' | $ELIDO echo X | tr "\n" " ")
expected="1.500 2.500 3.500 4.500 "
test "$actual" = "$expected"

# But backticks see the type-converted object. Useful to keep backticks short.
actual=$(seq 1 4 | sed -e 's/$/.500/g' | $ELIDO echo '%X%' | tr "\n" " ")
expected="1.5 2.5 3.5 4.5 "
test "$actual" = "$expected"

# ... unless --keep_input_as_string is given.
actual=$(seq 1 4 | sed -e 's/$/.500/g' | $ELIDO --keep_input_as_string echo '%X%' | tr "\n" " ")
expected="1.500 2.500 3.500 4.500 "
test "$actual" = "$expected"

# But backticks see the type-converted object. Useful to keep backticks short.
actual=$(seq 1 4 | sed -e 's/$/.500/g' | $ELIDO echo '%X/2%' | tr "\n" " ")
expected="0.75 1.25 1.75 2.25 "
test "$actual" = "$expected"

# ... unless --keep_input_as_string is given. This will error out (can't divide strings).
actual=$(seq 1 4 | sed -e 's/$/.500/g' | $ELIDO --keep_input_as_string echo '%X/2%' || echo FAIL)
expected="FAIL"
test "$actual" = "$expected"

# --define is always type-converted.
actual=$(seq 1 4 | $ELIDO --define K 2 echo '%X*K%' | tr "\n" " ")
expected="2 4 6 8 "
test "$actual" = "$expected"

actual=$(seq 1 4 | $ELIDO --keep_input_as_string --define K 2 echo '%X*K%' | tr "\n" " ")
expected="11 22 33 44 "
test "$actual" = "$expected"

# float
actual=$(seq 1 4 | $ELIDO --define K 2.5 echo '%X*K%' | tr "\n" " ")
expected="2.5 5.0 7.5 10.0 "
test "$actual" = "$expected"

