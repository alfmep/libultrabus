#
# Copyright (C) 2026 Dan Arrhenius <dan@ultramarin.se>
#
# This file is part of ultrabus.
#
# ultrabus is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
cmake_minimum_required (VERSION 3.25)

if (CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    add_compile_options ("$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wextra;-Wno-unused-parameter>>")
elseif (CMAKE_BUILD_TYPE MATCHES "^[Mm]em[Dd]ebug")
    add_compile_options ("$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wextra;-Wno-unused-parameter>>")
endif()


add_executable (dbus-interface-with-only-methods)
target_sources (dbus-interface-with-only-methods PRIVATE dbus-interface-with-only-methods.cpp)
target_include_directories (dbus-interface-with-only-methods PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (dbus-interface-with-only-methods PRIVATE ultrabus)


add_executable (dbus-interface-with-only-properties)
target_sources (dbus-interface-with-only-properties PRIVATE dbus-interface-with-only-properties.cpp)
target_include_directories (dbus-interface-with-only-properties PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (dbus-interface-with-only-properties PRIVATE ultrabus)


add_executable (dbus-service-with-multiple-objects)
target_sources (dbus-service-with-multiple-objects
    PRIVATE
    dbus-service-with-multiple-objects.cpp
    interface_foo_bar_tick_manager.cpp
    interface_foo_bar_tick.cpp
)
target_include_directories (dbus-service-with-multiple-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (dbus-service-with-multiple-objects PRIVATE ultrabus)


# add_executable (APP_NAME)
# target_sources (APP_NAME PRIVATE APP_NAME.cpp)
# target_include_directories (APP_NAME PRIVATE
#     ${CMAKE_SOURCE_DIR}/lib
#     ${CMAKE_BINARY_DIR}/lib
# )
# target_link_libraries (APP_NAME PRIVATE ultrabus)
