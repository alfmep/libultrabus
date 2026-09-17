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
#include <ultrabus/object_handler.hpp>
#include <cstring>

#include <iostream>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object_handler::object_handler (connection& connection)
        : conn (connection)
    {
        // Initialize function pointers in DBusObjectPathVTable
        auto* vtable = dynamic_cast<DBusObjectPathVTable*> (this);
        memset (vtable, 0, sizeof(*vtable));
        unregister_function = dbus_on_unregister;
        message_function    = dbus_on_message;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object_handler::~object_handler ()
    {
        const std::lock_guard lock (opaths_lock);
        if (conn.is_connected()) {
            for (auto& opath : opath_map)
                dbus_connection_unregister_object_path (conn.handle(), opath.first.c_str());
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_handler::register_object_path (
            const std::string& opath,
            method_call_cb_t callback,
            bool fallback)
    {
        const std::lock_guard lock (opaths_lock);

        auto entry = opath_map.find (opath);
        if (entry != opath_map.end()) {
            // Path already registered, replace callback
            entry->second.second = callback;
            return false;
        }

        if (!conn.is_connected())
            return false;

        entry = opath_map.emplace (opath, std::pair(this, callback)).first;
        std::pair<object_handler*, method_call_cb_t>* user_data = &(entry->second);

        bool result {false};
        if (fallback) {
            result = dbus_connection_try_register_fallback (conn.handle(),
                                                            opath.c_str(),
                                                            this,
                                                            user_data,
                                                            nullptr);
        }else{
            result = dbus_connection_try_register_object_path (conn.handle(),
                                                               opath.c_str(),
                                                               this,
                                                               user_data,
                                                               nullptr);
        }
        if ( ! result)
            opath_map.erase (entry);

        return result;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_handler::unregister_object_path (const std::string& opath)
    {
        const std::lock_guard lock (opaths_lock);

        if (!conn.is_connected())
            return false;

        auto entry = opath_map.find (opath);
        if (entry == opath_map.end())
            return true; // Already unregistered

        if (TRUE == dbus_connection_unregister_object_path (conn.handle(), opath.c_str())) {
            opath_map.erase (entry);
            return true;
        }else{
            return false;
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_handler::on_method_call (message& msg)
    {
        std::cerr << __FUNCTION__ << " - Got method call " << msg.name() << std::endl;
        return false;
    }


    //--------------------------------------------------------------------------
    // Static method
    //--------------------------------------------------------------------------
    void object_handler::dbus_on_unregister (DBusConnection* connection, void* user_data)
    {
        //auto* self = static_cast<object_handler*> (user_data);
    }


    //--------------------------------------------------------------------------
    // Static method
    //--------------------------------------------------------------------------
    DBusHandlerResult object_handler::dbus_on_message (DBusConnection* connection,
                                                       DBusMessage* dbus_msg,
                                                       void* user_data)
    {
        std::pair<object_handler*, method_call_cb_t>* oh_cb;
        oh_cb = static_cast<std::pair<object_handler*, method_call_cb_t>*> (user_data);
        auto* self = oh_cb->first;

        self->opaths_lock.lock ();

        message msg (dbus_msg);

        if (oh_cb->second == nullptr) {
            self->opaths_lock.unlock ();
            return self->on_method_call(msg) ?
                DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        method_call_cb_t callback = oh_cb->second;
        self->opaths_lock.unlock ();
        return callback(msg) ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }


}
