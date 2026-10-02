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
#ifndef ULTRABUS_INTERFACE_HPP
#define ULTRABUS_INTERFACE_HPP

#include <ultrabus/object.hpp>
#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <string>
#include <functional>
#include <tuple>
#include <map>


namespace ultrabus {


    /**
     *
     */
    class interface {
    public:
        using iface_method_cb_t = std::function<void (message& msg)>;
        using get_property_cb_t = std::function<bool (const std::string& name,
                                                      dbus_variant& value)>;
        using set_property_cb_t = std::function<bool (const std::string& name,
                                                      const dbus_variant& value)>;
        interface (const std::string& interface_name, object& object_arg);
        virtual ~interface ();

        const std::string name () const {return if_name;}
        const std::string to_xml () const;

        bool register_method (const std::string& name,
                              iface_method_cb_t callback);

        // out_param:  "[name:]signature"
        bool register_method (const std::string& name,
                              const std::string& out_param,
                              iface_method_cb_t callback);

        // in_param:   "[name:]signature"
        // out_param:  "[name:]signature"
        bool register_method (const std::string& name,
                              const std::string& in_param,
                              const std::string& out_param,
                              iface_method_cb_t callback);

        bool register_method (const std::string& name,
                              const std::vector<std::string>& in_params,
                              const std::string& out_param,
                              iface_method_cb_t callback);

        void unregister_method (const std::string& name);
        bool on_method (message& msg);

        bool register_property (const std::string& name,
                                const std::string& signature,
                                get_property_cb_t get_callback,
                                set_property_cb_t set_callback);
        void unregister_property (const std::string& name, bool emit_signal=false);
        bool get_property (const std::string& name, dbus_variant& value);
        bool set_property (const std::string& name, const dbus_variant& value);
        dbus_string_dict get_all_properties () const;

        bool emit_property_changed (const std::string& property_name, const dbus_variant& value);
        bool emit_property_invalid (const std::string& property_name);
        bool emit_properties_changed (const std::vector<std::string>& property_names,
                                      const std::vector<std::string>& invalid_names={});

        bool register_signal (const std::string& name);
        // argument:  "[name:]signature"
        bool register_signal (const std::string& name, const std::string& argument);
        // argument:  "[name:]signature"
        bool register_signal (const std::string& name, const std::vector<std::string>& arguments);
        void unregister_signal (const std::string& name);


        bool emit_signal (const std::string& name) {
            if (dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj.path(), if_name, name);
            return emit_signal_impl (signal);
        }

        template<typename T, typename... Targs>
        bool emit_signal (const std::string& name, const T& arg, const Targs&... args) {
            if (dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj.path(), if_name, name);
            signal.append_args (arg, args...);
            return emit_signal_impl (signal);
        }


    private:
        //                               in_sign      out_sign     callback
        using method_desc_t = std::tuple<std::string, std::string, iface_method_cb_t>;

        //                                 sign         get callback       set callback
        using property_desc_t = std::tuple<std::string, get_property_cb_t, set_property_cb_t>;

        //                                  name         signature
        using signal_arg_desc_t = std::pair<std::string, std::string>;


        std::string if_name; // The name of this interface
        object& obj;         // Object implementing this interface
        connection& conn;

        std::map<std::string, method_desc_t> methods;
        std::map<std::string, std::string> xml_methods;
        std::map<std::string, property_desc_t> properties;

        //       signal name            sig sign     arguments
        std::map<std::string, std::pair<std::string, std::vector<signal_arg_desc_t>>> signals;

        bool emit_signal_impl (message signal);
    };


};
#endif
