This tool should NOT BE USED ON UNTRUSED INPUT (it uses 'eval').
YOU HAVE BEEN WARNED.

Elido fixes an xargs annoyance that's bugged me for long: I frequently had to
learn a lot more about shell quoting rules than I wanted to, especially when
handling input lines containing spaces (e.g., filenames).

The basic philosophy of Elido is that we create a command-line using Python,
then execute it directly **WE DON'T USE THE SHELL TO EXECUTE THE COMMAND
LINE**.  I think this is cleaner since it saves from having to think about
multiple levels quoting.

Also input lines won't be split into individual arguments. The whole line will
be presented as is. Splitting a line into arguments is a misfeature IMHO.

INSTALLATION
============

Copy this file somewhere in your PATH and make it executable. It uses only the
Python standard library. Should work with both python2 and python3.

COMMAND-LINE SYNTAX
===================

elido [options ...] [command ...]

Example: seq 0 9 | elido convert input/X.jpg -colorspace GRAY output/X.jpg

Elido options come first. They always start with '--'. All arguments starting
from the first argument that doesnt' start with '--' will be treated as a
specification of the command to be run for each value in the input.

If the command you're trying to run itself starts with '--' (!!!), then add the
argument '--' before the command starts.

The above syntax has the unsatisfying property that --stdin, --stdout and
--stderr will appear before the command to be executed (opposite of shell
convention). But I'm going to keep it simple for now. Otherwise I'll have to
come with some fancy protocol to figure out what options are meant for elido
and what are for the command it executes if the command being run has the same
options as elido.

CONVENIENCES
============

Backtick Sequences
------------------

Sequences of '%...%' within the command line to be executed are treated as
Python expressions that will be evaluated in a context in which the variable
'X' will contain the line being processed.

Example: print the md5 checksum followed by the input of each line in a file:

  cat file.txt | elido echo '%md5(X)%' X

Straightforward Redirection of Executed Command's Output
--------------------------------------------------------

Afaik, xargs makes you use the "sh -c" trick, which means you have to think
about two levels of quoting. I write my utilties to read and write stdin/stdout
instead of also providing the ability write to files, and have to jump through
hoops to drive them with xargs. Elido makes this easy:

  cat file.txt | elido --stdin=X --stdout='output/%md5(X)%' myutil

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

  cat file.txt | elido --stdin=X --stdout='output/%md5(X)[0:2]%/%md5(X)%' myutil

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

  cat file.txt | elido --parallelism=4 --chunksize=2 convert X0 -colorspace GRAY X1
