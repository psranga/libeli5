Run the test as follows:

$ cat *.sh | ELIDO=elido bash && echo ALL_OK || echo FAIL

If ALL_OK is printed, all tests passed. If FAIL was printed some test failed.

Look at the command log to locate the failing test.

A better method *may* be adopted to run the test. Or not. This project likes ELI5.
