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

Symbol Substitution (--varname)
-------------------------------

The string X (configurable via --varname) occurring anywhere in the command
line to be executed will be replaced with the input line being processed. If
multiple lines are being processed at a time (via --chunksize), the the symbol
X0 will the first line of the chunk, X1 the second and so on.

The symbol XN will be replaced with the unpadded zero-based line number for
which this symbol substitution is being done.

These symbols are also available with in backtick sequences (below).

Backtick Expressions (%...%)
----------------------------

Sequences of '%...%' within the command line to be executed are treated as
Python expressions that will be evaluated in a context in which the variable
'X' will contain the line being processed.

Example: print the sha256 checksum followed by the input of each line in a file:

  cat file.txt | elido echo '%sha256(X)%' X

Type Inference (use --keep_input_as_string to disable)
------------------------------------------------------

If the input value is a float or int (decimal or hex (0x prefix)), the symbol
X will refer to a float or int-type object *ONLY IN BACKTICK EXPRESSIONS*. A
bare X will always get the verbatim value.

Example:

  $ seq 1 4 | elido echo '%X*2%'
  2
  4
  6
  8

  $ # Add unnecessary zeros to the fractional part.
  $ seq 1 4 | sed -e 's/$/.500' | elido echo X '%X%'
  1.500 1.5
  2.500 2.0
  3.500 3.5
  4.500 4.5

Use --keep_input_as_string to disable type inference and treat 'X' as a string.

Example:

  $ # 'A' * 2 means repeat 'A' two times in Python.
  $ seq 1 4 | elido echo --keep_input_as_string '%X*2%'
  11
  22
  33
  44

If you want the verbatim string in some backtick expressions but not
others, use X_raw to get the string and X to get the converted version.

  $ seq 1 4 | sed -e 's/$/.500' | elido echo '%X_raw%' '%X%'
  1.500 1.5
  2.500 2.0
  3.500 3.5
  4.500 4.5


Side Inputs (--define and --rawsym)
-----------------------------------

Even with Elido, I found myself in shell quoting hell when trying to pass
environment variables containing side inputs, that are values that have the same
value for all invocations of the command, but that you may still want to be
configurable at a higher level than the elido invocation (e.g., configurable
zoom factor to be applied to multiple images).

The --define switch simplifies and makes more readable code like this:

  $ # 'K' is a side input. Note double quote, since the shell does the
  $ # substitution. Elido sees '%X*2%'.
  $ export K=2 && seq 1 4 | elido echo "%X*$K%"

Example:

  $ # Note single quote. Elido itself does the substitution.
  $ seq 1 4 | elido --define K 2 echo '%X*K%'
  2
  4
  6
  8

Type inference will always be carried out on side inputs. Open a bug if think of
a good reason to have an option to disable this.

--rawsym is almost the same as --define, except that type inference is never
carried out symbols defined using --rawsym. May be useful to make backtick
expressions more readable in some cases.

Simple Redirection of Executed Command's Output (--stdin, --stdout, --stderr)
-----------------------------------------------------------------------------

Afaik, xargs makes you use the "sh -c" trick, which means you have to think
about two levels of quoting. I write my utilties to read and write stdin/stdout
instead of also providing the ability write to files, and have to jump through
hoops to drive them with xargs. Elido makes this easy:

Example:

  cat file.txt | elido --stdin=X --stdout='output/%sha256(X)%' myutil

One disadvantage is you need to do '--stdin=file' instead of '> file'. Also,
the redirection options come *before* the command, not after, as is the
case in shell command lines.

We think this is better than the evil of inventing more notation/conventions
and writing a parser to parse '> file' from the elido command line, and most
importantly requiring shell escaping to prevent the shell from interpreting
the '> file' phrase.

Can Create Intermediate Directories of Generated Output (--output)
------------------------------------------------------------------

Say if you have directory tree of input files, and you want to process each
file and recreate the directory tree under a different root.

You'll have to jump through hoops with xargs to do this. Since this is a common
use case, elido provides support for this using the '--output' command-line
flag. You tell elido what outputs the command you're running will create, and
Elido will create all required parent directories before running the command.

Example: You have a directory tree containing color images. You want to create
grayscale versions of those files with the same directory structure.
('relpath' below is the Python library function os.path.relpath).

  find /top/color -type f -name '*.jpg' | \
  elido --define TOPDIR /top/color --output='/top/gray/%relpath(X, TOPDIR)%' \
    convert X -colorspace gray '/top/gray/%relpath(X, TOPDIR)%'

Creates Intermediate Directories of Redirected Standard Output
--------------------------------------------------------------

Naturally, intermediate directories are automatically created, when standard
output and standard error are redirected also.

Example: If output files are named after the SHA256 of the input filename and put
in subdirectories named after the first two characters of the MD5 hex checksum.

  cat file.txt | elido --stdin=X --stdout='output/%sha256(X)[0:2]%/%sha256(X)%' myutil

Process N Lines at A Time (--chunksize)
---------------------------------------

Elido can read N lines of input and present the N values as a list. This is
useful if the input is formatted "vertically" i.e., each field on a separate
line instead of "horizontally" as in a CSV).

Example: Read in a file contain 1..4 interleaved with 11..14:

  $ paste -d "\n" <(seq 1 4) <(seq 11 14) | \
     elido --chunksize=2 echo X0+10 is X1
  1+10 is 11
  2+10 is 12
  3+10 is 13
  4+10 is 14


Execute N Jobs in Parallel (--parallelism)
------------------------------------------

Example: Execute 4 jobs in parallel.

  cat file.txt | elido --parallelism=4 convert X0 -colorspace GRAY X1

Cross-product of N inputs (elido cross_product)
-----------------------------------------------

Subcommand 'cross_product' generates all possible combinations of the
lines in N input files. '-' can occur *once* in the list of filename,
to indicate that standard input should be used instead of reading from
a file.

Example:

  $ elido cross_product <(echo A; echo B) <(echo 1; echo 2)
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

  $ elido cross_product <(echo A; echo B) <(echo 1; echo 2) | \
    elido --chunksize=2 echo X0 X1
  A 1
  A 2
  B 1
  B 2


Example: Suppose you have N images, and you'd like to create versions of the N
images at JPEG quality settings from 0 to 100 in steps of 10:

$ # Note how type inference make the backtick expression short.
$ seq 0 10 | ./elido echo '%X*10%' > quality.txt

$ find . -maxdepth 1 -name '*.jpg' | \
    elido cross_product - quality.txt | \
    elido --chunksize=2 \
      convert X0 -quality X1 output/'%file_root(X0)%'-X1.jpg
