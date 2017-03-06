#!/usr/bin/python2
# vim: set filetype=python:

"""This tool should NOT BE USED ON UNTRUSED INPUT (it uses 'eval').
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
"""

import base64
import logging
import hashlib
import sys
import argparse
import subprocess
import os
import re
import pipes
import itertools
import time

try:
    from Queue import Queue
except ImportError:
    from queue import Queue
from threading import Thread, Lock

# Will be replaced with an argparse-created object.
args = None
running_as_cross_product = False

num_cmd_invocations_done = 0
update_after_invocation_lock = Lock()

# The functions accessible to backtick sub-commands.
# TODO: add more functions here.
cmd_context = {
    '__builtins__': None,
    'basename': os.path.basename,
    'relpath': os.path.relpath,
    'dirname': os.path.dirname,
    'md5': lambda x: hashlib.md5(x).hexdigest(),
    'sha1': lambda x: hashlib.sha1(x).hexdigest(),
    'sha256': lambda x: hashlib.sha256(x).hexdigest(),
    'safe_base64': lambda x: base64.urlsafe_b64encode(x),
    'decode_safeb64': lambda x: base64.urlsafe_b64decode(x),
    'base64': lambda x: base64.standard_b64encode(x),
    'decode_base64': lambda x: base64.standard_b64decode(x),
    'padded_int': lambda width, x: '%0*d' % (width, int(x)),
    'readable_num': lambda width, decimals, x: '%*.*f' % (width, decimals, float(x)),
    'int': int,
    'float': float,
    'str': str,
    'file_root': lambda x: os.path.splitext(x)[0],
    'file_ext': lambda x: os.path.splitext(x)[1],
    'change_ext': lambda x, new_ext: os.path.splitext(x)[0] + new_ext}

cmd_queue = None


def parse_args(inargs=None):
    global args
    global running_as_cross_product

    parser = argparse.ArgumentParser(description='elido')
    parser.add_argument('--log_level', type=str,
                        default='WARN',
                        help='Logging level. E.g., DEBUG, INFO, WARN, ERROR.')
    parser.add_argument('--varname', type=str, default='X',
                        help='Placeholder in commands that will be replaced '
                             'with input line.')
    parser.add_argument('--stride', type=int, default=1,
                        help='Process every Nth line, instead of every.')
    parser.add_argument('--ignore_first_n', type=int, default=0,
                        help='Ignore these many lines from the beginning.')
    parser.add_argument('--chunksize', type=int, default=1,
                        help='Process inputs in chunks of this many lines at a '
                             'time. First line is available as X0, second and '
                             'X1 and so on. **INCOMPLETE CHUNKS AT THE END OF '
                             'THE FILE WILL NOT BE PROCESSED!**')
    parser.add_argument('--fd0', '--stdin', type=str, default='',
                        help='File to redirect standard input of executed '
                             'command. Can contain backtick sequences.')
    parser.add_argument('--fd1', '--stdout', type=str, default='',
                        help='Standard output redirection (similar to '
                             '--stdin).')
    parser.add_argument('--fd2', '--stderr', type=str, default='',
                        help='Standard error redirection (similar to '
                             '--stdin).')
    parser.add_argument('--output', type=str, default=[], action='append',
                        help='Declare files that the command will generate, so '
                             'that Elido can create parent directories before '
                             'running command. Can contain backtick sequences. '
                             'Can be specified multiple times to declare '
                             'multiple outputs.')
    parser.add_argument('--parallelism', type=int, default=1,
                        help='Number of commands to run at once.')
    parser.add_argument('--progress', type=int, default=0,
                        help='Report progress every N command invocations to '
                             'stderr. 0 (default) means no progress.')
    parser.add_argument('--dry_run', action='store_true',
                        help="Don't actually execute the command. "
                             'Dump the commands to be executed with Bash '
                             'quoting.')
    parser.add_argument('cmd', nargs=argparse.REMAINDER,
                        help='The command to execute.')

    inargs_to_use = inargs if args else sys.argv[1:]

    running_as_cross_product = False
    if len(inargs_to_use) > 0 and inargs_to_use[0] == 'cross_product':
        running_as_cross_product = True
        inargs_to_use = inargs_to_use[1:]

    try:
        dash_dash_pos = inargs_to_use.index('--')
        args_to_use = inargs_to_use[:dash_dash_pos]
        cmd_range = (dash_dash_pos + 1, len(inargs_to_use))
    except ValueError:
        last_opt_pos = -1
        for i in range(len(inargs_to_use)):
            if inargs_to_use[i][0:2] == '--':
                last_opt_pos = i
            else:
                break
        args_to_use = inargs_to_use[:last_opt_pos + 1]
        cmd_range = (last_opt_pos + 1, len(inargs_to_use))

    args = parser.parse_args(args_to_use)
    args.cmd = inargs_to_use[cmd_range[0]:cmd_range[1]]

    # varname will be used in a regular expression. Escape each character to
    # prevent XSS.
    args.varname_escaped = ''.join(['\\' + c for c in args.varname])

    logging.basicConfig(stream=sys.stderr, level=getattr(
        logging, args.log_level.upper()))

    logging.debug('config: ' + str(args))


def replace_backticks_and_variables_in_expr(
        expr, values, varname, varname_escaped, XN_value, cmd_context,
        really_eval=True):
    def eval_or_get_value(m):
        if m.group(1):
            return eval(m.group(1), cmd_context)
        elif m.group(2):
            return cmd_context[m.group(2)]
        elif m.group(3):
            return cmd_context[m.group(3)]

    cmd_context[varname] = values[0]
    for i in range(len(values)):
        cmd_context[varname + str(i)] = values[i]
    cmd_context[varname + 'N'] = XN_value

    # Rely on re module's caching and keep things simple. An implementation
    # using pre-compiled objects was not faster.
    re_string = '%([^%]*)%|({varname_escaped}N)|({varname_escaped}[0-9]*)'
    output = re.sub(re_string.format(varname_escaped=varname_escaped),
                    eval_or_get_value, expr)
    return output


def replace_backticks_and_variables(
        params, values, varname, varname_escaped, XN_value, cmd_context):
    return [replace_backticks_and_variables_in_expr(
        p, values, varname, varname_escaped, XN_value,
        cmd_context, True) for p in params]


def create_intermediate_dirs(fn):
    dirn = os.path.dirname(fn)
    if dirn == '':
        return True

    logging.debug('Creating directory: ' + dirn)
    try:
        os.makedirs(dirn)
    except OSError:
        if not os.path.isdir(dirn):
            raise
    return True


def open_creating_intermediate_dirs(fn, mode):
    create_intermediate_dirs(fn)
    return open(fn, mode)


def compute_fds_to_use(fd_filenames):
    default_streams = [sys.stdin, sys.stdout, sys.stderr]

    def open_or_error(fn, mode, default_value):
        if fn == '':
            return default_value
        try:
            return open_creating_intermediate_dirs(fn, mode)
        except IOError as e:
            logging.error('Unable to open ' + fn + ' for ' +
                          ('writing' if mode == 'w' else 'reading') + ': ' +
                          str(e))
            return None

    output = [open_or_error(fn, mode, dfl_stream) for fn, mode, dfl_stream in zip(
        fd_filenames, ['r', 'w', 'w'], default_streams)]
    ok = None not in output
    return ok, output


def close_fds_if_needed(fds_to_use, fd_filenames):
    for i in range(len(fd_filenames)):
        if fd_filenames[i] != '':
            fds_to_use[i].close()


def exec_worker():
    global cmd_queue
    global num_cmd_invocations_done

    while True:
        worker_input = cmd_queue.get()
        cmd, fd_filenames, output_filenames, progress_frequency = worker_input
        logging.debug('cmd: ' + str(cmd) +
                      ' fd_filenames: ' + str(fd_filenames) +
                      ' output_filenames: ' + str(output_filenames))

        open_ok, fds_to_use = compute_fds_to_use(fd_filenames)
        if open_ok:
            try:
                map(create_intermediate_dirs, output_filenames)
            except:
                close_fds_if_needed(fds_to_use, fd_filenames)
            else:
                subprocess.call(cmd, stdin=fds_to_use[0], stdout=fds_to_use[
                                1], stderr=fds_to_use[2], shell=False)
                close_fds_if_needed(fds_to_use, fd_filenames)
        else:
            logging.error('Error computing fds: ' + str(worker_input))

        cmd_queue.task_done()

        print_progress = False
        with update_after_invocation_lock:
            num_cmd_invocations_done += 1
            if progress_frequency > 0:
                if (num_cmd_invocations_done % progress_frequency) == 0:
                    print_progress = True
        if print_progress:
            sys.stderr.write('Finished: {num_done} invocations.\n'.format(
                num_done=num_cmd_invocations_done))


def start_exec_workers(num_workers):
    for i in range(num_workers):
        t = Thread(target=exec_worker)
        t.daemon = True
        t.start()


def dump_command(cmd, fd_filenames, output_filenames):
    cmd_with_quoting = ' '.join(map(pipes.quote, cmd))
    redirs = []
    cand_dirs_to_create = map(os.path.dirname, output_filenames)
    for inout_sym, crdir, fn in zip(
            ['<', '>', '>'], [False, True, True], fd_filenames):
        if fn != '':
            if crdir:
                cand_dirs_to_create.append(os.path.dirname(fn))
            redirs.append(inout_sym + ' ' + pipes.quote(fn))

    dirs_to_create = filter(lambda dirn: dirn != '', cand_dirs_to_create)
    quoted_dirs_to_create = map(pipes.quote, dirs_to_create)
    mkdir_cmd = ''
    if len(quoted_dirs_to_create) > 0:
        mkdir_cmd = 'mkdir -p ' + ' '.join(quoted_dirs_to_create) + ' && '

    redir_subcmd = ' '.join(redirs)
    sys.stdout.write(mkdir_cmd)
    sys.stdout.write(cmd_with_quoting)
    sys.stdout.write(' ')
    sys.stdout.write(redir_subcmd)
    sys.stdout.write('\n')


def do_cross_product(filenames):
    handles = [open(fn, 'r') if fn != '-' else sys.stdin for fn in filenames]
    for combo in itertools.product(*handles):
        for elem in combo:
            sys.stdout.write(elem)


def do_elido():
    global cmd_queue

    cmd_queue = Queue()
    start_exec_workers(args.parallelism)

    line_num = -1
    chunked_input = []
    for raw_line in sys.stdin:
        line_num += 1

        if line_num < args.ignore_first_n:
            continue

        if ((line_num - args.ignore_first_n) % args.stride) != 0:
            continue

        line = raw_line.strip()
        XN_value = str(line_num)

        chunked_input.append(line)
        if len(chunked_input) == args.chunksize:
            cmd = replace_backticks_and_variables(args.cmd, chunked_input,
                                                  args.varname, args.varname_escaped, XN_value, cmd_context)
            fd_filenames = replace_backticks_and_variables(
                [args.fd0, args.fd1, args.fd2], chunked_input,
                args.varname, args.varname_escaped, XN_value, cmd_context)
            output_filenames = replace_backticks_and_variables(
                args.output, chunked_input, args.varname,
                args.varname_escaped, XN_value, cmd_context)
            worker_input = (cmd, fd_filenames, output_filenames, args.progress)
            if not args.dry_run:
                logging.debug('queuing: ' + str(worker_input))
                cmd_queue.put(worker_input)
            else:
                dump_command(cmd, fd_filenames, output_filenames)
            chunked_input = []

    # This is running in the main thread. So check for all items to be removed
    # from queue in non-blocking manner.  Otherwise cmd_queue.join() will block
    # until all items are dequeued, preventing handling Ctrl-C in the main code
    # block below.
    while not cmd_queue.empty():
        time.sleep(0.1)

    # Queue empty just means items were dequeued. Now wait for them to be
    # processed. There should be at most args.parallelism jobs in flight
    # now. In most practical situations, hitting Ctrl-C here will break
    # out quickly, even though join() is blocking, because the Ctrl-C gets
    # sent all processes in the terminal, and most well-behaved processes will
    # exit on ^C, leading to this join() here unblocking pretty soon.
    #
    # Ideally, there should be a join variant with a timeout so that we
    # can join in a non-blocking manner. Consider refactoring this to use
    # different queueing API. E.g., multiprocessing.Pool.
    cmd_queue.join()


def main():
    try:
        parse_args()
        if running_as_cross_product:
            do_cross_product(args.cmd)
        else:
            do_elido()
    except KeyboardInterrupt:
        sys.exit(2)

if __name__ == '__main__':
    main()
