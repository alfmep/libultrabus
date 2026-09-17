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

add_executable (test_dbus_basic)
target_sources (test_dbus_basic PRIVATE test_dbus_basic.cpp)
target_include_directories (test_dbus_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_dbus_basic PRIVATE ultrabus)

add_test (NAME test_dbus_basic COMMAND test_dbus_basic)

add_executable (test_dbus_array)
target_sources (test_dbus_array PRIVATE test_dbus_array.cpp)
target_include_directories (test_dbus_array PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_dbus_array PRIVATE ultrabus)

add_test (NAME test_dbus_array COMMAND test_dbus_array)

add_executable (test_dbus_dict)
target_sources (test_dbus_dict PRIVATE test_dbus_dict.cpp)
target_include_directories (test_dbus_dict PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_dbus_dict PRIVATE ultrabus)

add_test (NAME test_dbus_dict COMMAND test_dbus_dict)

add_executable (test_dbus_variant)
target_sources (test_dbus_variant PRIVATE test_dbus_variant.cpp)
target_include_directories (test_dbus_variant PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_dbus_variant PRIVATE ultrabus)

add_test (NAME test_dbus_variant COMMAND test_dbus_variant)

add_executable (test_dbus_struct)
target_sources (test_dbus_struct PRIVATE test_dbus_struct.cpp)
target_include_directories (test_dbus_struct PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_dbus_struct PRIVATE ultrabus)

add_test (NAME test_dbus_struct COMMAND test_dbus_struct)

add_executable (test_retvalue)
target_sources (test_retvalue PRIVATE test_retvalue.cpp)
target_include_directories (test_retvalue PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_retvalue PRIVATE ultrabus)

add_test (NAME test_retvalue COMMAND test_retvalue)

add_executable (test_message)
target_sources (test_message PRIVATE test_message.cpp)
target_include_directories (test_message PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_BINARY_DIR}/lib
)
target_link_libraries (test_message PRIVATE ultrabus)

add_test (NAME test_message COMMAND test_message)
