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

include(PackageConfigHelper)

set(OXIDE_MOCK_QTPLUGIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/tests/mock/plugins)
set(OXIDE_QMLPLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/out/imports)

set(OXIDE_QUICKLIB OxideQtQuick)

find_package(Qt5Core 5.2.0 REQUIRED)
find_package(Qt5Gui 5.2.0 REQUIRED)
find_package(Qt5Qml 5.2.0 REQUIRED)
# 5.2.0 is required for us to be able to set a share context
find_package(Qt5Quick 5.2.0 REQUIRED)
if(ENABLE_TESTS)
  find_package(Qt5Feedback REQUIRED)
  find_package(Qt5Positioning REQUIRED)
  find_package(Qt5Test REQUIRED)
  find_package(Qt5QuickTest REQUIRED)
  find_package(Qt5PlatformSupport REQUIRED)
endif()

if(NOT CMAKE_CROSSCOMPILING)
  # qmake isn't multi-arch aware as it installs arch-specific mkspec files
  # in to /usr/share, so we can't use it here (we'd need a qmake binary
  # for the host arch using data for the target arch)
  find_program(QMAKE_EXECUTABLE qmake)
  if(QMAKE_EXECUTABLE STREQUAL "QMAKE_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "qmake not found")
  endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_qmlapp.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run_qmlapp.sh
    IMMEDIATE @ONLY)
file(INSTALL ${CMAKE_CURRENT_BINARY_DIR}/run_qmlapp.sh
     DESTINATION ${OXIDE_BIN_OUTPUT_DIR}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
         GROUP_READ GROUP_EXECUTE
         WORLD_READ WORLD_EXECUTE)

configure_and_install_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/core/api/OxideQtCoreConfig.cmake.in)

# See https://launchpad.net/1578808 - QOpenGLFunctions methods in builds
#  that define QT_OPENGL_ES_2 use GLESv2 symbols inline
set(CMAKE_REQUIRED_INCLUDES ${Qt5Gui_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${Qt5Gui_LIBRARIES})
check_cxx_source_compiles(
    "#include <QOpenGLFunctions>
    #ifndef QT_OPENGL_ES_2
    # error
    #endif
    int main(int argc, char** argv) { return 0; }
    " QTGUI_OPENGL_REQUIRES_GL_LINKING)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/core/api/
        DESTINATION ${OXIDE_INSTALL_INCLUDEDIR}/OxideQtCore
        FILES_MATCHING PATTERN *.h
        PATTERN *_p.h EXCLUDE
        PATTERN includes* EXCLUDE)

set_target_properties(
    ${OXIDE_LIB} PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/core/api/includes
    INTERFACE_LINK_LIBRARIES Qt5::Core)

set(OXIDE_QT_PACKAGE_QMLPLUGIN_DIR qml)

add_subdirectory(qmlplugin)
add_subdirectory(quick)

if(ENABLE_UITK_WEBVIEW)
  add_subdirectory(uitk)
endif()

if(ENABLE_OXIDEQMLSCENE)
  if(NOT ${Qt5Quick_VERSION_STRING} VERSION_LESS "5.4.0")
    message(FATAL_ERROR
            "oxideqmlscene is not supported on Qt 5.4.0 and later - you can "
            "use qmlscene instead")
  elseif(NOT ${Qt5Quick_VERSION_STRING} VERSION_LESS "5.3.0")
    message(WARNING
            "oxideqmlscene is redundant on Qt 5.3.0 and later - you can use "
            "qmlscene instead")
  endif()
  add_subdirectory(qmlrunner)
endif()

if(ENABLE_DOCUMENTATION)
  add_subdirectory(documentation)
endif()

if(ENABLE_TESTS)
  add_test(NAME qt_unittests
           COMMAND ${CHROMIUM_PRODUCT_DIR}/oxide_qt_unittests)
  add_test(NAME qt_screen_unittests
           COMMAND ${CHROMIUM_PRODUCT_DIR}/oxide_qt_screen_unittests --qt-plugin-path=${OXIDE_MOCK_QTPLUGIN_DIR} -platform mock_qpa --single-process-tests)
  add_subdirectory(tests)
endif()

add_custom_target(
    test-package-qt-runner
    COMMAND ${CMAKE_COMMAND}
    -DSRCFILE=${CMAKE_CURRENT_SOURCE_DIR}/run_qmlapp.sh.in
    -DDESTINATION=${OXIDE_PACKAGE_STAGE_DIR}
    -DQML_DIR=${OXIDE_QT_PACKAGE_QMLPLUGIN_DIR}
    -DPRODUCT_DIR=${OXIDE_PLATFORM_FULLNAME} -DOXIDE_RENDERER=${OXIDE_RENDERER}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/build/scripts/make-test-package-qmlrunner.cmake)

add_custom_target(
    test-package-qt
    DEPENDS test-package-qt-quick test-package-qt-qmlplugin test-package-qt-runner)
if(ENABLE_UITK_WEBVIEW)
  add_dependencies(test-package-qt test-package-uitk-lib test-package-uitk-qmlplugin)
endif()
