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
#include <ultrabus/org_freedesktop_DBus_Peer.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <string>
#include <chrono>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<long> org_freedesktop_DBus_Peer::ping (const std::string& service,
                                                    int timeout)
    {
        retvalue<long> ret (-1);

        if (!dbus_validate_bus_name(service.c_str(), nullptr)) {
            ret.err (-1, "Invalid service name");
            return ret;
        }

        message msg (service, "/", DBUS_INTERFACE_PEER, "Ping");
        auto start = std::chrono::high_resolution_clock::now ();
        auto reply = conn.send_and_wait (msg, timeout);
        auto stop = std::chrono::high_resolution_clock::now ();

        if ( ! reply.is_error())
            ret = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
        else
            ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());

        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_Peer::ping (
            const std::string& service,
            std::function<void (retvalue<long>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(service.c_str(), nullptr))
            return false;

        message msg (service, "/", DBUS_INTERFACE_PEER, "Ping");
        if (!callback)
            return conn.send (msg);

        auto start = std::chrono::high_resolution_clock::now ();
        return conn.send (msg,
                          [callback, start](message& reply){
                              auto stop = std::chrono::high_resolution_clock::now ();
                              retvalue<long> ret (-1);
                              if (reply.is_error())
                                  ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                              else
                                  ret = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
                              callback (ret);
                          },
                          timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::string> org_freedesktop_DBus_Peer::get_machine_id (const std::string& service,
                                                                     int timeout)
    {
        retvalue<std::string> id;

        if (!dbus_validate_bus_name(service.c_str(), nullptr)) {
            id.err (-1, "Invalid service name");
            return id;
        }

        message msg (service, "/", DBUS_INTERFACE_PEER, "GetMachineId");
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            id.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        else if (!reply.get_args(id.get())) {
            std::string err_msg ("Invalid DBus reply argument, expected 's', got '");
            err_msg.append (reply.signature());
            err_msg.push_back ('\'');
            id.err (-1, err_msg);
        }
        return id;

    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_Peer::get_machine_id (
            const std::string& service,
            std::function<void (retvalue<std::string>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(service.c_str(), nullptr))
            return false;

        message msg (service, "/", DBUS_INTERFACE_PEER, "GetMachineId");
        if (!callback)
            return conn.send (msg);

        return conn.send (msg,
                          [callback](message& reply){
                              retvalue<std::string> id;
                              if (reply.is_error()) {
                                  id.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                              }else if (!reply.get_args(id.get())) {
                                  std::string err_msg ("Invalid DBus reply argument, expected 's', got '");
                                  err_msg.append (reply.signature());
                                  err_msg.push_back ('\'');
                                  id.err (-1, err_msg);
                              }
                              callback (id);
                          },
                          timeout);
    }


}
