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
#include <ultrabus/org_freedesktop_DBus_Introspectable.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <dbus/dbus.h>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::string> org_freedesktop_DBus_Introspectable::introspect (
            const std::string& service,
            const std::string& object_path,
            int timeout)
    {
        retvalue<std::string> xml;
        if (!dbus_validate_bus_name(service.c_str(), nullptr)) {
            xml.err (-1, "Invalid service name");
            return xml;
        }
        if (!dbus_validate_path(object_path.c_str(), nullptr)) {
            xml.err (-1, "Invalid object path");
            return xml;
        }

        message msg (service, object_path, DBUS_INTERFACE_INTROSPECTABLE, "Introspect");
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            xml.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        else if (!reply.get_args(xml.get())) {
            std::string err_msg ("Invalid DBus reply argument, expected 's', got '");
            err_msg.append (reply.signature());
            err_msg.push_back ('\'');
            xml.err (-1, err_msg);
        }
        return xml;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_Introspectable::introspect (
            const std::string& service,
            const std::string& object_path,
            std::function<void (retvalue<std::string>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(service.c_str(), nullptr))
            return false;
        if (!dbus_validate_path(object_path.c_str(), nullptr))
            return false;

        message msg (service, object_path, DBUS_INTERFACE_INTROSPECTABLE, "Introspect");
        if (!callback) {
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (msg,
                          [callback](message& reply){
                              retvalue<std::string> xml;
                              if (reply.is_error()) {
                                  xml.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                              }
                              else if (!reply.get_args(xml.get())) {
                                  std::string err_msg ("Invalid DBus reply argument, expected 's', got '");
                                  err_msg.append (reply.signature());
                                  err_msg.push_back ('\'');
                                  xml.err (-1, err_msg);
                              }
                              callback (xml);
                          },
                          timeout);
    }


}
