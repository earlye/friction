#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#

cmake_minimum_required(VERSION 3.12)
project(frictiontests LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.15.13 COMPONENTS Core Gui Test REQUIRED)

# Tests here compile the specific core source files they need directly, rather
# than linking the frictioncore library — that library bundles the whole app
# (Skia, FFmpeg, ...) as one static archive, which is a lot of unrelated
# dependency surface to pull in just to test a pure, dependency-free function.
add_executable(tst_gizmos
    tst_gizmos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/gizmos.cpp
)
target_include_directories(tst_gizmos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/core)
target_compile_definitions(tst_gizmos PRIVATE CORE_LIBRARY)
target_link_libraries(tst_gizmos PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME tst_gizmos COMMAND tst_gizmos)
