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
    std::string get_node_path (const std::string& base, const std::string& sub)
    {
        if (sub[0] != '/')
            return sub;
        if (base == "/")
            return sub.substr (1);
        if (strncmp(base.c_str(), sub.c_str(), base.size()) != 0)
            return "";
        if (sub[base.size()] != '/')
            return "";
        return sub.substr (base.size()+1);
    }


} // Anonymous namespace


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object::object (connection& conn,
                    const std::string& object_path,
                    const bool implement_if_introspectable,
                    const bool implement_if_properties,
                    const bool implement_if_objectmanager)
        : object_handler(conn),
          opath (object_path),
          implements_introspectable {implement_if_introspectable},
          implements_properties {implement_if_properties},
          implements_objectmanager {implement_if_objectmanager},
          registered {false}
    {
        if (dbus_validate_path(object_path.c_str(), nullptr) != TRUE)
            throw std::invalid_argument ("Invalid object path");
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    object::~object ()
    {
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::register_object (bool register_nodes)
    {
        registered = register_object_path (opath.get(), nullptr);
        if ( ! registered)
            return false;

        bool retval = true;
        for (const auto& node : nodes) {
            if ( ! node.second->register_object(true))
                retval = false;
        }
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void object::unregister_object ()
    {
        for (const auto& node : nodes)
            node.second->unregister_object ();
        unregister_object_path (opath.get());
        registered = false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::register_interface (std::shared_ptr<interface> iface)
    {
        if (!iface)
            return false;
        interfaces.emplace (iface->name(), iface);

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<ultrabus::object> object::create_node (
            const std::string& sub_path,
            const bool implement_if_introspectable,
            const bool implement_if_properties,
            const bool implement_if_objectmanager)
    {
        std::string node_path = get_node_path (opath, sub_path);
        if (node_path.empty()) {
            return nullptr;
        }

        std::string full_path (opath);
        if (opath.get().back() != '/')
            full_path.push_back ('/');
        full_path.append (node_path);

        if (dbus_validate_path(full_path.c_str(), nullptr) != TRUE) {
            return nullptr;
        }

        std::shared_ptr<object> node (new object(conn,
                                                 full_path,
                                                 implement_if_introspectable,
                                                 implement_if_properties,
                                                 implement_if_objectmanager));
        if (node)
            nodes.emplace (node_path, node);
        return node;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::remove_node (const std::string& sub_path, bool emit_signal)
    {
        std::string node_path = get_node_path (opath, sub_path);
        auto i = nodes.find (node_path);
        if (i == nodes.end())
            return false;

        bool send_signal = emit_signal &&
            implements_objectmanager &&
            is_registered() &&
            i->second->is_registered();

        dbus_opath removed_path = i->second->path ();
        dbus_array removed_ifaces (DBUS_TYPE_STRING);
        if (send_signal) {
            for (auto& if_entry : i->second->interfaces)
                removed_ifaces.push_back (if_entry.first);
        }

        i->second->unregister_object ();
        nodes.erase (i);

        if (send_signal) {
            message signal (opath, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved");
            signal.append_args (removed_path, removed_ifaces);
            conn.send (signal);
        }
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::emit_interfaces_added (const dbus_opath& object_path)
    {
        if ( !implements_objectmanager || !is_registered())
            return false;

        std::string node_path = get_node_path (opath, object_path);
        auto i = nodes.find (node_path);
        if (i == nodes.end()  ||  i->second->is_registered()==false)
            return false;

        dbus_opath added_path = i->second->path ();
        dbus_string_dict added_ifaces ("a{sv}");
        for (auto& if_entry : i->second->interfaces)
            added_ifaces.set (if_entry.first, if_entry.second->get_all_properties());

        message signal (opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (added_path, added_ifaces);
        return conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::emit_interface_added (const dbus_opath& object_path,
                                       const std::string& interface_name)
    {
        std::vector<std::string> interface_names ({interface_name});
        return emit_interfaces_added (object_path, interface_names);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::emit_interfaces_added (const dbus_opath& object_path,
                                        const std::vector<std::string>& interface_names)
    {
        if ( !implements_objectmanager || !is_registered())
            return false;

        std::string node_path = get_node_path (opath, object_path);
        auto i = nodes.find (node_path);
        if (i == nodes.end()  ||  i->second->is_registered()==false)
            return false;

        auto& node = *i->second;
        dbus_opath added_path = node.path ();
        dbus_string_dict added_ifaces ("a{sv}");
        for (auto& iface_name : interface_names) {
            auto if_entry = node.interfaces.find (iface_name);
            if (if_entry == node.interfaces.end())
                continue;
            added_ifaces.set (if_entry->first, if_entry->second->get_all_properties());
        }
        if (added_ifaces.empty())
            return false;

        message signal (opath, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded");
        signal.append_args (added_path, added_ifaces);
        return conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::emit_interface_removed (const dbus_opath& object_path,
                                         const std::string& interface_name)
    {
        std::vector<std::string> interface_names ({interface_name});
        return emit_interfaces_removed (object_path, interface_names);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::emit_interfaces_removed (const dbus_opath& object_path,
                                          const std::vector<std::string>& interface_names)
    {
        if ( !implements_objectmanager || !is_registered())
            return false;

        dbus_array removed_ifaces (DBUS_TYPE_STRING);
        for (auto& iface_name : interface_names) {
            if (dbus_validate_interface(iface_name.c_str(), nullptr) != TRUE)
                return false;
            removed_ifaces.push_back (iface_name);
        }

        message signal (opath, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved");
        signal.append_args (object_path, removed_ifaces);
        return conn.send (signal);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool object::on_method_call (message& msg)
    {
        static std::set<std::string> if_properties_method_names {{"Get", "Set", "GetAll"}};
        bool handled {false};

        // Check standard interfaces
        //
        if (implements_introspectable  &&  msg.interface() == "org.freedesktop.DBus.Introspectable")
            handled = on_introspect_method (msg);
        else if (implements_properties  &&  msg.interface() == "org.freedesktop.DBus.Properties")
            handled = on_properties_method (msg);
        else if (implements_objectmanager  &&  msg.interface() == "org.freedesktop.DBus.ObjectManager")
            handled = on_objectmanager_method (msg);

        // Check registered interfaces
        //
        if (!handled) {
            auto entry = interfaces.find (msg.interface());
            if (entry != interfaces.end()) {
                handled = entry->second->on_method (msg);
            }
        }

        // Check standard interfaces again, in case the
        // method call has an empty interface name
        //
        if (!handled) {
            if (implements_introspectable  &&  msg.name() == "Introspect")
                handled = on_introspect_method (msg);
            else if (implements_properties  &&  if_properties_method_names.contains(msg.name()))
                handled = on_properties_method (msg);
            else if (implements_objectmanager  &&  msg.name() == "GetManagedObjects")
                handled = on_objectmanager_method (msg);
        }

        // // Check registered interfaces again, in case the
        // // method call has an empty interface name
        // //
        // if (!handled) {
        //     for (auto& iface : interfaces) {
        //         if (iface->second->has_method(msg.name()))
        //             handled = entry->second->on_method (msg);
        //     }
        // }

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
        for (const auto& entry : interfaces)
            xml.append (entry.second->to_xml());

        // Add nodes info
        std::set<std::string> node_names;
        for (const auto& node : nodes) {
            if ( ! node.second->is_registered())
                continue;
            auto& full_name = node.first;
            node_names.emplace (full_name.substr(0, full_name.find_first_of('/')));
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
        dbus_string iface;
        dbus_string prop_name;
        dbus_variant value;

        if (get_value) {
            if ( ! msg.get_args_strict(iface, prop_name))
                return false;
        }else{
            if ( ! msg.get_args_strict(iface, prop_name, value))
                return false;
        }

        auto entry = interfaces.find (iface);
        if (entry == interfaces.end()) {
            message reply (msg, true, "se.ultramarin.error", "Get/Set property error - no such interface");
            conn.send (reply);
            return true;
        }

        bool ok {false};
        if (get_value)
            ok = entry->second->get_property (prop_name, value);
        else
            ok = entry->second->set_property (prop_name, value);
        if (!ok) {
            message reply (msg, true, "se.ultramarin.error", "Get/Set property error");
            conn.send (reply);
            return true;
        }

        message reply (msg, false);
        if (get_value)
            reply.append_args (value);
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

        auto entry = interfaces.find (iface);
        if (entry == interfaces.end()) {
            message reply (msg, true, "se.ultramarin.error", "Get/Set property error - no such interface");
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
        if ( ! implements_objectmanager  ||  msg.name() != "GetManagedObjects")
            return false;

        dbus_opath_dict objs ("a{sa{sv}}");

        for (auto& entry : nodes) {
            auto& obj = *entry.second;
            if ( ! obj.is_registered())
                continue;
            dbus_string_dict ifaces ("a{sv}");
            for (auto& if_entry : obj.interfaces)
                ifaces.set (if_entry.first, if_entry.second->get_all_properties());
            objs.set (obj.path(), ifaces);
        }

        message reply (msg, false);
        reply.append_args (std::move(objs));
        return conn.send (reply);
    }


}
