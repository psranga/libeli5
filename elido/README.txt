This tool should NOT BE USED ON UNTRUSED INPUT (it uses 'eval').
YOU HAVE BEEN WARNED.

Elido fixes an xargs annoyance that's bugged me for long: I frequently had to
learn a lot more about shell quoting rules than I wanted to, especially when
handling input lines containing spaces (e.g., filenames).

The basic philosophy of Elido is that we create a command-line then execute it
directly: **WE DON'T USE THE SHELL TO EXECUTE THE COMMAND LINE**. I think this
is cleaner since it saves from having to think about multiple levels quoting.

Also input lines won't be split into individual arguments. The whole line will
be presented as is. Splitting a line into arguments is a misfeature IMHO. If
lines (e.g., filenames) you're processing may contain newlines don't use 
Elido. We'll eventually support NUL-terminated inputs.

INSTALLATION
============

Copy this file somewhere in your PATH and make it executable. It uses only the
Python standard library. Should work with both python2 and python3.

COMMAND-LINE SYNTAX
===================

elido [options ...] [command ...]
elido cross_product [options ...] filename [filename ...]

The vast majority of the time you'll be using the first variant, which is
inspired by 'xargs'. We added the 'cross_product' subcommand to Elido itself
(instead of making it a separate utility), so that you don't have to worry
about installing two utilities. We think it's useful enough that the decrease
in aesthetics is acceptable.

Example: seq 0 9 | elido convert input/X.jpg -colorspace GRAY output/X.jpg

Elido options come first. They always start with '--'. All arguments starting
from the first argument that doesnt' start with '--' will be treated as a
specification of the command to be run for each value in the input.

If the command you're trying to run itself starts with '--' (!!!), then add the
argument '--' before the command starts.

The above syntax has the unsatisfying property that elido's --stdin, --stdout
and --stderr options (see below) will appear before the command to be executed
(opposite of shell convention). But I'm going to keep it simple for now.
Otherwise I'll have to come with some fancy protocol to figure out what options
are meant for elido and what are for the command it executes if the command
being run has the same options as elido.

CONVENIENCES
============

Symbol Substitution
-------------------

The string X (configurable via --varname) occurring anywhere in the command
line to be executed will be replaced with the input line being processed. If
multiple lines are being processed at a time (via --chunksize), the the symbol
X0 will the first line of the chunk, X1 the second and so on.

The symbol XN will be replaced with the unpadded zero-based line number for
which this symbol substitution is being done.

These symbols are also available with in backtick sequences (below).

Backtick Sequences
------------------

Sequences of '%...%' within the command line to be executed are treated as
Python expressions that will be evaluated in a context in which the variable
'X' will contain the line being processed.

Example: print the sha256 checksum followed by the input of each line in a file:

  cat file.txt | elido echo '%sha256(X)%' X

Straightforward Redirection of Executed Command's Output
--------------------------------------------------------

Afaik, xargs makes you use the "sh -c" trick, which means you have to think
about two levels of quoting. I write my utilties to read and write stdin/stdout
instead of also providing the ability write to files, and have to jump through
hoops to drive them with xargs. Elido makes this easy:

  cat file.txt | elido --stdin=X --stdout='output/%sha256(X)%' myutil

Can Create Intermediate Directories of Generated Output
-------------------------------------------------------

Suppose if you have directory tree of input files, and you want to process each
file and recreate the directory tree under a different root.

You'll have to jump through hoops with xargs to do this. Since this is a common
use case, elido provides support for this using the '--output' command-line
flag. You tell elido what outputs the command you're running will create, and
Elido will create all required parent directories before running the command.

Example: You have a directory tree containing color images. You want to create
grayscale versions of those files with the same directory structure.
('relpath' below is the Python library function os.path.relpath).

  find /top/color -type f -name '*.jpg' | \
  elido --output='/top/gray/%relpath(X, "/top/color")%' \
    convert X -colorspace gray '/top/gray/%relpath(X, "/top/color")%'

Creates Intermediate Directories of Redirected Standard Output
--------------------------------------------------------------

For standard output and standard error, intermediate directories are
automatically created.

Example: If output files are named after the MD5 of the input filename and put
in subdirectories named after the first two characters of the MD5 hex checksum.

  cat file.txt | elido --stdin=X --stdout='output/%sha256(X)[0:2]%/%sha256(X)%' myutil

Process N Lines at A Time
-------------------------

Elido can read N lines of input and present the N values as an array. This is
useful if the input is formatted "vertically" i.e., each field on a separate
line instead of "horizontally" as in a CSV).

Example: input is a sequence of "input filename", "output filename" pairs, and
we're converting the inputs to grayscale.

  cat file.txt | elido --chunksize=2 convert X0 -colorspace GRAY X1

Execute N Jobs in Parallel
--------------------------

Example: Execute 4 jobs in parallel.

  cat file.txt | elido --parallelism=4 convert X0 -colorspace GRAY X1

Cross-product of N inputs
-------------------------

Subcommand 'cross_product' generates all possible combinations of the
lines in N input files. '-' can occur *once* in the list of filename,
to indicate that standard input should be used instead of reading from
a file.

Example:

  $ (echo A; echo B) > letters.txt
  $ (echo 1; echo 2) > numbers.txt
  $ elido cross_product letters.txt numbers.txt
  A
  1
  A
  2
  B
  1
  B
  2

IMPORTANT NOTE: Each element of the combination will occur on a separate
line. Use --chunksize to process the entire combination in each command
invocation.

Example:

  $ elido cross_product letters.txt numbers.txt | \
    elido --chunksize=2 echo X0 X1
  A 1
  A 2
  B 1
  B 2


Example: Suppose you have N images, and you'd like to create versions of the N
images at JPEG quality settings from 0 to 100 in steps of 10:

$ seq 0 110 | ./elido --chunksize=10 echo X0 > quality.txt
$ /bin/ls -1 *.jpg | python elido.py cross_product - quality.txt | \
    python elido.py --chunksize=2 \
      convert X0 -quality X1 output/'%file_root(X0)%'-X1.jpg
