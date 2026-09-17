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
#ifndef ULTRABUS_ORG_FREEDESKTOP_DBUS_PEER_HPP
#define ULTRABUS_ORG_FREEDESKTOP_DBUS_PEER_HPP

#include <ultrabus/connection.hpp>
#include <ultrabus/retvalue.hpp>
#include <string>
#include <functional>
#include <dbus/dbus.h>


namespace ultrabus {


    /**
     * Proxy for the standard <code>org.freedesktop.DBus.Peer</code> interface.
     *
     * The proxy retains a non-owning reference to its connection, which must
     * outlive the proxy.
     *
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-peer
     *       rel="noopener noreferrer" target="_blank">D-Bus Specification - org.freedesktop.DBus.Peer</a>
     */
    class org_freedesktop_DBus_Peer {
    public:
        /**
         * Construct a peer proxy.
         *
         * @param conn_arg Connection used for requests. It is not owned and
         *                 must outlive this proxy.
         */
        org_freedesktop_DBus_Peer (connection& conn_arg) : conn(conn_arg) {}

        /**
         * Ping a remote service and measure the round-trip time.
         * @param service The bus name of the service to ping.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return The round-trip time in microseconds on success; otherwise a
         *         retvalue whose err() and what() describe the failure.
         * @see retvalue
         */
        retvalue<long> ping (const std::string& service,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to ping a remote service.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param service The bus name of the service to ping.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        bool ping (const std::string& service,
                   std::function<void (retvalue<long>& result)> callback,
                   int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Retrieve the machine UUID of a remote service's host.
         * @param service The bus name of the service to query.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return The machine ID on success; otherwise a retvalue whose err()
         *         and what() describe the failure.
         * @see retvalue
         */
        retvalue<std::string> get_machine_id (const std::string& service,
                                              int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to retrieve the machine UUID of a remote service's host.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param service The bus name of the service to query.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        bool get_machine_id (const std::string& service,
                             std::function<void (retvalue<std::string>& result)> callback,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

    private:
        connection& conn;
    };


}
#endif
