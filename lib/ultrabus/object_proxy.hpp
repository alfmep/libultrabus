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
#ifndef ULTRABUS_OBJECT_PROXY_HPP
#define ULTRABUS_OBJECT_PROXY_HPP

#include <ultrabus/org_freedesktop_DBus_Properties.hpp>
#include <ultrabus/message_filter.hpp>
#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/retvalue.hpp>
#include <string>
#include <stdexcept>


namespace ultrabus {


    // Forward declarations
    class connection;


    /**
     * Proxy for communicating with one remote D-Bus object.
     *
     * The proxy retains a non-owning reference to its connection, which must
     * outlive the proxy. Callbacks registered on the proxy remain installed
     * until replaced, explicitly removed, or the proxy is destroyed.
     */
    class object_proxy : public message_filter {
    public:
        /**
         * Callback called when new interfaces are added to the object.
         * @param ifaces Map of interface names and properties. The reference
         *               is valid only for the callback invocation.
         */
        using object_ifaces_added_cb_t = std::function<void (std::map<std::string, dbus_string_dict>& ifaces)>;

        /**
         * Callback called when interfaces are removed from the object.
         * @param ifaces Removed interface names. The reference is valid only
         *               for the callback invocation.
         */
        using object_ifaces_removed_cb_t = std::function<void (std::set<std::string>& ifaces)>;

        /**
         * Callback for signals sent to the object proxy.
         * @param sig_msg Signal message. The reference is valid only for the
         *                callback invocation.
         * @return <code>true</code> if the signal was handled and
         *         no more message filter should be called.
         */
        using signal_cb_t = std::function<bool (message& sig_msg)>;

        /**
         * Constructor.
         * @param conn Connection used for calls and signals. It is not owned
         *             and must outlive this proxy.
         * @param bus_name The service that owns the object.
         * @param object_path The object we want to communicate with.
         * @param default_interface The default DBus interface to use
         *                          when calling methods on the object,
         *                          or an empty string if no default
         *                          interface should be set.
         * @throw std::invalid_argument If the name of the service,
         *                              object path, or default interface
         *                              is not a valid name.
         */
        object_proxy (connection& conn,
                      const std::string& bus_name,
                      const std::string& object_path,
                      const std::string& default_interface="");

        /**
         * Destroy the proxy and release its stored callback objects.
         */
        virtual ~object_proxy () = default;

        /**
         * Return the bus name of the service owning the object.
         *
         * @return A read-only reference owned by this proxy, valid until the
         *         proxy is destroyed.
         */
        const std::string& service () const {return target;}

        /**
         * Return the object path.
         *
         * @return A read-only reference owned by this proxy, valid until the
         *         proxy is destroyed.
         */
        const std::string& path () const {return opath;}

        /**
         * Return the default interface used for calls.
         *
         * @return A read-only reference owned by this proxy, valid until the
         *         proxy is destroyed.
         */
        const std::string& default_interface () const {return def_iface;}

        /**
         * Set the default interface we use to call methods on.
         * @param default_interface The default interface when
         *                          calling methods on this object.<br/>
         *                          Set an empty string to not use
         *                          a default interface.
         * @return <code>true</code> when the interface was set;
         *         <code>false</code> when the supplied name is invalid.
         */
        bool default_interface (const std::string& default_interface);

        /**
         * Set/remove a callback to be called when interfaces are added to this object.
         * @param callback Callback for added interfaces. Pass
         *                 <code>nullptr</code> to remove it; a non-null
         *                 callback is retained by this proxy.
         * @param om_root_path The path to the object implementing interface
         *                     <code>org.freedesktop.DBus.ObjectManager</code> that
         *                     is responsible for creating interfaces to this object.
         * @return <code>true</code> if the callback was installed or removed;
         *         <code>false</code> if the object-manager root path is invalid.
         */
        bool set_interfaces_added_callback (object_ifaces_added_cb_t callback, const std::string& om_root_path="/");

        /**
         * Set/remove a callback to be called when interfaces are removed from this object.
         * @param callback Callback for removed interfaces. Pass
         *                 <code>nullptr</code> to remove it; a non-null
         *                 callback is retained by this proxy.
         * @param om_root_path The path to the object implementing interface
         *                     <code>org.freedesktop.DBus.ObjectManager</code> that
         *                     is responsible for removing interfaces from this object.
         * @return <code>true</code> if the callback was installed or removed;
         *         <code>false</code> if the object-manager root path is invalid.
         */
        bool set_interfaces_removed_callback (object_ifaces_removed_cb_t callback, const std::string& om_root_path="/");

        /**
         * Set a callback to be called when the properties of this DBus object changes.
         * @param callback Callback for property changes. Pass
         *                 <code>nullptr</code> to remove it; a non-null
         *                 callback is retained by this proxy.
         * @return <code>true</code> if the callback was installed or removed.
         */
        bool set_properties_changed_callback (properties_changed_cb_t callback);

        /**
         * Set/remove a callback to be called when specific signals
         * arrives from this object.
         * @param interface The interface implementing the specific signal.
         *                  If an empty string, any interface emitting the
         *                  specific signal triggers the callback.
         * @param signal The name of the signal, it can't be an empty string.
         * @param callback Callback for matching signals. Pass
         *                 <code>nullptr</code> to remove it; a non-null
         *                 callback is retained by this proxy.
         * @return <code>true</code> if the callback was installed or removed;
         *         <code>false</code> if the signal name is invalid.
         */
        bool set_signal_callback (const std::string& interface,
                                  const std::string& signal,
                                  signal_cb_t callback);

        /**
         * Remove all signal callbacks set by set_signal_callback().
         *
         * Existing callback objects are released before this method returns.
         */
        void clear_signal_callbacks ();

        /**
         * Call a method on the default interface and wait for its reply.
         * @param method_name The name of the method to call.
         * @param timeout The maximum time in milliseconds to wait for a message reply.
         * @return The reply message.
         */
        message call (const std::string& method_name, int timeout=DBUS_TIMEOUT_USE_DEFAULT) {
            message msg (target, opath, def_iface, method_name);
            return conn.send_and_wait (msg, timeout);
        }

        /**
         * Call a method on the object wait for a result.
         * @param method_name The name of the method.
         * @tparam T Type of the first argument, constrained to a supported
         *          D-Bus or basic C++ type.
         * @tparam Targs Types of any remaining arguments.
         * @param arg First argument to append.
         * @param args Remaining arguments to append.
         * @return The reply message.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<any_dbus_or_basic_cpp_type T, typename... Targs>
        message call (const std::string& method_name, const T& arg, const Targs&... args) {
            message msg (target, opath, def_iface, method_name);
            msg.append_args (arg, args...);
            return conn.send_and_wait (msg);
        }

        /**
         * Call a method on the object wait for a result.
         * @param method_name The name of the method.
         * @tparam Targs Types of any remaining arguments.
         * @param arg Null-terminated string argument to append; it must not be null.
         * @param args Remaining arguments to append.
         * @return The reply message.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<typename... Targs>
        message call (const std::string& method_name, const char* arg, const Targs&... args) {
            message msg (target, opath, def_iface, method_name);
            msg.append_args (arg, args...);
            return conn.send_and_wait (msg);
        }

        /**
         * Call a method on the object and provide a callback to handle the result.
         * @param method_name The name of the method to call.
         * @param reply_cb Callback invoked for the reply. The connection
         *                 retains it until delivery; its message reference is
         *                 valid only during the callback.
         * @param timeout The maximum time in milliseconds to wait for a message
         *        reply, or <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool call (const std::string& method_name,
                   std::function<void (ultrabus::message&)> reply_cb,
                   int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            message msg (target, opath, def_iface, method_name);
            return conn.send (msg, reply_cb, timeout);
        }

        /**
         * Call a method on the object and provide a callback to handle the result.
         * @param method_name The name of the method.
         * @tparam T Type of the first argument, constrained to a supported
         *          D-Bus or basic C++ type.
         * @tparam Targs Types of any remaining arguments.
         * @param reply_cb Callback invoked for the reply; its message
         *                 reference is valid only during the callback.
         * @param arg First argument to append.
         * @param args Remaining arguments to append.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<any_dbus_or_basic_cpp_type T, typename... Targs>
        bool call (const std::string& method_name,
                   std::function<void (ultrabus::message&)> reply_cb,
                   const T& arg,
                   const Targs&... args)
        {
            message msg (target, opath, def_iface, method_name);
            msg.append_args (arg, args...);
            return conn.send (msg, reply_cb);
        }

        /**
         * Call a method on the object and provide a callback to handle the result.
         * @param method_name The name of the method.
         * @tparam Targs Types of any remaining arguments.
         * @param reply_cb Callback invoked for the reply; its message
         *                 reference is valid only during the callback.
         * @param arg Null-terminated string to append; it must not be null.
         * @param args Remaining arguments to append.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<typename... Targs>
        bool call (const std::string& method_name,
                   std::function<void (ultrabus::message&)> reply_cb,
                   const char* arg,
                   const Targs&... args)
        {
            message msg (target, opath, def_iface, method_name);
            msg.append_args (arg, args...);
            return conn.send (msg, reply_cb);
        }

        /**
         * Call a method on a specific interface and wait for a result.
         * @param interface The method interface.
         * @param method_name The name of the method.
         * @param timeout The maximum time in milliseconds to wait for a message
         *        reply, or <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return The reply message.
         */
        message call_iface (const std::string& interface,
                            const std::string& method_name,
                            int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            message msg (target, opath, interface, method_name);
            return conn.send_and_wait (msg, timeout);
        }

        /**
         * Call a method on a specific interface and wait for a result.
         * @param interface The method interface.
         * @param method_name The name of the method to call.
         * @return The reply message.
         * @tparam T Type of the first argument, constrained to a supported
         *          D-Bus or basic C++ type.
         * @tparam Targs Types of any remaining arguments.
         * @param arg First argument to append.
         * @param args Remaining arguments to append.
         * @return The reply message.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<any_dbus_or_basic_cpp_type T, typename... Targs>
        message call_iface (const std::string& interface,
                            const std::string& method_name,
                            const T& arg,
                            const Targs&... args)
        {
            message msg (target, opath, interface, method_name);
            msg.append_args (arg, args...);
            return conn.send_and_wait (msg);
        }

        /**
         * Call a method on a specific interface and wait for a result.
         * @param interface The method interface.
         * @param method_name The name of the method to call.
         * @tparam Targs Types of any remaining arguments.
         * @param arg Null-terminated string to append; it must not be null.
         * @param args Remaining arguments to append.
         * @return The reply message.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<typename... Targs>
        message call_iface (const std::string& interface,
                            const std::string& method_name,
                            const char* arg,
                            const Targs&... args)
        {
            message msg (target, opath, interface, method_name);
            msg.append_args (arg, args...);
            return conn.send_and_wait (msg);
        }

        /**
         * Call a method on a specific interface and provide a callback to handle the result.
         * @param interface The method interface.
         * @param method_name The name of the method.
         * @param reply_cb Callback invoked for the reply. The connection
         *                 retains it until delivery; its message reference is
         *                 valid only during the callback.
         * @param timeout The maximum time in milliseconds to wait for a message
         *        reply, or <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool call_iface (const std::string& interface,
                         const std::string& method_name,
                         std::function<void (ultrabus::message&)> reply_cb,
                         int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            message msg (target, opath, interface, method_name);
            return conn.send (msg, reply_cb, timeout);
        }

        /**
         * Call a method on a specific interface and provide a callback to handle the result.
         * @param interface The method interface.
         * @param method_name The name of the method to call.
         * @tparam T Type of the first argument, constrained to a supported
         *          D-Bus or basic C++ type.
         * @tparam Targs Types of any remaining arguments.
         * @param reply_cb Callback invoked for the reply; its message
         *                 reference is valid only during the callback.
         * @param arg First argument to append.
         * @param args Remaining arguments to append.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<any_dbus_or_basic_cpp_type T, typename... Targs>
        bool call_iface (const std::string& interface,
                         const std::string& method_name,
                         std::function<void (ultrabus::message&)> reply_cb,
                         const T& arg,
                         const Targs&... args)
        {
            message msg (target, opath, interface, method_name);
            msg.append_args (arg, args...);
            return conn.send (msg, reply_cb);
        }

        /**
         * Call a method on a specific interface and provide a callback to handle the result.
         * @param interface The method interface.
         * @param method_name The name of the method to call.
         * @tparam Targs Types of any remaining arguments.
         * @param reply_cb Callback invoked for the reply; its message
         *                 reference is valid only during the callback.
         * @param arg Null-terminated string to append; it must not be null.
         * @param args Remaining arguments to append.
         * @return <code>false</code> if the message couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         * @note When using this method, the timeout used is DBUS_TIMEOUT_USE_DEFAULT.
         */
        template<typename... Targs>
        bool call_iface (const std::string& interface,
                         const std::string& method_name,
                         std::function<void (ultrabus::message&)> reply_cb,
                         const char* arg,
                         const Targs&... args)
        {
            message msg (target, opath, interface, method_name);
            msg.append_args (arg, args...);
            return conn.send (msg, reply_cb);
        }

        /**
         * Query the object for all properties using the default interface.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return All properties on the default interface on success;
         *         otherwise a retvalue whose err() and what() describe the
         *         failure.
         */
        retvalue<dbus_string_dict> get_all_properties (int timeout=DBUS_TIMEOUT_USE_DEFAULT) {
            return get_all_iface_properties (def_iface, timeout);
        }

        /**
         * Asynchronous query for all properties using the default interface.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the query couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool get_all_properties (
                std::function<void (retvalue<dbus_string_dict>& result)> callback,
                int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return get_all_iface_properties (def_iface, callback, timeout);
        }

        /**
         * Query the object for all properties using a specific interface.
         *
         * @param interface D-Bus interface to query.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return All properties on the specified interface on success;
         *         otherwise a retvalue whose err() and what() describe the
         *         failure.
         */
        retvalue<dbus_string_dict> get_all_iface_properties (const std::string& interface,
                                                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous query for all properties using a specific interface.
         * @param interface_name The DBus interface to query.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the query couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool get_all_iface_properties (const std::string& interface_name,
                                       std::function<void (retvalue<dbus_string_dict>& result)> callback,
                                       int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Query the object for a property using the default interface.
         * @param property_name The name of the property.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return The property variant on success; otherwise a retvalue whose
         *         err() and what() describe the failure.
         */
        retvalue<dbus_variant> get_property (const std::string& property_name,
                                             int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return get_iface_property (def_iface, property_name, timeout);
        }

        /**
         * Query the object for a property using the default interface.
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value Destination for the property value; it is modified only
         *              when the value can be converted to <code>T</code>.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the property is available and
         *         could be read, <code>false</code> if not.
         * @see ultrabus::dbus_basic_cpp_types
         */
        template<dbus_basic_cpp_types T>
        bool get_property (const std::string& property_name,
                           T& value,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return get_iface_property (def_iface, property_name, value, timeout);
        }

        /**
         * Query the object for a property using the default interface.
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value Destination for the property value; it is modified only
         *              when its D-Bus signature matches.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the property is available and
         *         could be read, <code>false</code> if not.
         * @see ultrabus::any_dbus_type
         */
        template<any_dbus_type T>
        bool get_property (const std::string& property_name,
                           T& value,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return get_iface_property (def_iface, property_name, value, timeout);
        }

        /**
         * Asynchronous query for a property using the default interface.
         * @param property_name The name of the property.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the query couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool get_property (const std::string& property_name,
                           std::function<void (retvalue<dbus_variant>& result)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return get_iface_property (def_iface, property_name, callback, timeout);
        }

        /**
         * Query the object for a property using a specific interface.
         * @param interface The DBus interface to query.
         * @param property_name The name of the property.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return The property variant on success; otherwise a retvalue whose
         *         err() and what() describe the failure.
         */
        retvalue<dbus_variant> get_iface_property (const std::string& interface,
                                                   const std::string& property_name,
                                                   int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Query the object for a property using a specific interface.
         * @param interface The DBus interface to query.
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value Destination for the property value; it is modified only
         *              when the value can be converted to <code>T</code>.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the property is available and
         *         could be read, <code>false</code> if not.
         */
        template<dbus_basic_cpp_types T>
        bool get_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 T& value,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            auto result = get_iface_property (interface, property_name, timeout);
            if (result.err() != 0) {
                return false;
            }

            bool is_ok = true;
            dbus_type& prop = result.get().get(); // result.dbus_vaiant.get()

            if constexpr (std::same_as<T, std::string>) {
                switch (prop.type_code()) {
                case DBUS_TYPE_STRING:
                    value = std::move (prop.cast<dbus_string>().get());
                    break;
                case DBUS_TYPE_OBJECT_PATH:
                    value = std::move (prop.cast<dbus_opath>().get());
                    break;
                case DBUS_TYPE_SIGNATURE:
                    value = std::move (prop.cast<dbus_signature>().get());
                    break;
                default:
                    is_ok = false;
                }
            }else{
                if (prop.type_code() == dbus_basic<T>::make_type_code()) {
                    value = prop.cast<dbus_basic<T>>().get ();
                }else{
                    is_ok = false;
                }
            }
            return is_ok;
        }

        /**
         * Query the object for a property using a specific interface.
         * @param interface The DBus interface to query.
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value Destination for the property value; it is modified only
         *              when its D-Bus signature matches.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the property is available and
         *         could be read, <code>false</code> if not.
         */
        template<any_dbus_type T>
        bool get_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 T& value,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            auto result = get_iface_property (interface, property_name, timeout);
            if (result.err() != 0) {
                return false;
            }
            if constexpr (std::same_as<T, dbus_variant>) {
                value = std::move (result.get());
            }else{
                dbus_type& prop = result.get().get(); // result.dbus_vaiant.get()
                if (prop.signature() != value.signature())
                    return false;
                value = std::move (prop);
            }
            return true;
        }

        /**
         * Asynchronous query for a property using a specific interface.
         * @param interface The DBus interface to query.
         * @param property_name The name of the property.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the query couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool get_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 std::function<void (retvalue<dbus_variant>& result)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Set a property value in the default interface.
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure.
         */
        template<dbus_basic_cpp_types T>
        retvalue<bool> set_property (const std::string& property_name,
                                     const T& value,
                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (def_iface, property_name,
                                       dbus_variant(dbus_basic<T>(value)), timeout);
        }

        /**
         * Set a property value in the default interface.
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value New D-Bus value to send.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure.
         */
        template<any_dbus_type T>
        retvalue<bool> set_property (const std::string& property_name,
                                     const T& value,
                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if constexpr (std::same_as<T, dbus_variant>)
                return set_iface_property (def_iface, property_name, value, timeout);
            else
                return set_iface_property (def_iface, property_name, dbus_variant(value), timeout);
        }

        /**
         * Set a string property value in the default interface.
         * @param property_name The name of the property.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure,
         *         including a null <code>value</code>.
         */
        retvalue<bool> set_property (const std::string& property_name,
                                     const char* value,
                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if (!value) {
                retvalue<bool> ret (false);
                ret.err (-1, "Invalid null pointer argument");
                return ret;
            }
            return set_iface_property (def_iface, property_name,
                                       dbus_variant(dbus_string(value)), timeout);
        }

        /**
         * Asynchronous call to set a property value in the default interface.
         * The result of the operation is sent as a boolean parameter in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        template<dbus_basic_cpp_types T>
        bool set_property (const std::string& property_name,
                           const T& value,
                           std::function<void (retvalue<bool>& result)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (def_iface,
                                       property_name,
                                       dbus_variant(dbus_basic<T>(value)),
                                       callback,
                                       timeout);
        }

        /**
         * Asynchronous call to set a property value in the default interface.
         * The result of the operation is sent as a boolean parameter in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value New D-Bus value to send.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        template<any_dbus_type T>
        bool set_property (const std::string& property_name,
                           const T& value,
                           std::function<void (retvalue<bool>& result)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if constexpr (std::same_as<T, dbus_variant>) {
                return set_iface_property (def_iface,
                                           property_name,
                                           value,
                                           callback,
                                           timeout);
            }else{
                return set_iface_property (def_iface,
                                           property_name,
                                           dbus_variant(value),
                                           callback,
                                           timeout);
            }
        }

        /**
         * Asynchronous call to set a string property
         * value in the default interface.
         * The result of the operation is sent as a boolean parameter in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param property_name The name of the property.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool set_property (const std::string& property_name,
                           const char* value,
                           std::function<void (retvalue<bool>& result)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if (!value) {
                retvalue<bool> ret (false);
                ret.err (-1, "Invalid null pointer argument");
                return ret;
            }
            return set_iface_property (def_iface,
                                       property_name,
                                       dbus_variant(dbus_string(value)),
                                       callback,
                                       timeout);
        }


        /**
         * Set a property value in a specific interface.
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @param value New variant value to send.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure.
         */
        retvalue<bool> set_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           const dbus_variant& value,
                                           int timeout=DBUS_TIMEOUT_USE_DEFAULT);
        /**
         * Set a property value in a specific interface.
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure.
         */
        template<dbus_basic_cpp_types T>
        retvalue<bool> set_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           const T& value,
                                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (interface, property_name,
                                       dbus_variant(dbus_basic<T>(value)), timeout);
        }
        /**
         * Set a property value in a specific interface.
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value New D-Bus value to send.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure.
         */
        template<any_dbus_type T>
        retvalue<bool> set_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           const T& value,
                                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (interface, property_name,
                                       dbus_variant(value), timeout);
        }
        /**
         * Set a string property value in a specific interface.
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue whose err() and what() describe the failure,
         *         including a null <code>value</code>.
         */
        retvalue<bool> set_iface_property (const std::string& interface,
                                           const std::string& property_name,
                                           const char* value,
                                           int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if (!value) {
                retvalue<bool> ret (false);
                ret.err (-1, "Invalid null pointer argument");
                return ret;
            }
            return set_iface_property (interface, property_name,
                                       dbus_variant(dbus_string(value)), timeout);
        }


        /**
         * Asynchronous call to set a property value in a specific interface.
         * The result of the operation is sent as a boolean retvalue in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @param value New variant value to send.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool set_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 const dbus_variant& value,
                                 std::function<void (retvalue<bool>& result)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to set a property value in a specific interface.
         * The result of the operation is sent as a boolean retvalue in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        template<dbus_basic_cpp_types T>
        bool set_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 const T& value,
                                 std::function<void (retvalue<bool>& result)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (interface,
                                       property_name,
                                       dbus_variant(dbus_basic<T>(value)),
                                       callback,
                                       timeout);
        }
        /**
         * Asynchronous call to set a property value in a specific interface.
         * The result of the operation is sent as a boolean retvalue in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value New D-Bus value to send.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        template<any_dbus_type T>
        bool set_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 const T& value,
                                 std::function<void (retvalue<bool>& result)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set_iface_property (interface,
                                       property_name,
                                       dbus_variant(value),
                                       callback,
                                       timeout);
        }
        /**
         * Asynchronous call to set a string property
         * value in a specific interface.
         * The result of the operation is sent as a boolean retvalue in
         * the callback. The parameter is <code>true</code> if the property
         *         is available and could be set, <code>false</code> if not
         * @param interface The name of the interface.
         * @param property_name The name of the property.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>false</code> if the message
         *         couldn't be sent on the bus,
         *         Otherwise <code>true</code>.
         */
        bool set_iface_property (const std::string& interface,
                                 const std::string& property_name,
                                 const char* value,
                                 std::function<void (retvalue<bool>& result)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if (!value) {
                retvalue<bool> ret (false);
                ret.err (-1, "Invalid null pointer argument");
                return ret;
            }
            return set_iface_property (interface,
                                       property_name,
                                       dbus_variant(dbus_string(value)),
                                       callback,
                                       timeout);
        }




    protected:
        virtual bool on_signal (message& msg);


    private:
        std::string target;
        std::string target_unique_name;
        std::string opath;
        std::string def_iface;
        std::string iface_add_om_root;
        std::string iface_del_om_root;
        std::mutex cb_mutex;
        object_ifaces_added_cb_t object_ifaces_added_cb;
        object_ifaces_removed_cb_t object_ifaces_removed_cb;
        properties_changed_cb_t properties_changed_cb;
        std::map<std::pair<std::string, std::string>, signal_cb_t> callbacks;
        std::string make_signal_match_rule (const std::string& interface,
                                            const std::string& signal);
        bool on_iface_added (message& msg);
        bool on_iface_removed (message& msg);
        bool on_properties_changed (message& msg);
    };



}
#endif
