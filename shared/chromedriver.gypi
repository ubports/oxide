# Copyright (C) 2013 Canonical Ltd.

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

{
  'variables': {
    'protoc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
  },
  'includes': [ ],
  'targets': [
    {
      'target_name': 'oxide_chromedriver',
      'type': 'none',
      'hard_dependency': 1,
      'variables': { 'protoc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/protoc_out', },
      'dependencies': [
        '<(DEPTH)/chrome/chrome.gyp:chromedriver',
      ],
    },
  ],
}
