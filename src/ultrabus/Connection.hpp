/*
 * Copyright (C) 2017,2021,2022 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of libultrabus.
 *
 * libultrabus is free software; you can redistribute it and/or modify it
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
#ifndef ULTRABUS_CONNECTION_HPP
#define ULTRABUS_CONNECTION_HPP

#include <ultrabus/Message.hpp>
#include <functional>
#include <string>
#include <mutex>
#include <map>
#include <dbus/dbus.h>
#include <iomultiplex.hpp>


namespace ultrabus {


    /**
     * A DBus connection.
     */
    class Connection {
    public:
        /**
         * Default constructor.
         * Creates a connection object that uses an internal I/O handler.
         */
        Connection ();

        /**
         * Constructor.
         * @param io_handler An I/O handler object that shall be
         *                   used by this connection.
         */
        Connection (iomultiplex::iohandler_base& io_handler);

        /**
         * Destructor.
         * Close the connection and free resources.
         */
        ~Connection ();

        /**
         * Connect and register to a well known bus.
         * @param type The DBus to connect to, DBUS_BUS_SESSION or DBUS_BUS_SYSTEM.
         * @param private_connection Set to <code>true</code> for a private connection.
         * @param exit_on_disconnect If <code>true</code>, the process will
         *                           exit if the connection is disconnected.
         * @return 0 on success, -1 on failure.
         */
        int connect (const DBusBusType type=DBUS_BUS_SESSION,
                     const bool private_connection=false,
                     const bool exit_on_disconnect=true);

        /**
         * Connect and register to a specific bus address.
         * @param bus_address The address of the bus to connect to.
         * @param timeout Timeout in milliseconds when connecting to the bus.
         * @param private_connection Set to <code>true</code> for a private connection.
         * @param exit_on_disconnect If <code>true</code>, the process will
         *                           exit if the connection is disconnected.
         * @return 0 on success, -1 on failure.
         */
        int connect (const std::string& bus_address,
                     const int timeout=DBUS_TIMEOUT_USE_DEFAULT,
                     const bool private_connection=false,
                     const bool exit_on_disconnect=true);

        /**
         * Return true if connected to a bus.
         */
        bool is_connected () const;

        /**
         * Disconnect from the bus.
         */
        void disconnect ();

        /**
         * Get the unique name of the connection.
         * @return The unique name of the connection.
         *         Could be an empty string if it's
         *         not connected.
         */
        std::string unique_name () const;

        /**
         * Send a message on the bus.
         * @param msg The DBus message to send.
         * @param reply_cb A callback called when a message reply is received.
         * @param timeout The maximum time in milliseconds to wait for a message reply.
         * @return 0 on success, -1 on failure.
         */
        int send (const Message& msg,
                  std::function<void (ultrabus::Message&)> reply_cb,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Send a message on the bus without caring about a message reply.
         * @param msg The DBus message to send.
         * @return 0 on success, -1 on failure.
         */
        int send (const Message& msg);

        /**
         * Send a message on the bus and wait for a reply.
         * @param msg The DBus message to send.
         * @param timeout The maximum time in milliseconds to wait for a message reply.
         * @return A message reply.
         */
        Message send_and_wait (const Message& msg, int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Return the iohandler_base used by the connection object.
         */
        iomultiplex::iohandler_base& io_handler () {
            return *ioh;
        }

        /**
         * Return get DBus connection object.
         * Use this method if you're using the DBus api calls directly.
         */
        DBusConnection* handle () {
            return conn;
        }


    private:
        // libdbus-1 connection object
        DBusConnection* conn;
        bool private_connection;

        // I/O handler
        iomultiplex::iohandler_base* ioh;
        bool internal_io_handler;

        // Pending messages
        using pending_msg_cb_t = std::function<void (Message&)>;
        std::mutex pending_msg_mutex;
        std::map<DBusPendingCall*, pending_msg_cb_t> pending_messages;

        // DBus I/O
        std::mutex io_mutex;
        iomultiplex::timer_set* io_timers;
        std::map<DBusTimeout*, long> io_timeouts;
        std::map<DBusWatch*, iomultiplex::fd_connection> io_watches;

        void start_message_dispatcher ();

        void on_dispatch_status (DBusDispatchStatus status);
        void on_watch_rx_ready (iomultiplex::io_result_t& ior, DBusWatch* watch);
        void on_watch_tx_ready (iomultiplex::io_result_t& ior, DBusWatch* watch);

        // Static callbacks called from libdbus-1
        //
        static void dbus_pending_msg_cb (DBusPendingCall* pending, void* data);
        static void dbus_dispatch_status_cb (DBusConnection* c, DBusDispatchStatus status, void* data);

        static dbus_bool_t dbus_add_watch_cb (DBusWatch* watch, void* data);
        static void dbus_remove_watch_cb (DBusWatch* watch, void* data);
        static void dbus_toggled_watch_cb (DBusWatch* watch, void* data);

        static dbus_bool_t dbus_add_timeout_cb (DBusTimeout* timeout, void* data);
        static void dbus_remove_timeout_cb (DBusTimeout* timeout, void* data);
        static void dbus_toggled_timeout_cb (DBusTimeout* timeout, void* data);
    };


}


#endif
