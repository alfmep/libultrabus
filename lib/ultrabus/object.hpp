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
        object (connection& conn,
                const std::string& object_path,
                const bool implement_if_introspectable=true,
                const bool implement_if_properties=false,
                const bool implement_if_objectmanager=false);
        virtual ~object ();

        bool register_object (bool register_nodes=true);
        void unregister_object ();

        bool is_registered () const {return registered;}

        const dbus_opath& path () const {return opath;}
        connection& get_connection () {return conn;}

        bool register_interface (std::shared_ptr<interface> iface);

        std::shared_ptr<object> create_node (const std::string& sub_path,
                                             const bool implement_if_introspectable=true,
                                             const bool implement_if_properties=false,
                                             const bool implement_if_objectmanager=false);
        bool remove_node (const std::string& sub_path, bool emit_signal=true);


        bool emit_interface_added (const dbus_opath& object_path,
                                   const std::string& interface_name);
        bool emit_interfaces_added (const dbus_opath& object_path);
        bool emit_interfaces_added (const dbus_opath& object_path,
                                    const std::vector<std::string>& interface_names);

        bool emit_interface_removed (const dbus_opath& object_path,
                                     const std::string& interface_name);
        bool emit_interfaces_removed (const dbus_opath& object_path,
                                      const std::vector<std::string>& interface_names);


    protected:
        virtual bool on_method_call (message& msg);


    private:
        bool on_introspect_method (message& msg);

        bool on_properties_get_set_method (message& msg, bool get_value);
        bool on_properties_get_all_method (message& msg);
        bool on_properties_method (message& msg);

        bool on_objectmanager_method (message& msg);

        dbus_opath opath;
        std::map<std::string, std::shared_ptr<object>> nodes;
        std::map<std::string, std::shared_ptr<interface>> interfaces;
        bool implements_introspectable;
        bool implements_properties;
        bool implements_objectmanager;
        bool registered;
    };


}
#endif
