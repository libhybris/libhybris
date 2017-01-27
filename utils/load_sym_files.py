# Copyright (c) 2013 Adrian Negreanu <groleo@gmail.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

import os
import re
import subprocess

class LoadSymFiles(gdb.Command):
    """Add symbol files for files in /proc/pid/maps:  load-sym-file [symbols-directory] [lib-directory]"""
    """Usage example:
        start           # breakpoint 1
        set confirm off
        b dlfcn.c:56    # breakpoint 2, android_dlopen
        commands 2      # commands to run when breakpoint 2 is hit
        source ../utils/load_sym_files.py
        load-sym-files
        continue
        end             # end of commands for bkpt 2
    """
    def __init__(self):
        gdb.Command.__init__(self, "load-sym-files", gdb.COMMAND_FILES, gdb.COMPLETE_FILENAME, True)

    def invoke(self, arg, from_tty):
        arg_list = gdb.string_to_argv(arg)
        symdir = "/system/symbols"
        libdir = "/system/lib"

        if len(arg_list) >= 1:
            symdir = arg_list[0]

        if not os.path.isdir(symdir):
            print("error: invalid symbol directory '%s'" % symdir)
            print("usage: load-sym-files [symbols-dir] [lib-dir]")
            return

        if len(arg_list) == 2:
            libdir = arg_list[1]

        if not os.path.isdir(libdir):
            print("error: invalid library directory '%s'" % libdir)
            print("usage: load-sym-files [symbols-dir] [lib-dir]")
            return

        try:
            pid = gdb.selected_inferior().pid
        except AttributeError:
            # in case gdb-version < 7.4
            # the array can have more than one element,
            # but it's good enough when debugging test_egl
            if len(gdb.inferiors()) == 1:
                pid = gdb.inferiors()[0].pid
            else:
                print("error: no gdb support for more than 1 inferior")
                return

        if pid == 0:
            print("error: debugee not started yet")
            return

        maps = open("/proc/%d/maps"%pid,"rb")
        for line in maps:
            # b7fc9000-b7fcf000 r-xp 00000000 08:01 1311443    /system/lib/liblog.so
            # m = re.match("([0-9A-Fa-f]+)-[0-9A-Fa-f]+\s+r-xp.*(%s.*)" % libdir, str(line))
            m = re.match("([0-9A-Fa-f]+)-[0-9A-Fa-f]+\sr-xp\s.*\s(.*)\\n", line.decode('ascii'))
            if not m:
                continue

            start_addr = int(m.group(1), 16)
            lib        = m.group(2)
            text_addr  = 0

            if not lib.startswith(libdir):
                continue

            p = subprocess.Popen("objdump -h " + lib , shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
            for header in p.stdout.read().decode('ascii').split("\n"):
                #6 .text         00044ef7  00017f80  00017f80  00017f80  2**4
                t = re.match("\s*[0-9]+\s+\.text\s+([0-9A-Fa-f]+\s+){3}([0-9A-Fa-f]+)", header)
                if t:
                    text_addr = int(t.group(2),16)
                    break

            symfile = symdir + lib
            if os.path.isfile(symfile):
                gdb.execute("add-symbol-file %s 0x%X" % (symfile, start_addr+text_addr))

LoadSymFiles()
