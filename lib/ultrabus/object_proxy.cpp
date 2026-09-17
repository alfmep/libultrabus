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
#include <ultrabus/object_proxy.hpp>
#include <ultrabus/connection.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <sstream>


namespace ultrabus {

    static constexpr const char* dbus_iface_object_manager = "org.freedesktop.DBus.ObjectManager";


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object_proxy::object_proxy (connection& conn,
                                const std::string& bus_name,
                                const std::string& object_path,
                                const std::string& default_interface)
        : message_filter (conn),
          target (bus_name),
          opath (object_path),
          def_iface (default_interface)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr))
            throw std::invalid_argument ("Invalid DBus bus name");
        if (!dbus_validate_path(opath.c_str(), nullptr))
            throw std::invalid_argument ("Invalid DBus object path");
        if (!default_interface.empty() && !dbus_validate_interface(default_interface.c_str(), nullptr))
            throw std::invalid_argument ("Invalid DBus interface name");
        if (target[0] == ':')
            target_unique_name = target;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::default_interface (const std::string& default_interface)
    {
        if (!default_interface.empty() && !dbus_validate_interface(default_interface.c_str(), nullptr))
            return false;

        def_iface = default_interface;
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string object_proxy::make_signal_match_rule (const std::string& interface,
                                                      const std::string& signal)
    {
        std::string rule ("type='signal',sender='");
        //rule.append (target);
        rule.append (target_unique_name);
        rule.append ("',path='");
        rule.append (opath);
        rule.push_back ('\'');
        if (!interface.empty()) {
            rule.append (",interface='");
            rule.append (interface);
            rule.push_back ('\'');
        }
        if (!signal.empty()) {
            rule.append (",member='");
            rule.append (signal);
            rule.push_back ('\'');
        }
        return rule;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::set_signal_callback (const std::string& interface,
                                            const std::string& signal,
                                            signal_cb_t callback)
    {
        if (!interface.empty() && !dbus_validate_interface(interface.c_str(), nullptr))
            return false;
        if (!signal.empty() && !dbus_validate_member(signal.c_str(), nullptr))
            return false;

        if (target_unique_name.empty()) {
            auto owner = conn.get_name_owner (target);
            if (owner.err() != 0)
                return false;
            target_unique_name = owner;
        }

        const std::lock_guard lock (cb_mutex);

        auto key = std::make_pair (interface, signal);
        auto entry = callbacks.find (key);

        if (!callback) {
            // Remove the callback
            if (entry != callbacks.end()) {
                callbacks.erase (entry);
                remove_match (make_signal_match_rule(interface, signal));
            }
            return true;
        }

        // Set the callback

        if (entry != callbacks.end()) {
            entry->second = callback;
            return true;
        }

        callbacks.emplace (key, callback);
        if ( ! add_match(make_signal_match_rule(interface, signal))) {
            callbacks.erase (key);
            return false;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object_proxy::clear_signal_callbacks ()
    {
        const std::lock_guard lock (cb_mutex);
        for (const auto& i : callbacks) {
            const auto& iface = i.first.first;
            const auto& signal = i.first.second;
            remove_match (make_signal_match_rule(iface, signal));
        }
        callbacks.clear ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<dbus_variant> object_proxy::get_iface_property (
            const std::string& interface,
            const std::string& property_name,
            int timeout)
    {
        retvalue<dbus_variant> ret;
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret;
        }

        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "Get");
        msg.append_args (interface, property_name);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        else if (!reply.get_args(ret.get())) {
            std::string err_msg ("Invalid DBus reply argument, expected 'v', got '");
            err_msg.append (reply.signature());
            err_msg.push_back ('\'');
            ret.err (-1, err_msg);
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::get_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           std::function<void (retvalue<dbus_variant>& result)> callback,
                                           int timeout)
    {
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr))
            return false;

        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "Get");
        msg.append_args (interface, property_name);
        if (!callback) {
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (
                msg,
                [callback](message& reply){
                    retvalue<dbus_variant> ret;
                    if (reply.is_error()) {
                        ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                    }
                    else if (!reply.get_args(ret.get())) {
                        std::string err_msg ("Invalid DBus reply argument, expected 'v', got '");
                        err_msg.append (reply.signature());
                        err_msg.push_back ('\'');
                        ret.err (-1, err_msg);
                    }
                    callback (ret);
                },
                timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<dbus_string_dict> object_proxy::get_all_iface_properties (const std::string& interface,
                                                                       int timeout)
    {
        retvalue<dbus_string_dict> ret (DBUS_TYPE_VARIANT);
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret;
        }

        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "GetAll");
        msg.append_args (interface);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        else if (!reply.get_args_strict(ret.get())) {
            std::string err_msg ("Invalid DBus reply argument, expected 'a{sv}', got '");
            err_msg.append (reply.signature());
            err_msg.push_back ('\'');
            ret.err (-1, err_msg);
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::get_all_iface_properties (
            const std::string& interface,
            std::function<void (retvalue<dbus_string_dict>& result)> callback,
            int timeout)
    {
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr))
            return false;

        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "GetAll");
        msg.append_args (interface);
        if (!callback) {
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (
                msg,
                [callback](message& reply){
                    retvalue<dbus_string_dict> ret (DBUS_TYPE_VARIANT);
                    if (reply.is_error()) {
                        ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                    }
                    else if (!reply.get_args_strict(ret.get())) {
                        std::string err_msg ("Invalid DBus reply argument, expected 'a{sv}', got '");
                        err_msg.append (reply.signature());
                        err_msg.push_back ('\'');
                        ret.err (-1, err_msg);
                    }
                    callback (ret);
                },
                timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> object_proxy::set_iface_property (
            const std::string& interface,
            const std::string& property_name,
            const dbus_variant& value,
            int timeout)
    {
        retvalue<bool> ret (true);
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret=false;
        }
        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "Set");
        msg.append_args (interface, property_name, value);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            ret = false;
            ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::set_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           const dbus_variant& value,
                                           std::function<void (retvalue<bool>& result)> callback,
                                           int timeout)
    {
        retvalue<bool> ret (true);
        if (interface.empty() || !dbus_validate_interface(interface.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret=false;
        }
        message msg (target, opath, DBUS_INTERFACE_PROPERTIES, "Set");
        msg.append_args (interface, property_name, value);
        if (!callback) {
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (
                msg,
                [callback](message& reply){
                    retvalue<bool> ret (true);
                    if (reply.is_error()) {
                        ret = false;
                        ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                    }
                    callback (ret);
                },
                timeout);
    }


    namespace {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_object_iface_added_rule (const std::string& bus_name,
                                                  const std::string& opath,
                                                  const std::string& opath_arg)
        {
            std::string rule ("type='signal',sender='");
            rule.append (bus_name);
            rule.append ("',path='"); rule.append (opath);
            rule.append ("',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'");
            rule.append (",arg0path='"); rule.append (opath_arg); rule.push_back ('\'');
            return rule;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_object_iface_removed_rule (const std::string& bus_name,
                                                    const std::string& opath,
                                                    const std::string& opath_arg)
        {
            std::string rule ("type='signal',sender='");
            rule.append (bus_name);
            rule.append ("',path='"); rule.append (opath);
            rule.append ("',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved'");
            rule.append (",arg0path='"); rule.append (opath_arg); rule.push_back ('\'');
            return rule;
        }
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_props_changed_rule (const std::string& bus_name,
                                             const std::string& opath)
        {
            std::string rule ("type='signal'");
            rule.append(",sender='"); rule.append(bus_name);
            rule.append("',path='"); rule.append(opath);
            rule.append("',interface='" DBUS_INTERFACE_PROPERTIES "'");
            rule.append(",member='PropertiesChanged'");
            return rule;
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::set_interfaces_added_callback (object_ifaces_added_cb_t callback,
                                                      const std::string& om_root_path)
    {
        const std::lock_guard lock (cb_mutex);

        if (target_unique_name.empty()) {
            auto owner = conn.get_name_owner (target);
            if (owner.err() != 0)
                return false;
            target_unique_name = owner;
        }

        bool ok = true;
        if (callback) {
            // Set the callback
            if (object_ifaces_added_cb == nullptr) {
                ok = add_match (make_object_iface_added_rule(target_unique_name, om_root_path, opath));
            }else{
                if (om_root_path != iface_add_om_root) {
                    ok = add_match (make_object_iface_added_rule(target_unique_name, om_root_path, opath));
                    if (ok) {
                        // Remove old rule
                        remove_match (make_object_iface_added_rule(target_unique_name,
                                                                   iface_add_om_root,
                                                                   opath));
                    }
                }
            }
            if (ok)
                iface_add_om_root = om_root_path;
        }else{
            // Remove the callback
            if (object_ifaces_added_cb != nullptr)
                remove_match (make_object_iface_added_rule(target_unique_name, iface_add_om_root, opath));
            iface_add_om_root.clear ();
        }
        if (ok)
            object_ifaces_added_cb = callback;

        return ok;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::set_interfaces_removed_callback (object_ifaces_removed_cb_t callback,
                                                        const std::string& om_root_path)
    {
        const std::lock_guard lock (cb_mutex);

        if (target_unique_name.empty()) {
            auto owner = conn.get_name_owner (target);
            if (owner.err() != 0)
                return false;
            target_unique_name = owner;
        }

        bool ok = true;
        if (callback) {
            // Set the callback
            if (object_ifaces_removed_cb == nullptr) {
                ok = add_match (make_object_iface_removed_rule(target_unique_name, om_root_path, opath));
            }else{
                if (om_root_path != iface_del_om_root) {
                    ok = add_match (make_object_iface_removed_rule(target_unique_name, om_root_path, opath));
                    if (ok) {
                        // Remove old rule
                        remove_match (make_object_iface_removed_rule(target_unique_name,
                                                                     iface_del_om_root,
                                                                     opath));
                    }
                }
            }
            if (ok)
                iface_del_om_root = om_root_path;
        }else{
            // Remove the callback
            if (object_ifaces_removed_cb != nullptr)
                remove_match (make_object_iface_removed_rule(target_unique_name, iface_del_om_root, opath));
            iface_del_om_root.clear ();
        }
        if (ok)
            object_ifaces_removed_cb = callback;

        return ok;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::set_properties_changed_callback (properties_changed_cb_t callback)
    {
        const std::lock_guard lock (cb_mutex);

        if (target_unique_name.empty()) {
            auto owner = conn.get_name_owner (target);
            if (owner.err() != 0)
                return false;
            target_unique_name = owner;
        }

        bool ok = true;
        if (callback) {
            // Set the callback
            if (properties_changed_cb == nullptr)
                ok = add_match (make_props_changed_rule(target_unique_name, opath));
        }else{
            // Remove the callback
            if (properties_changed_cb != nullptr)
                remove_match (make_props_changed_rule(target_unique_name, opath));
        }
        if (ok)
            properties_changed_cb = callback;

        return ok;
    }


    //--------------------------------------------------------------------------
    // cb_mutex is locked !!!
    //--------------------------------------------------------------------------
    bool object_proxy::on_iface_added (message& msg)
    {
        // message signature: "oa{sa{sv}}"
        dbus_opath opath_arg;
        dbus_string_dict arg ("a{sv}");
        if (!msg.get_args_strict(opath_arg, arg))
            return false;
        if (opath_arg != opath)
            return false;

        std::map<std::string, dbus_string_dict> ifaces_props;
        for (auto& i : arg)
            ifaces_props.emplace (i.first.get(), std::move(i.second.cast<dbus_string_dict>()));

        auto cb = object_ifaces_added_cb;
        cb_mutex.unlock ();
        try {
            cb (ifaces_props);
        }catch (...) {
            cb_mutex.lock ();
            throw;
        }
        cb_mutex.lock ();
        return true;
    }


    //--------------------------------------------------------------------------
    // cb_mutex is locked !!!
    //--------------------------------------------------------------------------
    bool object_proxy::on_iface_removed (message& msg)
    {
        // message signature: "oas"
        dbus_opath opath_arg;
        dbus_array arg ("s");
        if (!msg.get_args_strict(opath_arg, arg))
            return false;
        if (opath_arg != opath)
            return false;

        std::set<std::string> removed_ifaces;
        for (auto& i : arg)
            removed_ifaces.emplace (i.to_string());

        auto cb = object_ifaces_removed_cb;
        cb_mutex.unlock ();
        try {
            cb (removed_ifaces);
        }catch (...) {
            cb_mutex.lock ();
            throw;
        }
        cb_mutex.lock ();
        return true;
    }


    //--------------------------------------------------------------------------
    // cb_mutex is locked !!!
    //--------------------------------------------------------------------------
    bool object_proxy::on_properties_changed (message& msg)
    {
        std::string iface_name;
        dbus_string_dict dbus_changed_props (DBUS_TYPE_VARIANT);
        dbus_array dbus_invalidated_props (DBUS_TYPE_STRING);
        if (!msg.get_args_strict(iface_name, dbus_changed_props, dbus_invalidated_props))
            return false; // Invalid signal parameters

        std::set<std::string> invalidated_properties;
        for (auto& prop_name : dbus_invalidated_props)
            invalidated_properties.emplace (prop_name.to_string());

        auto cb = properties_changed_cb;
        cb_mutex.unlock ();
        try {
            cb (iface_name, dbus_changed_props, invalidated_properties);
        }catch (...) {
            cb_mutex.lock ();
            throw;
        }
        cb_mutex.lock ();
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object_proxy::on_signal (message& msg)
    {
        if (msg.sender() != target_unique_name)
            return false;

        const std::string wildcard;
        const std::string iface = msg.interface ();
        const std::string signal = msg.name ();

        const std::lock_guard lock (cb_mutex);

        if (object_ifaces_added_cb &&
            iface==dbus_iface_object_manager &&
            signal=="InterfacesAdded" &&
            msg.path() == iface_add_om_root)
        {
            if (on_iface_added(msg))
                return true;
        }
        else if (object_ifaces_removed_cb &&
                 iface==dbus_iface_object_manager &&
                 signal=="InterfacesRemoved" &&
                 msg.path() == iface_del_om_root)
        {
            if (on_iface_removed(msg))
                return true;
        }
        else if (msg.path() != opath) {
            return false;
        }
        else if (properties_changed_cb && iface==DBUS_INTERFACE_PROPERTIES && signal=="PropertiesChanged") {
            if (on_properties_changed(msg))
                return true;
        }

        // Find callback mapped to a specific interface and a specific signal name
        auto entry = callbacks.find (std::make_pair(iface, signal));
        if (entry == callbacks.end()) {
            // Find callback mapped to any interface and a specific signal name
            entry = callbacks.find (std::make_pair(wildcard, signal));
            if (entry == callbacks.end()) {
                // Find callback mapped to a specific interface and any signal name
                entry = callbacks.find (std::make_pair(iface, wildcard));
                if (entry == callbacks.end()) {
                    // Find callback mapped to any interface and any signal name
                    entry = callbacks.find (std::make_pair(wildcard, wildcard));
                }
            }
        }

        bool retval = false;
        if (entry != callbacks.end()) {
            auto cb = entry->second;
            cb_mutex.unlock ();
            try {
                retval = cb (msg);
            }catch (...) {
                cb_mutex.lock ();
                throw;
            }
            cb_mutex.lock ();
        }
        return retval;
    }


}
