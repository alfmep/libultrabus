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
#include <ultrabus/object.hpp>
#include <ultrabus/interface.hpp>
#include <ultrabus/dbus_array.hpp>
#include <cstring>


namespace {

    // XML header for introspect data
    //
    static constexpr const char* xml_introspect_header =
        R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN")" "\n"
        R"( "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">)" "\n";

    // XML info for standard interface org.freedesktop.DBus.Introspectable
    //
    static constexpr const char* xml_introspectable_interface =
        R"(<interface name="org.freedesktop.DBus.Introspectable">)"
        R"(<method name="Introspect">)"
        R"(<arg name="xml" type="s" direction="out"/>)"
        R"(</method>)"
        R"(</interface>)";

    // XML info for standard interface org.freedesktop.DBus.Properties
    //
    static constexpr const char* xml_properties_interface =
        R"(<interface name="org.freedesktop.DBus.Properties">)"
        R"(<method name="Get">)"
        R"(<arg name="interface" type="s" direction="in"/>)"
        R"(<arg name="name" type="s" direction="in"/>)"
        R"(<arg name="value" type="v" direction="out"/>)"
        R"(</method>)"
        R"(<method name="Set">)"
        R"(<arg name="interface" type="s" direction="in"/>)"
        R"(<arg name="name" type="s" direction="in"/>)"
        R"(<arg name="value" type="v" direction="in"/>)"
        R"(</method>)"
        R"(<method name="GetAll">)"
        R"(<arg name="interface" type="s" direction="in"/>)"
        R"(<arg name="properties" type="a{sv}" direction="out"/>)"
        R"(</method>)"
        R"(<signal name="PropertiesChanged">)"
        R"(<arg name="interface" type="s"/>)"
        R"(<arg name="changed_properties" type="a{sv}"/>)"
        R"(<arg name="invalidated_properties" type="as"/>)"
        R"(</signal>)"
        R"(</interface>)";

    // XML info for standard interface org.freedesktop.DBus.ObjectManager
    //
    static constexpr const char* xml_objectmanager_interface =
        R"(<interface name="org.freedesktop.DBus.ObjectManager">)"
        R"(<method name="GetManagedObjects">)"
        R"(<arg name="objects" type="a{oa{sa{sv}}}" direction="out"/>)"
        R"(</method>)"
        R"(<signal name="InterfacesAdded">)"
        R"(<arg name="object" type="o"/>)"
        R"(<arg name="interfaces" type="a{sa{sv}}"/>)"
        R"(</signal>)"
        R"(<signal name="InterfacesRemoved">)"
        R"(<arg name="object" type="o"/>)"
        R"(<arg name="interfaces" type="as"/>)"
        R"(</signal>)"
        R"(</interface>)";


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string get_relative_path (const std::string& base, const std::string& path)
    {
        if (path[0] != '/')
            return path; // Path is already a relative path

        // path is absolute (starting with '/')

        if (base == "/") {
            std::string rel_path = path.substr (1);
            if (rel_path[0] == '/')
                return "";
            else
                return rel_path;
        }
        if (strncmp(base.c_str(), path.c_str(), base.size()) != 0)
            return "";
        if (path[base.size()] != '/')
            return "";
        return path.substr (base.size()+1);
    }

    //--------------------------------------------------------------------------
    // Search upwards in the object tree for an object implementing
    // interface org.freedesktop.DBus.ObjectManager
    //--------------------------------------------------------------------------
    ultrabus::object* find_object_with_interface_objectmanager (ultrabus::object* obj)
    {
        while (obj && (!obj->is_enabled() || !obj->has_interface_objectmanager()))
            obj = obj->parent();
        return obj;
    }


} // Anonymous namespace


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object::object (connection& conn,
                    const dbus_opath& object_path,
                    const bool enable_interface_objectmanager,
                    const bool enable_interface_introspectable)
        : object_handler(conn),
          opath (object_path),
          parent_obj (nullptr),
          implements_properties {false},
          implements_objectmanager {enable_interface_objectmanager},
          implements_introspectable {enable_interface_introspectable},
          enabled {false}
    {
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object::~object ()
    {
        disable ();
        for (auto& entry : nodes)
            entry.second->parent_obj = nullptr;
        nodes.clear ();
        interfaces.clear ();
    }


    //--------------------------------------------------------------------------
    // interfaces_mutex and node_mutex are locked !
    //--------------------------------------------------------------------------
    void object::emit_all_interfaces_added ()
    {
        object* obj_with_om = find_object_with_interface_objectmanager (parent_obj);
        if (!obj_with_om)
            return;

        dbus_string_dict added_ifaces ("a{sv}");
        if (implements_introspectable)
            added_ifaces.set ("org.freedesktop.DBus.Introspectable", dbus_string_dict("v"));
        if (implements_properties)
            added_ifaces.set ("org.freedesktop.DBus.Properties", dbus_string_dict("v"));
        if (implements_objectmanager)
            added_ifaces.set ("org.freedesktop.DBus.ObjectManager", dbus_string_dict("v"));

        for (auto& if_entry : interfaces) {
            if (implements_properties)
                added_ifaces.set (if_entry.first, if_entry.second->get_all_properties());
            else
                added_ifaces.set (if_entry.first, dbus_string_dict("v"));
        }

        message signal (obj_with_om->opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (opath, added_ifaces);
        conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::enable (bool enable_sub_nodes)
    {
        if ( ! conn.is_connected()) {
            disable ();
            return false;
        }

        std::scoped_lock lock (interfaces_mutex, node_mutex);

        if ( ! enabled) {
            enabled = register_object_path (opath.get(), nullptr);
            if ( ! enabled)
                return false;
            emit_all_interfaces_added ();
        }

        bool retval = true;
        if (enable_sub_nodes) {
            for (auto& node : nodes) {
                if ( ! node.second->enable(true))
                    retval = false;
            }
        }
        return retval;
    }


    //--------------------------------------------------------------------------
    // interfaces_mutex and node_mutex are locked !
    //--------------------------------------------------------------------------
    void object::emit_all_interfaces_removed ()
    {
        object* obj_with_om = find_object_with_interface_objectmanager (parent_obj);
        if (!obj_with_om)
            return;

        dbus_array removed_ifaces ("s");
        if (has_interface_introspectable())
            removed_ifaces.push_back ("org.freedesktop.DBus.Introspectable");
        if (has_interface_properties())
            removed_ifaces.push_back ("org.freedesktop.DBus.Properties");
        if (has_interface_objectmanager())
            removed_ifaces.push_back ("org.freedesktop.DBus.ObjectManager");

        for (const auto& if_entry : interfaces)
            removed_ifaces.push_back (if_entry.first);

        message signal (obj_with_om->opath, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved");
        signal.append_args (opath, removed_ifaces);
        conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::disable ()
    {
        std::scoped_lock lock (interfaces_mutex, node_mutex);

        for (auto& node : nodes)
            node.second->disable ();

        if ( !enabled || !conn.is_connected()) {
            enabled = false;
            return;
        }
        unregister_object_path (opath.get());
        enabled = false;
        emit_all_interfaces_removed ();
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<ultrabus::object> object::create_node (
            const std::string& node_path,
            const bool enable_interface_objectmanager,
            const bool enable_interface_introspectable)
    {
        std::lock_guard lock (node_mutex);

        std::string relative_path = get_relative_path (opath, node_path);
        if (relative_path.empty()) {
            return nullptr;
        }
        std::string absolute_path (opath);
        if (opath.get().back() != '/')
            absolute_path.push_back ('/');
        absolute_path.append (relative_path);

        if (dbus_validate_path(absolute_path.c_str(), nullptr) != TRUE) {
            return nullptr;
        }

        std::shared_ptr<object> node (new object(conn,
                                                 absolute_path,
                                                 enable_interface_objectmanager,
                                                 enable_interface_introspectable));
        if (node) {
            node->parent_obj = this;
            nodes.emplace (relative_path, node);
        }
        return node;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::remove_node (const std::string& node_path)
    {
        std::lock_guard lock (node_mutex);

        std::string relative_path = get_relative_path (opath, node_path);
        if (relative_path.empty())
            return false;

        auto i = nodes.find (relative_path);
        if (i == nodes.end())
            return false;

        object& node = *i->second;
        node.disable ();
        node.parent_obj = nullptr;

        nodes.erase (i);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::emit_interface_added (interface* iface)
    {
        object* obj_with_om = find_object_with_interface_objectmanager (this);
        if (!obj_with_om)
            return;

        // Emit signal
        dbus_string_dict added_ifaces ("a{sv}");
        if (implements_properties)
            added_ifaces.set (iface->name(), iface->get_all_properties());
        else
            added_ifaces.set (iface->name(), dbus_string_dict("v"));
        message signal (obj_with_om->opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (opath, std::move(added_ifaces));
        conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::emit_interface_added (const std::string& interface_name)
    {
        object* obj_with_om = find_object_with_interface_objectmanager (this);
        if (!obj_with_om)
            return;

        // Emit signal
        dbus_string_dict added_ifaces ("a{sv}");
        added_ifaces.set (interface_name, dbus_string_dict("v"));

        message signal (obj_with_om->opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (opath, std::move(added_ifaces));
        conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::add_interface (std::shared_ptr<interface> iface)
    {
        std::lock_guard lock (interfaces_mutex);

        if (!iface || interfaces.contains(iface->name()) || iface->owner()!=nullptr)
            return false;

        iface->set_owner (this);
        interfaces.emplace (iface->name(), iface);

        if (iface->has_properties())
            enable_interface_properties_impl ();

        if (enabled && conn.is_connected())
            emit_interface_added (iface.get());

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::emit_interface_removed (const std::string& iface_name)
    {
        object* obj_with_om = find_object_with_interface_objectmanager (this);
        if (!obj_with_om)
            return;

        // Emit signal
        dbus_array removed_ifaces ("s");
        removed_ifaces.push_back (iface_name);
        message signal (obj_with_om->opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (opath, removed_ifaces);
        conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::remove_interface (const std::string& interface_name)
    {
        std::lock_guard lock (interfaces_mutex);
        auto entry = interfaces.find (interface_name);
        if (entry == interfaces.end())
            return false;

        entry->second->set_owner (nullptr);
        interfaces.erase (entry);

        if (enabled && conn.is_connected())
            emit_interface_removed (interface_name);

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::enable_interface_properties ()
    {
        std::lock_guard lock (interfaces_mutex);
        enable_interface_properties_impl ();
    }


    //--------------------------------------------------------------------------
    // interfaces_mutex is locked
    //--------------------------------------------------------------------------
    void object::enable_interface_properties_impl ()
    {
        if ( ! implements_properties) {
            implements_properties = true;
            if (enabled && conn.is_connected())
                emit_interface_added ("org.freedesktop.DBus.Properties");
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_method_call_without_interface_in_msg (message& msg)
    {
        static std::set<std::string> if_properties_method_names {{"Get", "GetAll", "Set"}};
        bool handled {false};

        // Check standard interfaces
        if (implements_introspectable  &&  msg.name() == "Introspect")
            handled = on_introspect_method (msg);
        else if (implements_properties  &&  if_properties_method_names.contains(msg.name()))
            handled = on_properties_method (msg);
        else if (implements_objectmanager  &&  msg.name() == "GetManagedObjects")
            handled = on_objectmanager_method (msg);

        // Check registered interfaces
        if (!handled) {
            std::unique_lock lock (interfaces_mutex);
            for (auto& entry : interfaces) {
                if (entry.second->has_method(msg.name())) {
                    auto iface = entry.second;
                    lock.unlock ();
                    handled = iface->on_method (msg);
                    break;
                }
            }
        }
        return handled;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_method_call (message& msg)
    {
        bool handled {false};

        if (msg.interface().empty()) {
            // No interface specified in the method call
            return on_method_call_without_interface_in_msg (msg);
        }

        // Check registered interfaces
        {
            std::unique_lock lock (interfaces_mutex);
            auto entry = interfaces.find (msg.interface());
            if (entry != interfaces.end()) {
                auto iface = entry->second;
                lock.unlock ();
                handled = iface->on_method (msg);
            }
        }

        if (!handled) {
            // Check standard interfaces
            if (implements_introspectable  &&  msg.interface() == "org.freedesktop.DBus.Introspectable")
                handled = on_introspect_method (msg);
            else if (implements_properties  &&  msg.interface() == "org.freedesktop.DBus.Properties")
                handled = on_properties_method (msg);
            else if (implements_objectmanager  &&  msg.interface() == "org.freedesktop.DBus.ObjectManager")
                handled = on_objectmanager_method (msg);
        }
        return handled;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_introspect_method (message& msg)
    {
        if ( !implements_introspectable  ||  msg.name() != "Introspect")
            return false;

        std::string xml (xml_introspect_header);
        xml.append ("<node>");

        // Add standard interfaces info
        xml.append (xml_introspectable_interface);
        if (implements_properties)
            xml.append (xml_properties_interface);
        if (implements_objectmanager)
            xml.append (xml_objectmanager_interface);

        // Add interfaces info
        {
            std::lock_guard lock (interfaces_mutex);
            for (const auto& entry : interfaces)
                xml.append (entry.second->to_xml());
        }

        // Add nodes info
        std::set<std::string> node_names;
        {
            std::lock_guard lock (node_mutex);
            for (const auto& node : nodes) {
                if ( ! node.second->is_enabled())
                    continue;
                auto& full_name = node.first;
                node_names.emplace (full_name.substr(0, full_name.find_first_of('/')));
            }
        }
        for (const auto& node_name : node_names) {
            xml.append (R"(<node name=")");
            xml.append (node_name);
            xml.append (R"("/>)");
        }

        xml.append ("</node>");
        message reply (msg, false);
        reply.append_args (xml);
        conn.send (reply);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_properties_get_set_method (message& msg, bool get_value)
    {
        dbus_string iface_name;
        dbus_string prop_name;
        dbus_variant value;
        message reply;

        if (get_value) {
            if ( ! msg.get_args_strict(iface_name, prop_name))
                return false;
        }else{
            if ( ! msg.get_args_strict(iface_name, prop_name, value))
                return false;
        }

        std::lock_guard lock (interfaces_mutex);
        auto entry = interfaces.find (iface_name);
        if (entry == interfaces.end()) {
            reply = msg.make_reply (DBUS_ERROR_FAILED, "Get/Set property error - no such interface");
            conn.send (reply);
            return true;
        }

        int result {};
        if (get_value)
            result = entry->second->get_property (prop_name, value);
        else
            result = entry->second->set_property (prop_name, value);

        switch (result) {
        case 0:
            reply = msg.make_reply ();
            if (get_value)
                reply.append_args (value);
            break;
        case -1:
            reply = msg.make_reply (DBUS_ERROR_UNKNOWN_PROPERTY, "No such property");
            break;
        case -2:
            if (get_value)
                reply = msg.make_reply ("org.freedesktop.DBus.Error.PropertyWriteOnly", "Property is write-only");
            else
                reply = msg.make_reply (DBUS_ERROR_PROPERTY_READ_ONLY, "Property is read-only");
            break;
        case -3:
            if (get_value)
                reply = msg.make_reply (DBUS_ERROR_FAILED, "Getting property value of wrong type");
            else
                reply = msg.make_reply (DBUS_ERROR_FAILED, "Setting property value of wrong type");
            break;
        case -4:
        default:
            if (get_value)
                reply = msg.make_reply (DBUS_ERROR_FAILED, "Error getting property value");
            else
                reply = msg.make_reply (DBUS_ERROR_FAILED, "Error setting property value");
            break;
        }

        conn.send (reply);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_properties_get_all_method (message& msg)
    {
        dbus_string iface;
        if ( ! msg.get_args_strict(iface))
            return false;

        std::lock_guard lock (interfaces_mutex);

        auto entry = interfaces.find (iface);
        if (entry == interfaces.end()) {
            message reply (msg, true, DBUS_ERROR_FAILED, "Get/Set property error - no such interface");
            conn.send (reply);
            return true;
        }

        message reply (msg, false);
        reply.append_args (entry->second->get_all_properties());
        conn.send (reply);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_properties_method (message& msg)
    {
        if ( ! implements_properties)
            return false;
        else if (msg.name() == "Get")
            return on_properties_get_set_method (msg, true);
        else if (msg.name() == "Set")
            return on_properties_get_set_method (msg, false);
        else if (msg.name() == "GetAll")
            return on_properties_get_all_method (msg);
        else
            return false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_objectmanager_method (message& msg)
    {
        std::lock_guard lock (node_mutex);

        if ( ! implements_objectmanager  ||  msg.name() != "GetManagedObjects")
            return false;

        dbus_opath_dict dict ("a{sa{sv}}");
        for (auto& entry : nodes) {
            auto& node = *entry.second;
            if ( ! node.is_enabled())
                continue;
            node.get_all_objects_ifaces_properties (dict);
        }

        message reply (msg, false);
        reply.append_args (std::move(dict));
        return conn.send (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::get_all_objects_ifaces_properties (dbus_opath_dict& dict)
    {
        std::scoped_lock lock (interfaces_mutex, node_mutex);

        dbus_string_dict ifaces ("a{sv}");
        if (implements_introspectable)
            ifaces.set ("org.freedesktop.DBus.Introspectable", dbus_string_dict("v"));
        if (implements_properties)
            ifaces.set ("org.freedesktop.DBus.Properties", dbus_string_dict("v"));
        if (implements_objectmanager)
            ifaces.set ("org.freedesktop.DBus.ObjectManager", dbus_string_dict("v"));
        for (auto& if_entry : interfaces) {
            if (implements_properties)
                ifaces.set (if_entry.first, if_entry.second->get_all_properties());
            else
                ifaces.set (if_entry.first, dbus_string_dict("v"));
        }
        dict.set (opath, ifaces);

        for (auto& entry : nodes) {
            if (entry.second->is_enabled())
                entry.second->get_all_objects_ifaces_properties (dict);
        }
    }


} // namespace ultrabus
