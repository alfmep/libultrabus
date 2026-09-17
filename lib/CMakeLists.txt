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


# Build library ultrabus
#
add_library (ultrabus)

# Set library version properties
#
set_target_properties (ultrabus PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Set include path
#
target_include_directories (ultrabus
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    INTERFACE
    $<INSTALL_INTERFACE:include>
)

# Set link libraries
#
target_link_libraries (ultrabus
    PUBLIC
    ${DBus1_LIBRARIES}
    INTERFACE
    ultrabus
)

# # Disable debugging messages in Release builds
# #
# if (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
#     set (ENABLE_DEBUG_MESSAGES 0)
# else()
#     set (ENABLE_DEBUG_MESSAGES 1)
# endif()


# Configuration file
#
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/ultrabus/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabus/config.hpp
)

# Source files
#
target_sources (ultrabus PRIVATE
    ultrabus/dbus_basic.cpp
    ultrabus/dbus_array.cpp
    ultrabus/dbus_struct.cpp
    ultrabus/dbus_dict.cpp
    ultrabus/dbus_variant.cpp
    ultrabus/connection.cpp
    ultrabus/message.cpp
    ultrabus/message_filter.cpp
    ultrabus/callback_message_filter.cpp
    ultrabus/object_proxy.cpp
    ultrabus/object_handler.cpp
    ultrabus/callback_object_handler.cpp
    ultrabus/utils.cpp
    ultrabus/org_freedesktop_DBus_Peer.cpp
    ultrabus/org_freedesktop_DBus_Introspectable.cpp
    ultrabus/org_freedesktop_DBus_Properties.cpp
    ultrabus/org_freedesktop_DBus_ObjectManager.cpp
)

# Header files
#
set (MAIN_PUBLIC_HEADER_FILE
    ultrabus.hpp
)
set (PUBLIC_HEADER_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabus/config.hpp
    ultrabus/retvalue.hpp
    ultrabus/dbus_type.hpp
    ultrabus/dbus_basic.hpp
    ultrabus/dbus_string.hpp
    ultrabus/dbus_opath.hpp
    ultrabus/dbus_signature.hpp
    ultrabus/dbus_unix_fd.hpp
    ultrabus/dbus_basic_types.hpp
    ultrabus/dbus_array.hpp
    ultrabus/dbus_struct.hpp
    ultrabus/dbus_dict.hpp
    ultrabus/dbus_variant.hpp
    ultrabus/connection.hpp
    ultrabus/message.hpp
    ultrabus/message_filter.hpp
    ultrabus/callback_message_filter.hpp
    ultrabus/object_proxy.hpp
    ultrabus/object_handler.hpp
    ultrabus/callback_object_handler.hpp
    ultrabus/utils.hpp
    ultrabus/org_freedesktop_DBus_Peer.hpp
    ultrabus/org_freedesktop_DBus_Introspectable.hpp
    ultrabus/org_freedesktop_DBus_Properties.hpp
    ultrabus/org_freedesktop_DBus_ObjectManager.hpp
)

#
# Install library, headers, and CMake package config files
#
include (CMakePackageConfigHelpers)

install (TARGETS ultrabus
    EXPORT ultrabusTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install (FILES ${MAIN_PUBLIC_HEADER_FILE}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install (FILES ${PUBLIC_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ultrabus
)

install (EXPORT ultrabusTargets
    FILE ultrabusTargets.cmake
    NAMESPACE ultrabus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ultrabus
)

configure_package_config_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ultrabusConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabusConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ultrabus
)
write_basic_package_version_file (
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabusConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install (FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabusConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ultrabus
)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ultrabus.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/ultrabus.pc
    @ONLY
)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/ultrabus.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
