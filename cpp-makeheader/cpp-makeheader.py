#!/usr/bin/python

"""
cpp-makeheader: A simple utility that creates a C++ header file from a C++
source file.

# Introduction

Why? DRY.

Function prototypes, class and struct definitions, typedefs are extracted.
Template specializations of functions and structs will also be extracted.

'using' directives are recorded and the corresponding symbols are converted to
fully-namespaced versions in the header file.

Everything within anonymous namespaces is ignored. Static functions, and
Diogenes unit tests will also be ignored.

Implemented as a simple line-by-line scan over the input source file.
Hopefully that means that you'll should be able to enhance this utility, if
needed, very quickly.

Eventually, this is should rewritten using Clang's LibFormat. Maybe. :)

# Core Idea

If all source code is reformatted according to some fixed convention, we can use
very simple recognizers and line-by-line scans over the input file to recognize
most of the program structure needed to generate a header file.

We use clang-format to reformat input source files into a fixed format. We do
this reformatting on the fly as a preprocessing step in a wrapper script.

# Data Structures and Algorithm

## Data Structures

### Byterange

The fundamental observation is that the header file is a *SUBSET* of the source
file, with some minor changes (e.g., adding a semicolon after function prototypes,
expanding namespace'd identifiers).

So the data structures reflect this. The core data structure is a tuple
representing a range of bytes from the input file, along with some bookkeeping info:

    tuple(start_pos, end_pos, suffix, flags, location_code)

If a byterange is selected for output to the header, this is essentially what
happens:

    sys.stdout.write(input[start_pos:end_pos] + suffix)

We scan the input file and build a list of byteranges to write out, and we
finally write them out.

So far, everything has been possible using only non-overlapping byteranges.  So
ordering them for writing is trivial: just sort by start position.

### Block Start

By reformatting appropriately, many interesting parts of the program can be
forced to lie within a single line, making it possible to recognize them
analyzing a single line. E.g., function prototypes.

But some like struct definitions won't fall within a single line. The *start*
of the struct will occur on a single line, but the end will be on a different
line.

We call such multi-line byteranges "blocks".

We solve the problem of identifying blocks by making multiple passes over the
input. In the first pass, we recognize the start of blocks. Using simple brace
matching we discover the end of the block. We then make a byterange tuple out
of it and add it to list of outputs to be written to the header file.

### Location Code

Location code is a list of numbers that's essentially a hash of location of a
byterange in the scope hierarchy of the program with the following property: if
one location code occurs as a prefix in another, the first one is a transitive
parent of the second.

This is used to track the byteranges in which 'using' directives should
be activated. A 'using' declaration should be applied to all byteranges
whose location code contains the 'using' declaration's location code as
a prefix.

We keep track of the location code for every line we process, by counting
brace characters. When rewriting outputs to account for 'using' directives,
we will use any 'using' directive that contains this byterange in its scope.

The location code is essentially the index of the child that was followed
at every level to arrive at a node in a tree.

For a tree with just two children, the roots location code is the empty list,
the right child's code is [0] and the left child's is [1].

If the right child has one child, it's code will be [0, 0] etc.

"""

import hashlib
import random
import re
import StringIO
import sys

# Read the input into a list of a lines.
inbuf = sys.stdin.read();
lines = inbuf.split('\n')

# Flags indicate the characteristics of a byterange. Multiple flags can
# be set for a byte range.
VERBATIM_OUTPUT = 0
ADD_NAMESPACE_TO_IDENTIFIERS = 1
IS_ANONYMOUS_NAMESPACE = 2
IS_NAMED_NAMESPACE = 4
IS_COMMENT_BLOCK = 8
ADD_SEMICOLON = 16
REMOVE_BRACE = 32

# Byte ranges of input to write out. List of tuples. Extracted #include and
# function prototypes are added in the first pass. In the second pass multiline
# structures (called 'blocks' in this program) like structs, classes are added.
outputs = [];

# List of byteranges where blocks start. Used to extract structs, classes etc.
# byterange is for the entire line containing the start of the block.
block_starts = [];

# using declarations. List of tuples. Tuple is (symbol, full path).
using_decls = [];

# If there are comments right before a chunk we write into the header,
# then include the comments also.
#
# Byteranges that contain C++-style comment blocks. Consecutive lines
# of C++-style comments will be treated as a single comment block.
# C-style comment blocks are not handled.
comment_blocks = []

# Bookkeeping to track where the current comment block begins and ends.
cand_comment_block_start = -1
cand_comment_block_end = -1

# Bookkeeping to track the index of the beginning and end of the line
# currently being processed.
line_start_pos = 0
line_end_pos = 0

# Define a location code to the index of the various children that need to
# be traversed to get to a certain node. This is way to encode a path
# through a binary tree like: take first child, then second child.
#
# If a node P's location code is a prefix to another node C's location
# code, then P is a transitive parent of C.
#
# Used to check if a 'using' directive applies to a byterange.
line_location_code = []

# Used to keep track of number of children at any level. Index i
# contains the number of children discovered so far at that level.
# Used to generate location codes.
num_children_at_level = []

# Current scope level. Used to assign location codes.
scope_level = 0

# Updates num. children using characters in line. Returns new block level.
def update_num_children_at_level(line, num_children_at_level, scope_level):
  for c in line:
    if c == '{':
      scope_level += 1
      if len(num_children_at_level) < scope_level:
        num_children_at_level.append(1)
      else:
        num_children_at_level[scope_level-1] += 1
    elif c == '}':
      scope_level -= 1
  return scope_level

# Scan the input line by line.
for line in lines:
  line_start_pos = line_end_pos
  line_end_pos = line_start_pos + len(line) + 1

  # location code of line beginning is the location code for all chars in line.
  line_location_code = ','.join([str(x) for x in num_children_at_level[0:scope_level]])

  scope_level = update_num_children_at_level(line, num_children_at_level, scope_level)

  # Compute byteranges containing comment blocks. Every time a line
  # with a comment is found, start a new candidate comment block or extend
  # the candidate. When we encounter a non-comment line, add the
  # candidate comment block to list of comment blocks, if it's non-zero
  # length.
  if (line[0:2] == '//'):
    if (line_start_pos == cand_comment_block_end):
      # Extend the comment
      cand_comment_block_end = line_end_pos
    else:
      # Start a comment
      cand_comment_block_start = line_start_pos
      cand_comment_block_end = line_end_pos
    continue
  elif (cand_comment_block_end - cand_comment_block_start) > 0:
    comment_blocks.append((cand_comment_block_start, cand_comment_block_end, '', IS_COMMENT_BLOCK, line_location_code))
    cand_comment_block_start = -1
    cand_comment_block_end = -1

  # Skip blank lines.
  if len(line) == 0:
    pass

  # Pass through #include lines.
  elif line[0:9] == '#include ':
    outputs.append((line_start_pos, line_end_pos, '', VERBATIM_OUTPUT, line_location_code))
  
  # Pass through #define lines.
  elif line[0:8] == '#define ':
    outputs.append((line_start_pos, line_end_pos, '', VERBATIM_OUTPUT, line_location_code))
  
  # Track the mapping between short names and fully-namespaced names.
  # This mapping will be used to perform a final rewrite.
  # NOTE: Doesn't handle 'using foo = ns::internal::Foo', only
  # 'using ns::internal::Foo'.
  elif line[0:6] == 'using ':
    # Simple regex-based parser.
    match_obj = re.match('using\s+([^\s]+)\s*;\s*$', line)
    assert match_obj != None

    # Extract the last component of namespaced var.
    symbol = match_obj.group(1).split('::')[-1]

    # Save this using declaration.
    using_decls.append((symbol, match_obj.group(1), line_location_code))

  # Note down where structs and class definitions start. In
  # another pass, we'll extract the full struct.
  # matches multiline structs (ending with open brace) or single-line structs
  # ending with close brace and semicolon.
  elif ((line[-1] == '{') or (line[-2:] == '};')) and (
         ((line[0:7] == 'struct ') or (line[0:6] == 'class ') or (line[0:7] == 'inline ')) or
         ((line[0:10] == 'template <') and
           ((line.find('struct ', 10) != -1) or (line.find('class ', 10) != -1)))):
    block_starts.append((line_start_pos, line_end_pos, '', ADD_NAMESPACE_TO_IDENTIFIERS | ADD_SEMICOLON, line_location_code))

  # Note down where anonymous namespaces start. In another pass,
  # we'll extract the full extent and ignore definitions within.
  elif (line == 'namespace {'):
    block_starts.append((line_start_pos, line_end_pos, '', IS_ANONYMOUS_NAMESPACE, line_location_code))

  # Note down where named namespaces start. In another pass,
  # we'll extract the full extent and write these out.
  elif (line[0:10] == 'namespace ') and (line[-1] == '{'):
    block_starts.append((line_start_pos, line_end_pos, '', IS_NAMED_NAMESPACE, line_location_code))

  # Pass through typedefs.
  elif (line[0:8] == 'typedef '):
    outputs.append((line_start_pos, line_end_pos, '', ADD_NAMESPACE_TO_IDENTIFIERS, line_location_code))

  # Pass through function prototype lines. A line that:
  #   does not start with 'struct' or 'class' or whitespace.
  #   ends with '{'
  #   no '::' before '( in it (to avoid member function implementations).
  #   doesn't have DioTest in it (not a unit test).
  elif ((line[0:7] != 'struct ') and (line[0:6] != 'class') and (line[0:9] != 'namespace') and
        (line[0] != ' ') and
        ((line.find('::') == -1) or (line.find('::') > line.find('('))) and
        (line.find('DioTest') == -1) and (line[-1] == '{')):
    outputs.append((line_start_pos, line_end_pos-2, ';\n',
                    ADD_NAMESPACE_TO_IDENTIFIERS | ADD_SEMICOLON | REMOVE_BRACE, line_location_code))

# List of tuples of the for (start, end) of anonymous namespaces.
# Used to avoid writing out outputs (functions, structs etc) that
# lie within anonymous namespaces.
anonymous_namespaces = []

# Extract the full struct, class, and anonymous namespace definitions.
# Simple algo: Find the matching close brace.
for block_start in block_starts:
  block_start_pos = block_start[0]
  block_end_pos = block_start[0]
  flags = block_start[3]
  block_location_code = block_start[4]
  add_namespace_to_identifiers = ((flags & ADD_NAMESPACE_TO_IDENTIFIERS) != 0)
  is_anonymous_namespace = ((flags & IS_ANONYMOUS_NAMESPACE) != 0)
  is_named_namespace = ((flags & IS_NAMED_NAMESPACE) != 0)
  add_semicolon = ((flags & ADD_SEMICOLON) != 0)

  level = 0
  num_one_crossings = 0
  while (block_end_pos < len(inbuf)) and ((level > 0) or (num_one_crossings < 2)):
    if inbuf[block_end_pos] == '{':
      level += 1;
      if level == 1:
        num_one_crossings += 1
    if inbuf[block_end_pos] == '}':
      if level == 1:
        num_one_crossings += 1
      level -= 1;
    block_end_pos += 1
  if level == 0:
    if is_anonymous_namespace:
      anonymous_namespaces.append((block_start_pos, block_end_pos))
    elif is_named_namespace:
      # Write out two outputs, one each for the start and end of the namespace.
      outputs.append(block_start)

      # Hack. Don't precisely calculate the block end's location code. It's
      # not needed. yet.
      block_end_location_code = block_location_code

      # the -1 is account for the increment to block_end_pos after level hits 0.
      outputs.append((block_end_pos-1, block_end_pos, '\n', flags, block_end_location_code))
    else:
      outputs.append((block_start_pos, block_end_pos, ';' if add_semicolon else '', flags, block_location_code))
  else:
    print 'Unbalanced starting at: ', block_start_pos

# sort by starting position of range.
outputs.sort()

# TODO: Make this more robust. Ignore with strings. One pass
# algo instead of multipass.
def do_replacements(add_namespace_to_identifiers, inbuf, output, cand_replacements):
  if add_namespace_to_identifiers:
    replacements = []
    for replacement in cand_replacements:
      if output[4].startswith(replacement[2]):
        replacements.append(replacement)

    s = inbuf[output[0]:output[1]]
    for replacement in replacements:
      s = s.replace(replacement[0], replacement[1])

    t = output[2]
    for replacement in replacements:
      t = t.replace(replacement[0], replacement[1])

    return s + t
  else:
    return inbuf[output[0]:output[1]]

def output_is_within_anonymous_namespace(output, anonymous_namespaces):
  start = output[0]
  endpos = output[1]
  for anonymous_namespace in anonymous_namespaces:
    ns_start = anonymous_namespace[0]
    ns_endpos = anonymous_namespace[1]
    if (start >= ns_start) and (endpos <= ns_endpos):
      return True
  return False

def comment_block_ending_before_output(output, comment_blocks):
  for comment_block in comment_blocks:
    if output[0] == comment_block[1]:
      return comment_block
  return None

def write_byterange(output):
  flags = output[2]
  add_semicolon = ((flags & ADD_SEMICOLON) != 0)
  remove_brace = ((flags & REMOVE_BRACE) != 0)

  out_str = inbuf[output[0]:output[1]]
  if remove_brace:
    out_str = inbuf[out_str.rfind('{')+1:]

  sys.stdout.write(out_str)

  if add_semicolon:
    sys.stdout.write(';\n\n')

def write_string_and_update_hash(s, flags, hasher, output_buffer):
  if ((flags & IS_COMMENT_BLOCK) != 0):
    output_buffer.write('\n')
    hasher.update('\n')

    output_buffer.write(s)
    hasher.update(s)
  else:
    output_buffer.write(s)
    hasher.update(s)

    output_buffer.write('\n')
    hasher.update('\n')

# Write out include guard etc.
define_guard = str(random.getrandbits(128))
output_buffer = StringIO.StringIO()
hasher = hashlib.sha1()

for output in outputs:
  if not output_is_within_anonymous_namespace(output, anonymous_namespaces):
    cand_comment_block = comment_block_ending_before_output(output, comment_blocks)
    if cand_comment_block != None:
      write_string_and_update_hash(inbuf[cand_comment_block[0]:cand_comment_block[1]],
                   cand_comment_block[3], hasher, output_buffer)

    add_namespace_to_identifiers = ((output[3] & ADD_NAMESPACE_TO_IDENTIFIERS) != 0)
    output_string = do_replacements(add_namespace_to_identifiers, inbuf, output, using_decls)
    write_string_and_update_hash(output_string, output[3], hasher, output_buffer)

sys.stdout.write('#ifndef MH' + hasher.hexdigest() + '\n')
sys.stdout.write('#define MH' + hasher.hexdigest() + '\n')
sys.stdout.write('\n');
sys.stdout.write(output_buffer.getvalue())
sys.stdout.write('#endif\n')
