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
#include <ultrabus/org_freedesktop_DBus_ObjectManager.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_array.hpp>
#include <dbus/dbus.h>


namespace ultrabus {

    static constexpr const char* dbus_iface_object_manager = "org.freedesktop.DBus.ObjectManager";


    namespace {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        void handle_get_managed_objects_reply (message& reply,
                                               retvalue<managed_objects_t>& ret)
        {
            if (reply.is_error()) {
                ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                return;
            }

            dbus_opath_dict all ("a{sa{sv}}");
            if (!reply.get_args_strict(all)) {
                std::string err_msg ("Invalid DBus reply argument, expected 'a{oa{sa{sv}}}', got '");
                err_msg.append (reply.signature());
                err_msg.push_back ('\'');
                ret.err (-1, err_msg);
                return;
            }

            auto& mo = ret.get ();
            for (auto& op : all) {
                std::map<std::string, dbus_string_dict> ifaces_properties;
                const auto& opath = op.first.get ();
                auto& ifaces = op.second.cast<dbus_string_dict> ();
                for (auto& i : ifaces) {
                    const auto& iface = i.first.get ();
                    auto& properties = i.second.cast<dbus_string_dict> ();
                    ifaces_properties.emplace (iface, std::move(properties));
                }
                mo.emplace (opath, std::move(ifaces_properties));
            }
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<managed_objects_t> org_freedesktop_DBus_ObjectManager::get_managed_objects (
            const std::string& bus_name,
            const std::string& opath,
            int timeout)
    {
        retvalue<managed_objects_t> ret;
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid bus name");
            return ret;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            ret.err (-1, "Invalid object path");
            return ret;
        }

        message msg (bus_name, opath, dbus_iface_object_manager, "GetManagedObjects");
        auto reply = conn.send_and_wait (msg, timeout);
        handle_get_managed_objects_reply (reply, ret);
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_ObjectManager::get_managed_objects (
            const std::string& bus_name,
            const std::string& opath,
            std::function<void (retvalue<managed_objects_t>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr))
            return false;
        if (!dbus_validate_path(opath.c_str(), nullptr))
            return false;

        message msg (bus_name, opath, dbus_iface_object_manager, "GetManagedObjects");
        if (!callback) {
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (
                msg,
                [callback](message& reply){
                    retvalue<managed_objects_t> ret;
                    handle_get_managed_objects_reply (reply, ret);
                    callback (ret);
                },
                timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_ObjectManager::set_interfaces_added_callback (
            const std::string& bus_name,
            const std::string& opath,
            iface_added_cb callback)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<bool> ret (false);
            ret.err (-1, "Invalid bus name");
            return ret;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            retvalue<bool> ret (false);
            ret.err (-1, "Invalid object path");
            return ret;
        }

        const bool is_bus_name_unique = bus_name[0] == ':';
        if (is_bus_name_unique) {
            return set_interfaces_added_callback_impl (bus_name, opath, callback);
        }else{
            const auto unique_bus_name = conn.get_name_owner (bus_name);
            if (unique_bus_name.err() != 0) {
                retvalue<bool> ret (false);
                ret.err (-1, unique_bus_name.what());
                return ret;
            }
            return set_interfaces_added_callback_impl (unique_bus_name, opath, callback);
        }
    }


    namespace {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_iface_added_rule (const std::string& bus_name,
                                           const std::string& opath)
        {
            std::string rule ("type='signal',sender='");
            rule.append (bus_name);
            rule.append ("',path='"); rule.append (opath);
            rule.append ("',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'");
            return rule;
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_ObjectManager::set_interfaces_added_callback_impl (
            const std::string& bus_name,
            const std::string& opath,
            iface_added_cb callback)
    {
        const std::lock_guard lock (iface_mutex);
        retvalue<bool> ret (true);
        auto key = std::make_pair (bus_name, opath);
        auto entry = iface_added_callbacks.find (key);
        if (callback) {
            if (entry == iface_added_callbacks.end()) {
                // Add callback
                auto entry = iface_added_callbacks.emplace(key, callback).first;
                ret = add_match (make_iface_added_rule(bus_name, opath));
                if ( ! ret)
                    iface_added_callbacks.erase (entry);
            }else{
                // Replace callback
                entry->second = callback;
            }
        }else{
            if (entry != iface_added_callbacks.end()) {
                // Remove callback
                iface_added_callbacks.erase (entry);
                remove_match (make_iface_added_rule(bus_name, opath));
            }
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_ObjectManager::set_interfaces_removed_callback (
            const std::string& bus_name,
            const std::string& opath,
            iface_removed_cb callback)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            retvalue<bool> ret (false);
            ret.err (-1, "Invalid bus name");
            return ret;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            retvalue<bool> ret (false);
            ret.err (-1, "Invalid object path");
            return ret;
        }

        const bool is_bus_name_unique = bus_name[0] == ':';
        if (is_bus_name_unique) {
            return set_interfaces_removed_callback_impl (bus_name, opath, callback);
        }else{
            const auto unique_bus_name = conn.get_name_owner (bus_name);
            if (unique_bus_name.err() != 0) {
                retvalue<bool> ret (false);
                ret.err (-1, unique_bus_name.what());
                return ret;
            }
            return set_interfaces_removed_callback_impl (unique_bus_name, opath, callback);
        }
    }


    namespace {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_iface_removed_rule (const std::string& bus_name,
                                             const std::string& opath)
        {
            std::string rule ("type='signal',sender='");
            rule.append (bus_name);
            rule.append ("',path='");
            rule.append (opath);
            rule.append ("',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved'");
            return rule;
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_ObjectManager::set_interfaces_removed_callback_impl (
            const std::string& bus_name,
            const std::string& opath,
            iface_removed_cb callback)
    {
        const std::lock_guard lock (iface_mutex);
        retvalue<bool> ret (true);
        auto key = std::make_pair (bus_name, opath);
        auto entry = iface_removed_callbacks.find (key);
        if (callback) {
            if (entry == iface_removed_callbacks.end()) {
                // Add callback
                auto entry = iface_removed_callbacks.emplace(key, callback).first;
                ret = add_match (make_iface_removed_rule(bus_name, opath));
                if ( ! ret)
                    iface_removed_callbacks.erase (entry);
            }else{
                // Replace callback
                entry->second = callback;
            }
        }else{
            if (entry != iface_removed_callbacks.end()) {
                // Remove callback
                iface_removed_callbacks.erase (entry);
                remove_match (make_iface_removed_rule(bus_name, opath));
            }
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_ObjectManager::on_signal (message& msg)
    {
        if (msg.interface() != dbus_iface_object_manager)
            return false;

        if (msg.name() == "InterfacesAdded") {
            const std::lock_guard lock (iface_mutex);
            auto key = std::make_pair (msg.sender(), msg.path());
            auto entry = iface_added_callbacks.find (key);
            if (entry != iface_added_callbacks.end()) {
                auto cb = entry->second;
                handle_added_ifaces (msg, cb);
            }
        }
        else if (msg.name() == "InterfacesRemoved") {
            const std::lock_guard lock (iface_mutex);
            auto key = std::make_pair (msg.sender(), msg.path());
            auto entry = iface_removed_callbacks.find (key);
            if (entry != iface_removed_callbacks.end()) {
                auto cb = entry->second;
                handle_removed_ifaces (msg, cb);
            }
        }
        return false;
    }


    //--------------------------------------------------------------------------
    // iface_mutex is locked
    //--------------------------------------------------------------------------
    void org_freedesktop_DBus_ObjectManager::handle_added_ifaces (
            message& msg,
            iface_added_cb cb)
    {
        // message signature: "oa{sa{sv}}"
        dbus_opath opath;
        dbus_string_dict arg ("a{sv}");
        if (!msg.get_args_strict(opath, arg)) {
            return; // This will probably never happen
        }

        std::map<std::string, dbus_string_dict> ifaces_props;
        for (auto& i : arg)
            ifaces_props.emplace (i.first.get(), std::move(i.second.cast<dbus_string_dict>()));

        iface_mutex.unlock ();
        try {
            cb (opath.get(), ifaces_props);
        }
        catch (...) {
            iface_mutex.lock ();
            throw;
        }
        iface_mutex.lock ();
    }


    //--------------------------------------------------------------------------
    // iface_mutex is locked
    //--------------------------------------------------------------------------
    void org_freedesktop_DBus_ObjectManager::handle_removed_ifaces (
            message& msg,
            iface_removed_cb cb)
    {
        // message signature: "oas"
        dbus_opath opath;
        dbus_array arg ("s");
        if (!msg.get_args_strict(opath, arg)) {
            return; // This will probably never happen
        }
        std::set<std::string> ifaces;
        for (auto& i : arg)
            ifaces.emplace (i.to_string());

        iface_mutex.unlock ();
        try {
            cb (opath.get(), ifaces);
        }
        catch (...) {
            iface_mutex.lock ();
            throw;
        }
        iface_mutex.lock ();
    }


}
