#!/usr/bin/python
# vim:expandtab:shiftwidth=2:tabstop=2:

# Copyright (C) 2013-2016 Canonical Ltd.

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
import sys
from urlparse import urlsplit

sys.dont_write_bytecode = True
os.environ["PYTHONDONTWRITEBYTECODE"] = "1"

sys.path.insert(0, os.path.join(os.path.dirname(__file__), os.pardir, "build", "python"))
from constants import (
  OXIDEDEPS_FILE,
  OXIDESRC_DIR,
  TOPSRC_DIR
)
from utils import (
  CheckCall,
  LoadJsonFromPath
)

def AddOriginPushUrl(path, origin, user_id):
  u = urlsplit(origin)
  if u.netloc != "git.launchpad.net":
    print("Unexpected origin '%s' found for git checkout '%s'" %
          (u.netloc, path), file=sys.stderr)
    sys.exit(1)
  u = u._replace(scheme="git+ssh")
  u = u._replace(netloc="%s@%s" % (user_id, u.netloc))
  CheckCall(["git", "remote", "set-url", "--push", "origin", u.geturl()], path)

class Options(OptionParser):
  def __init__(self):
    OptionParser.__init__(self, description="""
Configure repositories listed in DEPS.oxide to allow pushing directly to
origin. In a normal checkout, origin is configured with a read-only URL
by default.

Note that working branches should be pushed to your personal repository. Most
workflows should not need to use this script.""")

    self.set_usage("%prog [options] [path1] [path2] [path..]")
    self.add_option("-a", "--all", action="store_true",
                    help="Configure all branches listed in DEPS.oxide to allow "
                         "pushing directly to origin")
    self.add_option("-u", "--user-id", help="Your Launchpad user ID")

def main():
  o = Options()
  (options, args) = o.parse_args()
  if options.all and len(args) > 0:
    print("Mixing --all with specific paths does not make sense",
          file=sys.stderr)
    o.print_usage(file=sys.stderr)
    sys.exit(1)

  if not options.user_id:
    print("Missing --user-id option", file=sys.stderr)
    o.print_usage(file=sys.stderr)
    sys.exit(1)

  topsrc_dir_parent = os.path.dirname(TOPSRC_DIR)
  deps = LoadJsonFromPath(OXIDEDEPS_FILE)

  if options.all:
    for path in deps:
      AddOriginPushUrl(os.path.join(topsrc_dir_parent, path),
                       deps[path]["origin"],
                       options.user_id)
  else:
    if len(args) == 0:
      print("Missing paths to git checkouts", file=sys.stderr)
      o.print_usage(file=sys.stderr)
      sys.exit(1)
    paths = [ os.path.join(topsrc_dir_parent, path) for path in deps ]
    paths.append(OXIDESRC_DIR)
    for arg in args:
      path = os.path.abspath(arg)
      if path not in paths:
        print("'%s' is not the path of a repository found in DEPS.oxide" %
              path, file=sys.stderr)
        sys.exit(1)
      AddOriginPushUrl(path,
                       deps[os.path.relpath(path, topsrc_dir_parent)]["origin"],
                       options.user_id)

if __name__ == "__main__":
  main()