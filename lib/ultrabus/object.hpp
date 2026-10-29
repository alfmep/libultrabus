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
#ifndef ULTRABUS_OBJECT_HPP
#define ULTRABUS_OBJECT_HPP

#include <ultrabus/object_handler.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_opath.hpp>
#include <string>
#include <memory>
#include <vector>
#include <map>


namespace ultrabus {


    // Forward declarations
    class interface;


    /**
     * A DBus Object.
     */
    class object : public object_handler {
    public:
        /**
         * Creates a DBus object.
         * @param conn A DBus connection.
         * @param object_path The Object Path of the object.
         * @param enable_interface_objectmanager If <code>true</code>,
         *                                 this object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.ObjectManager.
         * @param enable_interface_introspectable If <code>true</code>,
         *                                 this object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.Introspectable.
         */
        object (connection& conn,
                const std::string& object_path,
                const bool enable_interface_objectmanager=false,
                const bool enable_interface_introspectable=true);

        virtual ~object (); /*< Disable this object and all child nodes. */

        const dbus_opath& path () const {return opath;}

        /**
         * Enable this object.
         * @param enable_sub_nodes If <code>true</code>, also
         *                         enable all child nodes.
         */
        bool enable (bool enable_sub_nodes=true);

        /**
         * Disable this object and all child nodes.
         */
        void disable ();

        bool is_enabled () const {return enabled;}

        std::shared_ptr<object> create_node (const std::string& node_path,
                                             const bool enable_interface_objectmanager=false,
                                             const bool enable_interface_introspectable=true);

        bool remove_node (const std::string& node_path); // Disable and remove

        connection& get_connection () {return conn;}
        const connection& get_connection () const {return conn;}



        bool add_interface (std::shared_ptr<interface> iface);
        bool remove_interface (const std::string& interface_name);

        bool has_interface_introspectable () const {return implements_introspectable;}
        bool has_interface_properties () const {return implements_properties;}
        bool has_interface_objectmanager () const {return implements_objectmanager;}

        void enable_interface_properties ();

        object* parent () {return parent_obj;}
        const object* parent () const {return parent_obj;}


    protected:
        virtual bool on_method_call (message& msg);


    private:
        void emit_all_interfaces_added ();
        void emit_all_interfaces_removed ();
        void emit_interface_added (interface* iface);
        void emit_interface_added (const std::string& interface_name);
        void emit_interface_removed (const std::string& iface_name);

        bool on_introspect_method (message& msg);
        bool on_properties_get_set_method (message& msg, bool get_value);
        bool on_properties_get_all_method (message& msg);
        bool on_properties_method (message& msg);
        bool on_objectmanager_method (message& msg);
        void get_all_objects_ifaces_properties (dbus_opath_dict& dict);

        dbus_opath opath;
        object* parent_obj;
        std::map<std::string, std::shared_ptr<object>> nodes;
        std::map<std::string, std::shared_ptr<interface>> interfaces;
        bool implements_properties;
        bool implements_objectmanager;
        bool implements_introspectable;
        bool enabled;
    };


} // namespace ultrabus
#endif
