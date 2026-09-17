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

#
# Project name and info
#
project (libultrabus
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "A C++ library for making DBus applications."
)

#
# Options
#
option (BUILD_SHARED_LIBS "Build shared library." ON)
option (BUILD_DOC "Generate documentation (if doxygen is found)." ON)
option (BUILD_TESTING "Build test applications." OFF)
#option (BUILD_EXAMPLES "Build example applications." ON)

#
# Dependencies
#
find_package (DBus1 REQUIRED)

#
# C++ standard
#
set (CMAKE_CXX_STANDARD 20)

#
# Default GNU installation directories
#
include (GNUInstallDirs)

#
# Common compiler flags
#
set (gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
add_compile_options (
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-pipe;-Wall>>"
)
if (CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    # Build test applications
    set (BUILD_TESTING ON)

    # If build type is debug, add flag -Og for GCC et al.
    add_compile_options (
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Og;-Werror;-fsanitize=address>>"
    )
    add_link_options (
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-fsanitize=address;-static-libasan>>"
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "")
    # Default build type, optimize using -O2
    add_compile_options (
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-O2>>"
    )
endif()

#
# libultrabus
#
add_subdirectory (lib)

#
# Test applications
#
if (BUILD_TESTING)
    enable_testing ()
    add_subdirectory (test)
endif()

# #
# # Example applications
# #
# if (BUILD_EXAMPLES)
#     add_subdirectory (examples)
# endif()

#
# Doxygen documentation
#
set (PREDEFINED_IN_DOXYGEN "")
if (BUILD_DOC)
    find_package (Doxygen)
    if (DOXYGEN_FOUND)
        add_subdirectory (doc)
    endif()
endif()

#
# Uninstall target
#
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    @ONLY
)
add_custom_target (uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    COMMENT "Uninstalling ultrabus"
)

#
# Distributable tarballs (CPack)
#
set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
set (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set (CPACK_PACKAGE_VENDOR "Dan Arrhenius")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set (CPACK_PACKAGE_CONTACT "Dan Arrhenius <dan@ultramarin.se>")
set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")

set (CPACK_GENERATOR "TGZ")
set (CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_IGNORE_FILES
    "/\\\\.git/"
    "/\\\\.github/"
    "/build/"
    "/\\\\.gitignore\$"
)
include (CPack)


#
# Print summary
#
message (STATUS "Summary:")
#if (CMAKE_BUILD_TYPE MATCHES "^$")
if (CMAKE_BUILD_TYPE STREQUAL "")
    message (STATUS "    Build type........................... [default]")
else()
    message (STATUS "    Build type........................... ${CMAKE_BUILD_TYPE}")
endif()
message (STATUS "    Installation prefix.................. ${CMAKE_INSTALL_PREFIX}")
if (DOXYGEN_FOUND)
    message (STATUS "    Generate API documentation........... yes")
else()
    message (STATUS "    Generate API documentation........... no")
endif()
if (BUILD_TESTING)
    message (STATUS "    Build test applications.............. yes")
else()
    message (STATUS "    Build test applications.............. no")
endif()
# if (BUILD_EXAMPLES)
#     message (STATUS "    Build example applications........... yes")
# else()
#     message (STATUS "    Build example applications........... no")
# endif()
