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
#ifndef ULTRABUS_ORG_FREEDESKTOP_DBUS_PROPERTIES_HPP
#define ULTRABUS_ORG_FREEDESKTOP_DBUS_PROPERTIES_HPP

#include <ultrabus/connection.hpp>
#include <ultrabus/message_filter.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/retvalue.hpp>
#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/retvalue.hpp>
#include <functional>
#include <string>
#include <mutex>
#include <map>
#include <dbus/dbus.h>


namespace ultrabus {


    /**
     * Callback for changed properties of a D-Bus object.
     *
     * @param interface The interface of the changed properties.
     * @param changed_properties Changed property names and values. The
     *                           reference is valid only for the callback.
     * @param invalidated_properties Invalidated property names. The reference
     *                               is valid only for the callback.
     */
    using properties_changed_cb_t = std::function<void (const std::string& interface,
                                                        dbus_string_dict& changed_properties,
                                                        std::set<std::string>& invalidated_properties)>;


    /**
     * Proxy for the standard <code>org.freedesktop.DBus.Properties</code>
     * interface.
     *
     * The proxy retains a non-owning reference to its connection, which must
     * outlive the proxy.
     *
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
     *       rel="noopener noreferrer" target="_blank">D-Bus Specification - org.freedesktop.DBus.Properties</a>
     */
    class org_freedesktop_DBus_Properties : public message_filter {
    public:
        /**
         * Construct a properties proxy.
         *
         * @param conn Connection used for requests and signals. It is not
         *             owned and must outlive this proxy.
         */
        org_freedesktop_DBus_Properties (connection& conn) : message_filter(conn) {}

        /** Destroy the proxy and release any registered property callback. */
        virtual ~org_freedesktop_DBus_Properties () = default;

        /**
         * Get a single property of a remote DBus object.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to get.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return The property value on success; otherwise a retvalue whose
         *         err() and what() describe the failure.
         * @see retvalue
         */
        retvalue<dbus_variant> get (const std::string& bus_name,
                                    const std::string& opath,
                                    const std::string& interface_name,
                                    const std::string& property_name,
                                    int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get a single property of a remote DBus object.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to get.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        bool get (const std::string& bus_name,
                  const std::string& opath,
                  const std::string& interface_name,
                  const std::string& property_name,
                  std::function<void (retvalue<dbus_variant>& result)> callback,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Get all properties of a specific interface of a remote DBus object.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the properties.
         * @param interface_name The interface whose properties are retrieved.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A dictionary of property names and values on success;
         *         otherwise a retvalue whose err() and what() describe the
         *         failure.
         * @see retvalue
         */
        retvalue<dbus_string_dict> get_all (const std::string& bus_name,
                                            const std::string& opath,
                                            const std::string& interface_name,
                                            int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get all properties of a specific interface
         * of a remote DBus object.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the properties.
         * @param interface_name The interface whose properties are retrieved.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        bool get_all (const std::string& bus_name,
                      const std::string& opath,
                      const std::string& interface_name,
                      std::function<void (retvalue<dbus_string_dict>& result)> callback,
                      int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Set a property of a remote DBus object to a value convertible
         * to a DBus basic type.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue describing the failure.
         * @see retvalue
         */
        template<dbus_basic_cpp_types T>
        retvalue<bool> set (const std::string& bus_name,
                            const std::string& opath,
                            const std::string& interface_name,
                            const std::string& property_name,
                            const T& value,
                            int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(dbus_basic<T>(value)), timeout);
        }
        /**
         * Set a property of a remote DBus object to a value of any DBus type.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param value New D-Bus value to send.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue describing the failure.
         * @see retvalue
         */
        template<any_dbus_type T>
        retvalue<bool> set (const std::string& bus_name,
                            const std::string& opath,
                            const std::string& interface_name,
                            const std::string& property_name,
                            const T& value,
                            int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(value), timeout);
        }
        /**
         * Set a property of a remote DBus object to a string value.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> on success, or an error on failure
         *         (including a null <code>value</code>).
         * @see retvalue
         */
        retvalue<bool> set (const std::string& bus_name,
                            const std::string& opath,
                            const std::string& interface_name,
                            const std::string& property_name,
                            const char* value,
                            int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            if (!value) {
                retvalue<bool> ret (false);
                ret.err (-1, "Invalid null pointer argument");
                return ret;
            }
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(dbus_string(value)), timeout);
        }
        /**
         * Set a property of a remote DBus object.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @param value New variant value to send.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue describing the failure.
         * @see retvalue
         */
        retvalue<bool> set (const std::string& bus_name,
                            const std::string& opath,
                            const std::string& interface_name,
                            const std::string& property_name,
                            const dbus_variant& value,
                            int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Asynchronous call to set a property of a remote DBus object to a
         * value convertible to a DBus basic type.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @tparam T C++ type accepted by dbus_basic_cpp_types.
         * @param value New value to convert to a D-Bus basic type.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        template<dbus_basic_cpp_types T>
        bool set (const std::string& bus_name,
                  const std::string& opath,
                  const std::string& interface_name,
                  const std::string& property_name,
                  const T& value,
                  std::function<void (retvalue<bool>& result)> callback,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(dbus_basic<T>(value)), callback, timeout);
        }
        /**
         * Asynchronously set a property to a D-Bus value.
         *
         * @tparam T D-Bus wrapper type accepted by any_dbus_type.
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @param value New D-Bus value to send.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the request was queued; otherwise
         *         <code>false</code>.
         */
        template<any_dbus_type T>
        bool set (const std::string& bus_name,
                  const std::string& opath,
                  const std::string& interface_name,
                  const std::string& property_name,
                  const T& value,
                  std::function<void (retvalue<bool>& result)> callback,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(value), callback, timeout);
        }
        /**
         * Asynchronously set a property to a string value.
         *
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @param value Null-terminated string to copy as the new value; it
         *              must not be null.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the request was queued; otherwise
         *         <code>false</code>.
         */
        bool set (const std::string& bus_name,
                  const std::string& opath,
                  const std::string& interface_name,
                  const std::string& property_name,
                  const char* value,
                  std::function<void (retvalue<bool>& result)> callback,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            return set (bus_name, opath, interface_name, property_name,
                        dbus_variant(dbus_string(value)), callback, timeout);
        }

        /**
         * Asynchronously set a property to a variant value.
         *
         * @param bus_name The bus name of the service owning the object.
         * @param opath The object path owning the property.
         * @param interface_name The interface owning the property.
         * @param property_name The name of the property to set.
         * @param value New variant value to send.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the request was queued; otherwise
         *         <code>false</code>.
         */
        bool set (const std::string& bus_name,
                  const std::string& opath,
                  const std::string& interface_name,
                  const std::string& property_name,
                  const dbus_variant& value,
                  std::function<void (retvalue<bool>& result)> callback,
                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        // retvalue<dbus_variant> wait_for_property (
        //         const std::string& bus_name,
        //         const std::string& opath,
        //         const std::string& interface_name,
        //         const std::string& property_name,
        //         int timeout);


        /**
         * Set or remove the callback for changed properties of a D-Bus object.
         * @param bus_name A bus name.
         * @param opath The object owning the properties we wish to monitor.
         * @param callback Callback invoked for matching change signals. A
         *                 non-null callback is retained until it is replaced,
         *                 removed with <code>nullptr</code>, or this proxy is
         *                 destroyed.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue when the callback cannot be configured.
         */
        retvalue<bool> set_properties_changed_callback (
                const std::string& bus_name,
                const std::string& opath,
                properties_changed_cb_t callback);


    protected:
        virtual bool on_signal (message& msg);


    private:
        //                 bus_name     opath         callback
        std::map<std::pair<std::string, std::string>, properties_changed_cb_t> props_changed_callbacks;
        std::mutex props_changed_mutex;

        retvalue<bool> set_properties_changed_callback_impl (
                const std::string& bus_name,
                const std::string& opath,
                properties_changed_cb_t callback);
    };


}
#endif
