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
#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>


namespace ultrabus {

    std::atomic_bool connection::dbus_threads_initialized = false;

    //
    static constexpr int invalid_file_desc = -1;

    //
    static constexpr long milliseconds_in_a_second = 1000L;
    static constexpr long nanoseconds_in_a_second = 1000000000L;
    static constexpr long nanoseconds_in_a_milliseconds = 1000000L;

    // Timer entry tuple index constants
    static constexpr int idx_handle   = 0;
    static constexpr int idx_abs_time = 1;
    static constexpr int idx_repeat   = 2;

    // Clock used for timers
    static constexpr const clockid_t timer_clock_id = CLOCK_MONOTONIC;


    /**
     * Functor for comparing objects of type <code>struct timespec</code>.
     */
    struct timespec_less_t {
        /**
         * Compare two <code>struct timespec</code> objects to see if one is less than the other (lhs<rhs).
         * @param lhs The <code>struct timespec</code> on the left hand side of the comparison.
         * @param rhs The <code>struct timespec</code> on the right hand side of the comparison.
         * @return <code>true</code> if <code>lhs</code> is less than <code>rhs</code>.
         */
        bool operator() (const struct timespec& lhs, const struct timespec& rhs) const {
            if (lhs.tv_sec != rhs.tv_sec)
                return lhs.tv_sec < rhs.tv_sec;
            else
                return lhs.tv_nsec < rhs.tv_nsec;
        }
    };



    namespace {
        // const std::string epoll_events_to_string (uint32_t events)
        // {
        //     std::string str;
        //     bool got_events = false;

        //     if (!events)
        //         return "(none)";

        //     if (events & EPOLLIN) {
        //         str.append ("EPOLLIN");
        //         got_events = true;
        //     }
        //     if (events & EPOLLOUT) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("EPOLLOUT");
        //         got_events = true;
        //     }
        //     if (events & EPOLLHUP) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("EPOLLHUP");
        //         got_events = true;
        //     }
        //     if (events & EPOLLERR) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("EPOLLERR");
        //         got_events = true;
        //     }
        //     return str;
        // }

        // const std::string dbus_watch_flags_to_string (unsigned flags)
        // {
        //     std::string str;
        //     bool got_events = false;

        //     if (!flags)
        //         return "(none)";

        //     if (flags & DBUS_WATCH_READABLE) {
        //         str = "WATCH_READABLE";
        //         got_events = true;
        //     }
        //     if (flags & DBUS_WATCH_WRITABLE) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("WATCH_WRITABLE");
        //         got_events = true;
        //     }
        //     if (flags & DBUS_WATCH_ERROR) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("WATCH_ERROR");
        //         got_events = true;
        //     }
        //     if (flags & DBUS_WATCH_HANGUP) {
        //         if (got_events)
        //             str.push_back ('|');
        //         str.append ("WATCH_HANGUP");
        //         got_events = true;
        //     }
        //     return str;
        // }


        //----------------------------------------------------------------------
        // Namespace: anonymous
        //----------------------------------------------------------------------
        void inc_abs_time (struct timespec& abs, unsigned msec)
        {
            while (msec >= milliseconds_in_a_second) {
                ++abs.tv_sec;
                msec -= milliseconds_in_a_second;
            }
            abs.tv_nsec += msec * nanoseconds_in_a_milliseconds;
            if (abs.tv_nsec >= nanoseconds_in_a_second) {
                ++abs.tv_sec;
                abs.tv_nsec -= nanoseconds_in_a_second;
            }
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        bool compare_timer_entry (const std::tuple<DBusTimeout*, struct timespec, unsigned>& lhs_entry,
                                  const std::tuple<DBusTimeout*, struct timespec, unsigned>& rhs_entry)
        {
            const struct timespec& lhs = std::get<idx_abs_time> (lhs_entry);
            const struct timespec& rhs = std::get<idx_abs_time> (rhs_entry);
            if (lhs.tv_sec != rhs.tv_sec)
                return lhs.tv_sec < rhs.tv_sec;
            else
                return lhs.tv_nsec < rhs.tv_nsec;
        };


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        inline retvalue<bool> sync_call_method_reply_with_void (
                connection& conn,
                message& msg,
                int timeout)
        {
            retvalue<bool> retval (true);
            auto reply = conn.send_and_wait (msg, timeout);
            if (reply.is_error()) {
                retval = false;
                retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
            }
            return retval;
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        inline bool async_call_method_reply_with_void (
                connection& conn,
                message& msg,
                std::function<void (retvalue<bool>& retval)>& callback,
                int timeout)
        {
            if (callback==nullptr) {
                msg.want_reply (false);
                return conn.send (msg);
            }
            return conn.send (msg,
                              [callback](message& reply){
                                  retvalue<bool> retval (true);
                                  if (reply.is_error()) {
                                      retval = false;
                                      retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                                  }
                                  callback (retval);
                              },
                              timeout);
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        template<dbus_basic_cpp_types T>
        retvalue<T> sync_call_method (
                connection& conn,
                message& msg,
                int timeout)
        {
            retvalue<T> retval;
            if constexpr ( ! std::same_as<T, std::string>)
                retval = 0;

            auto reply = conn.send_and_wait (msg, timeout);
            if (reply.is_error()) {
                retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
            }
            else if (!reply.get_args(retval.get())) {
                std::string err_msg ("Invalid DBus reply argument, expected '%c', got '");
                err_msg.push_back ((char)dbus_basic<T>::make_type_code());
                err_msg.append (reply.signature());
                err_msg.push_back ('\'');
                retval.err (-1, err_msg);
            }
            return retval;
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        template<dbus_basic_cpp_types T>
        bool async_call_method (
                connection& conn,
                message& msg,
                std::function<void (retvalue<T>& retval)>& callback,
                int timeout)
        {
            if (callback==nullptr) {
                msg.want_reply (false);
                return conn.send (msg);
            }
            return conn.send (msg,
                              [callback](message& reply){
                                  retvalue<T> retval;
                                  if constexpr ( ! std::same_as<T, std::string>)
                                      retval = 0;
                                  if (reply.is_error()) {
                                      retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                                  }else if (!reply.get_args(retval.get())) {
                                      std::string err_msg ("Invalid DBus reply argument, expected '%c', got '");
                                      err_msg.push_back ((char)dbus_basic<T>::make_type_code());
                                      err_msg.append (reply.signature());
                                      err_msg.push_back ('\'');
                                      retval.err (-1, err_msg);
                                  }
                                  callback (retval);
                              },
                              timeout);
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        retvalue<std::set<std::string>> handle_set_string_reply (message& reply)
        {
            retvalue<std::set<std::string>> retval;
            dbus_array names ("s");
            if (reply.is_error()) {
                retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
            }
            else if (!reply.get_args(names)) {
                std::string err_msg ("Invalid DBus reply argument, expected 'as', got '");
                err_msg.append (reply.signature());
                err_msg.push_back ('\'');
                retval.err (-1, err_msg);
            }
            else{
                for (auto& name : names)
                    retval.get().emplace (name.cast<dbus_string>().get());
            }
            return retval;
        }


        //--------------------------------------------------------------------------
        // Namespace: anonymous
        //--------------------------------------------------------------------------
        retvalue<dbus_dict<dbus_string>> handle_dict_str_variant_reply (message& reply)
        {
            retvalue<dbus_dict<dbus_string>> retval;
            retval.get().reset (DBUS_TYPE_VARIANT);

            if (reply.is_error()) {
                retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
            }
            else if (!reply.get_args(retval.get())) {
                std::string err_msg ("Invalid DBus reply argument, expected 'a{sv}', got '");
                err_msg.append (reply.signature());
                err_msg.push_back ('\'');
                retval.err (-1, err_msg);
            }
            return retval;
        }


    } // End of namespace: anonymous




    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    connection::connection ()
        : message_filter (*this, true),
          ctx (nullptr),
          conn_type (DBUS_BUS_SESSION),
          timerfd (invalid_file_desc),
          pollfd (invalid_file_desc),
          name_owner_changed_cb (nullptr),
          name_lost_cb (nullptr),
          name_acquired_cb (nullptr),
          private_connection (false),
          stop_io_thread (true),
          stop_dispatch_thread (true)
    {
        if (!dbus_threads_initialized.exchange(true))
            dbus_threads_init_default ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    connection::~connection ()
    {
        disconnect ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::connect (const DBusBusType type,
                              const bool private_connection_arg,
                              const bool exit_on_disconnect)
    {
        if (is_connected()) {
            if (conn_addr.empty() &&
                type == conn_type &&
                private_connection == private_connection_arg)
            {
                // Already connected to same bus
                return true;
            }else{
                // Already connected to another bus
                errno = EBUSY;
                return false;
            }
        }

        pollfd = epoll_create1 (EPOLL_CLOEXEC);
        if (pollfd < 0) {
            pollfd = invalid_file_desc;
            return false;
        }
        timerfd = timerfd_create (timer_clock_id, TFD_NONBLOCK|TFD_CLOEXEC);
        if (timerfd < 0) {
            timerfd = invalid_file_desc;
            disconnect ();
            return false;
        }else{
            struct epoll_event epev {};
            epev.data.fd = timerfd;
            epev.events = EPOLLIN;
            if (epoll_ctl(pollfd, EPOLL_CTL_ADD, timerfd, &epev) < 0) {
                disconnect ();
                return false;
            }
        }

        conn_type = type;
        private_connection = private_connection_arg;

        // Connect to the bus
        //
        if (private_connection)
            ctx = dbus_bus_get_private (type, nullptr);
        else
            ctx = dbus_bus_get (type, nullptr);
        if (ctx == nullptr) {
            disconnect ();
            return false;
        }

        dbus_connection_set_exit_on_disconnect (ctx, (dbus_bool_t)exit_on_disconnect);

        if (!install_filter_handler()) {
            disconnect ();
            return false;
        }

        start_io_handler ();

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::is_connected () const
    {
        return ctx!=nullptr && dbus_connection_get_is_connected(ctx)==TRUE;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::disconnect ()
    {
        if (is_connected()) {
            remove_filter_handler ();
            if (private_connection)
                dbus_connection_close (ctx);
            dbus_connection_unref (ctx);
        }

        pending_mutex.lock ();
        for (auto& entry : pending_messages) {
            dbus_pending_call_cancel (entry.first);
            dbus_pending_call_unref (entry.first);
        }
        pending_messages.clear ();
        pending_mutex.unlock ();

        stop_io_thread = true;
        if (io_thread.joinable())
            io_thread.join ();

        stop_dispatch_thread = true;
        dispatch_cond.notify_one ();
        if (dispatch_thread.joinable())
            dispatch_thread.join ();

        if (pollfd != invalid_file_desc)
            close (pollfd);
        if (timerfd != invalid_file_desc)
            close (timerfd);

        timers.clear ();
        fd_map.clear ();

        timerfd = invalid_file_desc;
        pollfd = invalid_file_desc;
        ctx = nullptr;
        conn_addr.clear ();
        unique_bus_name.clear ();
        name_owner_changed_cb = nullptr;
        name_lost_cb = nullptr;
        name_acquired_cb = nullptr;
        conn_type = DBUS_BUS_SESSION;
        private_connection = false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::send (message& msg,
                           std::function<void (ultrabus::message&)> reply_cb,
                           int timeout)
    {
        const std::lock_guard lock (pending_mutex);
        DBusPendingCall* pending = nullptr;

        auto result = dbus_connection_send_with_reply (
                ctx,
                msg.handle(),
                &pending,
                timeout);
        if (result==FALSE || pending==nullptr)
            return false;

        dbus_pending_call_set_notify (pending,
                                      static_pending_call_cb,
                                      this,
                                      nullptr);
        pending_messages.emplace (pending, reply_cb);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::send (message& msg, uint32_t& serial)
    {
        return TRUE == dbus_connection_send(ctx, msg.handle(), &serial);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    message connection::send_and_wait (message& msg, int timeout)
    {
        DBusMessage* result {};
        DBusError err;
        dbus_error_init (&err);

        result = dbus_connection_send_with_reply_and_block (ctx,
                                                            msg.handle(),
                                                            timeout,
                                                            &err);
        if (result == FALSE) {
            if (msg.serial() == 0)
                dbus_message_set_serial (msg.handle(), 1);
            auto error_reply = msg.make_reply (true, err.name, err.message);
            dbus_error_free (&err);
            return error_reply;
        }
        dbus_error_free (&err);

        message reply (result);
        dbus_message_unref (reply.handle()); // ref count increased in message constructor
        return reply;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<uint32_t> connection::request_name (const std::string& bus_name,
                                                 uint32_t flags,
                                                 int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<uint32_t> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid bus name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "RequestName");
        msg.append_args (bus_name, flags);
        return sync_call_method<uint32_t> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::request_name (const std::string& bus_name,
                                   uint32_t flags,
                                   std::function<void (retvalue<uint32_t>& retval)> callback,
                                   int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "RequestName");
        msg.append_args (bus_name, flags);
        return async_call_method<uint32_t> (*this, msg, callback, timeout);
    }




    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<uint32_t> connection::release_name (const std::string& bus_name,
                                                 int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<uint32_t> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid bus name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ReleaseName");
        msg.append_args (bus_name);
        return sync_call_method<uint32_t> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::release_name (const std::string& bus_name,
                                   std::function<void (retvalue<uint32_t>& retval)> callback,
                                   int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ReleaseName");
        msg.append_args (bus_name);
        return async_call_method<uint32_t> (*this, msg, callback, timeout);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::set<std::string>> connection::list_queued_owners (const std::string& bus_name,
                                                                    int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<std::set<std::string>> ret;
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid bus name");
            return ret;
        }

        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListQueuedOwners");
        msg.append_args (bus_name);
        auto reply = send_and_wait (msg, timeout);
        return handle_set_string_reply (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::list_queued_owners (
            const std::string& bus_name,
            std::function<void (retvalue<std::set<std::string>>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListQueuedOwners");
        msg.append_args (bus_name);
        if (callback == nullptr) {
            msg.want_reply (false);
            return send (msg);
        }
        return send (msg,
                     [callback](message& reply){
                         auto retval = handle_set_string_reply (reply);
                         callback (retval);
                     },
                     timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::set<std::string>> connection::list_names (int timeout)
    {
        if (ctx==nullptr)
            return retvalue<std::set<std::string>> {-1, "Not connected"};
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListNames");
        auto reply = send_and_wait (msg, timeout);
        return handle_set_string_reply (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::list_names (
            std::function<void (retvalue<std::set<std::string>>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr)
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListNames");
        if (callback == nullptr) {
            msg.want_reply (false);
            return send (msg);
        }
        return send (msg,
                     [callback](message& reply){
                         auto retval = handle_set_string_reply (reply);
                         callback (retval);
                     },
                     timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::set<std::string>> connection::list_activatable_names (int timeout)
    {
        if (ctx==nullptr)
            return retvalue<std::set<std::string>> {-1, "Not connected"};
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListActivatableNames");
        auto reply = send_and_wait (msg, timeout);
        return handle_set_string_reply (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::list_activatable_names (
            std::function<void (retvalue<std::set<std::string>>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr)
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "ListActivatableNames");
        if (callback == nullptr) {
            msg.want_reply (false);
            return send (msg);
        }
        return send (msg,
                     [callback](message& reply){
                         auto retval = handle_set_string_reply (reply);
                         callback (retval);
                     },
                     timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> connection::name_has_owner (const std::string& bus_name,
                                               int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<bool> ret (false);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid bus name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner");
        msg.append_args (bus_name);
        return sync_call_method<bool> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::name_has_owner (const std::string& bus_name,
                                     std::function<void (retvalue<bool>& retval)> callback,
                                     int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner");
        msg.append_args (bus_name);
        return async_call_method<bool> (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<uint32_t> connection::start_service_by_name (const std::string& service,
                                                          uint32_t flags,
                                                          int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr)) {
            retvalue<uint32_t> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid service name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "StartServiceByName");
        msg.append_args (service, flags);
        return sync_call_method<uint32_t> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::start_service_by_name (
            const std::string& service,
            uint32_t flags,
            std::function<void (retvalue<uint32_t>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "StartServiceByName");
        msg.append_args (service, flags);
        return async_call_method<uint32_t> (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> connection::update_activation_environment (
            const std::map<std::string, std::string>& env,
            int timeout)
    {
        if (ctx==nullptr) {
            retvalue<bool> ret (false);
            ret.err (-1, "Not connected");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "UpdateActivationEnvironment");
        dbus_dict<dbus_string> dict (DBUS_TYPE_STRING);
        for (const auto& entry : env)
            dict.set (entry.first, entry.second);
        msg.append_args (dict);
        return sync_call_method_reply_with_void (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::update_activation_environment (
            const std::map<std::string, std::string>& env,
            std::function<void (retvalue<bool>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr)
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "UpdateActivationEnvironment");
        dbus_dict<dbus_string> dict (DBUS_TYPE_STRING);
        for (const auto& entry : env)
            dict.set (entry.first, entry.second);
        msg.append_args (dict);
        return async_call_method_reply_with_void (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::string> connection::get_name_owner (const std::string& bus_name,
                                                      int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<std::string> ret;
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid bus name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetNameOwner");
        msg.append_args (bus_name);
        return sync_call_method<std::string> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::get_name_owner (const std::string& bus_name,
                                     std::function<void (retvalue<std::string>& retval)> callback,
                                     int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetNameOwner");
        msg.append_args (bus_name);
        return async_call_method<std::string> (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<uint32_t> connection::get_connection_unix_user (const std::string& service,
                                                             int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr)) {
            retvalue<uint32_t> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid service name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionUnixUser");
        msg.append_args (service);
        return sync_call_method<uint32_t> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::get_connection_unix_user (
            const std::string& service,
            std::function<void (retvalue<uint32_t>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionUnixUser");
        msg.append_args (service);
        return async_call_method<uint32_t> (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<uint32_t> connection::get_connection_unix_process_id (const std::string& service,
                                                                   int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr)) {
            retvalue<uint32_t> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid service name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionUnixProcessID");
        msg.append_args (service);
        return sync_call_method<uint32_t> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::get_connection_unix_process_id (
            const std::string& service,
            std::function<void (retvalue<uint32_t>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionUnixProcessID");
        msg.append_args (service);
        return async_call_method<uint32_t> (*this, msg, callback, timeout);
    }



    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<dbus_dict<dbus_string>> connection::get_connection_credentials (
            const std::string& service,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr)) {
            retvalue<dbus_dict<dbus_string>> ret (0);
            if (ctx==nullptr)
                ret.err (-1, "Not connected");
            else
                ret.err (-1, "Invalid service name");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionCredentials");
        msg.append_args (service);
        auto reply = send_and_wait (msg, timeout);
        return handle_dict_str_variant_reply (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::get_connection_credentials (
            const std::string& service,
            std::function<void (retvalue<dbus_dict<dbus_string>>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr || FALSE==dbus_validate_bus_name(service.c_str(), nullptr))
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetConnectionCredentials");
        msg.append_args (service);
        if (callback == nullptr) {
            msg.want_reply (false);
            return send (msg);
        }
        return send (msg,
                     [callback](message& reply){
                         auto retval = handle_dict_str_variant_reply (reply);
                         callback (retval);
                     },
                     timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<std::string> connection::get_id (int timeout)
    {
        if (ctx==nullptr)
            return retvalue<std::string> {-1, "Not connected"};
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetId");
        return sync_call_method<std::string> (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::get_id (
            std::function<void (retvalue<std::string>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr)
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "GetId");
        return async_call_method<std::string> (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> connection::become_monitor (const std::list<std::string>& rules,
                                               int timeout)
    {
        if (ctx==nullptr) {
            retvalue<bool> ret (false);
            ret.err (-1, "Not connected");
            return ret;
        }
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "BecomeMonitor");
        dbus_array array ("s");
        for (const auto& rule : rules)
            array.push_back (rule);
        msg.append_args (array, (uint32_t)0);
        return sync_call_method_reply_with_void (*this, msg, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::become_monitor (
            const std::list<std::string>& rules,
            std::function<void (retvalue<bool>& retval)> callback,
            int timeout)
    {
        if (ctx==nullptr)
            return false;
        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "BecomeMonitor");
        dbus_array array ("s");
        for (const auto& rule : rules)
            array.push_back (rule);
        msg.append_args (array, (uint32_t)0);
        return async_call_method_reply_with_void (*this, msg, callback, timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::set_name_owner_changed_cb (name_owner_changed_cb_t callback)
    {
        if (!is_connected())
            return false;

        static const std::string rule =
            "type='signal'"
            ",sender='org.freedesktop.DBus'"
            ",path='/org/freedesktop/DBus'"
            ",interface='org.freedesktop.DBus'"
            ",member='NameOwnerChanged'";

        bool retval = true;
        const std::lock_guard lock (cb_mutex);

        if (callback) {
            // Set callback
            if (!name_owner_changed_cb)
                retval = add_match (rule, nullptr);
        }else{
            // Remove callback
            if (name_owner_changed_cb)
                retval = remove_match (rule, nullptr);
        }
        if (retval)
            name_owner_changed_cb = callback;
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::set_name_lost_cb (bus_name_cb_t callback)
    {
        if (!is_connected())
            return false;

        static const std::string rule =
            "type='signal'"
            ",sender='org.freedesktop.DBus'"
            ",path='/org/freedesktop/DBus'"
            ",interface='org.freedesktop.DBus'"
            ",member='NameLost'";

        bool retval = true;
        const std::lock_guard lock (cb_mutex);

        if (callback) {
            // Set callback
            if (!name_lost_cb)
                retval = add_match (rule, nullptr);
        }else{
            // Remove callback
            if (name_lost_cb)
                retval = remove_match (rule, nullptr);
        }
        if (retval)
            name_lost_cb = callback;
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::set_name_acquired_cb (bus_name_cb_t callback)
    {
        if (!is_connected())
            return false;

        static const std::string rule =
            "type='signal'"
            ",sender='org.freedesktop.DBus'"
            ",path='/org/freedesktop/DBus'"
            ",interface='org.freedesktop.DBus'"
            ",member='NameAcquired'";

        bool retval = true;
        const std::lock_guard lock (cb_mutex);

        if (callback) {
            // Set callback
            if (!name_acquired_cb)
                retval = add_match (rule, nullptr);
        }else{
            // Remove callback
            if (name_acquired_cb)
                retval = remove_match (rule, nullptr);
        }
        if (retval)
            name_acquired_cb = callback;
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::start_io_handler ()
    {
        stop_io_thread = false;
        io_thread = std::thread ([this](){io_handler();});

        dbus_connection_set_wakeup_main_function (ctx,
                                                  static_wakeup_main_cb,
                                                  this,
                                                  nullptr);

        dbus_connection_set_dispatch_status_function (ctx,
                                                      static_dispatch_status_cb,
                                                      this,
                                                      nullptr);
        dbus_connection_set_watch_functions (ctx,
                                             static_add_watch_cb,
                                             static_remove_watch_cb,
                                             static_toggled_watch_cb,
                                             this,
                                             nullptr);
        dbus_connection_set_timeout_functions (ctx,
                                               static_add_timeout_cb,
                                               static_remove_timeout_cb,
                                               static_toggled_timeout_cb,
                                               this,
                                               nullptr);

        stop_dispatch_thread = false;
        dispatch_thread = std::thread ([this](){msg_dispatcher();});
        get_id ();
    }


    //--------------------------------------------------------------------------
    // watcher_mutex locked
    //--------------------------------------------------------------------------
    void connection::on_timerfd_event (uint32_t events)
    {
        timer_mutex.lock ();
        if ( ! (events & EPOLLIN)) {
            timer_mutex.unlock ();
            return;
        }
        uint64_t expirations {};
        if (read(timerfd, &expirations, sizeof(expirations)) <= 0) {
            ; // Could this ever happen ?
        }

        if (timers.empty()) {
            timer_mutex.unlock ();
            return;
        }

        auto& t = timers.front ();
        DBusTimeout* handle = std::get<idx_handle> (t);
        inc_abs_time (std::get<idx_abs_time>(t), std::get<idx_repeat>(t));
        timers.sort (compare_timer_entry);

        itimerspec it {};
        it.it_value = std::get<idx_abs_time> (timers.front());
        if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &it, nullptr)) {
            ; // Could this ever happen ?
        }
        timer_mutex.unlock ();
        watcher_mutex.unlock ();
        dbus_timeout_handle (handle);
        watcher_mutex.lock ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::io_handler ()
    {
        std::array<struct epoll_event, max_epoll_events> epoll_result {};
        while (!stop_io_thread) {
            const int num_events = epoll_wait (pollfd, epoll_result.data(), epoll_result.size(), 100);
            if (num_events <= 0) {
                if (num_events < 0) {
                    ; // epoll_wait error
                }
                continue;
            }
            watcher_mutex.lock ();
            handle_epoll_result (epoll_result, num_events);
            watcher_mutex.unlock ();
        }
    }


    //--------------------------------------------------------------------------
    // watcher_mutex is locked !
    //--------------------------------------------------------------------------
    void connection::handle_epoll_result (
            std::array<struct epoll_event, max_epoll_events>& epoll_result,
            const int num_events)
    {
        int num_events_left_to_handle = num_events;
        for (auto& io_events : epoll_result) {
            if (num_events_left_to_handle-- == 0)
                break;

            const int fd = io_events.data.fd;

            if (fd == timerfd) {
                on_timerfd_event (io_events.events);
                continue;
            }

            auto fd_map_i = fd_map.find (fd);
            if (fd_map_i == fd_map.end()) {
                // "Internal error: can't find info on fd %d", fd
                continue;
            }

            auto w_iter = fd_map_i->second.begin ();
            while (w_iter != fd_map_i->second.end()) {
                DBusWatch* dbus_watch_obj = w_iter->first;
                const uint32_t event_mask = w_iter->second;
                ++w_iter;
                handle_io_watch_event (dbus_watch_obj, io_events, event_mask);
                if (!fd_map.contains(fd))
                    break;
            }
        }
    }


    namespace {
        //----------------------------------------------------------------------
        // Namespace: anonymous
        //----------------------------------------------------------------------
        unsigned epoll_events_to_dbus_events (const uint32_t epoll_events)
        {
            unsigned dbus_events {};
            if (epoll_events & EPOLLIN)
                dbus_events |= DBUS_WATCH_READABLE;
            if (epoll_events & EPOLLOUT)
                dbus_events |= DBUS_WATCH_WRITABLE;
            if (epoll_events & EPOLLERR)
                dbus_events |= DBUS_WATCH_ERROR;
            if (epoll_events & EPOLLHUP)
                dbus_events |= DBUS_WATCH_HANGUP;
            return dbus_events;
        }
    }


    //--------------------------------------------------------------------------
    // watcher_mutex is locked !
    //--------------------------------------------------------------------------
    void connection::handle_io_watch_event (DBusWatch* dbus_watch_obj,
                                            struct epoll_event& io_events,
                                            const uint32_t event_mask)
    {
        unsigned dbus_events = 0;
        const uint32_t triggered_events = io_events.events & (event_mask | EPOLLHUP | EPOLLERR);

        // TRACE_DBUS_CONNECTION (
        //         "Check watch %08lx, fd: %d, epoll events: %s, watched events: %s, triggered events: %s",
        //         (unsigned long)dbus_watch_obj,
        //         fd,
        //         epoll_events_to_string(io_events.events).c_str(),
        //         epoll_events_to_string(event_mask).c_str(),
        //         epoll_events_to_string(triggered_events).c_str());

        if (triggered_events == 0)
            return;

        dbus_events = epoll_events_to_dbus_events (triggered_events);
        if (dbus_events == 0)
            return;

        if (dbus_watch_get_enabled(dbus_watch_obj)) {
            // TRACE_DBUS_CONNECTION ("Call dbus_watch_handle(%08lx, %s)",
            //                        (unsigned long)dbus_watch_obj,
            //                        dbus_watch_flags_to_string(dbus_events).c_str());
            watcher_mutex.unlock ();
            dbus_watch_handle (dbus_watch_obj, dbus_events);
            watcher_mutex.lock ();
            // TRACE_DBUS_CONNECTION ("dbus_watch_handle(%08lx) done", (unsigned long)dbus_watch_obj);
        }else{
            ; // TRACE_DBUS_CONNECTION ("Watch not enabled");
        }
        io_events.events &= ~triggered_events; // un-mark the handled events
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::msg_dispatcher ()
    {
        while (!stop_dispatch_thread) {
            std::unique_lock lock (dispatch_cond_mutex);
            dispatch_cond.wait (lock, [this](){
                return stop_dispatch_thread ||
                    dbus_connection_get_dispatch_status(ctx) != DBUS_DISPATCH_COMPLETE;
            });
            while (!stop_dispatch_thread && dbus_connection_dispatch(ctx) == DBUS_DISPATCH_DATA_REMAINS) {
                ; // Message dispatched
            }
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::on_signal (message& msg)
    {
        if (msg.interface() != "org.freedesktop.DBus" ||
            msg.path() != "/org/freedesktop/DBus")
        {
            return false;
        }

        const std::lock_guard lock (cb_mutex);

        if (!name_owner_changed_cb && !name_lost_cb && !name_acquired_cb)
            return false;

        if (unique_bus_name.empty()) {
            // Fetch the unique bus name of 'org.freedesktop.DBus' before calling on_signal_impl
            return get_name_owner ("org.freedesktop.DBus", [this, msg](retvalue<std::string>& retval) mutable {
                const std::lock_guard lock (cb_mutex);
                unique_bus_name = retval.get ();
                on_signal_impl (msg);
            });
        }else{
            return on_signal_impl (msg);
        }
    }


    //--------------------------------------------------------------------------
    // cb_mutex is locked
    //--------------------------------------------------------------------------
    bool connection::on_signal_impl (message& msg)
    {
        if (msg.sender() != unique_bus_name)
            return false;

        bool retval = false;
        if (msg.name() == "NameOwnerChanged" && name_owner_changed_cb) {
            std::string name;
            std::string old_owner;
            std::string new_owner;
            if (msg.get_args(name, old_owner, new_owner)) {
                auto callback = name_owner_changed_cb;
                cb_mutex.unlock ();
                callback (name, old_owner, new_owner);
                cb_mutex.lock ();
                retval = true;
            }
        }
        else if (msg.name() == "NameLost" && name_lost_cb) {
            std::string name;
            if (msg.get_args(name)) {
                auto callback = name_lost_cb;
                cb_mutex.unlock ();
                callback (name, false);
                cb_mutex.lock ();
                retval = true;
            }
        }
        else if (msg.name() == "NameAcquired" && name_acquired_cb) {
            std::string name;
            if (msg.get_args(name)) {
                auto callback = name_acquired_cb;
                cb_mutex.unlock ();
                callback (name, true);
                cb_mutex.lock ();
                retval = true;
            }
        }
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_pending_call (DBusPendingCall* pending)
    {
        pending_mutex.lock ();

        auto entry = pending_messages.find (pending);
        if (entry == pending_messages.end()) {
            pending_mutex.unlock (); // Pending call not found !?!
            return;
        }
        auto callback = entry->second;
        pending_messages.erase (entry);
        pending_mutex.unlock ();

        if (callback) {
            message reply (dbus_pending_call_steal_reply(pending));
            dbus_message_unref (reply.handle()); // ref count increased in message constructor
            dbus_pending_call_unref (pending);
            callback (reply);
        }else{
            dbus_pending_call_unref (pending);
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_wakeup_main ()
    {
        dispatch_cond.notify_one (); // Wake up message dispatcher
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_dispatch_status (DBusDispatchStatus /*new_status*/)
    {
        dispatch_cond.notify_one (); // Wake up message dispatcher
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::on_add_watch (DBusWatch* watch)
    {
        const std::lock_guard lock (watcher_mutex);

        int fd = dbus_watch_get_unix_fd (watch);
        unsigned watch_flags = 0;

        if (dbus_watch_get_enabled(watch))
            watch_flags = dbus_watch_get_flags (watch);

        // TRACE_DBUS_CONNECTION ("Add watch (%08lx), fd: %d, enabled: %s, events: %s",
        //                        (unsigned long)watch,
        //                        fd,
        //                        (dbus_watch_get_enabled(watch)?"1":"0"),
        //                        dbus_watch_flags_to_string(dbus_watch_get_flags(watch)).c_str());

        uint32_t fd_events = 0;
        struct epoll_event ev {};
        ev.data.fd = fd;

        if (watch_flags & DBUS_WATCH_READABLE)
            fd_events |= EPOLLIN;
        if (watch_flags & DBUS_WATCH_WRITABLE)
            fd_events |= EPOLLOUT;

        auto i = fd_map.find (fd);
        if (i == fd_map.end()) {
            ev.events = fd_events;
            auto& we = fd_map.emplace(fd, std::map<DBusWatch*, uint32_t>()).first->second;
            we.emplace (watch, fd_events);
            const int res = epoll_ctl (pollfd, EPOLL_CTL_ADD, fd, &ev);
            if (res < 0) {
                fd_map.erase (fd);
                // DBG_DBUS_CONNECTION ("epoll_ctl (add) failed: %s", strerror(errno));
                return false;
            }
        }else{
            i->second.emplace (watch, fd_events);
            for (const auto& entry : i->second)
                fd_events |= entry.second;

            ev.events = fd_events;
            if (epoll_ctl(pollfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
                i->second.erase (watch);
                // DBG_DBUS_CONNECTION ("epoll_ctl (mod) failed: %s", strerror(errno));
                return false;
            }
        }

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_remove_watch (DBusWatch* watch)
    {
        const std::lock_guard lock (watcher_mutex);

        const int fd = dbus_watch_get_unix_fd (watch);

        // TRACE_DBUS_CONNECTION ("Remove watch (%08lx), fd: %d, events: %s",
        //                        (unsigned long)watch, fd,
        //                        dbus_watch_flags_to_string(dbus_watch_get_flags(watch)).c_str());

        auto entry = fd_map.find (fd);
        if (entry == fd_map.end())
            return;

        entry->second.erase (watch);
        if (entry->second.empty()) {
            fd_map.erase (entry);
            epoll_ctl (pollfd, EPOLL_CTL_DEL, fd, nullptr);
        }else{
            struct epoll_event ev {};
            ev.data.fd = fd;
            ev.events = 0;
            for (const auto& e : entry->second)
                ev.events |= e.second;
            epoll_ctl (pollfd, EPOLL_CTL_MOD, fd, &ev);
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_toggled_watch (DBusWatch* watch)
    {
        const std::lock_guard lock (watcher_mutex);

        const int fd = dbus_watch_get_unix_fd (watch);
        unsigned watch_flags = 0;

        if (dbus_watch_get_enabled(watch))
            watch_flags = dbus_watch_get_flags (watch);

        // TRACE_DBUS_CONNECTION ("Toggle watch (%08lx), fd: %d, enabled: %s, events: %s",
        //                        (unsigned long)watch,
        //                        fd,
        //                        (dbus_watch_get_enabled(watch)?"1":"0"),
        //                        dbus_watch_flags_to_string(dbus_watch_get_flags(watch)).c_str());
        uint32_t fd_events = 0;
        struct epoll_event ev {};
        ev.data.fd = fd;

        if (watch_flags & DBUS_WATCH_READABLE)
            fd_events |= EPOLLIN;
        if (watch_flags & DBUS_WATCH_WRITABLE)
            fd_events |= EPOLLOUT;

        auto i = fd_map.find (fd);
        if (i == fd_map.end())
            return;

        for (auto& entry : i->second) {
            if (entry.first == watch)
                entry.second = fd_events;
            fd_events |= entry.second;
        }
        ev.events = fd_events;
        epoll_ctl (pollfd, EPOLL_CTL_MOD, fd, &ev);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool connection::on_add_timeout (DBusTimeout* timeout)
    {
        struct timespec abs {};
        clock_gettime (timer_clock_id, &abs);

        const std::lock_guard lock (timer_mutex);

        unsigned repeat = dbus_timeout_get_interval (timeout);
        const bool enabled = dbus_timeout_get_enabled (timeout);

        // TRACE_DBUS_CONNECTION ("Add timer %08lx, repeat: %u, enabled: %d",
        //                        (unsigned long)timeout, repeat, (enabled?1:0));
        if (!enabled)
            return true;

        if (repeat == 0)
            repeat = 1;

        inc_abs_time (abs, repeat);

        // Find where in the list to put the timer entry
        const timespec_less_t t_less;
        auto pos = timers.begin ();
        for (; pos!=timers.end(); ++pos) {
            const struct timespec& ts = std::get<idx_abs_time> (*pos);
            if (t_less(abs, ts))
                break;
        }
        const bool activate_timer = pos == timers.begin();

        // Create the timer entry
        //timers.emplace (pos, std::make_tuple(timeout, abs, repeat));
        timers.emplace (pos, timeout, abs, repeat);

        // Activate timerfd if needed
        if (activate_timer) {
            itimerspec it {};
            it.it_value = abs;
            it.it_interval.tv_sec = 0;
            it.it_interval.tv_nsec = 0;
            if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &it, nullptr)) {
                return false;
            }
        }

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_remove_timeout (DBusTimeout* timeout)
    {
        const std::lock_guard lock (timer_mutex);

        if (timers.empty()) {
            return;
        }

        auto pos = timers.begin ();
        for (; pos != timers.end(); ++pos) {
            if (std::get<idx_handle>(*pos) == timeout)
                break;
        }
        if (pos == timers.end()) {
            return;
        }

        itimerspec it {};
        const bool update_timer = pos == timers.begin ();

        timers.erase (pos);

        if (timers.empty()) {
            if (timerfd_settime(timerfd, 0, &it, nullptr)) {
                ; // DBG_DBUS_CONNECTION ("timerfd_settime failed: %s", strerror(errno));
            }
        }else if (update_timer) {
            it.it_value = std::get<idx_abs_time> (timers.front());
            if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &it, nullptr)) {
                ; //DBG_DBUS_CONNECTION ("timerfd_settime failed: %s", strerror(errno));
            }
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::on_toggled_timeout (DBusTimeout* timeout)
    {
        if (dbus_timeout_get_enabled(timeout))
            on_add_timeout (timeout);
        else
            on_remove_timeout (timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_pending_call_cb (DBusPendingCall* pending, void* data)
    {
        static_cast<connection*>(data)->on_pending_call (pending);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_wakeup_main_cb (void* data)
    {
        static_cast<connection*>(data)->on_wakeup_main ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_dispatch_status_cb (DBusConnection* /*dbus_connection*/,
                                                DBusDispatchStatus new_status,
                                                void* data)
    {
        static_cast<connection*>(data)->on_dispatch_status (new_status);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    dbus_bool_t connection::static_add_watch_cb (DBusWatch* watch, void* data)
    {
        return static_cast<connection*>(data)->on_add_watch (watch);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_remove_watch_cb (DBusWatch* watch, void* data)
    {
        static_cast<connection*>(data)->on_remove_watch (watch);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_toggled_watch_cb (DBusWatch* watch, void* data)
    {
        static_cast<connection*>(data)->on_toggled_watch (watch);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    dbus_bool_t connection::static_add_timeout_cb (DBusTimeout* timeout, void* data)
    {
        return static_cast<connection*>(data)->on_add_timeout (timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_remove_timeout_cb (DBusTimeout* timeout, void* data)
    {
        static_cast<connection*>(data)->on_remove_timeout (timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void connection::static_toggled_timeout_cb (DBusTimeout* timeout, void* data)
    {
        static_cast<connection*>(data)->on_toggled_timeout (timeout);
    }


}
