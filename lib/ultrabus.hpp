/*
 * Copyright (C) 2026 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of ultrabus.
 *
 * ultrabus is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ULTRABUS_HPP
#define ULTRABUS_HPP

/**
 * Namespace for all symbols in libultrabus.
 */
namespace ultrabus {
}

#include <ultrabus/config.hpp>
#include <ultrabus/retvalue.hpp>
#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/message_filter.hpp>
#include <ultrabus/callback_message_filter.hpp>
#include <ultrabus/object_proxy.hpp>
#include <ultrabus/object_handler.hpp>
#include <ultrabus/callback_object_handler.hpp>
#include <ultrabus/utils.hpp>
#include <ultrabus/org_freedesktop_DBus_Peer.hpp>
#include <ultrabus/org_freedesktop_DBus_Introspectable.hpp>
#include <ultrabus/org_freedesktop_DBus_Properties.hpp>
#include <ultrabus/org_freedesktop_DBus_ObjectManager.hpp>

#endif
