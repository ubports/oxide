#!/usr/bin/python

# Copyright (C) 2016 Canonical Ltd.

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

from __future__ import print_function

from optparse import OptionParser
import os
import platform
import re
from StringIO import StringIO
import sys

EXTRA_ARGS_RE = r'([^=]+)=(.+)'

def HostArch():
  machine = platform.machine()

  if re.match(r'i.86', machine):
    return "x86"
  elif machine in ["x86_64", "amd64"]:
    if platform.architecture()[0] == "32bit":
      return "x86"
    return "x64"
  elif host_arch.startswith("arm"):
    return "arm"

def GetSymbolLevel(enabled, host_arch, is_component_build):
  if not enabled:
    return 0
  if (host_arch == "arm" or host_arch == "x86") and not is_component_build:
    return 1
  return 2

class Options(OptionParser):
  def __init__(self):
    OptionParser.__init__(self)

    self.add_option("--component-build", action="store_true",
                    help="Enable a component build")
    self.add_option("--enable-debug-symbols", action="store_true",
                    help="Whether to enable debug symbols")
    self.add_option("--libexec-subdir",
                    help="The subdirectory for Oxide components relative to "
                         "the core library location")
    self.add_option("--lib-extension", help="The core library file extension")
    self.add_option("--lib-name", help="The core library name")
    self.add_option("--output-dir",
                    help="The destination directory for build files")
    self.add_option("--platform", help="The Oxide platform to build")
    self.add_option("--renderer-name",
                    help="The filename of the renderer executable")

    self.add_option("-D", action="append", dest="extra_args", type="string",
                    help="Addition build arguments")

class ArgsGnWriter(object):
  def __init__(self):
    self._output_stream = StringIO()
    self._output_stream.write(
"""# THIS FILE IS AUTOMATICALLY GENERATED!
# All manual changes to this file will be lost.
# See "gn.oxide args <out_dir> --list" for available build arguments.
""")

  def WriteBool(self, prop, val):
    self._output_stream.write("%s = %s\n" % (prop, "true" if val else "false"))

  def WriteInt(self, prop, val):
    assert type(val) == int
    self._output_stream.write("%s = %s\n" % (prop, val))

  def WriteString(self, prop, val):
    self._output_stream.write("%s = \"%s\"\n" % (prop, val))

  def WriteStringList(self, prop, *args):
    self._output_stream.write("%s = [ " % prop)
    first_entry = True
    for arg in args:
      if not first_entry:
        self._output_stream.write(", ")
      first_entry = False
      self._output_stream.write("\"%s\"" % arg)
    self._output_stream.write(" ]\n")

  def SaveToFile(self, path):
    with open(path, "w") as fd:
      fd.write(self._output_stream.getvalue())

def WriteStaticArgs(writer):
  writer.WriteBool("is_oxide", True)
  writer.WriteStringList("root_extra_deps", "//oxide:oxide_all")

  # TODO(chrisccoulson): Support clang builds
  writer.WriteBool("is_clang", False)

  # We statically link tcmalloc in to the renderer binary, rather than
  # linking it in base
  writer.WriteString("use_allocator", "none")

  # The shim results in us overriding malloc for any app linked against us
  writer.WriteBool("use_experimental_allocator_shim", False)

  writer.WriteBool("enable_nacl", False)
  writer.WriteBool("is_component_ffmpeg", True)

  # XXX(chrisccoulson): This produces an error:
  #  "ERROR at //build/toolchain/gcc_toolchain.gni:520:30: Clobbering existing value."
  #  use_gold = true is the default anyway
  #writer.WriteBool("use_gold", True)
  writer.WriteBool("linux_use_bundled_gold", False)

  writer.WriteBool("use_sysroot", False)
  writer.WriteBool("use_ash", False)
  writer.WriteBool("use_aura", True)
  writer.WriteBool("use_cups", False)
  writer.WriteBool("use_gconf", False)
  writer.WriteBool("use_ozone", True)
  writer.WriteBool("toolkit_views", False)
  writer.WriteBool("enable_basic_printing", False)
  writer.WriteBool("enable_print_preview", False)
  writer.WriteBool("enable_extensions", True)

def WriteConfigurableArgs(writer, options):
  host_arch = HostArch()
  writer.WriteInt("symbol_level",
                  GetSymbolLevel(options.enable_debug_symbols,
                                 host_arch,
                                 options.component_build))

  if options.component_build:
    writer.WriteBool("is_component_build", True)

  writer.WriteString("oxide_libexec_subdir", options.libexec_subdir)
  writer.WriteString("oxide_lib_extension", options.lib_extension)
  writer.WriteString("oxide_lib_name", options.lib_name)
  writer.WriteString("oxide_platform", options.platform)
  writer.WriteString("oxide_renderer_name", options.renderer_name)

def WriteExtraArgs(writer, args):
  if not args:
    return

  for arg in args:
    m = re.match(EXTRA_ARGS_RE, arg)
    prop = m.groups()[0]
    value = m.groups()[1]
    if value in ("true", "false"):
      writer.WriteBool(prop, True if value == "true" else False)
    elif value.isdigit():
      writer.WriteInt(prop, int(value))
    else:
      writer.WriteString(prop, value)

def SanitizeOptions(options):
  if not options.output_dir:
    print("No output directory specified!", file=sys.stderr)
    sys.exit(1)

  if not options.libexec_subdir:
    print("Missing option --libexec-subdir", file=sys.stderr)
    sys.exit(1)
  if not options.lib_extension:
    print("Missing option --lib-extension", file=sys.stderr)
    sys.exit(1)
  if not options.lib_name:
    print("Missing option --lib-name", file=sys.stderr)
    sys.exit(1)
  if not options.platform:
    print("Missing option --platform", file=sys.stderr)
    sys.exit(1)
  if not options.renderer_name:
    print("Missing option --renderer-name", file=sys.stderr)
    sys.exit(1)

  if options.extra_args:
    for arg in options.extra_args:
      if not re.match(EXTRA_ARGS_RE, arg):
        print("Invalid extra argument '%s'" % arg, file=sys.stderr)
        sys.exit(1)

def WriteGnArgs(options):
  writer = ArgsGnWriter()

  WriteStaticArgs(writer)
  WriteConfigurableArgs(writer, options)
  WriteExtraArgs(writer, options.extra_args)

  output_dir = os.path.abspath(options.output_dir)

  if not os.path.isdir(output_dir):
    os.makedirs(output_dir)

  writer.SaveToFile(os.path.join(output_dir, "args.gn"))

def main(argv):
  optparser = Options()
  (opts, args) = optparser.parse_args(argv)

  SanitizeOptions(opts)
  WriteGnArgs(opts)

if __name__ == "__main__":
  main(sys.argv[1:])
