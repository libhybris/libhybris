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
    """Add symbol files for files in /proc/pid/maps:  load-sym-file symbols-directory"""
    def __init__(self):
        gdb.Command.__init__(self, "load-sym-files", gdb.COMMAND_FILES, gdb.COMPLETE_FILENAME, True)

    def invoke(self, arg, from_tty):
        arg_list = gdb.string_to_argv(arg)

        if len(arg_list) < 1:
            print "usage: load-sym-files symbols-dir"
            return

        symdir = arg_list[0]
        if not os.path.isdir(symdir):
            print "symbol directory is invalid"
            return

        pid = gdb.selected_inferior().pid
        maps = open("/proc/%d/maps"%pid,"rb")
        for line in maps:
            # b7fc9000-b7fcf000 r-xp 00000000 08:01 1311443    /system/lib/liblog.so
            m = re.match("([0-9A-Fa-f]+)-[0-9A-Fa-f]+\s+r-xp.*(/system/lib.*)", line)
            if not m:
                continue

            start_addr = int(m.group(1),16)
            lib        = m.group(2)
            text_addr  = 0

            p = subprocess.Popen("objdump -h "+lib , shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
            for header in  p.stdout.read().split("\n"):
                #6 .text         00044ef7  00017f80  00017f80  00017f80  2**4
                t = re.match("\s*[0-9]+\s+\.text\s+([0-9A-Fa-f]+\s+){3}([0-9A-Fa-f]+)",header)
                if t:
                    text_addr = int(t.group(2),16)
                    break
            symfile = symdir+lib
            if os.path.isfile(symfile):
                gdb.execute("add-symbol-file %s 0x%X" % (symfile,start_addr+text_addr))


LoadSymFiles()
