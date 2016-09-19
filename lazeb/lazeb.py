"""'Lazeb' is a tiny tool inspired by Bazel that makes it easy to build
projects that follow libeli5 conventions. It generates a Makefile from a simple
declarative config file. 'make' is ultimately used to finally build the
targets.

TODO(ranga): Document libel5 C++ coding conventions.

It's fast enough (about 50ms for a small project) to be used a preprocessing step
before all make commands.

Syntax
------

1. lazeb
2. lazeb <input BUILD file> <output Make file>

When run without any command-line arguments, will read a file named 'BUILD' from
the current directory and write output to a file named 'Makefile' in current
directory.

Otherwise you need to give two arguments. First one is config filename and
second one is the output Makefile.

Why Lazeb?
----------

We originally started out trying to use make's implicit rules, include files,
gcc -MM feature etc to avoid developing yet another build tool.

But it turned out to be too confusing to write a generic Makefile do this.
Since one of the goals of libeli5 is that the implementation should also
be clear, it felt reasonable write preprocessor tool that generates Makefiles.

We adopted a subset of the config file format of Bazel instead of developing
a new format. It should be pretty simple to convert Lazeb config files
to Bazel.

Why not Bazel itself?
---------------------

To heavyweight for small projects. Not DRY enough (need to write dependencies
in BUILD file in addition to specifying them in #includes in .cc files).

Example
-------

    config(header = \"\"\"
    include conventions.mk

    CXXFLAGS+=-g -O0
    \"\"\")

    # Make a binary named foo_main out of foo.cc.
    # Will also automatically link in all the (transitive)
    # .o files corresponding to the .h files included by
    # foo.cc.
    cc_binary(name="foo")

    # Make a binary named scratch_test_main out of scratch.cc
    # and files it includes. Will use the main() supplied by
    # Diogenes testing framework.
    cc_test(name="scratch")

    # Limited support for targets in subdirs.
    # Make a binary named x_foo_main in current directory using x/foo.cc and
    # files it includes.
    cc_binary(name="x_foo_main", sources=["x/foo.cc"])

    # To force the binary to go into a subdir, specify the
    # output explicitly.
    cc_binary(name="x_foo_main", sources=["x/foo.cc"], output="x/foo_main")

"""

#!/usr/bin/python

import sys
import os
import attr

# ================ methods callable from BUILD file ==================


def config(header):
    build.header = header


def cc_binary(name, sources=None, output=None):
    if output == None:
        output = name + '_main'

    if sources == None:
        sources = [name + '.cc']

    binary = Binary(name=name, sources=sources, output=output, is_test=False)
    build.binaries.append(binary)
    return binary


def cc_test(name, source=None, output=None):
    if output == None:
        output = name + '_test_main'

    if source == None:
        source = name + '.cc'

    test = Binary(name=name, sources=[source], output=output, is_test=True)
    build.binaries.append(test)
    return test


def cc_objfile(name, source=None, outputs=None):
    if outputs == None:
        outputs = [name + '.o', name + '.h']

    if source == None:
        source = name + '.cc'

    objfile = Objfile(name=name, source=source, outputs=outputs)
    build.objfiles.append(objfile)
    return objfile

# ========= Classes to represent a BUILD file in memory. =============


@attr.s
class Binary(object):
    """Representation of a cc_binary rule.

    Conceptually represents an invocation of the linker.

    """
    name = attr.ib(default='')
    output = attr.ib(default='')
    sources = attr.ib(default=attr.Factory(list))
    is_test = attr.ib(default=False)


@attr.s
class Objfile(object):
    """Representation of a cc_objfile rule.

    Conceptually represents the invocation of cc -c and cpp-makeheader.

    """
    name = attr.ib(default='')
    outputs = attr.ib(default=attr.Factory(list))
    source = attr.ib(default='')
    written = attr.ib(default=False)


@attr.s
class Build(object):
    """Represents all the rules in a BUILD file."""
    header = attr.ib(default='')
    binaries = attr.ib(default=attr.Factory(list))
    objfiles = attr.ib(default=attr.Factory(list))
    basedir = attr.ib(default='')

# ================= Routines that add implicit cc_objfile BUILD rules =========


def compute_included_files(filename):
    """Returns project header files included by filename."""
    output = []
    with open(filename, 'r') as f:
        for l in f.readlines():
            line = l.strip()
            if line.startswith('#include "'):
                output.append(line[10:-1])
    return output


def target_providing_filename(build, filename):
    for binary in build.binaries:
        if filename == binary.output:
            return binary

    for objfile in build.objfiles:
        if filename in objfile.outputs:
            return objfile

    return None


def add_implicit_objfile_rules_for_objfile(objfile, build):
    """An objfile rule outputs foo.o and foo.h. foo.o depends on foo.cc and all
    headers included by foo.cc. A so-included header bar.h depends on bar.cc
    and so on.

    If there no cc_objfile rule that generates bar.h in the BUILD file,
    we assume such a rule was intended. This method will literally call
    cc_objfile to add such a rule on behalf of the user.

    """

    source = objfile.source
    included_files = compute_included_files(
        os.path.join(build.basedir, source))

    for included_file in included_files:
        provider = target_providing_filename(build, included_file)
        if not provider:
            # No rule in the BUILD file generates bar.h. Assume it's
            # from bar.cc and generate an cc_objfile rule to generate
            # bar.h from bar.cc.
            root_ext = os.path.splitext(included_file)
            target_name = root_ext[0]
            objfile = cc_objfile(target_name)

            # Analyze the files included by bar.cc and generate it's deps.
            add_implicit_objfile_rules_for_objfile(objfile, build)


def add_implicit_objfile_rules_for_binary(binary, build):
    """A binary foo_main depends on foo.o and the .o file corresponding to all
    project headers it includes (transitively).

    Walk the tree and generate the cc_objfile rules for all the headers
    and .o files this binary depends on.

    """

    for source in binary.sources:
        root_ext = os.path.splitext(source)
        dot_o = root_ext[0] + '.o'
        if not target_providing_filename(build, dot_o):
            # No rule found. Assume user intended to generate foo.o
            # from foo.cc. Add an objfile file that does this.
            objfile = cc_objfile(root_ext[0])

            # And add objfile rules to generate the .h file foo.cc
            # depends on.
            add_implicit_objfile_rules_for_objfile(objfile, build)

# ================= Routines that add implicit cc_objfile BUILD rules =========

# ========== Routines that traverse the built tree and discover the set of
# files that need to be linked together to make a binary. ====================


def compute_link_set_for_objfile(objfile, build):
    """Figure out all the .o that need to be linked into binary to satisfy the.

    .o generated by objfile.

    The answer all the .o's corresponding to the .h's transitively
    included by the .cc file of this objfile rule.

    """

    linkset = []
    source = objfile.source
    included_files = compute_included_files(
        os.path.join(build.basedir, source))

    for included_file in included_files:
        provider = target_providing_filename(build, included_file)
        assert provider != None
        assert isinstance(provider, Objfile)

        # Link in the .o corresponding to every .h file included ...
        for output in provider.outputs:
            root_ext = os.path.splitext(output)
            if root_ext[1] == '.o':
                linkset.append(output)

        # ... and the .o's needed to statisfy *that* .o.
        linkset += compute_link_set_for_objfile(provider, build)

    return sorted(set(linkset))


def compute_link_set_for_binary(binary, build):
    """Figure out all the .o that need to be linked together to make a binary.

    foo_main links foo.o and .o of all project .h's it transitively
    includes.

    """

    # foo_main depends on foo.o.
    root_exts = [os.path.splitext(source) for source in binary.sources]
    source_dot_o_s = [root_ext[0] + '.o' for root_ext in root_exts]

    # ... and the .o's of recursively included header files.
    transitive_dot_o_s = []

    for source_dot_o in source_dot_o_s:

        # objrule foo.cc provides foo.o.
        objfile = target_providing_filename(build, source_dot_o)
        assert objfile != None
        assert isinstance(objfile, Objfile)

        # Figure out what .o's need to be linked to satisfy foo.cc.
        transitive_dot_o_s += compute_link_set_for_objfile(objfile, build)

    return sorted(set(source_dot_o_s + transitive_dot_o_s))


# ================= Routines that finally write Makefile rules ===========

def write_objfile_compile_rule(objfile, build, f):
    if objfile.written:
        return

    objfile.written = True

    source = objfile.source
    included_files = compute_included_files(
        os.path.join(build.basedir, source))

    # TODO(ranga): Fix this. A header file can be rebuilt in parallel
    # with a header file it includes. Making the header output of an
    # objfile rule depend on the header output another objfile file
    # (the output[1] case below), will serialize this step.
    for output in objfile.outputs:
        f.write(output + ': ' + source + ' ')
        f.write(' '.join(included_files))
        f.write('\n')

    for included_file in included_files:
        provider = target_providing_filename(build, included_file)
        # print 'provider(', included_file, ') =', provider
        write_objfile_compile_rule(provider, build, f)


def write_binary_link_rule(binary, build, f):
    root_exts = [os.path.splitext(source) for source in binary.sources]
    linkset = compute_link_set_for_binary(binary, build)
    f.write(binary.output + ': ')
    f.write(' '.join(linkset))
    f.write('\n')

    # write the rules for the objfiles the binary depends on.
    for dot_o in linkset:
        objfile = target_providing_filename(build, dot_o)
        assert objfile != None
        assert isinstance(objfile, Objfile)
        write_objfile_compile_rule(objfile, build, f)


def write_clean(build, f):
    rm_list = []
    for binary in build.binaries:
        rm_list.append(binary.output)

    for objfile in build.objfiles:
        for output in objfile.outputs:
            rm_list.append(output)

    f.write('clean:\n')
    f.write('\trm -f ' + ' '.join(rm_list))
    f.write('\n')


def write_makefile(build, f):
    for binary in build.binaries:
        add_implicit_objfile_rules_for_binary(binary, build)

    f.write('# build:')
    f.write(str(build))
    f.write('\n')

    f.write(build.header)
    f.write('\n')

    # Write an all target that builds all binaries.
    f.write('all: ')
    f.write(' '.join([binary.output for binary in build.binaries]))
    f.write('\n')

    for binary in build.binaries:
        write_binary_link_rule(binary, build, f)

    write_clean(build, f)

# ================= End of routines that write Makefile rules ============


# ========== Start of main() =============

assert (len(sys.argv) == 1) or (len(sys.argv) == 3)

build = Build()

build_filename = 'BUILD'
if len(sys.argv) == 3:
    build_filename = sys.argv[1]

output_filename = 'Makefile'
if len(sys.argv) == 3:
    output_filename = sys.argv[2]

build.basedir = os.path.dirname(build_filename)

run_context = {'build': build, 'cc_binary': cc_binary,
               'cc_test': cc_test, 'cc_objfile': cc_objfile, 'config': config}
execfile(build_filename, run_context)

with open(output_filename, 'w') as of:
    write_makefile(build, of)