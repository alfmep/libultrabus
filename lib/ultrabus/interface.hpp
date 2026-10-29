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
#include <memory>
#include <functional>
#include <tuple>
#include <map>


namespace ultrabus {


    /**
     * A class used for implementing DBus interfaces.
     */
    class interface {
    public:
        /**
         * A callback used for implementing DBus method calls.
         * @param conn A DBus connection object.
         * @param msg The DBus method call message.
         * @note It is the responsibility of this callback
         *       to send a reply message when it has finished
         *       doing what it is suppose to do. Either a normal
         *       reply with or without arguments, or an
         *       error reply.<br/>
         *       If no reply is sent, the entity making the
         *       DBus method call will have to wait until it
         *       gets a timeout (no response) error.<br/>
         *       <br/>
         *       An example of the simplest way of sending a reply
         *       from within the callback:
         *       <pre>
         *       conn.send (msg.make_reply());
         *       </pre>
         *       A simple example of sending an error reply from within
         *       the callback:
         *       <pre>
         *       conn.send (msg.make_reply("an.error.name", "An error description"));
         *       </pre>
         */
        using iface_method_cb_t = std::function<void (connection& conn, message& msg)>;

        /**
         * A callback used for getting an interface property value.
         * When implementing this callback, <code>true</code> must
         * be returned if the callback could successfully assign
         * the <code>value</code> parameter with the property value.<br/>
         * If this callback returns <code>false</code>, the entity trying
         * to get the property value will instead get an error response.
         * @param name The name of the property.
         * @param value A <code>dbus_variant</code> that will be
         *              assigned the property value.
         * @return <code>true</code> if the value could be retrieved
         *         without error. <code>false</code> if the property
         *         couldn't be retrieved for some reason.
         */
        using get_property_cb_t = std::function<bool (const std::string& name,
                                                      dbus_variant& value)>;

        /**
         * A callback used for setting an interface property value.
         * When implementing this callback, <code>true</code> must
         * be returned if the callback could successfully assign
         * the new value to the property.<br/>
         * If this callback returns <code>false</code>, the entity trying
         * to set the property value will instead get an error response.
         * @param name The name of the property.
         * @param value A <code>dbus_variant</code> that contains
         *              the value to be assigned to the property.
         * @return <code>true</code> if the value could be assigned
         *         without error. <code>false</code> if the property
         *         couldn't be assigned the new value for some reason.
         */
        using set_property_cb_t = std::function<bool (const std::string& name,
                                                      const dbus_variant& value)>;

        /**
         * Create an instance of this class.
         * @param interface_name The name of the newly created interface.
         *                       This must be a valid DBus interface name.
         * @return A shared pointer to an <code>interface</code> instance.
         *         Or a <code>nullptr</code> if the interface name isn't
         *         a valid DBus interface name.
         */
        static std::shared_ptr<interface> create (const std::string& interface_name);

        virtual ~interface (); /**< Destructor. Free resources. */

        /**
         * Get the name of this DBus interface.
         * @return A const reference to the DBus interface name.
         * @note The returned reference to the interface name is
         *       only valid for the lifetime of this instance.
         */
        const std::string& name () const {return if_name;}

        /**
         * Add a DBus method to this interface that takes no
         * arguments and doesn't return a value.
         * Note that the supplied callback must still send back
         * a reply, even if that reply doesn't include a return value
         * as argument.
         * @param name The name of the DBus method we wish to add.
         * @param callback A callback that will be called whenever
         *           some other enitiy on the Dbus calls this method
         *           on this interface instance.
         * @return <code>true</code> if the method was successfully added.<br/>
         *         <code>false</code> if the method name isn't a valid DBus
         *         method call name, or if a method with the same name has
         *         already been added to this interface.
         * @see iface_method_cb_t A callback implementing the DBus method call.
         */
        bool add_method (const std::string& name,
                         iface_method_cb_t callback);

        // in_param:   "[name:]signature"
        // out_param:  "[name:]signature"
        /**
         * Add a DBus method to this interface that may take
         * one argument and may include a return value in the reply.
         * @param name The name of the DBus method we wish to add.
         * @param callback A callback that will be called whenever
         *           some other enitiy on the Dbus calls this method
         *           on this interface.
         */
        bool add_method (const std::string& name,
                         const std::string& in_param,
                         const std::string& out_param,
                         iface_method_cb_t callback);

        bool add_method (const std::string& name,
                         const std::vector<std::string>& in_params,
                         const std::string& out_param,
                         iface_method_cb_t callback);

        void remove_method (const std::string& name);

        bool has_method (const std::string& name) const;



        bool add_property (const std::string& name,
                           const std::string& signature,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        bool add_property (const std::string& name,
                           const dbus_type& initial_value,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        bool add_property (const std::string& name,
                           dbus_type&& initial_value,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        bool add_property (const std::string& name,
                           const std::string& signature,
                           get_property_cb_t get_callback,
                           set_property_cb_t set_callback=nullptr);

        void remove_property (const std::string& name, bool emit_signal=true);

        int get_property (const std::string& name, dbus_type& value) const;

        template<dbus_basic_cpp_types T>
        int get_property (const std::string& name, T& value) const {
            dbus_basic<T> v;
            int retval = get_property (name, v);
            if (retval == 0)
                value = v.get ();
            return retval;
        }

        /**
         * Check if this interface instance has any properties.
         * @return <code>true</code> if this interface
         *         instance has one or more properties.
         */
        bool has_properties () const {return !properties.empty();}

        /**
         * Return a pointer to a <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @param name The name of the property we are interested in.
         * @return A pointer to the <code>dbus_variant</code> containing
         *         the property value. Or a <code>nullptr</code> if
         *         no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        dbus_variant* property (const std::string& name);
        /**
         * Return a pointer to a const <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @param name The name of the property we are interested in.
         * @return A pointer to a const <code>dbus_variant</code>
         *         containing the property value. Or a
         *         <code>nullptr</code> if no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        const dbus_variant* property (const std::string& name) const;

        int set_property (const std::string& name, const dbus_type& value, bool emit_if_changed=true);
        template<dbus_basic_cpp_types T>
        int set_property (const std::string& name, const T& value, bool emit_if_changed=true) {
            return set_property (name, dbus_basic<T>(value), emit_if_changed);
        }

        dbus_string_dict get_all_properties () const;


        bool emit_property_changed (const std::string& property_name);
        bool emit_property_changed (const std::string& property_name, const dbus_type& value);
        template<dbus_basic_cpp_types T>
        bool emit_property_changed (const std::string& property_name, const T& value) {
            return emit_property_changed (property_name, dbus_basic<T>(value));
        }

        bool emit_property_invalid (const std::string& property_name);
        bool emit_properties_changed (const std::vector<std::string>& property_names,
                                      const std::vector<std::string>& invalid_names={});


        bool add_signal (const std::string& name);
        // argument:  "[name:]signature"
        bool add_signal (const std::string& name, const std::string& argument);
        // argument:  "[name:]signature"
        bool add_signal (const std::string& name, const std::vector<std::string>& arguments);
        void remove_signal (const std::string& name);


        bool emit_signal (const std::string& name) {
            if (!obj || dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj->path(), if_name, name);
            return emit_signal_impl (signal);
        }

        template<typename T, typename... Targs>
        bool emit_signal (const std::string& name, const T& arg, const Targs&... args) {
            if (!obj || dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj->path(), if_name, name);
            signal.append_args (arg, args...);
            return emit_signal_impl (signal);
        }

        object* owner () const {return obj;}

        const std::string to_xml () const;

    protected:
        interface (const std::string& interface_name);


    private:
        friend class object;

        //                               in_sign      out_sign     callback
        using method_desc_t = std::tuple<std::string, std::string, iface_method_cb_t>;

        //                                  name         signature
        using signal_arg_desc_t = std::pair<std::string, std::string>;

        struct property_t {
            dbus_variant value;
            std::string signature;
            get_property_cb_t get_cb;
            set_property_cb_t set_cb;
            bool readable;
            bool writable;
            bool disable_emit_on_set_property;
            property_t () : value("v"), readable{false}, writable{false}, disable_emit_on_set_property{false} {}
        };

        std::string if_name; // The name of this interface
        object* obj;         // Object implementing this interface
        std::map<std::string, method_desc_t> methods;
        std::map<std::string, std::string> xml_methods;
        std::map<std::string, property_t> properties;

        //       signal name            sig sign     arguments
        std::map<std::string, std::pair<std::string, std::vector<signal_arg_desc_t>>> signals;

        bool on_method (message& msg);
        void set_owner (object* obj_arg) {obj = obj_arg;}
        bool add_property_impl (const std::string& name,
                                const std::string& signature,
                                const dbus_type* initial_value,
                                const bool move_initial_value,
                                const bool readable,
                                const bool writable,
                                get_property_cb_t get_callback,
                                set_property_cb_t set_callback,
                                const bool disable_emit_on_set_property);
        bool emit_signal_impl (message signal);
    };


} // namespace ultrabus
#endif
