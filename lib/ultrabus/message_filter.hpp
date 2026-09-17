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
#ifndef ULTRABUS_MESSAGE_FILTER_HPP
#define ULTRABUS_MESSAGE_FILTER_HPP

#include <ultrabus/message.hpp>
#include <ultrabus/retvalue.hpp>
#include <functional>
#include <stdexcept>
#include <string>
#include <mutex>
#include <set>
#include <dbus/dbus.h>


namespace ultrabus {


    // Forward declarations
    class connection;


    /**
     * Base class for D-Bus message handlers registered on a connection.
     *
     * A filter is registered with <code>conn</code> for its lifetime and does
     * not own that connection; therefore, the connection must outlive the
     * filter. Incoming-message hooks are invoked by the connection's dispatch
     * thread. Derived classes must synchronize any state shared with other
     * threads, and must not retain <code>msg</code> references beyond a hook
     * invocation.
     */
    class message_filter {
    public:
        /**
         * Constructor.
         * @param conn The connection to register this filter on.
         *        It must remain alive until this filter is destroyed.
         * @throws std::system_error if the underlying filter cannot be
         *         registered because libdbus runs out of memory.
         */
        message_filter (connection& conn);

        /**
         * Destructor.
         * Remove tracked match rules and unregister this handler when the
         * connection is still open. Destruction must not race with dispatching
         * a message to this filter.
         */
        virtual ~message_filter ();

        /**
         * Adds a match rule to match messages going through the message bus.
         * @param rule Match rule to add to the connection.
         * @param timeout Timeout in milliseconds, or
         *        <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return A successful value on success; otherwise a false value with
         *         error details, including when the connection is closed.
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-add-match
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: AddMatch</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-routing-match-rules
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Match Rules</a>
         */
        retvalue<bool> add_match (const std::string& rule,
                                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to add a match rule to match messages going through the message bus.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function. The callback executes
         * on the connection's dispatch thread, receives a reference valid only
         * for the call, and is not invoked when this function returns
         * <code>false</code>.
         * @param rule Match rule to add to the connection.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *        <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         *
         */
        bool add_match (const std::string& rule,
                        std::function<void (retvalue<bool>& retval)> callback,
                        int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Removes a previously-added match rule.
         * @param rule The match rule to remove.
         * @param timeout Timeout in milliseconds, or
         *        <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return A successful value on success; otherwise a false value with
         *         error details, including when the connection is closed.
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-routing-match-rules
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Match Rules</a>
         */
        retvalue<bool> remove_match (const std::string& rule,
                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Asynchronously remove a previously-added match rule. The callback
         * executes on the connection's dispatch thread, receives a reference
         * valid only for the call, and is not invoked when this function
         * returns <code>false</code>.
         * @param rule The match rule to remove.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *        <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-routing-match-rules
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Match Rules</a>
         */
        bool remove_match (const std::string& rule,
                           std::function<void (retvalue<bool>& retval)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT);


    protected:
        /**
         * A reference to the connection on which this filter is registered.
         * It remains valid only while the connection outlives this filter.
         */
        connection& conn;

        /**
         * Handle an incoming method call.
         *
         * Called by <code>dispatch_msg()</code> on the connection's dispatch
         * thread. The default implementation returns <code>false</code>.
         * @param msg The incoming method call.
         * @return true if the message was handled,
         *         false if not. The default implementation
         *         returns false.
         */
        virtual bool on_method_call (message& msg);

        /**
         * Handle an incoming signal.
         *
         * Called by <code>dispatch_msg()</code> on the connection's dispatch
         * thread. The default implementation returns <code>false</code>.
         * @param msg The incoming signal.
         * @return true if the message was handled,
         *         false if not. The default implementation
         *         returns false.
         */
        virtual bool on_signal (message& msg);

        /**
         * Handle an incoming message.
         *
         * Called on the connection's dispatch thread. The default implementation
         * calls <code>dispatch_msg()</code>, which routes method calls and
         * signals to their corresponding hooks.
         * @return true if the message was handled,
         *         false if not.
         */
        virtual bool on_message (message& msg);

        /**
         * Dispatch a method call or signal to its typed hook.
         *
         * Messages of other types are not handled.
         * @param msg The incoming message to dispatch.
         * @return true if the message was handled,
         *         false if not.
         */
        bool dispatch_msg (message& msg);


    private:
        friend class connection;
        std::mutex match_rule_mutex;
        std::set<std::string> match_rules;

        message_filter (connection& conn, bool delay_filter_install);
        bool install_filter_handler ();
        void remove_filter_handler ();

        static DBusHandlerResult static_dbus_handler (
                DBusConnection* dbconn,
                DBusMessage* dbmsg,
                void* user_data);
    };



}
#endif
