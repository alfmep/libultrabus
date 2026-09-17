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
#ifndef ULTRABUS_OBJECT_HANDLER_HPP
#define ULTRABUS_OBJECT_HANDLER_HPP

#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <functional>
#include <string>
#include <mutex>
#include <map>
#include <dbus/dbus.h>



namespace ultrabus {


    /**
     * Registers object paths and dispatches incoming D-Bus method calls.
     *
     * The handler retains a non-owning reference to the supplied connection;
     * the connection must outlive the handler. Its destructor unregisters any
     * remaining paths while the connection is active.
     */
    class object_handler : public DBusObjectPathVTable {
    public:
        /**
         * Callback for an incoming method call to a registered object path.
         *
         * @param msg Received method-call message. The reference is valid only
         *            for the duration of the callback.
         * @return <code>true</code> if the message was handled by
         *         the callback. If not, return <code>false</code>.
         */
        using method_call_cb_t = std::function<bool (message& msg)>;

        /**
         * Construct an object handler bound to a connection.
         *
         * @param connection Connection used to register paths and dispatch
         *                   calls. It is not owned and must outlive this
         *                   handler.
         */
        object_handler (connection& connection);

        /** Destroy the handler and release its path-registration state. */
        virtual ~object_handler ();

        /**
         * Register an object path to be handled by this instance.
         * @param opath The object path to register.
         * @param callback Callback invoked for method calls to the path.
         *                 If the path is already registered,
         *                 its callback is replaced.
         * @param fallback If <code>true</code>, the <code>on_message()</code>
         *                 will be called for all objects in this subdirectory.
         *                 If <code>false</code>, the <code>on_message()</code>
         *                 will be called for this specific object path only.
         * @return <code>true</code> when a new path was registered,
         *         <code>false</code> if the path cannot be registered.
         */
        virtual bool register_object_path (
                const std::string& opath,
                method_call_cb_t callback,
                bool fallback=false);

        /**
         * Unregister an object path handled by this instance.
         * @param opath The object path to unregister.
         * @return <code>true</code> when the path was unregistered or was not
         *         registered, <code>false</code> when it cannot be
         *         unregistered.
         */
        virtual bool unregister_object_path (const std::string& opath);


    protected:
        /**
         * A referencd to the connection used by this handler.
         * The referenced connection must outlive this handler.
         */
        connection& conn;

        /**
         * Handle an incoming method call not handled by a registered callback.
         *
         * Override to provide fallback handling for derived handlers.
         *
         * @param msg Incoming D-Bus method-call message. The reference is
         *            valid only for the duration of this call.
         * @return <code>true</code> if the message was handled,
         *         <code>false</code> if not.
         */
        virtual bool on_method_call (message& msg);


    private:
        std::map<std::string, std::pair<object_handler*, method_call_cb_t>> opath_map;
        std::mutex opaths_lock;

        static void dbus_on_unregister (DBusConnection* connection, void* user_data);
        static DBusHandlerResult dbus_on_message (DBusConnection* connection, DBusMessage* dbus_msg, void* user_data);
    };



}
#endif
