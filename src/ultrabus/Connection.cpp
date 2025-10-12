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
#include <ultrabus/Connection.hpp>
#include <condition_variable>


//#define TRACE_DEBUG

#ifdef TRACE_DEBUG
#  include <cstdio>
#  include <unistd.h>
#  include <sys/types.h>
#  define DBG_LOG(format, ...) fprintf(stderr, "[%d] %s:%s:%d: " format "\n", \
                                       (int)gettid(), __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#  define DBG_LOG(format, ...)
#endif


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    Connection::Connection ()
        : conn {nullptr},
          private_connection {false},
          ioh (new iomultiplex::default_iohandler(SIGRTMIN)),
          internal_io_handler {true},
          io_timers (new iomultiplex::timer_set(*ioh))
    {
        dbus_threads_init_default ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    Connection::Connection (iomultiplex::iohandler_base& io_handler)
        : conn {nullptr},
          private_connection {false},
          ioh (&io_handler),
          internal_io_handler {false},
          io_timers (new iomultiplex::timer_set(*ioh))
    {
        dbus_threads_init_default ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    Connection::~Connection ()
    {
        disconnect ();
        delete io_timers;
        if (internal_io_handler)
            delete ioh;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    int Connection::connect (const DBusBusType type,
                             const bool private_connection,
                             const bool exit_on_disconnect)
    {
        if (is_connected()) {
            // TBD
            return -1;
        }

        this->private_connection = private_connection;

        // Connect to the bus
        //
        if (private_connection)
            conn = dbus_bus_get_private (type, nullptr);
        else
            conn = dbus_bus_get (type, nullptr);
        if (!conn)
            return -1;

        dbus_connection_set_exit_on_disconnect (conn, exit_on_disconnect);

        start_message_dispatcher ();
        return 0;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    int Connection::connect (const std::string& bus_address,
                             const int timeout,
                             const bool private_connection,
                             const bool exit_on_disconnect)
    {
        if (is_connected()) {
            // TBD
            return -1;
        }

        DBG_LOG ("Connect to bus %s", bus_address.c_str());

        this->private_connection = private_connection;

        // Connect to the bus
        //
        if (private_connection)
            conn = dbus_connection_open_private (bus_address.c_str(), nullptr);
        else
            conn = dbus_connection_open (bus_address.c_str(), nullptr);
        if (!conn)
            return -1;

        dbus_connection_set_exit_on_disconnect (conn, exit_on_disconnect);

        DBG_LOG ("Start message dispatcher");
        start_message_dispatcher ();

        // Register the connection with the bus
        //
        DBG_LOG ("Register the connection with the bus");
        Message hello_msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "Hello");
        auto reply = send_and_wait (hello_msg, timeout);
        if (reply.is_error()) {
            DBG_LOG ("Error registering the connection with the bus: %s - %s",
                     reply.error_name().c_str(), reply.error_msg().c_str());
            disconnect ();
            return -1;
        }

        // Set the unique bus id
        //
        dbus_basic id_arg ("");
        if (reply.get_args(&id_arg, nullptr) && !id_arg.str().empty()) {
            DBG_LOG ("Got unique bus name: %s", id_arg.str().c_str());
            dbus_bus_set_unique_name (conn, id_arg.str().c_str());
        }else{
            DBG_LOG ("Error registering the connection with the bus: "
                     "Invalid reply parameter when expecting a bus ID");
            disconnect ();
            return -1;
        }

        return 0;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    bool Connection::is_connected () const
    {
        return conn!=nullptr && dbus_connection_get_is_connected(conn)==TRUE;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::disconnect ()
    {
        if (!conn)
            return;

        if (private_connection && dbus_connection_get_is_connected(conn))
            dbus_connection_close (conn);
        dbus_connection_unref (conn);

        pending_msg_mutex.lock ();
        for (auto& e : pending_messages)
            dbus_pending_call_unref (e.first);
        pending_messages.clear ();
        pending_msg_mutex.unlock ();

        {
            std::lock_guard<std::mutex> lock (io_mutex);
            io_watches.clear ();
            io_timers->clear ();
            io_timeouts.clear ();
        }
        if (internal_io_handler) {
            ioh->stop ();
            ioh->join ();
        }

        conn = nullptr;
        private_connection = false;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string Connection::unique_name () const
    {
        const char* id = nullptr;
        if (conn)
            id = dbus_bus_get_unique_name (conn);
        return std::string (id ? id : "");
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    int Connection::send (const Message& msg)
    {
        uint32_t serial = 0;
        if (dbus_connection_send(conn,
                                 const_cast<Message&>(msg).handle(),
                                 &serial))
        {
            return 0;
        }else{
            return -1;
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    int Connection::send (const Message& msg,
                          std::function<void (ultrabus::Message&)> reply_cb,
                          int timeout)
    {
        if (!reply_cb)
            return send (msg);

        // Make sure we post the message in the scope of the worker thread
        //
        if (io_handler().same_context()) {
            bool result;
            DBusPendingCall* pending = nullptr;
            std::lock_guard<std::mutex> lock (pending_msg_mutex);
            result = dbus_connection_send_with_reply (conn,
                                                      const_cast<Message&>(msg).handle(),
                                                      &pending,
                                                      timeout);
            if (!result || !pending)
                return -1;
            pending_messages.emplace (pending, reply_cb);
            dbus_pending_call_set_notify (pending, dbus_pending_msg_cb, this, nullptr);
        }else{
            io_timers->set (0, [this, msg, reply_cb, timeout](iomultiplex::timer_set& ts,
                                                              long timer_id)
                {
                    bool result;
                    DBusPendingCall* pending = nullptr;
                    std::unique_lock<std::mutex> lock (pending_msg_mutex);
                    result = dbus_connection_send_with_reply (conn,
                                                              const_cast<Message&>(msg).handle(),
                                                              &pending,
                                                              timeout);
                    if (!result || !pending) {
                        // Return an error response
                        Message reply (dbus_message_new(DBUS_MESSAGE_TYPE_ERROR));
                        reply.dec_ref (); // ref count increased in Message constructor
                        reply.error_name ("se.ultramarin.ultrabus.Error.ENOMEM");
                        reply << std::string("Unable to allocate memory for DBus message");
                        lock.unlock ();
                        reply_cb (reply);
                        return;
                    }
                    pending_messages.emplace (pending, reply_cb);
                    dbus_pending_call_set_notify (pending, dbus_pending_msg_cb, this, nullptr);
                });
        }
        return 0;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    Message Connection::send_and_wait (const Message& msg, int timeout)
    {
        std::condition_variable cv;
        std::mutex m;
        volatile bool got_reply = false;
        Message reply;

        // Send the message
        auto result = send (msg, [&](Message& r)
            {
                // Save the reply and notify the waiting thread
                std::unique_lock<std::mutex> lock (m);
                reply = std::move (r);
                got_reply = true;
                cv.notify_one ();
            },
            timeout);

        if (result) {
            // Failed to send the message, return an error reply
            Message reply (dbus_message_new(DBUS_MESSAGE_TYPE_ERROR));
            reply.dec_ref (); // ref count increased in Message constructor
            reply.error_name ("se.ultramarin.ultrabus.Error.ENOMEM");
            reply << std::string("Unable to allocate memory for DBus message");
            return reply;
        }

        // Wait for the message reply
        std::unique_lock<std::mutex> lock (m);
        while (!got_reply)
            cv.wait (lock, [&got_reply]{return got_reply;});
        return reply;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::start_message_dispatcher ()
    {
        if (!conn)
            return;

        if (internal_io_handler)
            ioh->run (true); // Start I/O worker thread

        dbus_connection_set_dispatch_status_function (conn,
                                                      dbus_dispatch_status_cb,
                                                      this,
                                                      nullptr);
        dbus_connection_set_watch_functions (conn,
                                             dbus_add_watch_cb,
                                             dbus_remove_watch_cb,
                                             dbus_toggled_watch_cb,
                                             this,
                                             nullptr);
        dbus_connection_set_timeout_functions (conn,
                                               dbus_add_timeout_cb,
                                               dbus_remove_timeout_cb,
                                               dbus_toggled_timeout_cb,
                                               this,
                                               nullptr);
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::on_dispatch_status (DBusDispatchStatus status)
    {
        switch (status) {
        case DBUS_DISPATCH_DATA_REMAINS:
            DBG_LOG ("Status: DBUS_DISPATCH_DATA_REMAINS");
            break;
        case DBUS_DISPATCH_COMPLETE:
            DBG_LOG ("Status: DBUS_DISPATCH_COMPLETE");
            break;
        case DBUS_DISPATCH_NEED_MEMORY:
            DBG_LOG ("Status: DBUS_DISPATCH_NEED_MEMORY");
            break;
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::on_watch_rx_ready (iomultiplex::io_result_t& ior, DBusWatch* watch)
    {
        DBG_LOG ("RX ready");

        dbus_watch_handle (watch, DBUS_WATCH_READABLE);
        while (dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS)
            ;

        std::lock_guard<std::mutex> lock (io_mutex);
        if (io_watches.find(watch) == io_watches.end())
            return; // Watch removed in the dispatch function

        auto flags = dbus_watch_get_flags (watch);
        bool enabled = dbus_watch_get_enabled (watch);
        if (enabled && (flags & DBUS_WATCH_READABLE)) {
            ior.conn.wait_for_rx ([this, watch](iomultiplex::io_result_t& ior)->bool
                {
                    if (!ior.errnum)
                        on_watch_rx_ready (ior, watch);
                    return false;
                });
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::on_watch_tx_ready (iomultiplex::io_result_t& ior, DBusWatch* watch)
    {
        DBG_LOG ("TX ready");

        dbus_watch_handle (watch, DBUS_WATCH_WRITABLE);

        std::lock_guard<std::mutex> lock (io_mutex);
        if (io_watches.find(watch) == io_watches.end())
            return; // Watch removed in the watch_handle function

        auto flags = dbus_watch_get_flags (watch);
        bool enabled = dbus_watch_get_enabled (watch);
        if (enabled && (flags & DBUS_WATCH_WRITABLE)) {
            ior.conn.wait_for_tx ([this, watch](iomultiplex::io_result_t& ior)->bool
                {
                    if (!ior.errnum)
                        on_watch_tx_ready (ior, watch);
                    return false;
                });
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_pending_msg_cb (DBusPendingCall* pending, void* data)
    {
        Connection* self = static_cast<Connection*> (data);
        pending_msg_cb_t callback = nullptr;

        {
            std::lock_guard<std::mutex> lock (self->pending_msg_mutex);
            auto entry = self->pending_messages.find (pending);
            if (entry == self->pending_messages.end())
                return;

            callback = entry->second;
            self->pending_messages.erase (entry);
        }

        if (callback) {
            Message reply (dbus_pending_call_steal_reply(pending));
            reply.dec_ref (); // ref count increased in Message constructor
            dbus_pending_call_unref (pending);
            callback (reply);
        }else{
            dbus_pending_call_unref (pending);
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_dispatch_status_cb (DBusConnection* c,
                                              DBusDispatchStatus status,
                                              void* data)
    {
        Connection* self = static_cast<Connection*> (data);
        self->on_dispatch_status (status);
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    dbus_bool_t Connection::dbus_add_watch_cb (DBusWatch* watch, void* data)
    {
        DBG_LOG ("Add watch");

        int fd = dbus_watch_get_unix_fd (watch);
        if (fd < 0)
            return true;

        Connection* self = static_cast<Connection*> (data);
        std::lock_guard<std::mutex> lock (self->io_mutex);

        auto entry = self->io_watches.find (watch);
        if (entry == self->io_watches.end())
            entry = self->io_watches.emplace(watch, iomultiplex::fd_connection(*self->ioh, fd, true)).first;
        iomultiplex::fd_connection& fdc = entry->second;

        if (dbus_watch_get_enabled(watch)) {
            auto flags = dbus_watch_get_flags (watch);
            if (flags & DBUS_WATCH_READABLE) {
                fdc.wait_for_rx ([self, watch](iomultiplex::io_result_t& ior)->bool
                    {
                        if (!ior.errnum)
                            self->on_watch_rx_ready (ior, watch);
                        return false;
                    });
            }
            if (flags & DBUS_WATCH_WRITABLE) {
                fdc.wait_for_tx ([self, watch](iomultiplex::io_result_t& ior)->bool
                    {
                        if (!ior.errnum)
                            self->on_watch_tx_ready (ior, watch);
                        return false;
                    });
            }
        }

        return true;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_remove_watch_cb (DBusWatch* watch, void* data)
    {
        DBG_LOG ("Remove watch");

        Connection* self = static_cast<Connection*> (data);

        std::lock_guard<std::mutex> lock (self->io_mutex);
        auto entry = self->io_watches.find (watch);
        if (entry != self->io_watches.end())
            self->io_watches.erase (entry);
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_toggled_watch_cb (DBusWatch* watch, void* data)
    {
        Connection* self = static_cast<Connection*> (data);

        std::lock_guard<std::mutex> lock (self->io_mutex);
        auto entry = self->io_watches.find (watch);
        if (entry == self->io_watches.end())
            return;
        iomultiplex::fd_connection& fdc = entry->second;

        bool enabled = dbus_watch_get_enabled (watch);
        auto flags = dbus_watch_get_flags (watch);
        if (enabled) {
            DBG_LOG ("Toggle watch - enable");
            if (flags & DBUS_WATCH_READABLE) {
                DBG_LOG ("    Enble watch for RX");
                fdc.wait_for_rx ([self, watch](iomultiplex::io_result_t& ior)->bool
                    {
                        if (!ior.errnum)
                            self->on_watch_rx_ready (ior, watch);
                        return false;
                    });
            }
            if (flags & DBUS_WATCH_WRITABLE) {
                DBG_LOG ("    Enble watch for TX");
                fdc.wait_for_tx ([self, watch](iomultiplex::io_result_t& ior)->bool
                    {
                        if (!ior.errnum)
                            self->on_watch_tx_ready (ior, watch);
                        return false;
                    });
            }
        }else{
#ifdef TRACE_DEBUG
            DBG_LOG ("Toggle watch - disable");
            if (flags & DBUS_WATCH_READABLE)
                DBG_LOG ("    Disable watch for RX");
            if (flags & DBUS_WATCH_WRITABLE)
                DBG_LOG ("    Disable watch for TX");
#endif
            fdc.cancel ((flags & DBUS_WATCH_READABLE),  // Cancel RX if readable
                        (flags & DBUS_WATCH_WRITABLE),
                        false); // Cancel TX if readable
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    dbus_bool_t Connection::dbus_add_timeout_cb (DBusTimeout* timeout, void* data)
    {
        DBG_LOG ("Add timer");
        Connection* self = static_cast<Connection*> (data);
        std::lock_guard<std::mutex> lock (self->io_mutex);

        auto entry = self->io_timeouts.find (timeout);
        if (entry == self->io_timeouts.end()) {
            entry = self->io_timeouts.emplace(timeout, -1).first;
        }
        long& timer_id = entry->second;

        if (dbus_timeout_get_enabled(timeout)) {
            if (timer_id >= 0) {
                self->io_timers->cancel (timer_id);
                timer_id = -1;
            }
            auto interval = dbus_timeout_get_interval (timeout);
            if (interval >= 0) {
                DBG_LOG ("Set timer: %d", interval);
                timer_id = self->io_timers->set (interval,
                                                 [self, timeout](iomultiplex::timer_set& ts, long timer_id)
                    {
                        // Timer expiration callback
                        DBG_LOG ("timed out");
                        dbus_timeout_handle (timeout);
                        while (dbus_connection_dispatch(self->conn) == DBUS_DISPATCH_DATA_REMAINS)
                            ;
                    });
            }
        }else{
            if (timer_id >= 0) {
                self->io_timers->cancel (timer_id);
                timer_id = -1;
            }
            DBG_LOG ("Cancel timer");
        }

        return true;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_remove_timeout_cb (DBusTimeout* timeout, void* data)
    {
        DBG_LOG ("Remove timer");
        Connection* self = static_cast<Connection*> (data);

        std::lock_guard<std::mutex> lock (self->io_mutex);
        auto entry = self->io_timeouts.find (timeout);
        if (entry != self->io_timeouts.end()) {
            long& timer_id = entry->second;
            if (timer_id >= 0)
                self->io_timers->cancel (timer_id);
            self->io_timeouts.erase (entry);
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Connection::dbus_toggled_timeout_cb (DBusTimeout* timeout, void* data)
    {
        DBG_LOG ("Toggle timer");
        Connection* self = static_cast<Connection*> (data);

        std::lock_guard<std::mutex> lock (self->io_mutex);
        auto entry = self->io_timeouts.find (timeout);
        if (entry == self->io_timeouts.end())
            return;
        long& timer_id = entry->second;

        // Cancel the timer if it is active
        if (timer_id >= 0) {
            self->io_timers->cancel (timer_id);
            timer_id = -1;
        }

        if (dbus_timeout_get_enabled(timeout)) {
            auto interval = dbus_timeout_get_interval (timeout);
            DBG_LOG ("Enable timer, interval: %d", (int)interval);
            if (interval >= 0) {
                timer_id = self->io_timers->set (interval,
                                                 [self, timeout](iomultiplex::timer_set& ts, long timer_id)
                    {
                        // Timer expiration callback
                        DBG_LOG ("timed out");
                        dbus_timeout_handle (timeout);
                        while (dbus_connection_dispatch(self->conn) == DBUS_DISPATCH_DATA_REMAINS)
                            ;
                    });
            }
        }else{
            // Timer already cancelled above
            DBG_LOG ("Cancel timer");
        }
    }


}
