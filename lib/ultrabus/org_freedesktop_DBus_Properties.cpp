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
#include <ultrabus/org_freedesktop_DBus_Properties.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/message.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<dbus_variant> org_freedesktop_DBus_Properties::get (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            const std::string& property_name,
            int timeout)
    {
        retvalue<dbus_variant> ret;
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid bus name");
            return ret;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            ret.err (-1, "Invalid object path");
            return ret;
        }
        if (!dbus_validate_interface(interface_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "Get");
        msg.append_args (interface_name, property_name);
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
    bool org_freedesktop_DBus_Properties::get (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            const std::string& property_name,
            std::function<void (retvalue<dbus_variant>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr) ||
            !dbus_validate_path(opath.c_str(), nullptr) ||
            !dbus_validate_interface(interface_name.c_str(), nullptr))
        {
            return false;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "Get");
        msg.append_args (interface_name, property_name);
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
    retvalue<dbus_string_dict> org_freedesktop_DBus_Properties::get_all (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            int timeout)
    {
        retvalue<dbus_string_dict> ret (DBUS_TYPE_VARIANT);
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid bus name");
            return ret;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            ret.err (-1, "Invalid object path");
            return ret;
        }
        if (!dbus_validate_interface(interface_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "GetAll");
        msg.append_args (interface_name);
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
    bool org_freedesktop_DBus_Properties::get_all (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            std::function<void (retvalue<dbus_string_dict>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr) ||
            !dbus_validate_path(opath.c_str(), nullptr) ||
            !dbus_validate_interface(interface_name.c_str(), nullptr))
        {
            return false;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "GetAll");
        msg.append_args (interface_name);
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
    retvalue<bool> org_freedesktop_DBus_Properties::set (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            const std::string& property_name,
            const dbus_variant& value,
            int timeout)
    {
        retvalue<bool> ret (true);
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid bus name");
            return ret=false;
        }
        if (!dbus_validate_path(opath.c_str(), nullptr)) {
            ret.err (-1, "Invalid object path");
            return ret=false;
        }
        if (!dbus_validate_interface(interface_name.c_str(), nullptr)) {
            ret.err (-1, "Invalid interface name");
            return ret=false;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "Set");
        msg.append_args (interface_name, property_name, value);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            ret = false;
            ret.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_Properties::set (
            const std::string& bus_name,
            const std::string& opath,
            const std::string& interface_name,
            const std::string& property_name,
            const dbus_variant& value,
            std::function<void (retvalue<bool>& result)> callback,
            int timeout)
    {
        if (!dbus_validate_bus_name(bus_name.c_str(), nullptr) ||
            !dbus_validate_path(opath.c_str(), nullptr) ||
            !dbus_validate_interface(interface_name.c_str(), nullptr))
        {
            return false;
        }

        message msg (bus_name, opath, DBUS_INTERFACE_PROPERTIES, "Set");
        msg.append_args (interface_name, property_name, value);
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


    // //--------------------------------------------------------------------------
    // //--------------------------------------------------------------------------
    // retvalue<dbus_variant> org_freedesktop_DBus_Properties::wait_for_property (
    //         const std::string& bus_name,
    //         const std::string& opath,
    //         const std::string& interface_name,
    //         const std::string& property_name,
    //         int timeout)
    // {
    //     retvalue<dbus_variant> value;
    //     auto cb_ret = set_properties_changed_cb (bus_name,
    //                                              opath,
    //                                              interface_name,
    //                                              property_name
    // }


    namespace {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::string make_props_changed_rule (const std::string& bus_name,
                                             const std::string& opath)
        {
            std::string rule ("type='signal'");
            rule.append(",sender='"); rule.append(bus_name); rule.push_back('\'');
            rule.append(",path='"); rule.append(opath); rule.push_back('\'');
            rule.append(",interface='" DBUS_INTERFACE_PROPERTIES "'");
            rule.append(",member='PropertiesChanged'");
            return rule;
        }
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_Properties::set_properties_changed_callback (
            const std::string& bus_name,
            const std::string& opath,
            properties_changed_cb_t callback)
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
            return set_properties_changed_callback_impl (bus_name, opath, callback);
        }else{
            auto unique_bus_name = conn.get_name_owner (bus_name);
            if (unique_bus_name.err() != 0) {
                retvalue<bool> ret (false);
                ret.err (-1, unique_bus_name.what());
                return ret;
            }
            return set_properties_changed_callback_impl (unique_bus_name, opath, callback);
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> org_freedesktop_DBus_Properties::set_properties_changed_callback_impl (
            const std::string& bus_name,
            const std::string& opath,
            properties_changed_cb_t callback)
    {
        const std::lock_guard lock (props_changed_mutex);
        retvalue<bool> ret (true);
        auto key = std::make_pair (bus_name, opath);
        auto entry = props_changed_callbacks.find (key);
        if (callback) {
            if (entry == props_changed_callbacks.end()) {
                // Add callback
                auto entry = props_changed_callbacks.emplace(key, callback).first;
                ret = add_match (make_props_changed_rule(bus_name, opath));
                if ( ! ret)
                    props_changed_callbacks.erase (entry);
            }else{
                // Replace callback
                entry->second = callback;
            }
        }else{
            if (entry != props_changed_callbacks.end()) {
                // Remove callback
                props_changed_callbacks.erase (entry);
                remove_match (make_props_changed_rule(bus_name, opath));
            }
        }
        return ret;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool org_freedesktop_DBus_Properties::on_signal (message& msg)
    {
        if (msg.interface() != DBUS_INTERFACE_PROPERTIES ||
            msg.name() != "PropertiesChanged")
        {
            return false;
        }

        std::string iface_name;
        dbus_string_dict dbus_changed_props (DBUS_TYPE_VARIANT);
        dbus_array dbus_invalidated_props (DBUS_TYPE_STRING);
        if (!msg.get_args_strict(iface_name, dbus_changed_props, dbus_invalidated_props))
            return false; // Invalid signal parameters

        std::set<std::string> invalidated_properties;
        for (auto& prop_name : dbus_invalidated_props)
            invalidated_properties.emplace (prop_name.to_string());


        const std::lock_guard lock (props_changed_mutex);
        const auto key = std::make_pair (msg.sender(), msg.path());
        auto entry = props_changed_callbacks.find (key);
        if (entry == props_changed_callbacks.end() || entry->second==nullptr)
            return false;
        auto cb = entry->second;
        props_changed_mutex.unlock ();
        try {
            cb (iface_name, dbus_changed_props, invalidated_properties);
        }
        catch (...) {
            props_changed_mutex.lock ();
            throw;
        }
        props_changed_mutex.lock ();

        return false;
    }


}
