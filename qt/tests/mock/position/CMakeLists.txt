# vim:expandtab:shiftwidth=2:tabstop=2:

# Copyright (C) 2014-2015 Canonical Ltd.

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

set(PLUGIN mock_position)

set(PLUGIN_SRCS
    source.cc
    plugin.cc)

add_library(${PLUGIN} SHARED ${PLUGIN_SRCS})
target_link_libraries(${PLUGIN}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Positioning_LIBRARIES})
target_compile_definitions(${PLUGIN} PRIVATE -DQT_PLUGIN)
set_target_properties(
    ${PLUGIN} PROPERTIES
    AUTOMOC TRUE
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../plugins/position)
