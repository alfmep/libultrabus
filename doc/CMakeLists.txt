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


configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
    @ONLY
)


# Generate doxygen documentation
#
add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg.in
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/config.hpp.in
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/retvalue.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_type.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_basic_types.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_basic.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_string.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_opath.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_signature.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_unix_fd.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_array.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_struct.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_dict.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/dbus_variant.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/connection.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/message.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/message_filter.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/callback_message_filter.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/object_handler.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/callback_object_handler.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/object_proxy.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/utils.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/org_freedesktop_DBus_Peer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/org_freedesktop_DBus_Introspectable.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/org_freedesktop_DBus_Properties.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ultrabus/org_freedesktop_DBus_ObjectManager.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation"
    VERBATIM
)
add_custom_target (doxygen ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
)

# Remove html output on "make clean"
set_directory_properties (PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/html)

# Install generated HTML documentation
#
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
