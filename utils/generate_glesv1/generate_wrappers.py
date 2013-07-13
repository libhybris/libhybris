#!/usr/bin/python
# gnerate_wrappers.py: Parse header and output libhybris binding code
# Adapted for libhybris from apkenv-wrapper-generator
#
# apkenv-wrapper-generator version:
# Copyright (C) 2012 Thomas Perl <m@thp.io>; 2012-10-19
#
# libhybris version:
# Copyright (C) 2013 Jolla Ltd.
# Contact: Thomas Perl <thomas.perl@jollamobile.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import re
import os
import sys

funcs = []

# Some special cases to to avoid having to parse the full C grammar
FIXED_MAPPINGS = {
        'GLint *': 'GLint *',
        'GLuint *': 'GLuint *',
        'GLsizei *': 'GLsizei *',
        'GLfloat eqn[4]': 'GLfloat[4]',
        'GLfixed eqn[4]': 'GLfixed[4]',
        'GLfixed mantissa[16]': 'GLfixed[16]',
        'GLint exponent[16]': 'GLint[16]',
        'GLfloat eqn[4]': 'GLfloat[4]',
        'GLvoid **': 'GLvoid **',
        'GLfloat *': 'GLfloat *',
        'GLfixed *': 'GLfixed *',
}

# Never generate wrappers for these functions
BLACKLISTED_FUNCS = [
]

def clean_arg(arg):
    arg = arg.replace('__restrict', '')
    arg = arg.replace('__const', '')
    arg = arg.replace('const', '')
    return arg.strip()

def mangle(name):
    return 'my_' + name

class Argument:
    def __init__(self, type_, name):
        self.type_ = type_
        self.name = name

class Function:
    def __init__(self, retval, name, args):
        self.retval = retval
        self.name = name
        self.raw_args = args
        self.parsing_error = None
        self.args = list(self._parse_args(args))

    def _parse_args(self, args):
        for arg in map(clean_arg, args.split(',')):
            arg = clean_arg(arg)
            xarg = re.match(r'^(.*?)([A-Za-z0-9_]+)$', arg)
            if not xarg:
                # Unknown argument
                if arg in FIXED_MAPPINGS:
                    yield Argument(FIXED_MAPPINGS[arg], '__undefined__')
                    continue
                self.parsing_error = 'Could not parse: ' + repr(arg)
                print self.parsing_error
                continue
            type_, name = xarg.groups()
            if type_ == '':
                type_ = name
                name = '__undefined__'
            yield Argument(type_, name)

if len(sys.argv) != 2:
    print >>sys.stderr, """
    Usage: %s glesv1_functions.h
    """ % (sys.argv[0],)
    sys.exit(1)

filename = sys.argv[1]

for line in open(filename):
    if line.startswith('/*'):
        continue
    retval, funcname, args = re.match(r'^(.+ [*]?)([A-Za-z0-9_]+)\((.*)\);\s*$', line).groups()
    retval, funcname, args = [x.strip() for x in (retval, funcname, args)]
    if funcname not in BLACKLISTED_FUNCS:
        funcs.append(Function(retval, funcname, args))

LIBRARY_NAME = 'glesv1_cm'

for function in funcs:
    args = [a.type_.strip() for a in function.args if a.type_.strip() not in ('', 'void')]
    if function.retval == 'void':
        print 'HYBRIS_IMPLEMENT_VOID_FUNCTION%d(%s, %s);' % (len(args), LIBRARY_NAME, ', '.join([function.name] + args))
    else:
        print 'HYBRIS_IMPLEMENT_FUNCTION%d(%s, %s, %s);' % (len(args), LIBRARY_NAME, function.retval, ', '.join([function.name] + args))

